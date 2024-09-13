#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <xf86drmMode.h>
#include <xf86drm.h>
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <drm_fourcc.h>
#include <virtgpu_drm.h>
#include <i915_drm.h>
#include <xf86drm.h>
#include <poll.h>
#include <sys/mman.h>
#include <time.h>
#include <gbm.h>

#define I915_TILING_4 9

#define ALOGE printf
#define ALOGV printf
#define ALOGI printf

struct buffer {
	struct gbm_bo *bo;
	uint32_t handle;
	uint32_t stride;
};

int width = 2560;
int height = 1440;

struct intel_info {
  int fd;
  uint32_t batch_handle;
  uint32_t *vaddr;
  uint32_t *cur;
  uint64_t size;
  int init;
  struct {
    uint32_t blitter_src;
    uint32_t blitter_dst;
  } mocs;
};

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define MI_NOOP (0)
#define MI_FLUSH_DW (0x26 << 23)
#define MI_BATCH_BUFFER_END (0x0a << 23)

#define XY_SRC_COPY_BLT_CMD ((0x2 << 29) | (0x53 << 22) | 8)  // gen >> 8
#define XY_SRC_COPY_BLT_WRITE_ALPHA (1 << 21)
#define XY_SRC_COPY_BLT_WRITE_RGB (1 << 20)
#define XY_SRC_COPY_BLT_SRC_TILED (1 << 15)
#define XY_SRC_COPY_BLT_DST_TILED (1 << 11)

#define XY_TILE_LINEAR                           0
#define XY_TILE_X                                1
#define XY_TILE_4                                2
#define XY_TILE_64                               3

// GEN 125
#define HALIGN_16                                0
#define HALIGN_32                                1
#define HALIGN_64                                2
#define HALIGN_128                               3
#define VALIGN_4                                 1
#define VALIGN_8                                 2
#define VALIGN_16                                3

// See https://gfxspecs.intel.com/Predator/Home/Index/21523
#define XY_BLOCK_COPY_BLT_CMD ((0x2 << 29) | (0x41 << 22) | (0x14))

static void batch_reset(struct intel_info *info) {
  info->cur = info->vaddr;
}

/**
 * struct drm_i915_gem_mmap_offset - Retrieve an offset so we can mmap this buffer object.
 *
 * This struct is passed as argument to the `DRM_IOCTL_I915_GEM_MMAP_OFFSET` ioctl,
 * and is used to retrieve the fake offset to mmap an object specified by &handle.
 *
 * The legacy way of using `DRM_IOCTL_I915_GEM_MMAP` is removed on gen12+.
 * `DRM_IOCTL_I915_GEM_MMAP_GTT` is an older supported alias to this struct, but will behave
 * as setting the &extensions to 0, and &flags to `I915_MMAP_OFFSET_GTT`.
 */
struct drm_i915_gem_mmap_offset {
	/** @handle: Handle for the object being mapped. */
	__u32 handle;
	/** @pad: Must be zero */
	__u32 pad;
	/**
	 * @offset: The fake offset to use for subsequent mmap call
	 *
	 * This is a fixed-size type for 32/64 compatibility.
	 */
	__u64 offset;

	/**
	 * @flags: Flags for extended behaviour.
	 *
	 * It is mandatory that one of the `MMAP_OFFSET` types
	 * should be included:
	 *
	 * - `I915_MMAP_OFFSET_GTT`: Use mmap with the object bound to GTT. (Write-Combined)
	 * - `I915_MMAP_OFFSET_WC`: Use Write-Combined caching.
	 * - `I915_MMAP_OFFSET_WB`: Use Write-Back caching.
	 * - `I915_MMAP_OFFSET_FIXED`: Use object placement to determine caching.
	 *
	 * On devices with local memory `I915_MMAP_OFFSET_FIXED` is the only valid
	 * type. On devices without local memory, this caching mode is invalid.
	 *
	 * As caching mode when specifying `I915_MMAP_OFFSET_FIXED`, WC or WB will
	 * be used, depending on the object placement on creation. WB will be used
	 * when the object can only exist in system memory, WC otherwise.
	 */
	__u64 flags;

#define I915_MMAP_OFFSET_GTT	0
#define I915_MMAP_OFFSET_WC	1
#define I915_MMAP_OFFSET_WB	2
#define I915_MMAP_OFFSET_UC	3
#define I915_MMAP_OFFSET_FIXED	4

	/**
	 * @extensions: Zero-terminated chain of extensions.
	 *
	 * No current extensions defined; mbz.
	 */
	__u64 extensions;
};

#define DRM_IOCTL_I915_GEM_MMAP_OFFSET	DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_MMAP_GTT, struct drm_i915_gem_mmap_offset)

static int batch_create(struct intel_info *info) {
  struct drm_i915_gem_create create;
  struct drm_i915_gem_mmap_offset mmap_arg;
  int ret = 0;
  memset(&create, 0, sizeof(create));
  memset(&mmap_arg, 0, sizeof(mmap_arg));
  create.size = info->size;
  ret = ioctl(info->fd, DRM_IOCTL_I915_GEM_CREATE, &create);
  if (ret < 0) {
    ALOGE("failed to create buffer\n");
    info->batch_handle = 0;
    return ret;
  }
  info->batch_handle = create.handle;
  mmap_arg.handle = create.handle;
  mmap_arg.flags = I915_MMAP_OFFSET_FIXED;
  ret = ioctl(info->fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg);
  if (ret < 0) {
    drmCloseBufferHandle(info->fd, info->batch_handle);
    info->batch_handle = 0;
    ALOGE("buffer map failure\n");
    return ret;
  }
  info->vaddr = mmap(NULL, info->size, PROT_WRITE|PROT_READ, MAP_SHARED, info->fd, mmap_arg.offset);
  assert (info->vaddr != MAP_FAILED);
  batch_reset(info);
  return ret;
}

__attribute__((unused))
static int batch_count(struct intel_info *info) {
  return info->cur - info->vaddr;
}

static void batch_dword(struct intel_info *info, uint32_t dword) {
  *info->cur++ = dword;
}

static void batch_destroy(struct intel_info *info) {
  if (info->batch_handle) {
    drmCloseBufferHandle(info->fd, info->batch_handle);
    info->batch_handle = 0;
  }
}

static int batch_init(struct intel_info *info, int fd) {
  int ret;
  info->size = 4096;
  info->fd = fd;
  ret = batch_create(info);
  return ret;
}

static int batch_submit(struct intel_info *info, uint32_t src, uint32_t dst,
                        uint64_t src_offset, uint64_t dst_offset,
                        uint32_t in_fence_handle, uint32_t *out_fence_handle) {
  int ret;
  batch_dword(info, MI_BATCH_BUFFER_END);
  struct drm_i915_gem_exec_object2 obj[3];
  struct drm_i915_gem_execbuffer2 execbuf;
  struct drm_i915_gem_exec_fence fence_array[2] = {
    {
      .handle = *out_fence_handle,
      .flags = I915_EXEC_FENCE_SIGNAL,
    },
    {
      .handle = in_fence_handle,
      .flags = I915_EXEC_FENCE_WAIT,
    },
  };
  memset(obj, 0, sizeof(obj));
  obj[0].handle = dst;
  obj[0].offset = dst_offset;
  obj[0].flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_WRITE;

  obj[1].handle = src;
  obj[1].offset = src_offset;
  obj[1].flags = EXEC_OBJECT_PINNED;
  obj[2].handle = info->batch_handle;
  obj[2].offset = 0;
  obj[2].flags = EXEC_OBJECT_PINNED;
  memset(&execbuf, 0, sizeof(execbuf));
  execbuf.buffers_ptr = (__u64)&obj;
  execbuf.buffer_count = 3;
  execbuf.flags = I915_EXEC_BLT;
  execbuf.flags |= I915_EXEC_NO_RELOC;
  execbuf.flags |= I915_EXEC_FENCE_ARRAY;
  execbuf.cliprects_ptr = (__u64)(fence_array);
  execbuf.num_cliprects = ARRAY_SIZE(fence_array) - (in_fence_handle == 0 ? 1 : 0);
  ret = ioctl(info->fd, DRM_IOCTL_I915_GEM_EXECBUFFER2_WR, &execbuf);
  if (ret < 0) {
    ALOGE("submit batchbuffer failure, ret:%d\n", ret);
    return -1;
  }
  struct drm_i915_gem_wait wait;
  memset(&wait, 0, sizeof(wait));
  wait.bo_handle = info->batch_handle;
  wait.timeout_ns = 1000 * 1000 * 1000;
  ioctl(info->fd, DRM_IOCTL_I915_GEM_WAIT, &wait);

  batch_reset(info);
  return 0;
}

__attribute__((unused))
static int emit_src_blit_commands(struct intel_info *info,
                                  uint32_t stride, uint32_t bpp,
                                  uint32_t tiling,
                                  uint16_t width, uint16_t height,
                                  uint64_t src_offset, uint64_t dst_offset) {
  uint32_t cmd, br13, pitch;
  if (!info->init) {
    ALOGE("Blitter is not initialized\n");
    return -1;
  }

  cmd = XY_SRC_COPY_BLT_CMD;
  br13 = 0xcc << 16;
  pitch = stride;
  switch (bpp) {
    case 1:
      break;
    case 2:
      br13 |= (1 << 24);
      break;
    case 4:
      br13 |= (1 << 24) | (1 << 25);
      cmd |= XY_SRC_COPY_BLT_WRITE_ALPHA | XY_SRC_COPY_BLT_WRITE_RGB;
      break;
    default:
      ALOGE("unknown bpp (%u)\n", bpp);
      return -1;
  }
  if (tiling != I915_TILING_NONE) {
    pitch >>= 3;
    cmd |= XY_SRC_COPY_BLT_DST_TILED;
    cmd |= XY_SRC_COPY_BLT_SRC_TILED;
  }
  batch_dword(info, cmd);
  batch_dword(info, br13 | (pitch & 0xffff));
  batch_dword(info, 0);
  batch_dword(info, (height << 16) | width);
  batch_dword(info, dst_offset);
  batch_dword(info, dst_offset >> 32);

  batch_dword(info, 0);
  batch_dword(info, (pitch & 0xffff));
  batch_dword(info, src_offset);
  batch_dword(info, src_offset >> 32);

  batch_dword(info, MI_FLUSH_DW | 2);
  batch_dword(info, 0);
  batch_dword(info, 0);
  batch_dword(info, 0);

  return 0;
}

static uint32_t tiling_to_xy_block_tiling(uint32_t tiling) {
  switch (tiling) {
  case I915_TILING_4:
    return XY_TILE_4;
  case I915_TILING_X:
    return XY_TILE_X;
  case I915_TILING_NONE:
    return XY_TILE_LINEAR;
  default:
    ALOGE("Invalid tiling for XY_BLOCK_COPY_BLT");
  }
  return XY_TILE_LINEAR;
}

// For some reson unknown to me, BLOCK_BLIT command is much slower than
// SRC_BLIT. So we prefer the latter one in spite of the fact that SRC_BLIT
// will be remvoed in GPUs in future generations.
__attribute__((unused))
static int emit_block_blit_commands(struct intel_info *info,
                                    uint32_t stride, uint32_t bpp,
                                    uint32_t tiling,
                                    uint16_t width, uint16_t height,
                                    uint64_t src_offset, uint64_t dst_offset) {
  uint32_t cmd, pitch;
  uint32_t color_depth;
  if (!info->init) {
    return -1;
  }

  switch (bpp) {
  case 1:
    color_depth = 0b00;
    break;
  case 2:
    color_depth = 0b01;
    break;
  case 4:
    color_depth = 0b10;
    break;
  case 8:
    color_depth = 0b11;
    break;
  default:
    ALOGE("unknown bpp (%u)\n", bpp);
    return -1;
  }
  cmd = XY_BLOCK_COPY_BLT_CMD | (color_depth << 19);
  pitch = stride;
  if (tiling != I915_TILING_NONE) {
    pitch >>= 2;
  }
  batch_dword(info, cmd);
  batch_dword(info, (tiling_to_xy_block_tiling(tiling) << 30) | (info->mocs.blitter_dst << 21) | (pitch & 0xffff));
  batch_dword(info, 0); // dst y1 (top) x1 (left)
  batch_dword(info, (height << 16) | width); // dst y2 (bottom) x2 (right)
  // 4
  batch_dword(info, dst_offset);
  batch_dword(info, dst_offset >> 32);
  batch_dword(info, (0x1 << 31)); // system memory
  batch_dword(info, 0); // src y1 (top) x1 (left)
  // 8
  batch_dword(info, (tiling_to_xy_block_tiling(tiling) << 30) | (info->mocs.blitter_src << 21) | (pitch & 0xffff));
  batch_dword(info, src_offset);
  batch_dword(info, src_offset >> 32);
  batch_dword(info, (0x0 << 31)); // local memory
  // 12
  batch_dword(info, 0);
  batch_dword(info, 0);
  batch_dword(info, 0);
  batch_dword(info, 0);
  // 16
  batch_dword(info, (0x1 << 29) | ((width - 1) << 14) | (height - 1));
  batch_dword(info, pitch << 4); // Q Pitch can be zero?
  batch_dword(info, (VALIGN_4 << 3) | (HALIGN_32));
  batch_dword(info, (0x1 << 29) | ((width - 1) << 14) | (height - 1));
  // 20
  batch_dword(info, pitch << 4); // Q Pitch can be zero?
  batch_dword(info, (VALIGN_4 << 3) | (HALIGN_32));

  batch_dword(info, MI_FLUSH_DW | 2);
  batch_dword(info, 0);
  batch_dword(info, 0);
  batch_dword(info, 0);

  return 0;
}

int intel_blit(struct intel_info *info, uint32_t dst, uint32_t src,
               uint32_t stride, uint32_t bpp, uint32_t tiling, uint16_t width,
               uint16_t height, int in_fence, int *out_fence) {
  uint32_t in_fence_handle = 0;
  uint32_t out_fence_handle = 0;
  const uint64_t kSrcOffset = 16 * 1024 * 1024;
  const uint64_t kDstOffset = 256 * 1024 * 1024;
  int ret;

  ret = drmSyncobjCreate(info->fd, 0, &out_fence_handle);
  if (ret) {
    ALOGE("failed to create sync object\n");
    goto out;
  }

  if (in_fence >= 0) {
    ret = drmSyncobjCreate(info->fd, 0, &in_fence_handle);
    if (ret) {
      ALOGE("%s:%u: failed to create syncobj\n", __func__, __LINE__);
      goto out;
    }
    ret = drmSyncobjImportSyncFile(info->fd, in_fence_handle, in_fence);
    if (ret) {
      ALOGE("failed to import syncobj (fd=%d)\n", in_fence);
      goto out;
    }
  }

  ret = emit_src_blit_commands(info, stride, bpp, tiling, width, height, kSrcOffset, kDstOffset);
  if (ret) {
    ALOGE("failed to fill commands\n");
    goto out;
  }

  ret = batch_submit(info, src, dst, kSrcOffset, kDstOffset, in_fence_handle, &out_fence_handle);
  if (ret) {
    ALOGE("failed to submit batch\n");
    goto out;
  }
  ret = drmSyncobjExportSyncFile(info->fd, out_fence_handle, out_fence);
  if (ret) {
    ALOGE("failed to export syncobj (handle=%u)\n", out_fence_handle);
    goto out;
  }

out:
  if (in_fence_handle) {
    drmSyncobjDestroy(info->fd, in_fence_handle);
  }
  if (out_fence_handle) {
    drmSyncobjDestroy(info->fd, out_fence_handle);
  }
  return 0;
}

int intel_blit_destroy(struct intel_info *info) {
  if (info->init) {
    batch_destroy(info);
    info->init = 0;
  }
  return 0;
}

int intel_blit_init(struct intel_info *info, int fd) {
  int ret = 0;
  memset(info, 0, sizeof(*info));
  ret = batch_init(info, fd);
  if (ret) {
    return ret;
  }
  info->init = 1;
  info->mocs.blitter_dst = 2 << 1;
  info->mocs.blitter_src = 2 << 1;
  ALOGV("gpubilit init success\n");
  return 0;
}

struct buffer *create_buffer(struct gbm_device *gbm_device) {
	struct gbm_bo *gbm_bo;
	struct buffer *buf = malloc(sizeof(*buf));
	union gbm_bo_handle gbm_bo_handle;

	assert(buf);
	gbm_bo = gbm_bo_create(gbm_device,
			       width,
			       height,
			       GBM_BO_FORMAT_XRGB8888,
			       GBM_BO_USE_SCANOUT | GBM_BO_USE_LINEAR);
	assert (gbm_bo);
	gbm_bo_handle = gbm_bo_get_handle_for_plane(gbm_bo, 0);
	buf->handle = gbm_bo_handle.u32;
	buf->stride = gbm_bo_get_stride_for_plane(gbm_bo, 0);
	buf->bo = gbm_bo;
	return buf;
}

int main(int argc, char *argv[]) {
	struct buffer *src, *dst;
	int ret;
	int fence_fd, prime_fd;
	uint32_t prime_handle;
	struct intel_info info;
	struct pollfd pfd = { 0 };
	struct timespec start, end;
	double elapsed_time;

	if (argc >= 3) {
		width = atoi(argv[1]);
		height = atoi(argv[2]);
	}

	// dGPU
	int fd = open("/dev/dri/card1", O_RDWR | O_CLOEXEC);
	// iGPU
	int fd0 = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);

	assert (fd >= 0 && fd0 >= 0);
	struct gbm_device *gbm_device = gbm_create_device(fd);
	assert (gbm_device);
	src = create_buffer(gbm_device);
	assert (src);
	dst = create_buffer(gbm_device);
	assert (dst);

	// Migrate to system memory
	ret = drmPrimeHandleToFD(fd, dst->handle, 0, &prime_fd);
	assert (ret == 0);
	ret = drmPrimeFDToHandle(fd0, prime_fd, &prime_handle);
	assert (ret == 0);

	ret = intel_blit_init(&info, fd);
	assert (ret == 0);
	clock_gettime(CLOCK_MONOTONIC, &start);
	for (int i = 0; i < 1000; ++i) {
		ret = intel_blit(&info, dst->handle, src->handle, dst->stride, 4, I915_TILING_NONE, width, height, -1, &fence_fd);
		assert (ret == 0);

		pfd.fd = fence_fd;
		pfd.events = POLLIN;
		ret = poll(&pfd, 1, 1000000);
		assert (ret != -1);
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	elapsed_time = (end.tv_sec * 1000.f + end.tv_nsec / 1000000.f) - (start.tv_sec * 1000.f + end.tv_nsec / 1000000.f);
	printf("elapsed time = %lf ms, %li s\n", elapsed_time, end.tv_sec - start.tv_sec);
	printf("done\n");	
}
