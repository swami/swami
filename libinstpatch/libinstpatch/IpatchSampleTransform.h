/*
 * libInstPatch
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1
 * of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA or on the web at http://www.gnu.org.
 */
#ifndef __IPATCH_SAMPLE_TRANSFORM_H__
#define __IPATCH_SAMPLE_TRANSFORM_H__

#include <stdio.h>
#include <glib.h>

typedef struct _IpatchSampleTransform IpatchSampleTransform;

/**
 * IpatchSampleTransformFunc:
 * @transform: Sample transform object
 *
 * Audio conversion handler prototype. Handler will modify
 * <structfield>samples</structfield> of @transform if number of samples
 * changes (a change in the number of channels occurs).
 */
typedef void (*IpatchSampleTransformFunc)(IpatchSampleTransform *transform);

#include <libinstpatch/sample.h>

/* sample transform object */
struct _IpatchSampleTransform
{
  guint16 src_format;		/* source sample format */
  guint16 dest_format;		/* destination sample format */
  guint8 channel_map[8];        /* channel mapping for multi-channel audio */
  guint8 buf1_max_frame;	/* max bytes per frame for buf1 */
  guint8 buf2_max_frame;	/* max bytes per frame for buf2 */
  guint8 func_count;		/* number of functions in funcs array */
  guint8 free_buffers;		/* set to TRUE if buffers need to be freed */
  guint max_frames;		/* max frames that can be converted in batch */
  guint frames;			/* number of frames to transform */
  guint samples; /* # of samples for current transform func (not frames!)*/
  gpointer buf1;		/* buffer 1 (first input) */
  gpointer buf2;		/* buffer 2 */
  guint combined_size;	/* size in bytes of both buffers (if combined) */
  gpointer reserved1;           /* Reserved field */
  gpointer reserved2;           /* Reserved field */

  /* array of transform funcs */
  IpatchSampleTransformFunc funcs[IPATCH_SAMPLE_MAX_TRANSFORM_FUNCS];
};

GType ipatch_sample_transform_get_type (void);
IpatchSampleTransform *ipatch_sample_transform_new
  (int src_format, int dest_format, guint32 channel_map);
void ipatch_sample_transform_free (IpatchSampleTransform *transform);
IpatchSampleTransform *ipatch_sample_transform_duplicate (const IpatchSampleTransform *transform);
void ipatch_sample_transform_init (IpatchSampleTransform *transform);
IpatchSampleTransform *ipatch_sample_transform_pool_acquire
  (int src_format, int dest_format, guint32 channel_map);
void ipatch_sample_transform_pool_release (IpatchSampleTransform *transform);
void ipatch_sample_transform_set_formats (IpatchSampleTransform *transform,
					  int src_format, int dest_format,
                                          guint32 channel_map);
void ipatch_sample_transform_alloc (IpatchSampleTransform *transform,
				    guint frames);
int ipatch_sample_transform_alloc_size (IpatchSampleTransform *transform,
					guint size);
void ipatch_sample_transform_free_buffers (IpatchSampleTransform *transform);
guint ipatch_sample_transform_set_buffers_size (IpatchSampleTransform *transform,
						gpointer buf, guint size);
void ipatch_sample_transform_get_buffers (IpatchSampleTransform *transform,
					  gpointer *buf1, gpointer *buf2);
void ipatch_sample_transform_get_frame_sizes (IpatchSampleTransform *transform,
					      guint *buf1_size,
					      guint *buf2_size);
guint ipatch_sample_transform_get_max_frames (IpatchSampleTransform *transform);
gpointer ipatch_sample_transform_convert (IpatchSampleTransform *transform,
					  gconstpointer src, gpointer dest,
					  guint frames);
gpointer ipatch_sample_transform_convert_sizes (IpatchSampleTransform *transform,
                                                gconstpointer src, guint src_size,
                                                guint *dest_size);
gpointer ipatch_sample_transform_convert_single (IpatchSampleTransform *transform,
						 guint frames);
#endif
