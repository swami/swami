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
#ifndef __IPATCH_SF2_GEN_H__
#define __IPATCH_SF2_GEN_H__

#include <glib.h>
#include <glib-object.h>

#include <libinstpatch/IpatchItem.h>

/* total number of generators */
#define IPATCH_SF2_GEN_COUNT 59

/* forward type declarations */
typedef union _IpatchSF2GenAmount IpatchSF2GenAmount;
typedef struct _IpatchSF2GenArray IpatchSF2GenArray;
typedef struct _IpatchSF2GenInfo IpatchSF2GenInfo;

/* IpatchSF2GenArray has a glib boxed type */
#define IPATCH_TYPE_SF2_GEN_ARRAY   (ipatch_sf2_gen_array_get_type ())

/**
 * IpatchSF2GenPropsType:
 *
 * Generator property type (defines which gens are valid and their ranges).
 * Note that TRUE/FALSE can be used to designate PRESET/INST (backwards
 * compatible with previous function).  Also note that global properties can
 * be treated as a flag: #IPATCH_SF2_GEN_PROPS_GLOBAL_FLAG.
 */
typedef enum
{
  IPATCH_SF2_GEN_PROPS_INST = 0,	/* instrument "absolute" properties */
  IPATCH_SF2_GEN_PROPS_PRESET = 1,	/* preset "offset" properties */
  IPATCH_SF2_GEN_PROPS_INST_GLOBAL = 2,	/* inst properties with no sample link */
  IPATCH_SF2_GEN_PROPS_PRESET_GLOBAL = 3  /* preset props with no inst link */
} IpatchSF2GenPropsType;

/* can treat GLOBAL enums from IpatchSF2GenPropsType as a flag */
#define IPATCH_SF2_GEN_PROPS_GLOBAL_FLAG	0x02

/* mask of props type without global flag */
#define IPATCH_SF2_GEN_PROPS_MASK		0x01

/* Generator amount (effect parameter amount) */
union _IpatchSF2GenAmount
{
  /*< public >*/
  gint16 sword;			/* signed 16 bit value */
  guint16 uword;		/* unsigned 16 bit value */
  struct
  {
    guint8 low;			/* low value of range */
    guint8 high;		/* high value of range */
  } range;			/* range values, low - high */
};

/* Generator (effect parameter) */
struct _IpatchSF2Gen
{
  /*< public >*/
  guint16 id;			/* generator #IPGenType ID */
  IpatchSF2GenAmount amount;	/* generator value */
};

/* generator array */
struct _IpatchSF2GenArray
{
  /*< public >*/
  guint64 flags; /* 1 bit for each generator indicating if it is set */
  IpatchSF2GenAmount values[IPATCH_SF2_GEN_COUNT]; /* gen values */
};


/* calculate the set bit value for a given generator ID */
#define IPATCH_SF2_GENID_SET(genid) ((guint64)0x1 << (genid))

/* macros for manipulating individual set flag bits in a generator array
   Note: These macros don't take into account multi-thread locking. */
#define IPATCH_SF2_GEN_ARRAY_TEST_FLAG(array, genid) \
  (((array)->flags & ((guint64)0x1 << (genid))) != 0)
#define IPATCH_SF2_GEN_ARRAY_SET_FLAG(array, genid) \
  ((array)->flags |= ((guint64)0x1 << (genid)))
#define IPATCH_SF2_GEN_ARRAY_CLEAR_FLAG(array, genid) \
  ((array)->flags &= ~((guint64)0x1 << (genid)))

/* generator (effect parameter) types */
typedef enum
{
  IPATCH_SF2_GEN_SAMPLE_START = 0, /* sample start offset */
  IPATCH_SF2_GEN_SAMPLE_END = 1,	/* sample end offset */
  IPATCH_SF2_GEN_SAMPLE_LOOP_START = 2,/* sample loop start offset */
  IPATCH_SF2_GEN_SAMPLE_LOOP_END = 3, /* sample loop end offset */
  IPATCH_SF2_GEN_SAMPLE_COARSE_START = 4, /* sample start coarse offset */
  IPATCH_SF2_GEN_MOD_LFO_TO_PITCH = 5, /* modulation LFO to pitch */
  IPATCH_SF2_GEN_VIB_LFO_TO_PITCH = 6, /* vibrato LFO to pitch */
  IPATCH_SF2_GEN_MOD_ENV_TO_PITCH = 7, /* modulation envelope to pitch */
  IPATCH_SF2_GEN_FILTER_CUTOFF = 8,	/* initial filter cutoff */
  IPATCH_SF2_GEN_FILTER_Q = 9,	/* filter Q */
  IPATCH_SF2_GEN_MOD_LFO_TO_FILTER_CUTOFF = 10, /* mod LFO to filter cutoff */
  IPATCH_SF2_GEN_MOD_ENV_TO_FILTER_CUTOFF = 11, /* mod envelope to filter cutoff */
  IPATCH_SF2_GEN_SAMPLE_COARSE_END = 12, /* sample end course offset */
  IPATCH_SF2_GEN_MOD_LFO_TO_VOLUME = 13, /* modulation LFO to volume */
  IPATCH_SF2_GEN_UNUSED1 = 14,
  IPATCH_SF2_GEN_CHORUS = 15, /* chorus */
  IPATCH_SF2_GEN_REVERB = 16, /* reverb */
  IPATCH_SF2_GEN_PAN = 17,	/* panning */
  IPATCH_SF2_GEN_UNUSED2 = 18,
  IPATCH_SF2_GEN_UNUSED3 = 19,
  IPATCH_SF2_GEN_UNUSED4 = 20,
  IPATCH_SF2_GEN_MOD_LFO_DELAY = 21, /* modulation LFO delay */
  IPATCH_SF2_GEN_MOD_LFO_FREQ = 22, /* modulation LFO frequency */
  IPATCH_SF2_GEN_VIB_LFO_DELAY = 23, /* vibrato LFO delay */
  IPATCH_SF2_GEN_VIB_LFO_FREQ = 24, /* vibrato LFO frequency */
  IPATCH_SF2_GEN_MOD_ENV_DELAY = 25, /* modulation envelope delay */
  IPATCH_SF2_GEN_MOD_ENV_ATTACK = 26, /* modulation envelope attack */
  IPATCH_SF2_GEN_MOD_ENV_HOLD = 27, /* modulation envelope hold */
  IPATCH_SF2_GEN_MOD_ENV_DECAY = 28, /* modulation envelope decay */
  IPATCH_SF2_GEN_MOD_ENV_SUSTAIN = 29, /* modulation envelope sustain */
  IPATCH_SF2_GEN_MOD_ENV_RELEASE = 30, /* modulation envelope release */
  IPATCH_SF2_GEN_NOTE_TO_MOD_ENV_HOLD = 31, /* MIDI note to mod envelope hold */
  IPATCH_SF2_GEN_NOTE_TO_MOD_ENV_DECAY = 32, /* MIDI note to mod env decay */
  IPATCH_SF2_GEN_VOL_ENV_DELAY = 33, /* volume envelope delay */
  IPATCH_SF2_GEN_VOL_ENV_ATTACK = 34, /* volume envelope attack */
  IPATCH_SF2_GEN_VOL_ENV_HOLD = 35, /* volume envelope hold */
  IPATCH_SF2_GEN_VOL_ENV_DECAY = 36, /* volume envelope decay */
  IPATCH_SF2_GEN_VOL_ENV_SUSTAIN = 37, /* volume envelope sustain */
  IPATCH_SF2_GEN_VOL_ENV_RELEASE = 38, /* volume envelope release */
  IPATCH_SF2_GEN_NOTE_TO_VOL_ENV_HOLD = 39, /* MIDI note to vol envelope hold */
  IPATCH_SF2_GEN_NOTE_TO_VOL_ENV_DECAY = 40, /* MIDI note to volume env decay */
  IPATCH_SF2_GEN_INSTRUMENT_ID = 41, /* instrument ID */
  IPATCH_SF2_GEN_RESERVED1 = 42,
  IPATCH_SF2_GEN_NOTE_RANGE = 43,	/* note range */
  IPATCH_SF2_GEN_VELOCITY_RANGE = 44, /* note on velocity range */
  IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_START = 45, /* sample coarse loop start */
  IPATCH_SF2_GEN_FIXED_NOTE = 46, /* MIDI fixed note */
  IPATCH_SF2_GEN_FIXED_VELOCITY = 47, /* MIDI fixed velocity */
  IPATCH_SF2_GEN_ATTENUATION = 48, /* initial volume attenuation */
  IPATCH_SF2_GEN_RESERVED2 = 49,
  IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_END = 50, /* sample end loop course ofs */
  IPATCH_SF2_GEN_COARSE_TUNE = 51, /* course tuning */
  IPATCH_SF2_GEN_FINE_TUNE_OVERRIDE = 52,	/* fine tune override */
  IPATCH_SF2_GEN_SAMPLE_ID = 53,	/* sample ID */
  IPATCH_SF2_GEN_SAMPLE_MODES = 54, /* sample flags (IpatchSF2GenSampleModes)*/
  IPATCH_SF2_GEN_RESERVED3 = 55,
  IPATCH_SF2_GEN_SCALE_TUNE = 56, /* scale tuning (tuning per MIDI note) */
  IPATCH_SF2_GEN_EXCLUSIVE_CLASS = 57, /* exclusive class (only 1 at a time) */
  IPATCH_SF2_GEN_ROOT_NOTE_OVERRIDE = 58	/* root note override */
} IpatchSF2GenType;

/* Flags for IPATCH_SF2_GEN_SAMPLE_MODES generator */
typedef enum
{
  IPATCH_SF2_GEN_SAMPLE_MODE_NOLOOP = 0,
  IPATCH_SF2_GEN_SAMPLE_MODE_LOOP   = 1 << 0,
  IPATCH_SF2_GEN_SAMPLE_MODE_LOOP_RELEASE = 1 << 1
} IpatchSF2GenSampleModes;

/* generator info and constraints structure */
struct _IpatchSF2GenInfo
{
  /*< public >*/
  IpatchSF2GenAmount min;	/* minimum value allowed */
  IpatchSF2GenAmount max;	/* maximum value allowed */
  IpatchSF2GenAmount def;	/* default value */
  gint16 unit;			/* #IpatchUnitType type */
  char *label;			/* short descriptive label */
  char *descr;			/* more complete description */
};

extern IpatchSF2GenArray *ipatch_sf2_gen_ofs_array;
extern IpatchSF2GenArray *ipatch_sf2_gen_abs_array;
extern guint64 ipatch_sf2_gen_ofs_valid_mask;
extern guint64 ipatch_sf2_gen_abs_valid_mask;
extern guint64 ipatch_sf2_gen_add_mask;
extern const IpatchSF2GenInfo ipatch_sf2_gen_info[]; /* IpatchSF2Gen_tables.c */

gboolean ipatch_sf2_gen_is_valid (guint genid, IpatchSF2GenPropsType propstype);

GType ipatch_sf2_gen_array_get_type (void);
IpatchSF2GenArray *ipatch_sf2_gen_array_new (gboolean clear);
void ipatch_sf2_gen_array_free (IpatchSF2GenArray *genarray);
IpatchSF2GenArray *
ipatch_sf2_gen_array_duplicate (const IpatchSF2GenArray *array);

void ipatch_sf2_gen_array_init (IpatchSF2GenArray *array, gboolean offset,
				gboolean set);
gboolean ipatch_sf2_gen_array_offset (IpatchSF2GenArray *abs_array,
				      const IpatchSF2GenArray *ofs_array);
gboolean ipatch_sf2_gen_array_intersect_test (const IpatchSF2GenArray *array1,
                                              const IpatchSF2GenArray *array2);
guint ipatch_sf2_gen_array_count_set (IpatchSF2GenArray *array);

void ipatch_sf2_gen_amount_to_value (guint genid, const IpatchSF2GenAmount *amt,
				     GValue *value);

void ipatch_sf2_gen_default_value (guint genid, gboolean ispreset,
				   IpatchSF2GenAmount *out_amt);
gboolean ipatch_sf2_gen_offset (guint genid, IpatchSF2GenAmount *dst,
				const IpatchSF2GenAmount *ofs);

void ipatch_sf2_gen_clamp (guint genid, int *sfval, gboolean ispreset);
gboolean ipatch_sf2_gen_range_intersect (IpatchSF2GenAmount *dst,
					 const IpatchSF2GenAmount *src);
gboolean ipatch_sf2_gen_range_intersect_test (const IpatchSF2GenAmount *amt1,
                                              const IpatchSF2GenAmount *amt2);

G_CONST_RETURN char *ipatch_sf2_gen_get_prop_name (guint genid);

#endif
