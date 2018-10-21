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
/**
 * SECTION: IpatchSF2VoiceCache
 * @short_description: SoundFont voice cache object
 * @see_also: 
 * @stability: Stable
 *
 * This is used for pre-processing instruments into arrays of SoundFont
 * compatible voices which can then be accessed very quickly without
 * multi-thread locking or other issues (during synthesis for example).
 */
#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>

#include "IpatchSF2VoiceCache.h"
#include "IpatchSF2Mod.h"
#include "IpatchTypeProp.h"
#include "i18n.h"

/* SoundFont voice native sample format */
#define VOICE_SAMPLE_FORMAT  \
    (IPATCH_SAMPLE_16BIT | IPATCH_SAMPLE_MONO | IPATCH_SAMPLE_ENDIAN_HOST)

static void ipatch_sf2_voice_cache_class_init (IpatchSF2VoiceCacheClass *klass);
static void ipatch_sf2_voice_cache_init (IpatchSF2VoiceCache *cache);
static void ipatch_sf2_voice_cache_finalize (GObject *gobject);

static gpointer parent_class = NULL;
static IpatchSF2Voice def_voice;	/* default voice structure */


/* default selection info */
static IpatchSF2VoiceSelInfo default_sel_info[] =
{
  { IPATCH_SF2_VOICE_SEL_NOTE },
  { IPATCH_SF2_VOICE_SEL_VELOCITY }
};

GType
ipatch_sf2_voice_cache_get_type (void)
{
  static GType item_type = 0;

  if (!item_type) {
    static const GTypeInfo item_info = {
      sizeof (IpatchSF2VoiceCacheClass), NULL, NULL,
      (GClassInitFunc)ipatch_sf2_voice_cache_class_init, NULL, NULL,
      sizeof (IpatchSF2VoiceCache),
      0,
      (GInstanceInitFunc)ipatch_sf2_voice_cache_init,
    };

    item_type = g_type_register_static (G_TYPE_OBJECT,
					"IpatchSF2VoiceCache", &item_info, 0);

    /* install voice update function type property */
    ipatch_type_install_property
      (g_param_spec_pointer ("sf2voice-update-func", "sf2voice-update-func",
			     "sf2voice-update-func", G_PARAM_READWRITE));
  }

  return (item_type);
}

static void
ipatch_sf2_voice_cache_class_init (IpatchSF2VoiceCacheClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  obj_class->finalize = ipatch_sf2_voice_cache_finalize;

  /* initialize default voice structure */
  def_voice.sample_data = NULL;
  def_voice.sample_store = NULL;
  def_voice.sample_size = 0;
  def_voice.loop_start = 0;
  def_voice.loop_end = 0;
  def_voice.rate = 44100;
  def_voice.root_note = 60;
  def_voice.fine_tune = 0;
  def_voice.reserved = 0;

  ipatch_sf2_gen_array_init (&def_voice.gen_array, FALSE, FALSE);
  def_voice.mod_list = NULL;

  def_voice.range_index = 0;    /* init in ipatch_sf2_voice_cache_add_voice */
}

static void
ipatch_sf2_voice_cache_init (IpatchSF2VoiceCache *cache)
{
  cache->sel_info = g_memdup (default_sel_info, sizeof (default_sel_info));
  cache->sel_count = 2;		/* default for velocity and note selection */
  cache->voices = g_array_new (FALSE, FALSE, sizeof (IpatchSF2Voice));
  cache->ranges = NULL;		/* initialized later */
  cache->default_mods = ipatch_sf2_mod_list_duplicate
    (ipatch_sf2_mod_list_get_default ());
  cache->default_loop_type = IPATCH_SAMPLE_LOOP_STANDARD;
}

static void
ipatch_sf2_voice_cache_finalize (GObject *gobject)
{
  IpatchSF2VoiceCache *cache = IPATCH_SF2_VOICE_CACHE (gobject);
  IpatchSF2Voice *voice;
  int i;

  g_free (cache->sel_info);

  for (i = 0; i < cache->voices->len; i++)
    {
      voice = &g_array_index (cache->voices, IpatchSF2Voice, i);

      if (voice->sample_data)	/* -- unref sample data */
	g_object_unref (voice->sample_data);

      if (voice->sample_store)	/* -- unref sample store */
	g_object_unref (voice->sample_store);

      if (voice->mod_list)	/* free modulators */
	ipatch_sf2_mod_list_free (voice->mod_list, TRUE);

      if (cache->voice_user_data_destroy && voice->user_data)
        cache->voice_user_data_destroy (voice->user_data);
    }

  g_array_free (cache->voices, TRUE);

  if (cache->ranges)
    g_array_free (cache->ranges, TRUE);

  if (cache->default_mods)
    ipatch_sf2_mod_list_free (cache->default_mods, TRUE);

  if (cache->user_data_destroy && cache->user_data)
    cache->user_data_destroy (cache->user_data);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

/* FIXME-GIR: Voice cache interface isn't currently binding friendly */

/**
 * ipatch_sf2_voice_cache_new: (skip)
 * @info: (nullable): Array of selection info structures (length should be @sel_count), use
 *   %NULL for default selection info (MIDI note and velocity)
 * @sel_count: Count of selection ranges for this cache (ignored if @info is %NULL)
 *
 * Create a new SoundFont voice cache object.  The @sel_count parameter
 * defines the number of selection ranges for the cache.  Examples of selection
 * ranges include MIDI note and velocity ranges for a voice.  The @info
 * parameter should be a pointer to an array of selection info structures
 * which describes each selection type.
 *
 * Returns: New SoundFont voice cache with a reference count of 1 which the
 *   caller owns.
 */
IpatchSF2VoiceCache *
ipatch_sf2_voice_cache_new (IpatchSF2VoiceSelInfo *info, int sel_count)
{
  IpatchSF2VoiceCache *cache;

  g_return_val_if_fail (!info || sel_count > 0, NULL);

  cache = IPATCH_SF2_VOICE_CACHE (g_object_new (IPATCH_TYPE_SF2_VOICE_CACHE,
						NULL));
  if (info)
    {
      g_free (cache->sel_info);
      cache->sel_info = g_memdup (info, sizeof (IpatchSF2VoiceSelInfo) * sel_count);
      cache->sel_count = sel_count;
    }

  return (cache);
}

/**
 * ipatch_sf2_voice_cache_set_default_mods: (skip)
 * @cache: Voice cache
 * @mods: (element-type IpatchSF2Mod) (transfer full): SoundFont modulator list
 *   to use as default (used directly)
 *
 * Set the default modulator list for the voice cache.  Modulator list is used
 * directly and the allocation of the list is taken over by the voice cache.
 */
void
ipatch_sf2_voice_cache_set_default_mods (IpatchSF2VoiceCache *cache,
					 GSList *mods)
{
  g_return_if_fail (IPATCH_IS_SF2_VOICE_CACHE (cache));

  if (cache->default_mods)
    ipatch_sf2_mod_list_free (cache->default_mods, TRUE);

  cache->default_mods = mods;
}

/**
 * ipatch_sf2_voice_cache_set_override_mods: (skip)
 * @cache: Voice cache
 * @mods: (element-type IpatchSF2Mod) (transfer full): SoundFont modulator list
 *   which overrides rendered voice modulators (used directly)
 *
 * Set the override modulator list for the voice cache.  Modulator list is used
 * directly and the allocation of the list is taken over by the voice cache.
 *
 * Since: 1.1.0
 */
void
ipatch_sf2_voice_cache_set_override_mods (IpatchSF2VoiceCache *cache,
                                          GSList *mods)
{
  g_return_if_fail (IPATCH_IS_SF2_VOICE_CACHE (cache));

  if (cache->override_mods)
    ipatch_sf2_mod_list_free (cache->override_mods, TRUE);

  cache->override_mods = mods;
}

/**
 * ipatch_sf2_voice_cache_add_voice: (skip)
 * @cache: Voice cache to create voice for
 * 
 * Adds a new initialized voice to a SoundFont voice cache.
 * 
 * Returns: (transfer none): Pointer to new voice which is owned by the @cache.
 * The sample pointer is set to NULL, generator array is initialized to default
 * absolute unset values, selection ranges are set to G_MININT-G_MAXINT and
 * all other fields are initialized to defaults.
 */
IpatchSF2Voice *
ipatch_sf2_voice_cache_add_voice (IpatchSF2VoiceCache *cache)
{
  IpatchSF2Voice *voice;
  int *ranges;
  int i;

  g_return_val_if_fail (IPATCH_IS_SF2_VOICE_CACHE (cache), NULL);

  /* create range integer array if this is the first voice */
  if (!cache->ranges)
    cache->ranges = g_array_new (FALSE, FALSE, 2 * sizeof (int)
				 * cache->sel_count);

  g_array_append_val (cache->voices, def_voice);

  voice = &g_array_index (cache->voices, IpatchSF2Voice, cache->voices->len-1);

  voice->range_index = cache->ranges->len * cache->sel_count * 2;

  /* add selection ranges for this voice to range array */
  g_array_set_size (cache->ranges, cache->ranges->len + 1);

  /* initialize ranges to wildcard range */
  ranges = &((int *)(cache->ranges->data))[voice->range_index];
  for (i = 0; i < cache->sel_count; i++)
    {
      ranges[i * 2] = G_MININT;
      ranges[i * 2 + 1] = G_MAXINT;
    }

  return (voice);
}

/**
 * ipatch_sf2_voice_cache_set_voice_range: (skip)
 * @cache: Voice cache object
 * @voice: Voice pointer in cache
 * @sel_index: Selection index
 * @low: Range low value
 * @high: Range high value
 *
 * Set a voice selection range. Selection ranges are used for selection criteria
 * such as MIDI velocity and note ranges.
 */
void
ipatch_sf2_voice_cache_set_voice_range (IpatchSF2VoiceCache *cache,
					IpatchSF2Voice *voice, guint sel_index,
					int low, int high)
{
  int *rangep;

  g_return_if_fail (IPATCH_IS_SF2_VOICE_CACHE (cache));
  g_return_if_fail (voice != NULL);
  g_return_if_fail (sel_index < cache->sel_count);
  g_return_if_fail (low <= high);

  rangep = (int *)(cache->ranges->data);
  rangep[voice->range_index + sel_index * 2] = low;
  rangep[voice->range_index + sel_index * 2 + 1] = high;
}

/**
 * ipatch_sf2_voice_set_sample_data: (skip)
 * @voice: SoundFont voice structure
 * @sample_data: Sample data to assign to voice
 * 
 * Assign sample data to a SoundFont voice.
 */
void
ipatch_sf2_voice_set_sample_data (IpatchSF2Voice *voice, IpatchSampleData *sample_data)
{
  g_return_if_fail (voice != NULL);
  g_return_if_fail (IPATCH_IS_SAMPLE_DATA (sample_data));

  if (voice->sample_data) g_object_unref (voice->sample_data);
  voice->sample_data = g_object_ref (sample_data);	/* ++ ref sample data */
  if (voice->sample_store) g_object_unref (voice->sample_store);
  voice->sample_store = NULL;
  voice->sample_size = ipatch_sample_data_get_size (voice->sample_data);
}

/**
 * ipatch_sf2_voice_cache_sample_data: (skip)
 * @voice: SoundFont voice structure
 * @err: Location to store error info or %NULL to ignore
 * 
 * Cache an already assigned sample data object of a voice.  The sample data is
 * cached as 16 bit mono native endian format, if not already cached, and the
 * new cached sample is assigned to the sample_store field.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set)
 */
gboolean
ipatch_sf2_voice_cache_sample_data (IpatchSF2Voice *voice, GError **err)
{
  g_return_val_if_fail (voice != NULL, FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);
  g_return_val_if_fail (voice->sample_data != NULL, FALSE);

  if (voice->sample_store) g_object_unref (voice->sample_store);

  /* FIXME - Do we need channel_map per voice? */
  /* ++ ref cached sample store for voice */
  voice->sample_store
    = ipatch_sample_data_get_cache_sample (voice->sample_data, VOICE_SAMPLE_FORMAT,
                                           IPATCH_SAMPLE_UNITY_CHANNEL_MAP, err);

  return (voice->sample_store != NULL);
}

/**
 * ipatch_sf2_voice_copy: (skip)
 * @dest: Destination voice to copy to (initialized to 0s or already valid)
 * @src: Source voice to copy from
 *
 * Copy a source (@src) voice's information to a destination voice (@dest).
 * Does not copy selection criteria integers in parent #IpatchSF2VoiceCache
 * objects.
 */
void
ipatch_sf2_voice_copy (IpatchSF2Voice *dest, IpatchSF2Voice *src)
{
  g_return_if_fail (dest != NULL);
  g_return_if_fail (src != NULL);

  if (dest->sample_data) g_object_unref (dest->sample_data);
  if (dest->sample_store) g_object_unref (dest->sample_store);

  if (src->sample_data) dest->sample_data = g_object_ref (src->sample_data);
  else dest->sample_data = NULL;

  if (src->sample_store) dest->sample_store = g_object_ref (src->sample_store);
  else dest->sample_store = NULL;

  dest->sample_size = src->sample_size;
  dest->loop_start = src->loop_start;
  dest->loop_end = src->loop_end;
  dest->rate = src->rate;
  dest->root_note = src->root_note;
  dest->fine_tune = src->fine_tune;
  dest->reserved = src->reserved;
  dest->gen_array = src->gen_array;

  dest->mod_list = ipatch_sf2_mod_list_duplicate (src->mod_list);
}

/**
 * ipatch_sf2_voice_cache_optimize: (skip)
 * @cache: SoundFont voice cache
 * 
 * Can be called after all voices have been added to a voice cache.
 * Will optimize voice cache for use with ipatch_sf2_voice_cache_select().
 * NOTE: Currently does nothing, but will in the future..
 */
void
ipatch_sf2_voice_cache_optimize (IpatchSF2VoiceCache *cache)
{
  /* FIXME */
}

/**
 * ipatch_sf2_voice_cache_select: (skip)
 * @cache: SoundFont voice cache
 * @select_values: Array of select values which must be the same length as
 *   the voice cache was initialized with.  Each selection value is tested
 *   against each voice's selection ranges (use #IPATCH_SF2_VOICE_SEL_WILDCARD
 *   as a wildcard selection value).
 * @index_array: Voice indexes are stored in this array.
 * @max_indexes: Maximum number of voices to match.  @index_array should be
 *   at least this value in size.
 * 
 * Stores pointers to voices matching @select_values criteria.
 *
 * Returns: Number of indexes stored to @index_array.
 */
int
ipatch_sf2_voice_cache_select (IpatchSF2VoiceCache *cache, int *select_values,
			       guint16 *index_array, guint16 max_indexes)
{
  IpatchSF2Voice *voice;
  guint16 *indexp = index_array;
  GArray *voices;
  int *range_array;
  int i, count, scount, sv, si, ri;
  int matches;

  g_return_val_if_fail (IPATCH_IS_SF2_VOICE_CACHE (cache), 0);
  g_return_val_if_fail (select_values != NULL, 0);
  g_return_val_if_fail (index_array != NULL, 0);
  g_return_val_if_fail (max_indexes > 0, 0);

  /* OPTME - This could be optimized, currently just iterates over array */

  count = cache->voices->len;
  voices = cache->voices;
  scount = cache->sel_count;

  if (!cache->ranges)   /* no ranges means no voices, return */
    return (0);

  range_array = (int *)(cache->ranges->data);  /* array will not change */

  /* loop over all voices */
  for (i = 0, matches = 0; i < count && matches < max_indexes; i++)
    {
      voice = &g_array_index (voices, IpatchSF2Voice, i);

      /* loop over selection ranges */
      for (si = 0, ri = voice->range_index; si < scount; si++, ri += 2)
	{
	  sv = select_values[si];

	  /* break out if no match (not -1 and not in range) */
	  if (sv != -1 && !(sv >= range_array[ri] && sv <= range_array[ri + 1]))
	    break;
	}

      if (si == scount)		/* all range selection criteria matched? */
	{
	  *indexp = i;	/* add voice index to index array */
	  indexp++;
	  matches++;
	}
    }

  return (matches);
}

/**
 * ipatch_sf2_voice_cache_updates: (skip)
 * @cache: Voice cache to get updates for
 * @select_values: The voice selection criteria to use, should be the same
 *   number of select values as in @cache
 * @cache_item: Original item @cache was created from
 * @item: Object for which a property changed (only really makes sense if it is
 *   a dependent item of @cache_item).
 * @pspec: Parameter specification of property which changed
 * @value: The new value of the property
 * @updates: Output array to store updates to
 * @max_updates: Size of @updates array (max possible update values).
 *
 * This function is used to re-calculate SoundFont effect generators for a
 * single object property change.  Useful for real time effect changes.
 *
 * Returns: Number of updates stored to @updates array or -1 if not handled
 *   (no handler for the given @item).  Will be 0 if no updates required.
 */
int
ipatch_sf2_voice_cache_update (IpatchSF2VoiceCache *cache,
			       int *select_values, GObject *cache_item,
			       GObject *item, GParamSpec *pspec,
			       const GValue *value,
			       IpatchSF2VoiceUpdate *updates,
			       guint max_updates)
{
  IpatchSF2VoiceCacheUpdateHandler handler;

  g_return_val_if_fail (IPATCH_IS_SF2_VOICE_CACHE (cache), -1);
  g_return_val_if_fail (select_values != NULL, -1);
  g_return_val_if_fail (G_IS_OBJECT (cache_item), -1);
  g_return_val_if_fail (G_IS_OBJECT (item), -1);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), -1);
  g_return_val_if_fail (G_IS_VALUE (value), -1);
  g_return_val_if_fail (updates != NULL, -1);

  ipatch_type_get (G_OBJECT_TYPE (cache_item), "sf2voice-update-func", &handler,
		   NULL);
  if (!handler) return (-1);
  if (max_updates == 0) return (0);

  return handler (cache, select_values, cache_item, item, pspec, value,
		  updates, max_updates);
}

#ifdef IPATCH_DEBUG
/**
 * ipatch_sf2_voice_cache_dump: (skip)
 *
 * Debugging function to dump a voice cache to stdout
 */
void
ipatch_sf2_voice_cache_dump (IpatchSF2VoiceCache *cache, int start, int count)
{
  IpatchSF2Voice *voice;
  char *selnames[] = { "Note", "Vel", "AftTch", "CC" };
  int *range;
  int i, si;

  g_return_if_fail (IPATCH_IS_SF2_VOICE_CACHE (cache));

  if (start == 0)
  {
    printf ("Voice cache selection criteria:\n");

    for (i = 0; i < cache->sel_count; i++)
    {
      switch (cache->sel_info[i].type)
      {
        case IPATCH_SF2_VOICE_SEL_NOTE:
          printf ("%d: Note\n", i);
          break;
        case IPATCH_SF2_VOICE_SEL_VELOCITY:
          printf ("%d: Velocity\n", i);
          break;
        case IPATCH_SF2_VOICE_SEL_AFTER_TOUCH:
          printf ("%d: After touch\n", i);
          break;
        case IPATCH_SF2_VOICE_SEL_MIDI_CC:
          printf ("%d: CC %d\n", i, cache->sel_info[i].param1);
          break;
      }
    }
  }

  for (i = start; i < start + count && i < cache->voices->len; i++)
  {
    voice = &g_array_index (cache->voices, IpatchSF2Voice, i);

    printf ("%d (S:%d,SD:%p,SS:%p) L:%d-%d R:%d RN:%d T:%d\n ",
            i, voice->sample_size, voice->sample_data, voice->sample_store,
            voice->loop_start, voice->loop_end, voice->rate, voice->root_note,
            voice->fine_tune);

    range = &g_array_index (cache->ranges, int, voice->range_index);

    for (si = 0; si < cache->sel_count; si++)
    {
      printf (" %s: %d-%d", selnames[cache->sel_info[si].type],
              range[si * 2], range[si * 2 + 1]);
    }

    printf ("\n");
  }
}

/**
 * ipatch_sf2_voice_cache_dump_select: (skip)
 */
void
ipatch_sf2_voice_cache_dump_select (IpatchSF2VoiceCache *cache, ...)
{
  IpatchSF2Voice *voice;
  va_list args;
  char *selnames[] = { "Note", "Vel", "AftTch", "CC" };
  guint16 indexes[256];
  int selvals[8];
  int *range;
  int count, i, vindex, si;

  va_start (args, cache);

  for (i = 0; i < cache->sel_count; i++)
    selvals[i] = va_arg (args, int);

  va_end (args);

  count = ipatch_sf2_voice_cache_select (cache, selvals, indexes, G_N_ELEMENTS (indexes));

  printf ("%d voices matched:\n", count);

  for (i = 0; i < count; i++)
  {
    vindex = indexes[i];
    voice = &g_array_index (cache->voices, IpatchSF2Voice, vindex);

    printf ("%d (S:%d,SD:%p,SS:%p) L:%d-%d R:%d RN:%d T:%d\n ",
            vindex, voice->sample_size, voice->sample_data, voice->sample_store,
            voice->loop_start, voice->loop_end, voice->rate, voice->root_note,
            voice->fine_tune);

    range = &g_array_index (cache->ranges, int, voice->range_index);

    for (si = 0; si < cache->sel_count; si++)
    {
      printf (" %s: %d-%d", selnames[cache->sel_info[si].type],
              range[si * 2], range[si * 2 + 1]);
    }

    printf ("\n");
  }
}
#endif
