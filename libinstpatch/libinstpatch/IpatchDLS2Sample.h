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
#ifndef __IPATCH_DLS2_SAMPLE_H__
#define __IPATCH_DLS2_SAMPLE_H__

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>

#include <libinstpatch/IpatchDLS2Info.h>
#include <libinstpatch/IpatchItem.h>
#include <libinstpatch/IpatchSampleData.h>

/* forward type declarations */

typedef struct _IpatchDLS2Sample IpatchDLS2Sample;
typedef struct _IpatchDLS2SampleClass IpatchDLS2SampleClass;
typedef struct _IpatchDLS2SampleInfo IpatchDLS2SampleInfo;

#define IPATCH_TYPE_DLS2_SAMPLE   (ipatch_dls2_sample_get_type ())
#define IPATCH_DLS2_SAMPLE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_DLS2_SAMPLE, \
  IpatchDLS2Sample))
#define IPATCH_DLS2_SAMPLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_DLS2_SAMPLE, \
  IpatchDLS2SampleClass))
#define IPATCH_IS_DLS2_SAMPLE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_DLS2_SAMPLE))
#define IPATCH_IS_DLS2_SAMPLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_DLS2_SAMPLE))
#define IPATCH_DLS2_SAMPLE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_DLS2_SAMPLE, \
  IpatchDLS2SampleClass))

#define IPATCH_TYPE_DLS2_SAMPLE_INFO  (ipatch_dls2_sample_info_get_type ())

/* SoundFont sample item */
struct _IpatchDLS2Sample
{
  IpatchItem parent_instance;

  int rate;                     /* sample rate */
  IpatchDLS2SampleInfo *sample_info; /* sample data info (optional) */
  IpatchDLS2Info *info;		/* info string values */

  IpatchSampleData *sample_data;    /* sample data object */

  guint8 *dlid;			/* 16 byte unique ID or NULL */
};

struct _IpatchDLS2SampleClass
{
  IpatchItemClass parent_class;
};

/* DLS2 sample info, can also be defined in regions */
struct _IpatchDLS2SampleInfo
{
  guint8 options; /* IpatchSampleLoopType and IpatchDLS2SampleFlags */
  guint8 root_note;		/* root MIDI note number */
  gint16 fine_tune;		/* fine tuning */
  gint32 gain;			/* gain to apply to sample */

  guint32 loop_start;		/* loop start offset (in samples) */
  guint32 loop_end;		/* loop end offset (in samples) */
};

/* default values to initialize static IpatchDLS2SampleInfo with */
#define IPATCH_DLS2_SAMPLE_INFO_INIT  { 0, 60, 0, 0, 0, 0 }

/**
 * IPATCH_DLS2_SAMPLE_INFO_FIRST_PROPERTY_ID: (skip)
 */
/* since sample info is also used by regions, we define a non-conflicting
   property ID here for the first sample info property */
#define IPATCH_DLS2_SAMPLE_INFO_FIRST_PROPERTY_ID   256

/**
 * IPATCH_DLS2_SAMPLE_INFO_PROPERTY_COUNT: (skip)
 */
/* count of sample info properties */
#define IPATCH_DLS2_SAMPLE_INFO_PROPERTY_COUNT      7

/* flags crammed into sample info options field */
typedef enum
{
  IPATCH_DLS2_SAMPLE_NO_TRUNCATION = 1 << 6,
  IPATCH_DLS2_SAMPLE_NO_COMPRESSION = 1 << 7
} IpatchDLS2SampleFlags;

#define IPATCH_DLS2_SAMPLE_LOOP_MASK   0x03
#define IPATCH_DLS2_SAMPLE_FLAGS_MASK  0x0C0

GType ipatch_dls2_sample_get_type (void);
IpatchDLS2Sample *ipatch_dls2_sample_new (void);

IpatchDLS2Sample *ipatch_dls2_sample_first (IpatchIter *iter);
IpatchDLS2Sample *ipatch_dls2_sample_next (IpatchIter *iter);

void ipatch_dls2_sample_set_data (IpatchDLS2Sample *sample,
				  IpatchSampleData *sampledata);
IpatchSampleData *ipatch_dls2_sample_get_data (IpatchDLS2Sample *sample);
IpatchSampleData *ipatch_dls2_sample_peek_data (IpatchDLS2Sample *sample);
void ipatch_dls2_sample_set_blank (IpatchDLS2Sample *sample);

GType ipatch_dls2_sample_info_get_type (void);
IpatchDLS2SampleInfo *ipatch_dls2_sample_info_new (void);
void ipatch_dls2_sample_info_free (IpatchDLS2SampleInfo *sample_info);
IpatchDLS2SampleInfo *ipatch_dls2_sample_info_duplicate
  (IpatchDLS2SampleInfo *sample_info);
void ipatch_dls2_sample_info_init (IpatchDLS2SampleInfo *sample_info);
void ipatch_dls2_sample_info_install_class_properties
  (GObjectClass *obj_class);
gboolean ipatch_dls2_sample_info_is_property_id_valid (guint property_id);
gboolean ipatch_dls2_sample_info_set_property
  (IpatchDLS2SampleInfo **sample_info, guint property_id,
   const GValue *value);
gboolean ipatch_dls2_sample_info_get_property
  (IpatchDLS2SampleInfo *sample_info, guint property_id, GValue *value);
void ipatch_dls2_sample_info_notify_changes (IpatchItem *item,
					     IpatchDLS2SampleInfo *newinfo,
					     IpatchDLS2SampleInfo *oldinfo);
#endif
