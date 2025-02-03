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

#include "media-io/video-io.h"
#include "obs.h"
#include "util/c99defs.h"
#include <obs-module.h>
#include "plugin-support.h"
#include <shm_ringbuffers.h>

struct srb_filter_data {
	obs_source_t *source;
	obs_source_t *parent;
	obs_view_t *view;
	video_t *video;
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
	d->view = NULL;
	d->video = NULL;
	return d;
}

static void srb_video_frame(void *param, struct video_data *frame)
{
	printf("\n\nB\n\n");
	// va_list args = {frame->linesize[0]};
	// blogva(LOG_INFO, "Captured frame of %d\n", args);
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(frame);
}

static void srb_video_tick(void *data, float seconds)
{
	struct srb_filter_data *d = data;

	if (d->parent == NULL) {
		d->parent = obs_filter_get_parent(d->source);
	}

	if (d->view == NULL && d->parent) {
		printf("\n\nAAA\n\n");
		d->view = obs_view_create();
		printf("\n\nA\n\n");

		obs_view_set_source(d->view, 0, d->parent);
		printf("\n\nA\n\n");

		/*
		struct obs_video_info ovi;
		ovi.base_width = ovi.output_width = 1920;
		ovi.base_height = ovi.output_height = 1080;
		ovi.output_format = VIDEO_FORMAT_BGRA;
		*/
		d->video = obs_view_add2(d->view, NULL);
		printf("\n\nA\n\n");

		/*
		const struct video_scale_info conversion = {
			.width = 1920,
			.height = 1080,
			//.format = VIDEO_FORMAT_RGBA,
			//.range = VIDEO_RANGE_FULL,
			//.colorspace = VIDEO_CS_DEFAULT,
		};
		*/
		if (video_output_connect(d->video, NULL, srb_video_frame, d)) {
			printf("\n\nA\n\n");
			obs_log(LOG_INFO, "Connected SRB video output.");
		} else {
			printf("\n\nA\n\n");
			obs_log(LOG_INFO,
				"Could not connect SRB video output.");
		}
	}

	UNUSED_PARAMETER(seconds);
}

/*
static void srb_filter_render(void *data, gs_effect_t *effect)
{
	struct srb_filter_data *d = data;

	if (d->parent == NULL) {
		d->parent = obs_filter_get_parent(d->source);
	}

	if (!obs_source_process_filter_begin(d->source, GS_RGBA,
					     OBS_ALLOW_DIRECT_RENDERING))
		return;

	obs_source_process_filter_end(
		d->source, obs_get_base_effect(OBS_EFFECT_DEFAULT), 0, 0);

	UNUSED_PARAMETER(effect);
}
*/

struct obs_source_info srb_filter = {
	.id = "srb_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = srb_filter_get_name,
	.create = srb_filter_create,
	.destroy = srb_filter_destroy,
	.video_tick = srb_video_tick,
	//.video_render = srb_filter_render,
};
