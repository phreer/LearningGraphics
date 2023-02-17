#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <drm/drm.h>
#include <drm/virtgpu_drm.h>
#include <drm/drm_mode.h>
#include <drm/drm_fourcc.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <xf86drm.h>
#include <sys/file.h>

#include "linux_dmabuf_client.h"

struct context {
  struct wl_display *display;
  struct wl_registry *registry;
  struct wl_compositor *compositor;
  struct zwp_linux_dmabuf_v1 *linux_dmabuf;
} ctx;

static void registry_global_handler(void *data,
		                                struct wl_registry *registry,
		                                uint32_t name,
		                                const char *interface,
		                                uint32_t version) {
  printf("%s(): name = %u, interface = %s, version = %u\n", __func__, name, interface, version);
  struct context *ctx = (struct context *) data;
  if (strcmp(interface, "wl_compositor") == 0) {
    ctx->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, version);
  } else if (strcmp(interface, "zwp_linux_dmabuf_v1") == 0) {
    assert (version >= 3);
    ctx->linux_dmabuf = wl_registry_bind(registry, name, &zwp_linux_dmabuf_v1_interface, 3);
  }
}

static const struct wl_registry_listener registry_listener = {
  .global = registry_global_handler,
};


int main() {
  ctx.display = wl_display_connect(NULL);
  assert(ctx.display);
  ctx.registry = wl_display_get_registry(ctx.display);
  assert(ctx.registry);
  wl_registry_add_listener(ctx.registry, &registry_listener, &ctx);
  wl_display_roundtrip(ctx.display);
  assert(ctx.linux_dmabuf);

  // int gpu_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
  int gpu_fd = drmOpenWithType("virtio_gpu", NULL, DRM_NODE_PRIMARY);
  assert(gpu_fd >= 0);
  struct drm_mode_create_dumb creq;
  struct drm_mode_destroy_dumb dreq;
  struct drm_mode_map_dumb mreq;
  uint32_t fb;
  int ret;
  int buffer_fd;
  void *map;

  /* create dumb buffer */
  memset(&creq, 0, sizeof(creq));
  creq.width = 300;
  creq.height = 300;
  creq.bpp = 32;
  ret = ioctl(gpu_fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq);
  // ret = drmIoctl(gpu_fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq);
  if (ret != 0) {
    perror("failed to create dumb buffer");
  }
  /* prepare buffer for memory mapping */
  memset(&mreq, 0, sizeof(mreq));
  mreq.handle = creq.handle;
  ret = drmIoctl(gpu_fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);
  assert(ret == 0);
  map = mmap(0, creq.size, PROT_READ | PROT_WRITE, MAP_SHARED, gpu_fd, mreq.offset);
  assert(map);
  memset(map, 0, creq.size);
  ret = drmPrimeHandleToFD(gpu_fd, creq.handle, 0, &buffer_fd);
  assert(ret == 0);

  struct wl_surface *surface = wl_compositor_create_surface(ctx.compositor);

  struct zwp_linux_buffer_params_v1 *buffer_params =
      zwp_linux_dmabuf_v1_create_params(ctx.linux_dmabuf);
  zwp_linux_buffer_params_v1_add(buffer_params, buffer_fd, 0, 0, creq.pitch, 0, 0);
  struct wl_buffer *buffer = zwp_linux_buffer_params_v1_create_immed(
      buffer_params, creq.width, creq.height, DRM_FORMAT_RGBA8888, 0);
  wl_surface_attach(surface, buffer, 0, 0);
}
