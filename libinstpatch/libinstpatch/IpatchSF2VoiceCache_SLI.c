/*
 * libInstPatch
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * Author of this file: (C) 2012 BALATON Zoltan <balaton@eik.bme.hu>
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

#include "IpatchSF2VoiceCache_SLI.h"
#include "IpatchConverter.h"
#include "IpatchConverterSF2VoiceCache.h"
#include "IpatchConverter_priv.h"

#include "IpatchSLIInst.h"
#include "IpatchSLIZone.h"
#include "IpatchSLISample.h"
#include "IpatchSF2VoiceCache.h"
#include "IpatchSample.h"

/**
 * _ipatch_sf2_voice_cache_init_SLI: (skip)
 */
void
_ipatch_sf2_voice_cache_init_SLI (void)
{
  g_type_class_ref (IPATCH_TYPE_CONVERTER_SLI_INST_TO_SF2_VOICE_CACHE);
  g_type_class_ref (IPATCH_TYPE_CONVERTER_SLI_ZONE_TO_SF2_VOICE_CACHE);
  g_type_class_ref (IPATCH_TYPE_CONVERTER_SLI_SAMPLE_TO_SF2_VOICE_CACHE);

  ipatch_register_converter_map
    (IPATCH_TYPE_CONVERTER_SLI_INST_TO_SF2_VOICE_CACHE, 0, 0,
     IPATCH_TYPE_SLI_INST, 0, 1, IPATCH_TYPE_SF2_VOICE_CACHE, 0, 1);
  ipatch_register_converter_map
    (IPATCH_TYPE_CONVERTER_SLI_ZONE_TO_SF2_VOICE_CACHE, 0, 0,
     IPATCH_TYPE_SLI_ZONE, 0, 1, IPATCH_TYPE_SF2_VOICE_CACHE, 0, 1);
  ipatch_register_converter_map
    (IPATCH_TYPE_CONVERTER_SLI_SAMPLE_TO_SF2_VOICE_CACHE, 0, 0,
     IPATCH_TYPE_SLI_SAMPLE, 0, 1, IPATCH_TYPE_SF2_VOICE_CACHE, 0, 1);
}

static gboolean
_sli_inst_to_sf2_voice_cache_convert (IpatchConverter *converter, GError **err)
{
  IpatchSLIInst *inst;
  IpatchSF2VoiceCache *cache;
  IpatchSF2Voice *voice;
  IpatchSLIZone *zone;
  IpatchSLISample *sample;
  IpatchSF2GenAmount *amt;
  IpatchItem *solo_item;
  GObject *obj;
  GSList *p;

  obj = IPATCH_CONVERTER_INPUT (converter);
  cache = IPATCH_SF2_VOICE_CACHE (IPATCH_CONVERTER_OUTPUT (converter));
  solo_item = ((IpatchConverterSF2VoiceCache *)converter)->solo_item;

  /* check if it is an instrument or a zone */
  if (IPATCH_IS_SLI_ZONE (obj))      /* ++ ref parent instrument */
    inst = IPATCH_SLI_INST (ipatch_item_get_parent (IPATCH_ITEM (obj)));
  else inst = IPATCH_SLI_INST (obj);

  ipatch_sf2_voice_cache_declare_item (cache, (GObject *)inst);

  IPATCH_ITEM_RLOCK (inst);	/* ++ LOCK instrument */

  for (p = inst->zones; p; p = p->next) /* loop over zones */
  {
    zone = (IpatchSLIZone *)(p->data);

    /* If a zone is currently solo, skip other zones */
    if (solo_item && (IpatchItem *)zone != solo_item) continue;

    ipatch_sf2_voice_cache_declare_item (cache, (GObject *)zone);

    IPATCH_ITEM_RLOCK (zone); /* ++ LOCK zone */

    sample = (IpatchSLISample *)(zone->sample);
    if (!sample)		/* skip zones without sample ref */
    {
      IPATCH_ITEM_RUNLOCK (zone);	/* -- unlock izone */
      continue;
    }

    voice = ipatch_sf2_voice_cache_add_voice (cache);
    voice->mod_list = ipatch_sf2_mod_list_duplicate (cache->default_mods);

    /* copy generator values */
    memcpy (&voice->gen_array, &zone->genarray, sizeof (IpatchSF2GenArray));

    /* set MIDI note and velocity ranges */
    amt = &voice->gen_array.values[IPATCH_SF2_GEN_NOTE_RANGE];
    ipatch_sf2_voice_cache_set_voice_range (cache, voice, 0,
                                            amt->range.low, amt->range.high);

    amt = &voice->gen_array.values[IPATCH_SF2_GEN_VELOCITY_RANGE];
    ipatch_sf2_voice_cache_set_voice_range (cache, voice, 1,
                                            amt->range.low, amt->range.high);

    ipatch_sf2_voice_cache_declare_item (cache, (GObject *)sample);
    ipatch_sf2_voice_set_sample_data (voice, sample->sample_data);

    voice->rate = sample->rate;
    voice->loop_start = sample->loop_start;
    voice->loop_end = sample->loop_end;
    voice->root_note = sample->root_note;
    voice->fine_tune = sample->fine_tune;

    IPATCH_ITEM_RUNLOCK (zone); /* -- UNLOCK zone */
  }

  IPATCH_ITEM_RUNLOCK (inst); /* -- UNLOCK instrument */

  /* if convert object was a zone, unref parent instrument */
  if ((void *)obj != (void *)inst)
    g_object_unref (inst);	/* -- unref parent instrument */

  return (TRUE);
}

/* use the instrument converter for zone also */
#define _sli_zone_to_sf2_voice_cache_convert \
  _sli_inst_to_sf2_voice_cache_convert

static gboolean
_sli_sample_to_sf2_voice_cache_convert (IpatchConverter *converter,
                                        GError **err)
{
  IpatchSLISample *sample;
  IpatchSF2VoiceCache *cache;
  IpatchSF2Voice *voice;
  IpatchSF2GenAmount *amt;

  sample = IPATCH_SLI_SAMPLE (IPATCH_CONVERTER_INPUT (converter));
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

  /* use the default loop type for the IpatchSF2VoiceCache */
  voice->gen_array.values[IPATCH_SF2_GEN_SAMPLE_MODES].sword
    = cache->default_loop_type;
  IPATCH_SF2_GEN_ARRAY_SET_FLAG (&voice->gen_array, IPATCH_SF2_GEN_SAMPLE_MODES);

  ipatch_sf2_voice_set_sample_data (voice, sample->sample_data);

  voice->rate = sample->rate;
  voice->loop_start = sample->loop_start;
  voice->loop_end = sample->loop_end;
  voice->root_note = sample->root_note;
  voice->fine_tune = sample->fine_tune;

  return (TRUE);
}

CONVERTER_CLASS_INIT(sli_inst_to_sf2_voice_cache);
CONVERTER_CLASS_INIT(sli_zone_to_sf2_voice_cache);
CONVERTER_CLASS_INIT(sli_sample_to_sf2_voice_cache);

CONVERTER_SF2_VOICE_CACHE_GET_TYPE(sli_inst_to_sf2_voice_cache, SLIInstToSF2VoiceCache);
CONVERTER_SF2_VOICE_CACHE_GET_TYPE(sli_zone_to_sf2_voice_cache, SLIZoneToSF2VoiceCache);
CONVERTER_SF2_VOICE_CACHE_GET_TYPE(sli_sample_to_sf2_voice_cache, SLISampleToSF2VoiceCache);
