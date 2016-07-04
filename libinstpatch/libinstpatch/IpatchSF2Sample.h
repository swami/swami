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
#ifndef __IPATCH_SF2_SAMPLE_H__
#define __IPATCH_SF2_SAMPLE_H__

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchItem.h>
#include <libinstpatch/IpatchSampleData.h>
#include <libinstpatch/compat.h>

/* forward type declarations */

typedef struct _IpatchSF2Sample IpatchSF2Sample;
typedef struct _IpatchSF2SampleClass IpatchSF2SampleClass;

#define IPATCH_TYPE_SF2_SAMPLE   (ipatch_sf2_sample_get_type ())
#define IPATCH_SF2_SAMPLE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SF2_SAMPLE, \
  IpatchSF2Sample))
#define IPATCH_SF2_SAMPLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SF2_SAMPLE, \
  IpatchSF2SampleClass))
#define IPATCH_IS_SF2_SAMPLE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SF2_SAMPLE))
#define IPATCH_IS_SF2_SAMPLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SF2_SAMPLE))
#define IPATCH_SF2_SAMPLE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_SF2_SAMPLE, \
  IpatchSF2SampleClass))

/* SoundFont sample item */
struct _IpatchSF2Sample
{
  IpatchItem parent_instance;

  IpatchSampleData *sample_data; /* sample data object */
  char *name;			/* name of sample */
  int rate;                     /* sample rate */
  guint32 loop_start;		/* loop start offset (in samples) */
  guint32 loop_end;		/* loop end offset (in samples, first sample
				   AFTER loop actually) */
  guint8 root_note;		/* root midi note number */
  gint8 fine_tune;		/* fine tuning in cents */
  guint8 channel;		/* IpatchSF2SampleChannel */
  GWeakRef linked;	        /* linked sample pointer or NULL */
};

struct _IpatchSF2SampleClass
{
  IpatchItemClass parent_class;
};

/* sampletype flag defines */
/**
 * IpatchSF2SampleChannel:
 * @IPATCH_SF2_SAMPLE_CHANNEL_MONO: Mono sample
 * @IPATCH_SF2_SAMPLE_CHANNEL_LEFT: Left channel of a stereo pair
 *   (#IpatchSF2Sample::linked-sample will be set to the right channel)
 * @IPATCH_SF2_SAMPLE_CHANNEL_RIGHT: Right channel of a stereo pair
 *   (#IpatchSF2Sample::linked-sample will be set to the left channel)
 *
 * Sample channel orientation.
 */
typedef enum
{
  IPATCH_SF2_SAMPLE_CHANNEL_MONO,
  IPATCH_SF2_SAMPLE_CHANNEL_LEFT,
  IPATCH_SF2_SAMPLE_CHANNEL_RIGHT
} IpatchSF2SampleChannel;

/* sample rate and length constraints */

/**
 * IPATCH_SF2_SAMPLE_RATE_MIN: (skip)
 */
#define IPATCH_SF2_SAMPLE_RATE_MIN	400 /* min sample rate (by standard) */

/**
 * IPATCH_SF2_SAMPLE_RATE_MAX: (skip)
 */
#define IPATCH_SF2_SAMPLE_RATE_MAX	50000 /* max rate (by the standard) */

/**
 * IPATCH_SF2_SAMPLE_LENGTH_MIN: (skip)
 */
#define IPATCH_SF2_SAMPLE_LENGTH_MIN	32 /* min length (by the standard) */


/**
 * IpatchSF2SampleFlags:
 * @IPATCH_SF2_SAMPLE_FLAG_ROM: IpatchItem flag for indicating ROM sample
 */
typedef enum
{
  IPATCH_SF2_SAMPLE_FLAG_ROM = (1 << IPATCH_ITEM_UNUSED_FLAG_SHIFT)
} IpatchSF2SampleFlags;

/**
 * IPATCH_SAMPLE_UNUSED_FLAG_SHIFT: (skip)
 */
/* we reserve flags for ROM flag and 3 for expansion */
#define IPATCH_SF2_SAMPLE_UNUSED_FLAG_SHIFT \
  (IPATCH_ITEM_UNUSED_FLAG_SHIFT + 4)

GType ipatch_sf2_sample_get_type (void);
IpatchSF2Sample *ipatch_sf2_sample_new (void);

IpatchSF2Sample *ipatch_sf2_sample_first (IpatchIter *iter);
IpatchSF2Sample *ipatch_sf2_sample_next (IpatchIter *iter);

void ipatch_sf2_sample_set_name (IpatchSF2Sample *sample, const char *name);
char *ipatch_sf2_sample_get_name (IpatchSF2Sample *sample);
void ipatch_sf2_sample_set_data (IpatchSF2Sample *sample,
				 IpatchSampleData *sampledata);
IpatchSampleData *ipatch_sf2_sample_get_data (IpatchSF2Sample *sample);
IpatchSampleData *ipatch_sf2_sample_peek_data (IpatchSF2Sample *sample);
void ipatch_sf2_sample_set_linked (IpatchSF2Sample *sample,
				   IpatchSF2Sample *linked);
IpatchSF2Sample *ipatch_sf2_sample_get_linked (IpatchSF2Sample *sample);
IpatchSF2Sample *ipatch_sf2_sample_peek_linked (IpatchSF2Sample *sample);
void ipatch_sf2_sample_set_blank (IpatchSF2Sample *sample);

#endif
