/*
 * modeset-atomic - DRM Atomic-API Modesetting Example
 * Written 2019 by Ezequiel Garcia <ezequiel@collabora.com>
 *
 * Dedicated to the Public Domain.
 */

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include <gbm.h>

#define WIDTH 	3840
#define HEIGHT 	2160

struct modeset_buf {
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	uint32_t size;
	uint32_t handle;
	uint32_t handle_virtgpu;
	uint8_t *map;
	uint32_t fb;
	void *map_data;
	struct gbm_bo *gbm_bo;
};

struct modeset_output {
	unsigned int front_buf;
	struct modeset_buf bufs[2];

	bool cleanup;

	uint8_t r, g, b;
	bool r_up, g_up, b_up;
};

static struct modeset_output *output = NULL;
static struct gbm_device *gbm_device;
static int fd_virtgpu;
/*
 * modeset_open() changes just a little bit. We now have to set that we're going
 * to use the KMS atomic API and check if the device is capable of handling it.
 */

static int modeset_open(int *out, const char *node)
{
	int fd, ret;
	uint64_t cap;

	fd = open(node, O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		ret = -errno;
		fprintf(stderr, "cannot open '%s': %m\n", node);
		return ret;
	}

	gbm_device = gbm_create_device(fd);
	if (gbm_device == NULL) {
		fprintf(stderr, "failed to create gbm devcie\n");
		return -1;
	}
	*out = fd;
	return 0;
}

/*
 * modeset_create_fb() stays the same.
 */

static int modeset_create_fb(int fd, struct modeset_buf *buf)
{
	struct drm_mode_create_dumb creq;
	struct drm_mode_destroy_dumb dreq;
	struct drm_mode_map_dumb mreq;
	int ret;
	uint32_t handle_virtgpu, handles[4] = {0}, pitches[4] = {0}, offsets[4] = {0};
	struct gbm_bo *gbm_bo;
	union gbm_bo_handle gbm_bo_handle;
	int num_planes;
	uint32_t map_stride;
	unsigned int j, k, off;
	int bo_fd;

	printf("allocate the buffer with gbm\n");
	gbm_bo = gbm_bo_create(gbm_device,
			       buf->width,
			       buf->height,
			       GBM_BO_FORMAT_XRGB8888,
			       GBM_BO_USE_SCANOUT | GBM_BO_USE_LINEAR);
	assert(gbm_bo);
	buf->gbm_bo = gbm_bo;
	num_planes = gbm_bo_get_plane_count(gbm_bo);
	assert(num_planes == 1);

	gbm_bo_handle = gbm_bo_get_handle_for_plane(gbm_bo, 0);
	buf->handle = gbm_bo_handle.u32;
	buf->stride = gbm_bo_get_stride_for_plane(gbm_bo, 0);

	/* create framebuffer object for the dumb-buffer */
	handles[0] = buf->handle;
	pitches[0] = buf->stride;

	printf("width = %u, height = %u, stride = %u\n", buf->width, buf->height, buf->stride);
	buf->map = gbm_bo_map(gbm_bo, GBM_BO_FORMAT_XRGB8888, GBM_BO_TRANSFER_WRITE,
				buf->width, buf->height, 0, &buf->stride, &buf->map_data);
	/* clear the framebuffer to 0 */
	for (j = 0; j < buf->height; ++j) {
		for (k = 0; k < buf->width; ++k) {
			off = buf->stride * j + k * 4;
			*(uint32_t*)&buf->map[off] = (uint32_t) (255 << 8);
		}
	}
	gbm_bo_unmap(gbm_bo, buf->map_data);
	buf->map = NULL;
	buf->map_data = NULL;
	bo_fd = gbm_bo_get_fd(gbm_bo);
	ret = drmPrimeFDToHandle(fd_virtgpu, bo_fd, &handle_virtgpu);
	if (ret) {
		fprintf(stderr, "cannot import buffer to virtio-GPU (%d): %m\n",
			errno);
		ret = -errno;
		goto err_destroy;
	}

	buf->handle_virtgpu = handle_virtgpu;

	return 0;

err_fb:
	drmModeRmFB(fd, buf->fb);
err_destroy:
	gbm_bo_destroy(gbm_bo);
	return ret;
}

/*
 * modeset_destroy_fb() stays the same.
 */

static void modeset_destroy_fb(int fd, struct modeset_buf *buf)
{
	struct drm_mode_destroy_dumb dreq;

	/* delete framebuffer */
	drmModeRmFB(fd, buf->fb);
	// TODO: Close GBM resources.
	gbm_bo_destroy(buf->gbm_bo);
}

static int modeset_setup_framebuffers(int fd, struct modeset_output *out)
{
	int i, ret;

	/* setup the front and back framebuffers */
	for (i = 0; i < 2; i++) {

		/* copy mode info to buffer */
		out->bufs[i].width = WIDTH;
		out->bufs[i].height = HEIGHT;

		/* create a framebuffer for the buffer */
		ret = modeset_create_fb(fd, &out->bufs[i]);
		if (ret) {
			/* the second framebuffer creation failed, so
			 * we have to destroy the first before returning */
			if (i == 1)
				modeset_destroy_fb(fd, &out->bufs[0]);
			return ret;
		}
	}

	return 0;
}


static struct modeset_output *modeset_output_create(int fd)
{
	int ret;
	struct modeset_output *out;

	/* creates an output structure */
	out = malloc(sizeof(*out));
	memset(out, 0, sizeof(*out));

	/* setup front/back framebuffers for this CRTC */
	ret = modeset_setup_framebuffers(fd, out);
	if (ret) {
		fprintf(stderr, "cannot create framebuffers\n");
		goto out_error;
	}

	return out;

out_error:
	free(out);
	return NULL;
}

/*
 * modeset_prepare() changes a little bit. Now we use the new function
 * modeset_output_create() to allocate memory and setup the output.
 */

static int modeset_prepare(int fd)
{
	/* create an output structure and free connector data */
	output = modeset_output_create(fd);

	return 0;
}


/*
 * A short helper function to compute a changing color value. No need to
 * understand it.
 */

static uint8_t next_color(bool *up, uint8_t cur, unsigned int mod)
{
	uint8_t next;

	next = cur + (*up ? 1 : -1) * (rand() % mod);
	if ((*up && next < cur) || (!*up && next > cur)) {
		*up = !*up;
		next = cur;
	}

	return next;
}

/*
 * Draw on back framebuffer before the page-flip is requested.
 */

static void modeset_paint_framebuffer(struct modeset_output *out)
{
	struct modeset_buf *buf;
	unsigned int j, k, off;

	buf = &out->bufs[out->front_buf ^ 1];
	buf->map = gbm_bo_map(buf->gbm_bo, GBM_BO_FORMAT_XRGB8888, GBM_BO_TRANSFER_WRITE, buf->width, buf->height, 0, &buf->stride, &buf->map_data);
	/* draw on back framebuffer */
	out->r = next_color(&out->r_up, out->r, 5);
	out->g = next_color(&out->g_up, out->g, 5);
	out->b = next_color(&out->b_up, out->b, 5);
	for (j = 0; j < buf->height / 2; ++j) {
		for (k = 0; k < buf->width; ++k) {
			off = buf->stride * j + k * 4;
			*(uint32_t*)&buf->map[off] =
				     (out->r << 16) | (out->g << 8) | out->b;
		}
	}
	for (j = buf->height / 2; j < buf->height; ++j) {
		for (k = 0; k < buf->width; ++k) {
			off = buf->stride * j + k * 4;
			*(uint32_t*)&buf->map[off] =
				     ((255 - out->r) << 16) | ((255 - out->g) << 8) | (255 - out->b);
		}
	}
	gbm_bo_unmap(buf->gbm_bo, buf->map_data);
	buf->map = NULL;
	buf->map_data = NULL;
}


/*
 * modeset_cleanup() stays the same.
 */

static void modeset_cleanup(int fd)
{
	struct modeset_output *iter;
	drmEventContext ev;
	int ret;
}

int main(int argc, char **argv)
{
	int ret, fd;
	const char *card;

	/* check which DRM device to open */
	if (argc > 1)
		card = argv[1];
	else
		card = "/dev/dri/card0";

	fprintf(stderr, "using card '%s'\n", card);

	/* open the DRM device */
	ret = modeset_open(&fd, card);
	if (ret)
		goto out_return;

	fd_virtgpu = drmOpen("virtio_gpu", NULL);
	if (ret) {
		fprintf(stderr, "failed to open virtio-GPU device: %s\n", strerror(errno));
		goto out_return;
	}

	/* prepare all connectors and CRTCs */
	ret = modeset_prepare(fd);
	if (ret)
		goto out_close;

	while (1) {
		sleep(1);
		// modeset_paint_framebuffer(output);
	}
	/* cleanup everything */
	modeset_cleanup(fd);

	ret = 0;

out_close:
	close(fd);
out_return:
	if (ret) {
		errno = -ret;
		fprintf(stderr, "modeset failed with error %d: %m\n", errno);
	} else {
		fprintf(stderr, "exiting\n");
	}
	return ret;
}

