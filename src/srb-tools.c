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

#include "graphics/graphics.h"
#include "obs-properties.h"
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
	obs_data_t *settings;
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
	if (d->srbh) {
		srb_close(d->srbh);
		d->srbh = NULL;
		d->video_srb = NULL;
	}
	if (d) {
		bfree(d);
	}
}

static bool srb_filter_connect(void *data)
{
	struct srb_filter_data *d = data;
	if (d->srbh) {
		obs_log(LOG_INFO, "Closing existing srb channel.");
		srb_close(d->srbh);
		d->srbh = NULL;
		d->video_srb = NULL;
	}

	char *shmname = (char *)obs_data_get_string(d->settings, "shmname");
	char *ringbuffer =
		(char *)obs_data_get_string(d->settings, "ringbuffer");
	obs_log(LOG_INFO, "Connecting to SRB:");
	obs_log(LOG_INFO, shmname);
	obs_log(LOG_INFO, "... Ring Buffer:");
	obs_log(LOG_INFO, ringbuffer);
	d->srbh = srb_client_new(shmname);
	if (d->srbh) {
		d->video_srb = srb_get_ring_by_description(d->srbh, ringbuffer);
		if (!d->video_srb) {
			obs_log(LOG_WARNING, "Could not find srb.\n");
			return false;
		}
	} else {
		obs_log(LOG_WARNING, "Could not connect to srb shmem.\n");
		return false;
	}
	return true;
}

static void *srb_filter_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(settings);
	struct srb_filter_data *d = bzalloc(sizeof(struct srb_filter_data));
	d->source = source;
	d->parent = NULL;
	d->texrender = NULL;
	d->stagesurface = NULL;
	d->srbh = NULL;
	d->video_srb = NULL;
	d->settings = settings;
	srb_filter_connect(d);
	return d;
}

static void srb_filter_render(void *data, gs_effect_t *effect)
{
	struct srb_filter_data *d = data;

	if (!obs_source_process_filter_begin(d->source, GS_RGBA,
					     OBS_ALLOW_DIRECT_RENDERING))
		return;

	obs_source_process_filter_end(
		d->source, obs_get_base_effect(OBS_EFFECT_DEFAULT), 0, 0);

	if (!d->video_srb || !obs_source_enabled(d->source)) {
		return;
	}

	if (!d->source) {
		obs_log(LOG_ERROR, "Source not set!");
		return;
	}

	if (!d->parent) {
		d->parent = obs_filter_get_parent(d->source);
		obs_log(LOG_INFO, "Parent:");
		obs_log(LOG_INFO, obs_source_get_name(d->parent));
	}

	if (!d->parent) {
		obs_log(LOG_ERROR, "No parent?");
	}

	uint32_t width = obs_source_get_base_width(d->parent);
	uint32_t height = obs_source_get_base_height(d->parent);

	uint8_t *dest_buf = srb_producer_next_write_buffer(d->video_srb);

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
	if (!gs_texrender_begin(d->texrender, width, height)) {
		obs_log(LOG_ERROR, "Failed to begin texrender.");
		return;
	}

	const struct vec4 blackclear = {0, 0, 0, 0};
	gs_clear(GS_CLEAR_COLOR | GS_CLEAR_DEPTH, &blackclear, 1.0f, 0);
	obs_source_video_render(d->parent);

	gs_texrender_end(d->texrender);

	// Copy rendered frame to a staging surface
	gs_texture_t *texture = gs_texrender_get_texture(d->texrender);
	if (texture) {
		gs_stage_texture(d->stagesurface, texture);
	} else {
		obs_log(LOG_WARNING, "Texture not returned from texrender.");
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

	UNUSED_PARAMETER(effect);
}

static void srb_filter_tick(void *data, float seconds)
{
	struct srb_filter_data *d = data;
	if (!d->video_srb) {
		return;
	}

	UNUSED_PARAMETER(seconds);
}

static bool srb_filter_connect_clicked(obs_properties_t *props,
				       obs_property_t *property, void *data)
{
	return srb_filter_connect(data);
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);
}

static obs_properties_t *srb_filter_get_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	obs_properties_add_text(props, "shmname", "Shared Memory Name",
				OBS_TEXT_DEFAULT);
	obs_properties_add_text(props, "ringbuffer", "Ring Buffer Name",
				OBS_TEXT_DEFAULT);
	obs_properties_add_button2(props, "connect", "Connect",
				   srb_filter_connect_clicked, data);
	return props;
}

static void srb_filter_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "shmname", "/obs_video");
	obs_data_set_default_string(settings, "ringbuffer", "video_frames");
}

static void srb_filter_update(void *data, obs_data_t *settings)
{
	struct srb_filter_data *d = data;
	d->settings = settings;
}

struct obs_source_info srb_filter = {
	.id = "srb_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = srb_filter_get_name,
	.create = srb_filter_create,
	.destroy = srb_filter_destroy,
	.video_render = srb_filter_render,
	.video_tick = srb_filter_tick,
	.get_defaults = srb_filter_get_defaults,
	.get_properties = srb_filter_get_properties,
	.update = srb_filter_update,
};
