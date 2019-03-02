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
#ifndef __IPATCH_DLS2_REGION_H__
#define __IPATCH_DLS2_REGION_H__

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchItem.h>
#include <libinstpatch/IpatchDLS2Conn.h>
#include <libinstpatch/IpatchDLS2Sample.h>

/* forward type declarations */

typedef struct _IpatchDLS2Region IpatchDLS2Region;
typedef struct _IpatchDLS2RegionClass IpatchDLS2RegionClass;
typedef struct _IpatchDLS2ParamArray IpatchDLS2ParamArray;

#define IPATCH_TYPE_DLS2_REGION   (ipatch_dls2_region_get_type ())
#define IPATCH_DLS2_REGION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_DLS2_REGION, \
  IpatchDLS2Region))
#define IPATCH_DLS2_REGION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_DLS2_REGION, \
  IpatchDLS2RegionClass))
#define IPATCH_IS_DLS2_REGION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_DLS2_REGION))
#define IPATCH_IS_DLS2_REGION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_DLS2_REGION))
#define IPATCH_DLS2_REGION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_DLS2_REGION, \
  IpatchDLS2RegionClass))

/* standard fixed connection parameter enums */
typedef enum
{
  IPATCH_DLS2_PARAM_MOD_LFO_FREQ,
  IPATCH_DLS2_PARAM_MOD_LFO_DELAY,

  IPATCH_DLS2_PARAM_VIB_LFO_FREQ,
  IPATCH_DLS2_PARAM_VIB_LFO_DELAY,

  IPATCH_DLS2_PARAM_VOL_EG_DELAY,
  IPATCH_DLS2_PARAM_VOL_EG_ATTACK,
  IPATCH_DLS2_PARAM_VOL_EG_HOLD,
  IPATCH_DLS2_PARAM_VOL_EG_DECAY,
  IPATCH_DLS2_PARAM_VOL_EG_SUSTAIN,
  IPATCH_DLS2_PARAM_VOL_EG_RELEASE,
  IPATCH_DLS2_PARAM_VOL_EG_SHUTDOWN,
  IPATCH_DLS2_PARAM_VOL_EG_VELOCITY_TO_ATTACK,
  IPATCH_DLS2_PARAM_VOL_EG_NOTE_TO_DECAY,
  IPATCH_DLS2_PARAM_VOL_EG_NOTE_TO_HOLD,

  IPATCH_DLS2_PARAM_MOD_EG_DELAY,
  IPATCH_DLS2_PARAM_MOD_EG_ATTACK,
  IPATCH_DLS2_PARAM_MOD_EG_HOLD,
  IPATCH_DLS2_PARAM_MOD_EG_DECAY,
  IPATCH_DLS2_PARAM_MOD_EG_SUSTAIN,
  IPATCH_DLS2_PARAM_MOD_EG_RELEASE,
  IPATCH_DLS2_PARAM_MOD_EG_VELOCITY_TO_ATTACK,
  IPATCH_DLS2_PARAM_MOD_EG_NOTE_TO_DECAY,
  IPATCH_DLS2_PARAM_MOD_EG_NOTE_TO_HOLD,

  IPATCH_DLS2_PARAM_SCALE_TUNE,
  IPATCH_DLS2_PARAM_RPN2_TO_NOTE,

  IPATCH_DLS2_PARAM_FILTER_CUTOFF,
  IPATCH_DLS2_PARAM_FILTER_Q,
  IPATCH_DLS2_PARAM_MOD_LFO_TO_FILTER_CUTOFF,
  IPATCH_DLS2_PARAM_MOD_LFO_CC1_TO_FILTER_CUTOFF,
  IPATCH_DLS2_PARAM_MOD_LFO_CHANNEL_PRESS_TO_FILTER_CUTOFF,
  IPATCH_DLS2_PARAM_MOD_EG_TO_FILTER_CUTOFF,
  IPATCH_DLS2_PARAM_VELOCITY_TO_FILTER_CUTOFF,
  IPATCH_DLS2_PARAM_NOTE_TO_FILTER_CUTOFF,
  IPATCH_DLS2_PARAM_MOD_LFO_TO_GAIN,
  IPATCH_DLS2_PARAM_MOD_LFO_CC1_TO_GAIN,
  IPATCH_DLS2_PARAM_MOD_LFO_CHANNEL_PRESS_TO_GAIN,
  IPATCH_DLS2_PARAM_VELOCITY_TO_GAIN,
  IPATCH_DLS2_PARAM_CC7_TO_GAIN,
  IPATCH_DLS2_PARAM_CC11_TO_GAIN,

  IPATCH_DLS2_PARAM_TUNE,
  IPATCH_DLS2_PARAM_PITCH_WHEEL_RPN0_TO_PITCH,
  IPATCH_DLS2_PARAM_NOTE_NUMBER_TO_PITCH,
  IPATCH_DLS2_PARAM_RPN1_TO_PITCH,
  IPATCH_DLS2_PARAM_VIB_LFO_TO_PITCH,
  IPATCH_DLS2_PARAM_VIB_LFO_CC1_TO_PITCH,
  IPATCH_DLS2_PARAM_VIB_LFO_CHANNEL_PRESS_TO_PITCH,
  IPATCH_DLS2_PARAM_MOD_LFO_TO_PITCH,
  IPATCH_DLS2_PARAM_MOD_LFO_CC1_TO_PITCH,
  IPATCH_DLS2_PARAM_MOD_LFO_CHANNEL_PRESS_TO_PITCH,
  IPATCH_DLS2_PARAM_MOD_EG_TO_PITCH,

  IPATCH_DLS2_PARAM_PAN,
  IPATCH_DLS2_PARAM_CC10_TO_PAN,
  IPATCH_DLS2_PARAM_CC91_TO_REVERB_SEND,
  IPATCH_DLS2_PARAM_REVERB_SEND,
  IPATCH_DLS2_PARAM_CC93_TO_CHORUS_SEND,
  IPATCH_DLS2_PARAM_CHORUS_SEND,
  IPATCH_DLS2_PARAM_COUNT
} IpatchDLS2Param;

/* DLS2 parameters array */
struct _IpatchDLS2ParamArray
{
  gint32 values[IPATCH_DLS2_PARAM_COUNT];
};

/* DLS2 region item */
struct _IpatchDLS2Region
{
  IpatchItem parent_instance;

  /*< private >*/

  guint8 note_range_low;	/* MIDI note range low value */
  guint8 note_range_high;	/* MIDI note range high value */
  guint8 velocity_range_low;	/* MIDI velocity range low value */
  guint8 velocity_range_high;	/* MIDI velocity range high value */

  guint16 key_group;		/* Exclusive key group number or 0 */
  guint16 layer_group;		/* layer group (descriptive only) */

  guint16 phase_group;		/* Phase locked group number or 0 */
  guint16 channel;	/* channel ID (IpatchDLS2RegionChannelType) */

  IpatchDLS2Info *info;		/* info string values */
  IpatchDLS2SampleInfo *sample_info; /* sample info override or NULL */
  IpatchDLS2Sample *sample;	/* referenced sample */

  IpatchDLS2ParamArray params;	/* array of standard parameter connections */
  GSList *conns; 		/* non-standard connections (modulators) */
};

struct _IpatchDLS2RegionClass
{
  IpatchItemClass parent_class;
};

/* channel steering enum */
typedef enum
{
  IPATCH_DLS2_REGION_CHANNEL_LEFT = 0,
  IPATCH_DLS2_REGION_CHANNEL_RIGHT = 1,
  IPATCH_DLS2_REGION_CHANNEL_CENTER = 2,
  IPATCH_DLS2_REGION_CHANNEL_LOW_FREQ = 3,
  IPATCH_DLS2_REGION_CHANNEL_SURROUND_LEFT = 4,
  IPATCH_DLS2_REGION_CHANNEL_SURROUND_RIGHT = 5,
  IPATCH_DLS2_REGION_CHANNEL_LEFT_OF_CENTER = 6,
  IPATCH_DLS2_REGION_CHANNEL_RIGHT_OF_CENTER = 7,
  IPATCH_DLS2_REGION_CHANNEL_SURROUND_CENTER = 8,
  IPATCH_DLS2_REGION_CHANNEL_SIDE_LEFT = 9,
  IPATCH_DLS2_REGION_CHANNEL_SIDE_RIGHT = 10,
  IPATCH_DLS2_REGION_CHANNEL_TOP = 11,
  IPATCH_DLS2_REGION_CHANNEL_TOP_FRONT_LEFT = 12,
  IPATCH_DLS2_REGION_CHANNEL_TOP_FRONT_CENTER = 13,
  IPATCH_DLS2_REGION_CHANNEL_TOP_FRONT_RIGHT = 14,
  IPATCH_DLS2_REGION_CHANNEL_TOP_REAR_LEFT = 15,
  IPATCH_DLS2_REGION_CHANNEL_TOP_REAR_CENTER = 16,
  IPATCH_DLS2_REGION_CHANNEL_TOP_REAR_RIGHT = 17
} IpatchDLS2RegionChannelType;

/* mono audio alias */
#define IPATCH_DLS2_REGION_CHANNEL_MONO  IPATCH_DLS2_REGION_CHANNEL_LEFT

/**
 * IpatchDLS2RegionFlags:
 * @IPATCH_DLS2_REGION_SELF_NON_EXCLUSIVE:
 * @IPATCH_DLS2_REGION_PHASE_MASTER:
 * @IPATCH_DLS2_REGION_MULTI_CHANNEL:
 * @IPATCH_DLS2_REGION_SAMPLE_INFO_OVERRIDE:
 */
typedef enum
{
  IPATCH_DLS2_REGION_SELF_NON_EXCLUSIVE = 1 << IPATCH_ITEM_UNUSED_FLAG_SHIFT,
  IPATCH_DLS2_REGION_PHASE_MASTER = 1 << (IPATCH_ITEM_UNUSED_FLAG_SHIFT + 1),
  IPATCH_DLS2_REGION_MULTI_CHANNEL = 1 << (IPATCH_ITEM_UNUSED_FLAG_SHIFT + 2),
  IPATCH_DLS2_REGION_SAMPLE_INFO_OVERRIDE=1 << (IPATCH_ITEM_UNUSED_FLAG_SHIFT+3)
} IpatchDLS2RegionFlags;

/**
 * IPATCH_DLS2_REGION_FLAG_MASK: (skip)
 */
#define IPATCH_DLS2_REGION_FLAG_MASK  (0x0F << IPATCH_ITEM_UNUSED_FLAG_SHIFT)

/**
 * IPATCH_DLS2_REGION_UNUSED_FLAG_SHIFT: (skip)
 */
/* 4 flags + 2 for expansion */
#define IPATCH_DLS2_REGION_UNUSED_FLAG_SHIFT (IPATCH_ITEM_UNUSED_FLAG_SHIFT + 6)


GType ipatch_dls2_region_get_type (void);
IpatchDLS2Region *ipatch_dls2_region_new (void);

IpatchDLS2Region *ipatch_dls2_region_first (IpatchIter *iter);
IpatchDLS2Region *ipatch_dls2_region_next (IpatchIter *iter);

char *ipatch_dls2_region_get_info (IpatchDLS2Region *region, guint32 fourcc);
void ipatch_dls2_region_set_info (IpatchDLS2Region *region, guint32 fourcc,
				  const char *val);

void ipatch_dls2_region_set_sample (IpatchDLS2Region *region,
				    IpatchDLS2Sample *sample);
IpatchDLS2Sample *ipatch_dls2_region_get_sample (IpatchDLS2Region *region);
IpatchDLS2Sample *ipatch_dls2_region_peek_sample (IpatchDLS2Region *region);

void ipatch_dls2_region_set_note_range (IpatchDLS2Region *region,
					int low, int high);
void ipatch_dls2_region_set_velocity_range (IpatchDLS2Region *region,
					    int low, int high);
gboolean ipatch_dls2_region_in_range (IpatchDLS2Region *region, int note,
				      int velocity);

void ipatch_dls2_region_set_param (IpatchDLS2Region *region,
				   IpatchDLS2Param param, gint32 val);
void ipatch_dls2_region_set_param_array (IpatchDLS2Region *region,
					 IpatchDLS2ParamArray *array);

GSList *ipatch_dls2_region_get_conns (IpatchDLS2Region *region);
void ipatch_dls2_region_set_conn (IpatchDLS2Region *region,
				  const IpatchDLS2Conn *conn);
void ipatch_dls2_region_unset_conn (IpatchDLS2Region *region,
				    const IpatchDLS2Conn *conn);
void ipatch_dls2_region_unset_all_conns (IpatchDLS2Region *region);
guint ipatch_dls2_region_conn_count (IpatchDLS2Region *region);

int ipatch_dls2_region_channel_map_stereo (IpatchDLS2RegionChannelType chan);

#endif
