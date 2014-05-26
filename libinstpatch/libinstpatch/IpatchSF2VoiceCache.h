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
#ifndef __IPATCH_SF2_VOICE_CACHE_H__
#define __IPATCH_SF2_VOICE_CACHE_H__

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>

#include <libinstpatch/IpatchSampleData.h>
#include <libinstpatch/IpatchSF2Gen.h>
#include <libinstpatch/IpatchSF2Mod.h>
#include <libinstpatch/IpatchSample.h>

/* forward type declarations */

typedef struct _IpatchSF2VoiceCache IpatchSF2VoiceCache;
typedef struct _IpatchSF2VoiceCacheClass IpatchSF2VoiceCacheClass;
typedef struct _IpatchSF2Voice IpatchSF2Voice;
typedef struct _IpatchSF2VoiceUpdate IpatchSF2VoiceUpdate;
typedef struct _IpatchSF2VoiceSelInfo IpatchSF2VoiceSelInfo;

#define IPATCH_TYPE_SF2_VOICE_CACHE   (ipatch_sf2_voice_cache_get_type ())
#define IPATCH_SF2_VOICE_CACHE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SF2_VOICE_CACHE, \
  IpatchSF2VoiceCache))
#define IPATCH_SF2_VOICE_CACHE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SF2_VOICE_CACHE, \
  IpatchSF2VoiceCacheClass))
#define IPATCH_IS_SF2_VOICE_CACHE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SF2_VOICE_CACHE))
#define IPATCH_IS_SF2_VOICE_CACHE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SF2_VOICE_CACHE))

/**
 * IpatchSF2VoiceCacheItemFunc:
 * @cache: Voice cache
 * @item: Item which voice cache is dependent on
 *
 * A callback function type which is called during voice cache population
 * for each item which the voice cache is dependent on. This can be useful
 * for determining when a voice cache needs to be updated or for real time
 * effects.
 */
typedef void (*IpatchSF2VoiceCacheItemFunc)(IpatchSF2VoiceCache *cache,
					    GObject *item);

/* SoundFont voice cache object */
struct _IpatchSF2VoiceCache
{
  GObject parent_instance;

  IpatchSF2VoiceSelInfo *sel_info;	/* array of selection criteria info */
  int sel_count;    /* count of selection ranges per voice (integer pairs) */

  GArray *voices;	/* array of IpatchSF2Voice structures */
  GArray *ranges;	/* array of selection integer pairs for each voice */

  GSList *default_mods;		/* default modulators */

  /* default loop type which can be used for objects that don't define it */
  IpatchSampleLoopType default_loop_type;

  /* dependent item callback function */
  IpatchSF2VoiceCacheItemFunc item_func;
  gpointer item_func_data;	/* user defined data used by item_func */

  /* IpatchSF2VoiceCache user defined */
  gpointer user_data;           /* Arbitrary data defined by IpatchSF2VoiceCache user */
  GDestroyNotify user_data_destroy; /* Optional callback to destroy user_data */
  GDestroyNotify voice_user_data_destroy; /* Optional callback to destroy user_data in each voice */

  /* Added with version 1.1.0 */
  GSList *override_mods;        /* override modulators (added with libInstPatch version 1.1.0) */
};

struct _IpatchSF2VoiceCacheClass
{
  GObjectClass parent_class;
};

/* a SoundFont voice */
struct _IpatchSF2Voice
{
  /* Set by SF2VoiceCache converter via ipatch_sf2_voice_set_sample_data() */
  IpatchSampleData *sample_data;	/* sample data for voice */
  IpatchSampleStore *sample_store;	/* Cached store */
  guint32 sample_size;			/* size of sample in frames */

  /* Set by SF2VoiceCache converter */
  guint32 loop_start;		/* loop start offset (in samples) */
  guint32 loop_end;   /* loop end offset (in samples, 1st sample after loop) */
  guint32 rate;			/* sample rate */
  guint8 root_note;		/* MIDI root note of sample */
  gint8 fine_tune;		/* fine tune (in cents, -99 - 99) */
  guint16 reserved;		/* reserved (should be 0) */

  IpatchSF2GenArray gen_array;	/* generator effect values */
  GSList *mod_list;		/* modulator list */

  /* IpatchSF2VoiceCache user defined */
  gpointer user_data;           /* Arbitrary data defined by IpatchSF2VoiceCache user */

  /* Set internally */
  int range_index;  /* index in ranges array (int *) to first selection range */
};

/* a voice parameter update (used for realtime effects) */
struct _IpatchSF2VoiceUpdate
{
  guint16 voice;	/* index of voice with parameter to update */

  union			/* new value for parameter */
  {
    gint16 ival;
    guint16 uval;
  };

  guint8 genid;		/* if type == IPATCH_SF2_VOICE_UPDATE_GEN: id of gen */
  guint8 reserved[3];	/* padding to 4 bytes */
};

/**
 * IpatchSF2VoiceCacheUpdateHandler:
 * @cache: Voice cache to get updates for
 * @select_values: The voice selection criteria to use, should be the same
 *   number of select values as in @cache
 * @cache_item: Original item @cache was created from
 * @item: Object for which a property changed
 * @pspec: Parameter specification of property which changed
 * @value: The new value of the property
 * @updates: Output array to store updates to
 * @max_updates: Size of @updates array (max possible update values).
 *
 * Function prototype used to re-calculate SoundFont effect generators for a
 * single object property change.  Useful for real time effect changes.
 *
 * Returns: Should return number of updates stored to @updates array.
 *   Will be 0 if no updates required.
 */
typedef int (*IpatchSF2VoiceCacheUpdateHandler)(IpatchSF2VoiceCache *cache,
					        int *select_values,
					        GObject *cache_item,
					        GObject *item, GParamSpec *pspec,
					        const GValue *value,
					        IpatchSF2VoiceUpdate *updates,
					        guint max_updates);
/* voice selection type */
typedef enum
{
  IPATCH_SF2_VOICE_SEL_NOTE,		/* MIDI note range */
  IPATCH_SF2_VOICE_SEL_VELOCITY,	/* MIDI velocity range */
  IPATCH_SF2_VOICE_SEL_AFTER_TOUCH,	/* MIDI aftertouch range */
  IPATCH_SF2_VOICE_SEL_MIDI_CC	/* MIDI custom controller (param1: ctrlnum) */
} IpatchSF2VoiceSelType;

/* selection info structure */
struct _IpatchSF2VoiceSelInfo
{
  IpatchSF2VoiceSelType type;
  int param1;
  int param2;				/* currently not used */
};

/* maximum allowed voice selection criteria (MIDI note, velocity, etc) */
#define IPATCH_SF2_VOICE_CACHE_MAX_SEL_VALUES   32

/* value used for wildcard selection */
#define IPATCH_SF2_VOICE_SEL_WILDCARD (G_MININT)

/* For voice cache propagation methods to declare dependent items */
#define ipatch_sf2_voice_cache_declare_item(cache, item) \
  if (cache->item_func) cache->item_func (cache, item)

/* Macro for retrieving a voice pointer from a cache */
#define IPATCH_SF2_VOICE_CACHE_GET_VOICE(cache, index) \
  (&g_array_index (cache->voices, IpatchSF2Voice, index))

GType ipatch_sf2_voice_cache_get_type (void);
IpatchSF2VoiceCache *ipatch_sf2_voice_cache_new (IpatchSF2VoiceSelInfo *info,
						 int sel_count);
void ipatch_sf2_voice_cache_set_default_mods (IpatchSF2VoiceCache *cache,
					      GSList *mods);
void ipatch_sf2_voice_cache_set_override_mods (IpatchSF2VoiceCache *cache,
                                               GSList *mods);

IpatchSF2Voice *ipatch_sf2_voice_cache_add_voice (IpatchSF2VoiceCache *cache);
void ipatch_sf2_voice_cache_set_voice_range (IpatchSF2VoiceCache *cache,
		IpatchSF2Voice *voice, guint sel_index, int low, int high);
void ipatch_sf2_voice_set_sample_data (IpatchSF2Voice *voice,
                                       IpatchSampleData *sample_data);
gboolean ipatch_sf2_voice_cache_sample_data (IpatchSF2Voice *voice, GError **err);
void ipatch_sf2_voice_copy (IpatchSF2Voice *dest, IpatchSF2Voice *src);
void ipatch_sf2_voice_cache_optimize (IpatchSF2VoiceCache *cache);
int ipatch_sf2_voice_cache_select (IpatchSF2VoiceCache *cache,
				   int *select_values,
				   guint16 *index_array,
				   guint16 max_indexes);
int ipatch_sf2_voice_cache_update (IpatchSF2VoiceCache *cache,
				   int *select_values,
				   GObject *cache_item,
				   GObject *item, GParamSpec *pspec,
				   const GValue *value,
				   IpatchSF2VoiceUpdate *updates,
				   guint max_updates);
#endif
