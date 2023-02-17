#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <drm.h>
#include <virtgpu_drm.h>
#include <i915_drm.h>
#include <drm_mode.h>
#include <drm_fourcc.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <xf86drm.h>
#include <sys/file.h>

#include "linux_dmabuf_client.h"
#include "xdg_shell_client.h"

#define WIDTH 300
#define HEIGHT 300

struct context {
  struct wl_display *display;
  struct wl_registry *registry;
  struct wl_compositor *compositor;
  struct xdg_wm_base *wm_base;
  struct zwp_linux_dmabuf_v1 *linux_dmabuf;
} ctx;

struct window {
  struct wl_buffer *buffer;
  struct wl_surface *surface;
  struct xdg_surface *xdg_surface;
  struct xdg_toplevel *toplevel;
};

struct wl_buffer* get_dmabuf_buffer() {
  // int gpu_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
  int gpu_fd = drmOpenWithType("amdgpu", NULL, DRM_NODE_PRIMARY);
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
  creq.width = WIDTH;
  creq.height = HEIGHT;
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
  memset(map, -1, creq.size);
  munmap(map, creq.size);
  ret = drmPrimeHandleToFD(gpu_fd, creq.handle, 0, &buffer_fd);
  assert(ret == 0);

  struct zwp_linux_buffer_params_v1 *buffer_params =
      zwp_linux_dmabuf_v1_create_params(ctx.linux_dmabuf);
  zwp_linux_buffer_params_v1_add(buffer_params, buffer_fd, 0, 0, creq.pitch, 0, 0);
  struct wl_buffer *buffer = zwp_linux_buffer_params_v1_create_immed(
      buffer_params, WIDTH, HEIGHT, DRM_FORMAT_ARGB8888, 0);
  return buffer;
}

void xdg_surface_configure_handler(void *data,
                                   struct xdg_surface *xdg_surface,
                                   uint32_t serial) {
  struct window *main_window = data;
  xdg_surface_ack_configure(xdg_surface, serial);
  wl_surface_attach(main_window->surface, main_window->buffer, 0, 0);
  wl_surface_commit(main_window->surface);
}

static const struct xdg_surface_listener xdg_surface_handler = {
  .configure = xdg_surface_configure_handler,
};

void xdg_vm_base_ping_handler(void *data,
                              struct xdg_wm_base *xdg_wm_base,
                              uint32_t serial) {
  xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_handler = {
  .ping = xdg_vm_base_ping_handler,
};

static void registry_global_handler(void *data,
		                                struct wl_registry *registry,
		                                uint32_t name,
		                                const char *interface,
		                                uint32_t version) {
  printf("%s(): name = %u, interface = %s, version = %u\n",
         __func__, name, interface, version);
  struct context *ctx = (struct context *) data;
  if (strcmp(interface, "wl_compositor") == 0) {
    ctx->compositor = wl_registry_bind(
        registry, name, &wl_compositor_interface, version);
  } else if (strcmp(interface, "zwp_linux_dmabuf_v1") == 0) {
    assert (version >= 3);
    ctx->linux_dmabuf = wl_registry_bind(
        registry, name, &zwp_linux_dmabuf_v1_interface, 3);
  } else if (strcmp(interface, "xdg_wm_base") == 0) {
    ctx->wm_base = wl_registry_bind(
        registry, name, &xdg_wm_base_interface, version);
    xdg_wm_base_add_listener(ctx->wm_base, &xdg_wm_base_handler, NULL);
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
  assert(ctx.compositor);
  assert(ctx.wm_base);

  struct window main_window;
  main_window.buffer = get_dmabuf_buffer();
  main_window.surface = wl_compositor_create_surface(ctx.compositor);
  main_window.xdg_surface = xdg_wm_base_get_xdg_surface(ctx.wm_base, main_window.surface);
  xdg_surface_add_listener(main_window.xdg_surface, &xdg_surface_handler,
                           &main_window);
  // xdg_surface_set_window_geometry(xdg_surface, 0, 0, WIDTH, HEIGHT);
  main_window.toplevel = xdg_surface_get_toplevel(main_window.xdg_surface);
  xdg_toplevel_set_title(main_window.toplevel, "test_dmabuf");
  wl_surface_attach(main_window.surface, main_window.buffer, 0, 0);
  wl_surface_commit(main_window.surface);
  while (1) {
    wl_display_dispatch(ctx.display);
  }
}
