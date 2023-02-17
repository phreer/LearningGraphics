#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <libdrm/drm.h>
#include <libdrm/i915_drm.h>
#include <xf86drm.h>

int main(int argc, char *argv[]) {
  int virtgpu_fd, i915_fd;
  int ret;

  int i915_handle, prime_fd;

  const int width = 300, height = 300;
  const int bo_size = width * height * 4;

  virtgpu_fd = drmOpenWithType("virtio-gpu", NULL, DRM_NODE_RENDER);
  if (virtgpu_fd == -1) {
    fprintf(stderr, "failed to open virtio-gpu\n");
    exit(1);
  }

  i915_fd = drmOpenWithType("i915", NULL, DRM_NODE_RENDER);
  if (i915_fd < 0) {
    fprintf(stderr, "failed to open i915\n");
    exit(EXIT_FAILURE);
  }

  struct drm_i915_gem_create create = { .size = bo_size };
  ret = ioctl(i915_fd, DRM_IOCTL_I915_GEM_CREATE, &create);
  if (ret == -1) {
    perror("failed to create i915 GEM");
    exit(EXIT_FAILURE);
  }
  i915_handle = create.handle;
  struct drm_prime_handle prime_data = { .handle = i915_handle };
  ret = ioctl(i915_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime_data);
  if (ret == -1) {
    perror("failed to convert handle to fd");
    exit(EXIT_FAILURE);
  }

  prime_fd = prime_data.fd;

  return 0;
}