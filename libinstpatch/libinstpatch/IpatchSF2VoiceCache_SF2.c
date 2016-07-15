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
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include "IpatchSF2VoiceCache_SF2.h"
#include "IpatchConverter.h"
#include "IpatchConverterSF2VoiceCache.h"
#include "IpatchConverter_priv.h"

#include "IpatchSF2Inst.h"
#include "IpatchSF2IZone.h"
#include "IpatchSF2Preset.h"
#include "IpatchSF2PZone.h"
#include "IpatchSF2Sample.h"
#include "IpatchSF2VoiceCache.h"
#include "IpatchSF2Zone.h"
#include "IpatchSF2GenItem.h"

/**
 * _ipatch_sf2_voice_cache_init_SF2: (skip)
 */
void
_ipatch_sf2_voice_cache_init_SF2 (void)
{
  g_type_class_ref (IPATCH_TYPE_CONVERTER_SF2_PRESET_TO_SF2_VOICE_CACHE);
  g_type_class_ref (IPATCH_TYPE_CONVERTER_SF2_PZONE_TO_SF2_VOICE_CACHE);
  g_type_class_ref (IPATCH_TYPE_CONVERTER_SF2_INST_TO_SF2_VOICE_CACHE);
  g_type_class_ref (IPATCH_TYPE_CONVERTER_SF2_IZONE_TO_SF2_VOICE_CACHE);
  g_type_class_ref (IPATCH_TYPE_CONVERTER_SF2_SAMPLE_TO_SF2_VOICE_CACHE);

  ipatch_register_converter_map
    (IPATCH_TYPE_CONVERTER_SF2_PRESET_TO_SF2_VOICE_CACHE, 0, 0,
     IPATCH_TYPE_SF2_PRESET, 0, 1, IPATCH_TYPE_SF2_VOICE_CACHE, 0, 1);
  ipatch_register_converter_map
    (IPATCH_TYPE_CONVERTER_SF2_PZONE_TO_SF2_VOICE_CACHE, 0, 0,
     IPATCH_TYPE_SF2_PZONE, 0, 1, IPATCH_TYPE_SF2_VOICE_CACHE, 0, 1);
  ipatch_register_converter_map
    (IPATCH_TYPE_CONVERTER_SF2_INST_TO_SF2_VOICE_CACHE, 0, 0,
     IPATCH_TYPE_SF2_INST, 0, 1, IPATCH_TYPE_SF2_VOICE_CACHE, 0, 1);
  ipatch_register_converter_map
    (IPATCH_TYPE_CONVERTER_SF2_IZONE_TO_SF2_VOICE_CACHE, 0, 0,
     IPATCH_TYPE_SF2_IZONE, 0, 1, IPATCH_TYPE_SF2_VOICE_CACHE, 0, 1);
  ipatch_register_converter_map
    (IPATCH_TYPE_CONVERTER_SF2_SAMPLE_TO_SF2_VOICE_CACHE, 0, 0,
     IPATCH_TYPE_SF2_SAMPLE, 0, 1, IPATCH_TYPE_SF2_VOICE_CACHE, 0, 1);
}

static gboolean
_sf2_preset_to_sf2_voice_cache_convert (IpatchConverter *converter,
				        GError **err)
{
  IpatchSF2Preset *preset;
  IpatchSF2Inst *inst;
  IpatchSF2VoiceCache *cache;
  IpatchSF2Voice *voice;
  IpatchSF2GenArray *gpz;
  IpatchSF2GenArray pz;
  IpatchSF2GenArray *giz;
  IpatchSF2Zone *pzone, *izone;
  IpatchSF2Sample *sample;
  IpatchSF2GenAmount *amt;
  IpatchItem *solo_item;
  GObject *obj;
  GSList *gpmods = NULL, *pmods; /* global and preset modulator lists */
  GSList *gimods;	/* global and instrument modulator lists */
  GSList *imods;	/* local instrument modulators */
  const GSList *defmods;	/* default modulator list */
  GSList *tmpmods;
  GSList *p, *p2;

  obj = IPATCH_CONVERTER_INPUT (converter);
  cache = IPATCH_SF2_VOICE_CACHE (IPATCH_CONVERTER_OUTPUT (converter));
  solo_item = ((IpatchConverterSF2VoiceCache *)converter)->solo_item;

  /* check if its a preset zone or a preset */
  if (IPATCH_IS_SF2_PZONE (obj))	/* ++ ref parent preset */
    preset = IPATCH_SF2_PRESET (ipatch_item_get_parent (IPATCH_ITEM (obj)));
  else preset = IPATCH_SF2_PRESET (obj);

  /* declare the preset as a dependent item */
  ipatch_sf2_voice_cache_declare_item (cache, (GObject *)preset);

  defmods = cache->default_mods;	/* get default mod list */

  IPATCH_ITEM_RLOCK (preset);	/* ++ LOCK preset */

  /* global generators and modulators (can access as long as preset is locked) */
  gpz = &preset->genarray;
  gpmods = preset->mods;

  for (p = preset->zones; p; p = p->next)	/* loop over preset zones */
    {
      pzone = (IpatchSF2Zone *)(p->data);

      /* If a zone is currently solo, skip other zones */
      if (solo_item && (IpatchItem *)pzone != solo_item) continue;

      ipatch_sf2_voice_cache_declare_item (cache, (GObject *)pzone);

      /* copy def/global gen values */
      memcpy (&pz, gpz, sizeof (IpatchSF2GenArray));

      IPATCH_ITEM_RLOCK (pzone); /* ++ LOCK preset zone */

      inst = (IpatchSF2Inst *)(pzone->item);

      if (!inst)	/* skip any zones without instrument ref */
	{
	  IPATCH_ITEM_RUNLOCK (pzone);
	  continue;
	}

      ipatch_sf2_gen_item_copy_set ((IpatchSF2GenItem *)pzone, &pz); /* process pzone */

      if (pzone->mods)
	pmods = ipatch_sf2_mod_list_override (gpmods, pzone->mods, FALSE);
      else pmods = gpmods;

      IPATCH_ITEM_RLOCK (inst); /* ++ LOCK inst */

      /* process global generators and modulators */
      giz = &inst->genarray;

      if (inst->mods)	/* can use modulators while inst is locked (no copy) */
	gimods = ipatch_sf2_mod_list_override (defmods, inst->mods, FALSE);     /* ++ alloc new mod list */
      else gimods = (GSList *)defmods;

      p2 = inst->zones;
      for (; p2; p2 = p2->next)		/* loop over instrument zones */
	{
	  izone = (IpatchSF2Zone *)(p2->data);
	  ipatch_sf2_voice_cache_declare_item (cache, (GObject *)izone);

	  IPATCH_ITEM_RLOCK (izone); /* ++ LOCK izone */

	  sample = (IpatchSF2Sample *)(izone->item);

          /* skip zones without sample or which don't note/velocity intersect */
	  if (!sample || !ipatch_sf2_gen_item_intersect_test ((IpatchSF2GenItem *)izone, &pz))
	    {
	      IPATCH_ITEM_RUNLOCK (izone);
	      continue;
	    }

	  voice = ipatch_sf2_voice_cache_add_voice (cache);

	  /* copy global inst gen values */
	  memcpy (&voice->gen_array, giz, sizeof (IpatchSF2GenArray));

	  /* zone overrides */
	  ipatch_sf2_gen_item_copy_set ((IpatchSF2GenItem *)izone,
					&voice->gen_array);

          if (izone->mods)  /* ++ override global/def mods with zone (no copy) */
	    imods = ipatch_sf2_mod_list_override (gimods, izone->mods, FALSE);
	  else imods = gimods;
 
	  /* igens += pgens */
	  ipatch_sf2_gen_array_offset (&voice->gen_array, &pz);

          /* ++ new modulator list of imods + pmods */
	  voice->mod_list = ipatch_sf2_mod_list_offset (imods, pmods);

          if (cache->override_mods)
          {
            tmpmods = voice->mod_list;
            voice->mod_list = ipatch_sf2_mod_list_override (tmpmods, cache->override_mods, TRUE);
            if (tmpmods) ipatch_sf2_mod_list_free (tmpmods, TRUE);
          }

	  /* set MIDI note and velocity ranges */
	  amt = &voice->gen_array.values[IPATCH_SF2_GEN_NOTE_RANGE];
	  ipatch_sf2_voice_cache_set_voice_range (cache, voice, 0,
                                                  amt->range.low, amt->range.high);

	  amt = &voice->gen_array.values[IPATCH_SF2_GEN_VELOCITY_RANGE];
	  ipatch_sf2_voice_cache_set_voice_range (cache, voice, 1,
					          amt->range.low, amt->range.high);

	  /* copy sample parameters */
	  ipatch_sf2_voice_cache_declare_item (cache, (GObject *)sample);

          ipatch_sf2_voice_set_sample_data (voice, sample->sample_data);

	  voice->loop_start = sample->loop_start;
	  voice->loop_end = sample->loop_end;
	  voice->rate = sample->rate;
	  voice->root_note = sample->root_note;
	  voice->fine_tune = sample->fine_tune;

	  IPATCH_ITEM_RUNLOCK (izone); /* -- UNLOCK izone */

	  /* free imods if not == gimods (only list, since mod data not copied) */
	  if (imods != gimods) ipatch_sf2_mod_list_free (imods, FALSE);
	}

      IPATCH_ITEM_RUNLOCK (inst);	/* -- UNLOCK inst */
      IPATCH_ITEM_RUNLOCK (pzone);	/* -- UNLOCK preset zone */

      /* free gimods if not defmods (mod data was not copied) */
      if (gimods != defmods) ipatch_sf2_mod_list_free (gimods, FALSE);

      /* free pmods list if not gpmods (mod data was not copied) */
      if (pmods != gpmods) ipatch_sf2_mod_list_free (pmods, FALSE);
    }

  IPATCH_ITEM_RUNLOCK (preset); /* -- UNLOCK preset */

  /* if convert object was preset zone, unref parent preset */
  if ((void *)obj != (void *)preset)
    g_object_unref (preset);	/* -- unref parent preset */

  return (TRUE);
}

/* use preset to voice cache converter function */
#define _sf2_pzone_to_sf2_voice_cache_convert \
  _sf2_preset_to_sf2_voice_cache_convert

static gboolean
_sf2_inst_to_sf2_voice_cache_convert (IpatchConverter *converter, GError **err)
{
  IpatchSF2Inst *inst;
  IpatchSF2VoiceCache *cache;
  IpatchSF2Voice *voice;
  IpatchSF2GenArray *giz;
  IpatchSF2Zone *izone;
  IpatchSF2Sample *sample;
  IpatchSF2GenAmount *amt;
  GObject *obj;
  GSList *gimods = NULL;
  const GSList *defmods;
  GSList *tmpmods;
  IpatchItem *solo_item;
  GSList *p;

  obj = IPATCH_CONVERTER_INPUT (converter);
  cache = IPATCH_SF2_VOICE_CACHE (IPATCH_CONVERTER_OUTPUT (converter));
  solo_item = ((IpatchConverterSF2VoiceCache *)converter)->solo_item;

  /* check if its a instrument zone or a instrument */
  if (IPATCH_IS_SF2_IZONE (obj))	/* ++ ref parent instrument */
    inst = IPATCH_SF2_INST (ipatch_item_get_parent (IPATCH_ITEM (obj)));
  else inst = IPATCH_SF2_INST (obj);

  /* declare the instrument as a dependent item */
  ipatch_sf2_voice_cache_declare_item (cache, (GObject *)inst);

  defmods = cache->default_mods;

  IPATCH_ITEM_RLOCK (inst);	/* ++ LOCK instrument */

  /* global generators and modulators (can access as long as inst is locked) */
  giz = &inst->genarray;

  if (inst->mods)	/* don't need to copy mods, under LOCK */
    gimods = ipatch_sf2_mod_list_override (defmods, inst->mods, FALSE);
  else gimods = (GSList *)defmods;

  p = inst->zones;
  for (; p; p = p->next)
    {
      izone = (IpatchSF2Zone *)(p->data);

      /* If a zone is currently solo, skip other zones */
      if (solo_item && (IpatchItem *)izone != solo_item) continue;

      ipatch_sf2_voice_cache_declare_item (cache, (GObject *)izone);

      IPATCH_ITEM_RLOCK (izone); /* ++ LOCK izone */

      sample = (IpatchSF2Sample *)(izone->item);

      if (!sample)		/* skip zones without sample ref */
	{
	  IPATCH_ITEM_RUNLOCK (izone);	/* -- unlock izone */
	  continue;
	}

      voice = ipatch_sf2_voice_cache_add_voice (cache);

      /* copy global gen values */
      memcpy (&voice->gen_array, giz, sizeof (IpatchSF2GenArray));

      /* zone overrides global */
      ipatch_sf2_gen_item_copy_set ((IpatchSF2GenItem *)izone, &voice->gen_array);

      
      if (cache->override_mods)
      { /* Override global/default mods with zone (no copy) and then with cache override mods (copy) */
        tmpmods = ipatch_sf2_mod_list_override (gimods, izone->mods, FALSE);
        voice->mod_list = ipatch_sf2_mod_list_override (tmpmods, cache->override_mods, TRUE);
        ipatch_sf2_mod_list_free (tmpmods, FALSE);
      } /* override global/default mods with zone (copy) */
      else voice->mod_list = ipatch_sf2_mod_list_override (gimods, izone->mods, TRUE);

      /* set MIDI note and velocity ranges */
      amt = &voice->gen_array.values[IPATCH_SF2_GEN_NOTE_RANGE];
      ipatch_sf2_voice_cache_set_voice_range (cache, voice, 0,
				    amt->range.low, amt->range.high);
      amt = &voice->gen_array.values[IPATCH_SF2_GEN_VELOCITY_RANGE];
      ipatch_sf2_voice_cache_set_voice_range (cache, voice, 1,
				    amt->range.low, amt->range.high);

      ipatch_sf2_voice_cache_declare_item (cache, (GObject *)sample);
      ipatch_sf2_voice_set_sample_data (voice, sample->sample_data);

      voice->loop_start = sample->loop_start;
      voice->loop_end = sample->loop_end;
      voice->rate = sample->rate;
      voice->root_note = sample->root_note;
      voice->fine_tune = sample->fine_tune;

      IPATCH_ITEM_RUNLOCK (izone); /* -- UNLOCK izone */
    }

  IPATCH_ITEM_RUNLOCK (inst); /* -- UNLOCK instrument */

  /* free gimods (mod data was not copied) */
  if (gimods != defmods) ipatch_sf2_mod_list_free (gimods, FALSE);

  /* if convert object was instrument zone, unref parent instrument */
  if ((void *)obj != (void *)inst)
    g_object_unref (inst);	/* -- unref parent instrument */

  return (TRUE);
}

/* use instrument to voice cache converter function */
#define _sf2_izone_to_sf2_voice_cache_convert \
  _sf2_inst_to_sf2_voice_cache_convert

static gboolean
_sf2_sample_to_sf2_voice_cache_convert (IpatchConverter *converter,
					GError **err)
{
  IpatchSF2Sample *sample;
  IpatchSF2VoiceCache *cache;
  IpatchSF2Voice *voice;
  IpatchSF2GenAmount *amt;

  sample = IPATCH_SF2_SAMPLE (IPATCH_CONVERTER_INPUT (converter));
  cache = IPATCH_SF2_VOICE_CACHE (IPATCH_CONVERTER_OUTPUT (converter));

  ipatch_sf2_voice_cache_declare_item (cache, (GObject *)sample);

  voice = ipatch_sf2_voice_cache_add_voice (cache);

  voice->mod_list = ipatch_sf2_mod_list_override (cache->default_mods,
                                                  cache->override_mods, TRUE);

  /* set MIDI note and velocity ranges */
  amt = &voice->gen_array.values[IPATCH_SF2_GEN_NOTE_RANGE];
  ipatch_sf2_voice_cache_set_voice_range (cache, voice, 0,
				amt->range.low, amt->range.high);
  amt = &voice->gen_array.values[IPATCH_SF2_GEN_VELOCITY_RANGE];
  ipatch_sf2_voice_cache_set_voice_range (cache, voice, 1,
				amt->range.low, amt->range.high);

  /* use the default loop type for the IpatchSF2VoiceCache */
  voice->gen_array.values[IPATCH_SF2_GEN_SAMPLE_MODES].sword
    = cache->default_loop_type;
  IPATCH_SF2_GEN_ARRAY_SET_FLAG (&voice->gen_array, IPATCH_SF2_GEN_SAMPLE_MODES);

  ipatch_sf2_voice_set_sample_data (voice, sample->sample_data);

  voice->loop_start = sample->loop_start;
  voice->loop_end = sample->loop_end;
  voice->rate = sample->rate;
  voice->root_note = sample->root_note;
  voice->fine_tune = sample->fine_tune;

  return (TRUE);
}

CONVERTER_CLASS_INIT(sf2_preset_to_sf2_voice_cache);
CONVERTER_CLASS_INIT(sf2_pzone_to_sf2_voice_cache);
CONVERTER_CLASS_INIT(sf2_inst_to_sf2_voice_cache);
CONVERTER_CLASS_INIT(sf2_izone_to_sf2_voice_cache);
CONVERTER_CLASS_INIT(sf2_sample_to_sf2_voice_cache);

CONVERTER_SF2_VOICE_CACHE_GET_TYPE(sf2_preset_to_sf2_voice_cache, SF2PresetToSF2VoiceCache);
CONVERTER_SF2_VOICE_CACHE_GET_TYPE(sf2_pzone_to_sf2_voice_cache, SF2PZoneToSF2VoiceCache);
CONVERTER_SF2_VOICE_CACHE_GET_TYPE(sf2_inst_to_sf2_voice_cache, SF2InstToSF2VoiceCache);
CONVERTER_SF2_VOICE_CACHE_GET_TYPE(sf2_izone_to_sf2_voice_cache, SF2IZoneToSF2VoiceCache);
CONVERTER_SF2_VOICE_CACHE_GET_TYPE(sf2_sample_to_sf2_voice_cache, SF2SampleToSF2VoiceCache);
