/*
OBS Shared-Memory Ring Buffer Tools
Copyright (C) 2025 Edward Flick directrix1@gmail.com

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include "obs.h"
#include "util/c99defs.h"
#include <obs-module.h>
#include "plugin-support.h"
#include <shm_ringbuffers.h>

struct srb_filter_data {
	obs_source_t *source;
	obs_source_t *parent;
	gs_texrender_t *texrender;
	gs_stagesurf_t *stagesurface;
	int connected;
	SRBHandle srbh;
	struct ShmRingBuffer *video_srb;
};

static const char *srb_filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "SRB Output Filter";
}

static void srb_filter_destroy(void *data)
{
	struct srb_filter_data *d = data;
	if (d) {
		bfree(d);
	}
}

static void *srb_filter_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(settings);
	struct srb_filter_data *d = bzalloc(sizeof(struct srb_filter_data));
	d->source = source;
	d->parent = NULL;
	d->srbh = srb_client_new("/srb_video_test");
	if (d->srbh) {
		d->video_srb =
			srb_get_ring_by_description(d->srbh, "video_frames");
		if (!d->video_srb) {
			obs_log(LOG_WARNING, "Could not find srb.\n");
		}
	} else {
		obs_log(LOG_WARNING, "Could not connect to srb shmem.\n");
	}
	return d;
}

/*
static void srb_video_tick(void *data, float seconds)
{
	struct srb_filter_data *d = data;

	if (d->parent == NULL) {
		d->parent = obs_filter_get_parent(d->source);
	}

	if (d->parent == NULL) {
		return;
	}

	struct obs_source_frame *frame = obs_source_get_frame(d->source);
	if (!frame) {
		obs_log(LOG_WARNING, "Frame not returned\n");
		return;
	} else {
		obs_log(LOG_WARNING, "Frame got'd\n");
	}

	if (frame->format != VIDEO_FORMAT_RGBA) {
		obs_log(LOG_WARNING, "Not RGBA!\n");
		obs_source_release_frame(d->parent, frame);
		return;
	}

	// TODO: copy into shared buffer

	obs_source_release_frame(d->parent, frame);
	UNUSED_PARAMETER(seconds);
}
*/

static void srb_filter_render(void *data, gs_effect_t *effect)
{
	struct srb_filter_data *d = data;
	uint8_t *dest_buf;

	if (d->parent == NULL) {
		d->parent = obs_filter_get_parent(d->source);
		dest_buf = srb_producer_first_write_buffer(d->video_srb);
	} else {
		dest_buf = srb_producer_next_write_buffer(d->video_srb);
	}

	uint32_t width = obs_source_get_base_width(d->parent);
	uint32_t height = obs_source_get_base_height(d->parent);
	size_t frame_size = width * height * 4; // RGBA = 4 bytes per pixel
	if (width == 0 || height == 0) {
		obs_log(LOG_WARNING, "Invalid source dimensions.");
		return;
	}

	// Create texture render and staging surface if not already created
	if (!d->texrender) {
		d->texrender = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
	}
	if (!d->stagesurface) {
		d->stagesurface =
			gs_stagesurface_create(width, height, GS_RGBA);
	}
	if (!d->texrender || !d->stagesurface) {
		obs_log(LOG_ERROR,
			"Failed to create texrender or stagesurface.");
		return;
	}

	// Render scene to texture
	gs_texrender_reset(d->texrender);
	gs_begin_scene();
	obs_source_video_render(d->parent);
	gs_end_scene();

	// Copy rendered frame to a staging surface
	gs_texture_t *texture = gs_texrender_get_texture(d->texrender);
	if (texture) {
		gs_stage_texture(d->stagesurface, texture);
	}

	// Map the staging surface and copy the data to RAM
	uint8_t *mapped_pixels;
	uint32_t linesize;
	if (gs_stagesurface_map(d->stagesurface, &mapped_pixels, &linesize)) {
		memcpy(dest_buf, mapped_pixels, frame_size); // Copy data to RAM
		gs_stagesurface_unmap(d->stagesurface);
	} else {
		obs_log(LOG_WARNING, "Failed to map staging surface.");
	}

	if (!obs_source_process_filter_begin(d->source, GS_RGBA,
					     OBS_ALLOW_DIRECT_RENDERING))
		return;

	obs_source_process_filter_end(
		d->source, obs_get_base_effect(OBS_EFFECT_DEFAULT), 0, 0);

	UNUSED_PARAMETER(effect);
}

struct obs_source_info srb_filter = {
	.id = "srb_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = srb_filter_get_name,
	.create = srb_filter_create,
	.destroy = srb_filter_destroy,
	.video_render = srb_filter_render,
	//.video_tick = srb_video_tick,
};
