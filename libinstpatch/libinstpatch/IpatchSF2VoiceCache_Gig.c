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
 * SECTION: IpatchSF2VoiceCache_Gig
 * @short_description: Voice cache converters for GigaSampler object types
 * @see_also: #IpatchSF2VoiceCache
 * @stability: Stable
 */
#include <stdio.h>
#include <glib.h>
#include <glib-object.h>

#include "IpatchSF2VoiceCache_Gig.h"
#include "IpatchSF2VoiceCache.h"
#include "IpatchConverter_priv.h"
#include "IpatchConverterSF2VoiceCache.h"

#include "IpatchDLS2Inst.h"
#include "IpatchDLS2Region.h"
#include "IpatchDLS2Sample.h"
#include "IpatchSF2Mod.h"
#include "IpatchGig.h"
#include "IpatchGigInst.h"
#include "IpatchGigRegion.h"
#include "IpatchGigEffects.h"
#include "IpatchSample.h"

// In IpatchSF2VoiceCache_DLS.c
gboolean _dls2_sample_to_sf2_voice_cache_convert (IpatchConverter *converter,
                                                  GError **err);

/**
 * _ipatch_sf2_voice_cache_init_gig: (skip)
 */
void
_ipatch_sf2_voice_cache_init_gig (void)
{
  g_type_class_ref (IPATCH_TYPE_CONVERTER_GIG_INST_TO_SF2_VOICE_CACHE);
  g_type_class_ref (IPATCH_TYPE_CONVERTER_GIG_SAMPLE_TO_SF2_VOICE_CACHE);

  ipatch_register_converter_map
    (IPATCH_TYPE_CONVERTER_GIG_INST_TO_SF2_VOICE_CACHE, 0, 0,
     IPATCH_TYPE_GIG_INST, 0, 1, IPATCH_TYPE_SF2_VOICE_CACHE, 0, 1);
  ipatch_register_converter_map
    (IPATCH_TYPE_CONVERTER_GIG_SAMPLE_TO_SF2_VOICE_CACHE, 0, 0,
     IPATCH_TYPE_GIG_SAMPLE, 0, 1, IPATCH_TYPE_SF2_VOICE_CACHE, 0, 1);
}

static gboolean
_gig_inst_to_sf2_voice_cache_convert (IpatchConverter *converter, GError **err)
{
  IpatchSF2VoiceCache *cache;
  IpatchSF2Voice *voice;
  IpatchDLS2Inst *inst;
  IpatchGigRegion *region;
  IpatchDLS2Region *dls_region;
  IpatchGigDimension *dimension;
  IpatchGigSubRegion *sub_region;
  IpatchDLS2Sample *sample;
  IpatchDLS2SampleInfo *sample_info;
  IpatchSF2VoiceSelInfo sel_info[IPATCH_SF2_VOICE_CACHE_MAX_SEL_VALUES];
  IpatchSF2VoiceSelInfo *infop;
  GHashTable *sel_index_hash;	/* dimension type -> selection index */
  int sel_count = 1;	/* 1 for note selection criteria */
  int dim_type;
  int index, sub_count;
  int i, i2, v;
  GSList *p;

  sel_index_hash = g_hash_table_new (NULL, NULL);

  cache = IPATCH_SF2_VOICE_CACHE (IPATCH_CONVERTER_OUTPUT (converter));

  inst = IPATCH_DLS2_INST (IPATCH_CONVERTER_INPUT (converter));
  ipatch_sf2_voice_cache_declare_item (cache, (GObject *)inst);

  sel_info[0].type = IPATCH_SF2_VOICE_SEL_NOTE; /* always have note selection */
  infop = &sel_info[1];

  IPATCH_ITEM_RLOCK (inst);	/* ++ LOCK instrument */

  /* determine all selection criteria types */
  p = inst->regions;
  for (; p; p = p->next)	/* loop over DLS regions */
    {
      region = (IpatchGigRegion *)(p->data);
      ipatch_sf2_voice_cache_declare_item (cache, (GObject *)region);

      /* NOTE: Dimensions and sub regions share the same mutex as region */
      IPATCH_ITEM_RLOCK (region); /* ++ LOCK region */

      for (i = 0; i < region->dimension_count; i++)  /* loop on dimensions */
	{
	  ipatch_sf2_voice_cache_declare_item (cache,
					(GObject *)(region->dimensions[i]));
	  dim_type = region->dimensions[i]->type;

	  /* channel region types don't entail selection criteria, but rather
	     channel audio routing */
	  if (dim_type == IPATCH_GIG_DIMENSION_CHANNEL) continue;

	  /* already added this region selection type? */
	  if (g_hash_table_lookup (sel_index_hash, GINT_TO_POINTER (dim_type)))
	    continue;

	  infop->type = -1;
	  switch (dim_type)
	    {
	    case IPATCH_GIG_DIMENSION_VELOCITY:
	      infop->type = IPATCH_SF2_VOICE_SEL_VELOCITY;
	      break;
	    case IPATCH_GIG_DIMENSION_AFTER_TOUCH:
	      infop->type = IPATCH_SF2_VOICE_SEL_AFTER_TOUCH;
	      break;
	    case IPATCH_GIG_DIMENSION_RELEASE_TRIG:
	      break;
	    case IPATCH_GIG_DIMENSION_KEYBOARD:
	      break;
	    case IPATCH_GIG_DIMENSION_ROUND_ROBIN:
	      break;
	    case IPATCH_GIG_DIMENSION_RANDOM:
	      break;
	    default:
	      if (dim_type < 0x80)
		{
		  infop->type = IPATCH_SF2_VOICE_SEL_MIDI_CC;
		  infop->param1 = dim_type;	/* type is MIDI CC # */
		}
	      break;
	    }

	  /* advance if selection criteria stored */
	  if (infop->type != -1)
	    { /* add to dim_type -> index hash + 1 (sel_count already + 1) */
	      g_hash_table_insert (sel_index_hash, GINT_TO_POINTER(dim_type),
				   GINT_TO_POINTER (sel_count));
	      infop++;
	      sel_count++;
	    }
	  else g_warning ("Unhandled Gig region type %d", dim_type);

	  /* max number of selection types reached? */
	  if (infop == &sel_info[IPATCH_SF2_VOICE_CACHE_MAX_SEL_VALUES])
	    {
	      g_warning ("Max voice selection types reached!");
	      break;
	    }
	}  /* dimension loop */

      IPATCH_ITEM_RUNLOCK (region); /* -- UNLOCK region */
    }  /* region loop */

  p = inst->regions;
  for (; p; p = p->next)	/* loop over DLS regions */
    {
      region = (IpatchGigRegion *)(p->data);
      dls_region = (IpatchDLS2Region *)region;

      IPATCH_ITEM_RLOCK (region); /* ++ LOCK region */

      /* loop over all sub regions */
      for (i = 0; i < region->sub_region_count; i++)
	{
	  sub_region = region->sub_regions[i];
	  ipatch_sf2_voice_cache_declare_item (cache, (GObject *)sub_region);

	  voice = ipatch_sf2_voice_cache_add_voice (cache);

	  /* convert GigaSampler effects to SoundFont 2 generator array */
	  ipatch_gig_effects_to_gen_array (&sub_region->effects,
					   &voice->gen_array);
	  /* set note range */
	  ipatch_sf2_voice_cache_set_voice_range (cache, voice, 0,
			dls_region->note_range_low, dls_region->note_range_high);

	  /* loop over dimension splits and set selection ranges */
	  for (i2 = 0; i2 < region->dimension_count; i2++)
	    {
	      dimension = region->dimensions[i2];
	      dim_type = dimension->type;

	      /* find selection info index for gig dimension type */
	      index = GPOINTER_TO_INT (g_hash_table_lookup (sel_index_hash,
						 GINT_TO_POINTER (dim_type)));
	      if (!index) continue;	/* not handled (FIXME) */

	      /* split index for this dimension */
	      v = (i & dimension->split_mask) >> dimension->split_shift;

	      /* dimension sub region count */
	      sub_count = (1 << dimension->split_count);

	      /* set range based on dimension split index */
	      ipatch_sf2_voice_cache_set_voice_range (cache, voice, index,
		    128 * v / sub_count, 128 * (v + 1) / sub_count - 1);
	    }

          voice->mod_list = ipatch_sf2_mod_list_override (cache->default_mods,
                                                          cache->override_mods, TRUE);

	  sample = (IpatchDLS2Sample *)(sub_region->sample);
	  ipatch_sf2_voice_cache_declare_item (cache, (GObject *)sample);

	  if (sub_region->sample_info)
	    sample_info = sub_region->sample_info;
	  else sample_info = sample->sample_info;

	  /* FIXME - what about stereo routing? */

          /* Assign sample */
          ipatch_sf2_voice_set_sample_data (voice, sample->sample_data);

	  /* copy sample parameters */
	  voice->rate = sample->rate;

	  if (sample_info)
	    {
	      voice->loop_start = sample_info->loop_start;
	      voice->loop_end = sample_info->loop_end;
	      voice->root_note = sample_info->root_note;
	      voice->fine_tune = sample_info->fine_tune;

	      switch (sample_info->options & IPATCH_DLS2_SAMPLE_LOOP_MASK)
		{
		case IPATCH_SAMPLE_LOOP_NONE:
		  v = IPATCH_SF2_GEN_SAMPLE_MODE_NOLOOP;
		  break;
		case IPATCH_SAMPLE_LOOP_RELEASE:
		  v = IPATCH_SF2_GEN_SAMPLE_MODE_LOOP_RELEASE;
		  break;
		default:	/* default to standard loop */
		  v = IPATCH_SF2_GEN_SAMPLE_MODE_LOOP;
		  break;
		}

	      /* set loop mode */
	      voice->gen_array.values[IPATCH_SF2_GEN_SAMPLE_MODES].sword = v;
	      IPATCH_SF2_GEN_ARRAY_SET_FLAG (&voice->gen_array,
					     IPATCH_SF2_GEN_SAMPLE_MODES);
	    }
	}	// sub region loop

      IPATCH_ITEM_RUNLOCK (region); /* -- UNLOCK region */
    }	// region loop

  IPATCH_ITEM_RUNLOCK (inst);	/* -- UNLOCK instrument */

  g_hash_table_destroy (sel_index_hash);

  return (TRUE);
}

/* use DLS sample converter function for Gig samples */
#define _gig_sample_to_sf2_voice_cache_convert \
  _dls2_sample_to_sf2_voice_cache_convert

CONVERTER_CLASS_INIT(gig_inst_to_sf2_voice_cache);
CONVERTER_CLASS_INIT(gig_sample_to_sf2_voice_cache);

CONVERTER_SF2_VOICE_CACHE_GET_TYPE(gig_inst_to_sf2_voice_cache, GigInstToSF2VoiceCache);
CONVERTER_SF2_VOICE_CACHE_GET_TYPE(gig_sample_to_sf2_voice_cache, GigSampleToSF2VoiceCache);
