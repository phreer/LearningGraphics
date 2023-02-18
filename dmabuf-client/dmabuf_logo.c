#include <assert.h>
#include <drm.h>
#include <drm_fourcc.h>
#include <drm_mode.h>
#include <i915_drm.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <virtgpu_drm.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <xf86drm.h>

#include "linux_dmabuf_client.h"
#include "xdg_shell_client.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

struct context {
  struct wl_display *display;
  struct wl_registry *registry;
  struct wl_compositor *compositor;
  struct xdg_wm_base *wm_base;
  struct zwp_linux_dmabuf_v1 *linux_dmabuf;
  int gpu_fd;
} ctx;

struct window {
  struct wl_buffer *buffer;
  struct wl_surface *surface;
  struct xdg_surface *xdg_surface;
  struct xdg_toplevel *toplevel;
  int32_t width;
  int32_t height;
  unsigned char *rgbx_data;
  unsigned char *alpha_data;
};

void load_rgba_image(const char *filename, int32_t *width, int32_t *height,
                     unsigned char **rgb_data, unsigned char **alpha_data) {
  assert(filename != NULL && width != NULL && height != NULL &&
         rgb_data != NULL && alpha_data != NULL);
  int32_t num_channels = 0;
  *rgb_data = stbi_load(filename, width, height, &num_channels, 4);
  assert(*rgb_data);
  *alpha_data = malloc(*width * *height * sizeof(unsigned char));
  assert(*alpha_data);
  for (uint32_t i = 0; i < (*width) * (*height); ++i) {
    (*alpha_data)[i] = (*rgb_data)[i * 4];
  }
}

int create_dmabuf_plane(int32_t width, int32_t height, uint32_t bpp,
                        unsigned char *data, uint32_t *stride) {
  struct drm_mode_create_dumb creq;
  struct drm_mode_destroy_dumb dreq;
  struct drm_mode_map_dumb mreq;
  uint32_t fb;
  int ret;
  int buffer_fd;
  void *map;
  int gpu_fd = ctx.gpu_fd;
  uint32_t Bpp = bpp / 8;

  /* create dumb buffer */
  memset(&creq, 0, sizeof(creq));
  creq.width = width;
  creq.height = height;
  creq.bpp = bpp;
  ret = ioctl(gpu_fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq);
  // ret = drmIoctl(gpu_fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq);
  if (ret != 0) {
    perror("failed to create dumb buffer");
  }
  *stride = creq.pitch;

  /* prepare buffer for memory mapping */
  memset(&mreq, 0, sizeof(mreq));
  mreq.handle = creq.handle;
  ret = drmIoctl(gpu_fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);
  assert(ret == 0);
  map = mmap(0, creq.size, PROT_READ | PROT_WRITE, MAP_SHARED, gpu_fd,
             mreq.offset);
  assert(map);
  // memset(map, -1, creq.size);
  for (int i = 0; i < height; ++i) {
    memcpy(map + i * creq.pitch, data + i * width * Bpp, width * Bpp);
  }
  munmap(map, creq.size);
  ret = drmPrimeHandleToFD(gpu_fd, creq.handle, 0, &buffer_fd);
  assert(ret == 0);
  return buffer_fd;
}

void buffer_params_created_handler(
    void *data, struct zwp_linux_buffer_params_v1 *zwp_linux_buffer_params_v1,
    struct wl_buffer *buffer) {
  struct window *window = data;
  window->buffer = buffer;
}

static const struct zwp_linux_buffer_params_v1_listener buffer_params_handler =
    {
        .created = buffer_params_created_handler,
};

void get_dmabuf_buffer(int32_t width, int32_t height, unsigned char *rgb_data,
                       unsigned char *alpha_data, struct window *window) {
  struct zwp_linux_buffer_params_v1 *buffer_params =
      zwp_linux_dmabuf_v1_create_params(ctx.linux_dmabuf);
  uint32_t buffer_stride = 0;
  uint32_t alpha_stride = 0;
  int buffer_fd = 0;
  int alpha_fd = 0;

  buffer_fd = create_dmabuf_plane(width, height, 32, rgb_data, &buffer_stride);
  alpha_fd = create_dmabuf_plane(width, height, 8, alpha_data, &alpha_stride);
  zwp_linux_buffer_params_v1_add(buffer_params, buffer_fd, 0, 0, buffer_stride,
                                 0, 0);
  // zwp_linux_buffer_params_v1_add(buffer_params, alpha_fd, 1, 0,
  //                        alpha_stride, 0, 0);
  zwp_linux_buffer_params_v1_create(buffer_params, width, height,
                                    DRM_FORMAT_ABGR8888, 0);
  zwp_linux_buffer_params_v1_add_listener(buffer_params, &buffer_params_handler,
                                          window);
}

void xdg_surface_configure_handler(void *data, struct xdg_surface *xdg_surface,
                                   uint32_t serial) {
  struct window *main_window = data;
  printf("%s(): handle configure event of xdg_surface", __func__);
  xdg_surface_ack_configure(xdg_surface, serial);
  wl_surface_attach(main_window->surface, main_window->buffer, 0, 0);
  wl_surface_commit(main_window->surface);
}

static const struct xdg_surface_listener xdg_surface_handler = {
    .configure = xdg_surface_configure_handler,
};

void xdg_vm_base_ping_handler(void *data, struct xdg_wm_base *xdg_wm_base,
                              uint32_t serial) {
  xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_handler = {
    .ping = xdg_vm_base_ping_handler,
};

static void registry_global_handler(void *data, struct wl_registry *registry,
                                    uint32_t name, const char *interface,
                                    uint32_t version) {
  printf("%s(): name = %u, interface = %s, version = %u\n", __func__, name,
         interface, version);
  struct context *ctx = (struct context *)data;
  if (strcmp(interface, "wl_compositor") == 0) {
    ctx->compositor =
        wl_registry_bind(registry, name, &wl_compositor_interface, version);
  } else if (strcmp(interface, "zwp_linux_dmabuf_v1") == 0) {
    assert(version >= 3);
    ctx->linux_dmabuf =
        wl_registry_bind(registry, name, &zwp_linux_dmabuf_v1_interface, 3);
  } else if (strcmp(interface, "xdg_wm_base") == 0) {
    ctx->wm_base =
        wl_registry_bind(registry, name, &xdg_wm_base_interface, version);
    xdg_wm_base_add_listener(ctx->wm_base, &xdg_wm_base_handler, NULL);
  }
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_global_handler,
};

static void usage(FILE *f, const char *prog) {
  fprintf(f, "usage: %s GPU\n", prog);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "expect GPU\n");
    usage(stderr, argv[0]);
    exit(EXIT_FAILURE);
  }
  ctx.display = wl_display_connect(NULL);
  assert(ctx.display);
  ctx.registry = wl_display_get_registry(ctx.display);
  assert(ctx.registry);
  wl_registry_add_listener(ctx.registry, &registry_listener, &ctx);
  wl_display_roundtrip(ctx.display);
  assert(ctx.linux_dmabuf);
  assert(ctx.compositor);
  assert(ctx.wm_base);

  // int gpu_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
  ctx.gpu_fd = drmOpenWithType(argv[1], NULL, DRM_NODE_PRIMARY);
  assert(ctx.gpu_fd >= 0);

  struct window main_window = {0};

  load_rgba_image("./LINUX-LOGO-768x848.png", &main_window.width,
                  &main_window.height, &main_window.rgbx_data,
                  &main_window.alpha_data);
  get_dmabuf_buffer(main_window.width, main_window.height,
                    main_window.rgbx_data, main_window.rgbx_data, &main_window);
  wl_display_roundtrip(ctx.display);
  while (1) {
    if (main_window.buffer)
      break;
  }
  main_window.surface = wl_compositor_create_surface(ctx.compositor);
  main_window.xdg_surface =
      xdg_wm_base_get_xdg_surface(ctx.wm_base, main_window.surface);
  xdg_surface_add_listener(main_window.xdg_surface, &xdg_surface_handler,
                           &main_window);
  // xdg_surface_set_window_geometry(xdg_surface, 0, 0, WIDTH, HEIGHT);
  main_window.toplevel = xdg_surface_get_toplevel(main_window.xdg_surface);
  xdg_toplevel_set_title(main_window.toplevel, argv[0]);
  wl_surface_commit(main_window.surface);
  while (1) {
    wl_display_dispatch(ctx.display);
  }
}
