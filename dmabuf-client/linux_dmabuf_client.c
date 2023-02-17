/* Generated by wayland-scanner 1.20.0 */

/*
 * Copyright © 2014, 2015 Collabora, Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"

#ifndef __has_attribute
# define __has_attribute(x) 0  /* Compatibility with non-clang compilers. */
#endif

#if (__has_attribute(visibility) || defined(__GNUC__) && __GNUC__ >= 4)
#define WL_PRIVATE __attribute__ ((visibility("hidden")))
#else
#define WL_PRIVATE
#endif

extern const struct wl_interface wl_buffer_interface;
extern const struct wl_interface wl_surface_interface;
extern const struct wl_interface zwp_linux_buffer_params_v1_interface;
extern const struct wl_interface zwp_linux_dmabuf_feedback_v1_interface;

static const struct wl_interface *linux_dmabuf_unstable_v1_types[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&zwp_linux_buffer_params_v1_interface,
	&zwp_linux_dmabuf_feedback_v1_interface,
	&zwp_linux_dmabuf_feedback_v1_interface,
	&wl_surface_interface,
	&wl_buffer_interface,
	NULL,
	NULL,
	NULL,
	NULL,
	&wl_buffer_interface,
};

static const struct wl_message zwp_linux_dmabuf_v1_requests[] = {
	{ "destroy", "", linux_dmabuf_unstable_v1_types + 0 },
	{ "create_params", "n", linux_dmabuf_unstable_v1_types + 6 },
	{ "get_default_feedback", "4n", linux_dmabuf_unstable_v1_types + 7 },
	{ "get_surface_feedback", "4no", linux_dmabuf_unstable_v1_types + 8 },
};

static const struct wl_message zwp_linux_dmabuf_v1_events[] = {
	{ "format", "u", linux_dmabuf_unstable_v1_types + 0 },
	{ "modifier", "3uuu", linux_dmabuf_unstable_v1_types + 0 },
};

WL_PRIVATE const struct wl_interface zwp_linux_dmabuf_v1_interface = {
	"zwp_linux_dmabuf_v1", 4,
	4, zwp_linux_dmabuf_v1_requests,
	2, zwp_linux_dmabuf_v1_events,
};

static const struct wl_message zwp_linux_buffer_params_v1_requests[] = {
	{ "destroy", "", linux_dmabuf_unstable_v1_types + 0 },
	{ "add", "huuuuu", linux_dmabuf_unstable_v1_types + 0 },
	{ "create", "iiuu", linux_dmabuf_unstable_v1_types + 0 },
	{ "create_immed", "2niiuu", linux_dmabuf_unstable_v1_types + 10 },
};

static const struct wl_message zwp_linux_buffer_params_v1_events[] = {
	{ "created", "n", linux_dmabuf_unstable_v1_types + 15 },
	{ "failed", "", linux_dmabuf_unstable_v1_types + 0 },
};

WL_PRIVATE const struct wl_interface zwp_linux_buffer_params_v1_interface = {
	"zwp_linux_buffer_params_v1", 4,
	4, zwp_linux_buffer_params_v1_requests,
	2, zwp_linux_buffer_params_v1_events,
};

static const struct wl_message zwp_linux_dmabuf_feedback_v1_requests[] = {
	{ "destroy", "", linux_dmabuf_unstable_v1_types + 0 },
};

static const struct wl_message zwp_linux_dmabuf_feedback_v1_events[] = {
	{ "done", "", linux_dmabuf_unstable_v1_types + 0 },
	{ "format_table", "hu", linux_dmabuf_unstable_v1_types + 0 },
	{ "main_device", "a", linux_dmabuf_unstable_v1_types + 0 },
	{ "tranche_done", "", linux_dmabuf_unstable_v1_types + 0 },
	{ "tranche_target_device", "a", linux_dmabuf_unstable_v1_types + 0 },
	{ "tranche_formats", "a", linux_dmabuf_unstable_v1_types + 0 },
	{ "tranche_flags", "u", linux_dmabuf_unstable_v1_types + 0 },
};

WL_PRIVATE const struct wl_interface zwp_linux_dmabuf_feedback_v1_interface = {
	"zwp_linux_dmabuf_feedback_v1", 4,
	1, zwp_linux_dmabuf_feedback_v1_requests,
	7, zwp_linux_dmabuf_feedback_v1_events,
};

