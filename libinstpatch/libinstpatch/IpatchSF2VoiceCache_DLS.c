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

#include "IpatchSF2VoiceCache_DLS.h"
#include "IpatchConverter.h"
#include "IpatchConverterSF2VoiceCache.h"
#include "IpatchConverter_priv.h"

#include "IpatchDLS2Inst.h"
#include "IpatchDLS2Region.h"
#include "IpatchDLS2Sample.h"
#include "IpatchSF2VoiceCache.h"
#include "IpatchSample.h"

/**
 * _ipatch_sf2_voice_cache_init_DLS: (skip)
 */
void
_ipatch_sf2_voice_cache_init_DLS (void)
{
  g_type_class_ref (IPATCH_TYPE_CONVERTER_DLS2_INST_TO_SF2_VOICE_CACHE);
  g_type_class_ref (IPATCH_TYPE_CONVERTER_DLS2_REGION_TO_SF2_VOICE_CACHE);
  g_type_class_ref (IPATCH_TYPE_CONVERTER_DLS2_SAMPLE_TO_SF2_VOICE_CACHE);

  ipatch_register_converter_map
    (IPATCH_TYPE_CONVERTER_DLS2_INST_TO_SF2_VOICE_CACHE, 0, 0,
     IPATCH_TYPE_DLS2_INST, 0, 1, IPATCH_TYPE_SF2_VOICE_CACHE, 0, 1);
  ipatch_register_converter_map
    (IPATCH_TYPE_CONVERTER_DLS2_REGION_TO_SF2_VOICE_CACHE, 0, 0,
     IPATCH_TYPE_DLS2_REGION, 0, 1, IPATCH_TYPE_SF2_VOICE_CACHE, 0, 1);
  ipatch_register_converter_map
    (IPATCH_TYPE_CONVERTER_DLS2_SAMPLE_TO_SF2_VOICE_CACHE, 0, 0,
     IPATCH_TYPE_DLS2_SAMPLE, 0, 1, IPATCH_TYPE_SF2_VOICE_CACHE, 0, 1);
}

static gboolean
_dls2_inst_to_sf2_voice_cache_convert (IpatchConverter *converter, GError **err)
{
  IpatchDLS2Inst *inst;
  IpatchSF2VoiceCache *cache;
  IpatchSF2Voice *voice;
  IpatchDLS2Region *region;
  IpatchDLS2Sample *sample;
  IpatchDLS2SampleInfo *sample_info;
  IpatchItem *solo_item;
  GObject *obj;
  GSList *p;
  int looptype;

  obj = IPATCH_CONVERTER_INPUT (converter);
  cache = IPATCH_SF2_VOICE_CACHE (IPATCH_CONVERTER_OUTPUT (converter));
  solo_item = ((IpatchConverterSF2VoiceCache *)converter)->solo_item;

  /* check if its an instrument or a region */
  if (IPATCH_IS_DLS2_REGION (obj))      /* ++ ref parent instrument */
    inst = IPATCH_DLS2_INST (ipatch_item_get_parent (IPATCH_ITEM (obj)));
  else inst = IPATCH_DLS2_INST (obj);

  ipatch_sf2_voice_cache_declare_item (cache, (GObject *)inst);

  IPATCH_ITEM_RLOCK (inst);	/* ++ LOCK instrument */

  for (p = inst->regions; p; p = p->next)
    {
      region = (IpatchDLS2Region *)(p->data);

      if (solo_item && (IpatchItem *)region != solo_item) continue;

      ipatch_sf2_voice_cache_declare_item (cache, (GObject *)region);

      voice = ipatch_sf2_voice_cache_add_voice (cache);

      IPATCH_ITEM_RLOCK (region); /* ++ LOCK region */

      /* convert DLS parameters to SoundFont generator array and modulators */
//      ipatch_dls2_region_load_sf2_gen_mods (region, &voice->gen_array,
//                                            &voice->mod_list);

      voice->mod_list = ipatch_sf2_mod_list_override (cache->default_mods,
                                                      cache->override_mods, TRUE);

      /* set MIDI note and velocity ranges */
      ipatch_sf2_voice_cache_set_voice_range (cache, voice, 0,
                                region->note_range_low, region->note_range_high);
      ipatch_sf2_voice_cache_set_voice_range (cache, voice, 1,
                      region->velocity_range_low, region->velocity_range_high);

      sample = (IpatchDLS2Sample *)(region->sample);
      ipatch_sf2_voice_cache_declare_item (cache, (GObject *)sample);

      ipatch_sf2_voice_set_sample_data (voice, sample->sample_data);

      voice->rate = sample->rate;

      if (region->sample_info)
        sample_info = region->sample_info;
      else sample_info = sample->sample_info;

      if (sample_info)
        {
          voice->loop_start = sample_info->loop_start;
          voice->loop_end = sample_info->loop_end;
          voice->root_note = sample_info->root_note;
          voice->fine_tune = sample_info->fine_tune;
  
          switch (sample_info->options & IPATCH_DLS2_SAMPLE_LOOP_MASK)
            {
            case IPATCH_SAMPLE_LOOP_NONE:
              looptype = IPATCH_SF2_GEN_SAMPLE_MODE_NOLOOP;
              break;
            case IPATCH_SAMPLE_LOOP_RELEASE:
              looptype = IPATCH_SF2_GEN_SAMPLE_MODE_LOOP_RELEASE;
              break;
            default:	/* default to standard loop */
              looptype = IPATCH_SF2_GEN_SAMPLE_MODE_LOOP;
              break;
            }
  
          /* set loop mode */
          voice->gen_array.values[IPATCH_SF2_GEN_SAMPLE_MODES].sword = looptype;
          IPATCH_SF2_GEN_ARRAY_SET_FLAG (&voice->gen_array,
                                         IPATCH_SF2_GEN_SAMPLE_MODES);
        }

      IPATCH_ITEM_RUNLOCK (region); /* -- UNLOCK region */
    }

  IPATCH_ITEM_RUNLOCK (inst); /* -- UNLOCK instrument */

  /* if convert object was region, unref parent instrument */
  if ((void *)obj != (void *)inst)
    g_object_unref (inst);	/* -- unref parent instrument */

  return (TRUE);
}

/* use the instrument converter for regions also */
#define _dls2_region_to_sf2_voice_cache_convert \
    _dls2_inst_to_sf2_voice_cache_convert

/**
 * _dls2_sample_to_sf2_voice_cache_convert: (skip)
 *
 * DLS2Sample voice cache converter - Not declared static, since used by
 * IpatchSF2VoiceCache_Gig.c
 */ 
gboolean
_dls2_sample_to_sf2_voice_cache_convert (IpatchConverter *converter,
                                         GError **err)
{
  IpatchDLS2Sample *sample;
  IpatchSF2VoiceCache *cache;
  IpatchSF2Voice *voice;
  IpatchSF2GenAmount *amt;
  int loopmode;

  sample = IPATCH_DLS2_SAMPLE (IPATCH_CONVERTER_INPUT (converter));
  cache = IPATCH_SF2_VOICE_CACHE (IPATCH_CONVERTER_OUTPUT (converter));

  ipatch_sf2_voice_cache_declare_item (cache, (GObject *)sample);

  voice = ipatch_sf2_voice_cache_add_voice (cache);
  voice->mod_list = ipatch_sf2_mod_list_duplicate (cache->default_mods);

  /* set MIDI note and velocity ranges */
  amt = &voice->gen_array.values[IPATCH_SF2_GEN_NOTE_RANGE];
  ipatch_sf2_voice_cache_set_voice_range (cache, voice, 0,
				amt->range.low, amt->range.high);
  amt = &voice->gen_array.values[IPATCH_SF2_GEN_VELOCITY_RANGE];
  ipatch_sf2_voice_cache_set_voice_range (cache, voice, 1,
				amt->range.low, amt->range.high);

  voice->mod_list = ipatch_sf2_mod_list_override (cache->default_mods,
                                                  cache->override_mods, TRUE);

  ipatch_sf2_voice_set_sample_data (voice, sample->sample_data);

  voice->rate = sample->rate;

  if (sample->sample_info)
    {
      voice->loop_start = sample->sample_info->loop_start;
      voice->loop_end = sample->sample_info->loop_end;
      voice->root_note = sample->sample_info->root_note;
      voice->fine_tune = sample->sample_info->fine_tune;

      switch (sample->sample_info->options & IPATCH_DLS2_SAMPLE_LOOP_MASK)
        {
        case IPATCH_SAMPLE_LOOP_NONE:
          loopmode = IPATCH_SF2_GEN_SAMPLE_MODE_NOLOOP;
          break;
        case IPATCH_SAMPLE_LOOP_RELEASE:
          loopmode = IPATCH_SF2_GEN_SAMPLE_MODE_LOOP_RELEASE;
          break;
        default:	/* default to standard loop */
          loopmode = IPATCH_SF2_GEN_SAMPLE_MODE_LOOP;
          break;
        }

      /* set loop mode */
      voice->gen_array.values[IPATCH_SF2_GEN_SAMPLE_MODES].sword = loopmode;
      IPATCH_SF2_GEN_ARRAY_SET_FLAG (&voice->gen_array,
                                     IPATCH_SF2_GEN_SAMPLE_MODES);
    }

  return (TRUE);
}

CONVERTER_CLASS_INIT(dls2_inst_to_sf2_voice_cache);
CONVERTER_CLASS_INIT(dls2_region_to_sf2_voice_cache);
CONVERTER_CLASS_INIT(dls2_sample_to_sf2_voice_cache);

CONVERTER_SF2_VOICE_CACHE_GET_TYPE(dls2_inst_to_sf2_voice_cache, DLS2InstToSF2VoiceCache);
CONVERTER_SF2_VOICE_CACHE_GET_TYPE(dls2_region_to_sf2_voice_cache, DLS2RegionToSF2VoiceCache);
CONVERTER_SF2_VOICE_CACHE_GET_TYPE(dls2_sample_to_sf2_voice_cache, DLS2SampleToSF2VoiceCache);
