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
#include <drm/i915_drm.h>

#define WIDTH 	1920
#define HEIGHT 	1080

#define DRM_I915_GEM_DEVICEPTR		0x3d

struct drm_i915_gem_deviceptr_item {
	__u64 offset;
	__u64 size;
};

/**
 * Create a DRM GEM object from scatter list of device memory addresses
 */
struct drm_i915_gem_deviceptr {
	__u32 vid;
	/* Must be zero */
	__u32 flags;
	__u32 count;
	/**
	 * @handle: Returned handle for the object.
	 *
	 * Object handles are nonzero.
	 */
	__u32 handle;
	struct drm_i915_gem_deviceptr_item items[1];
};

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
};

struct modeset_output {
	unsigned int front_buf;
	struct modeset_buf bufs[2];

	bool cleanup;

	uint8_t r, g, b;
	bool r_up, g_up, b_up;
};

#define DRM_IOCTL_I915_GEM_DEVICEPTR			DRM_IOWR (DRM_COMMAND_BASE + DRM_I915_GEM_DEVICEPTR, struct drm_i915_gem_deviceptr)

static struct modeset_output *output = NULL;
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

	*out = fd;
	return 0;
}

int main(int argc, char **argv)
{
	int ret, fd;
	const char *card;
	struct drm_i915_gem_deviceptr *deviceptr;

	/* check which DRM device to open */
	if (argc > 1)
		card = argv[1];
	else
		card = "/dev/dri/card0";

	/* open the DRM device */
	ret = modeset_open(&fd, card);
	if (ret)
		goto out_return;

	printf("use card %s\n", card);
	int count = 1;
	deviceptr = malloc(sizeof(*deviceptr));
	assert(deviceptr != NULL);
	deviceptr->vid = 1;
	deviceptr->flags = 0;
	deviceptr->count = count;
	deviceptr->items[0].offset = 0x800000;
	deviceptr->items[0].size = 8388608;
	ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_DEVICEPTR, deviceptr);
	if (ret) {
		fprintf(stderr, "failed to create bo, error: %s\n", strerror(errno));
		goto out_close;
	}
	ret = 0;

out_close:
	close(fd);
out_return:
	return ret;
}

