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
 * SECTION: IpatchSF2VoiceCache_VBank
 * @short_description: Voice cache converters for VBank types
 * @see_also: 
 * @stability: Stable
 */
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include "IpatchSF2VoiceCache_VBank.h"
#include "IpatchConverter.h"
#include "IpatchConverter_priv.h"

#include "IpatchSF2VoiceCache.h"
#include "IpatchVBankInst.h"
#include "IpatchVBankRegion.h"
#include "misc.h"
#include "i18n.h"

/**
 * _ipatch_sf2_voice_cache_init_VBank: (skip)
 */
void
_ipatch_sf2_voice_cache_init_VBank (void)
{
  g_type_class_ref (IPATCH_TYPE_CONVERTER_VBANK_INST_TO_SF2_VOICE_CACHE);
  g_type_class_ref (IPATCH_TYPE_CONVERTER_VBANK_REGION_TO_SF2_VOICE_CACHE);

  ipatch_register_converter_map
    (IPATCH_TYPE_CONVERTER_VBANK_INST_TO_SF2_VOICE_CACHE, 0, 0,
     IPATCH_TYPE_VBANK_INST, 0, 1, IPATCH_TYPE_SF2_VOICE_CACHE, 0, 1);
  ipatch_register_converter_map
    (IPATCH_TYPE_CONVERTER_VBANK_REGION_TO_SF2_VOICE_CACHE, 0, 0,
     IPATCH_TYPE_VBANK_REGION, 0, 1, IPATCH_TYPE_SF2_VOICE_CACHE, 0, 1);
}

static gboolean
_vbank_inst_to_sf2_voice_cache_convert (IpatchConverter *converter,
				        GError **err)
{
  IpatchVBankInst *inst;
  IpatchSF2VoiceCache *cache, *item_vcache;
  IpatchVBankRegion *region;
  IpatchConverter *itemconv;
  IpatchSF2Voice *voice, *item_voice;
  GObject *obj;
  GSList *p;
  int note_index;
  int *note_range;
  int note_low, note_high;
  GError *localerr = NULL;
  int voicendx;
  int root_note;

  obj = IPATCH_CONVERTER_INPUT (converter);
  cache = IPATCH_SF2_VOICE_CACHE (IPATCH_CONVERTER_OUTPUT (converter));

  /* find which selection index is the MIDI note range */
  for (note_index = 0; note_index < cache->sel_count; note_index++)
    if (cache->sel_info[note_index].type == IPATCH_SF2_VOICE_SEL_NOTE) break;

  if (note_index == cache->sel_count) note_index = -1;
  else note_index *= 2;	/* convert to integer pair offset in ranges array */

  /* check if its a vbank instrument or region */
  if (IPATCH_IS_VBANK_REGION (obj))	/* ++ ref parent instrument */
    inst = IPATCH_VBANK_INST (ipatch_item_get_parent (IPATCH_ITEM (obj)));
  else inst = IPATCH_VBANK_INST (obj);

  /* declare the instrument as a dependent item */
  ipatch_sf2_voice_cache_declare_item (cache, (GObject *)inst);

  IPATCH_ITEM_RLOCK (inst);     /* ++ lock instrument */

  for (p = inst->regions; p; p = p->next)
  {
    region = (IpatchVBankRegion *)(p->data);

    IPATCH_ITEM_RLOCK (region); /* ++ lock region */

    /* ++ ref new converter for region's item */
    itemconv = ipatch_create_converter (G_OBJECT_TYPE (region->item),
					IPATCH_TYPE_SF2_VOICE_CACHE);

    /* skip any regions with un-synthesizable items - and log a warning */
    if (!itemconv)
    {
      IPATCH_ITEM_RUNLOCK (region);     /* -- unlock region */
      ipatch_converter_log (converter, G_OBJECT (region), IPATCH_CONVERTER_LOG_WARN,
			    _("No voice handler for region item"));
      continue;
    }

    /* ++ ref - create voice cache for region's referenced item */
    item_vcache = ipatch_sf2_voice_cache_new (cache->sel_info, cache->sel_count);

    ipatch_converter_add_input (itemconv, (GObject *)(region->item));
    ipatch_converter_add_output (itemconv, (GObject *)item_vcache);

    if (!ipatch_converter_convert (itemconv, &localerr))
    {
      IPATCH_ITEM_RUNLOCK (region);     /* -- unlock region */
      ipatch_converter_log_printf (converter, G_OBJECT (region),
				   IPATCH_CONVERTER_LOG_WARN,
				   _("Failed to convert region item to voices: %s"),
				   ipatch_gerror_message (localerr));
      g_clear_error (&localerr);
      g_object_unref (itemconv);	/* -- unref */
      g_object_unref (item_vcache);	/* -- unref */
      continue;
    }

    g_object_unref (itemconv);		/* -- unref item converter */

    note_low = region->note_range.low;
    note_high = region->note_range.high;

    /* loop over voices in item's voice cache */
    for (voicendx = 0; voicendx < item_vcache->voices->len; voicendx++)
    {
      item_voice = IPATCH_SF2_VOICE_CACHE_GET_VOICE (item_vcache, voicendx);

      if (note_index >= 0)
	note_range = &g_array_index (item_vcache->ranges, int,
				     item_voice->range_index + note_index);
      else note_range = NULL;

      /* if intersect note range mode.. */
      if (region->note_range_mode == IPATCH_VBANK_REGION_NOTE_RANGE_MODE_INTERSECT
	  && note_range)
      {	/* note ranges are exclusive? - skip voice */
	if (note_range[0] != IPATCH_SF2_VOICE_SEL_WILDCARD
	    && note_range[1] != IPATCH_SF2_VOICE_SEL_WILDCARD
	    && ((note_low < note_range[0] && note_high < note_range[0])
		|| (note_low > note_range[1] && note_high > note_range[1])))
	  continue;
      }

      voice = ipatch_sf2_voice_cache_add_voice (cache);

      ipatch_sf2_voice_copy (voice, item_voice);

      /* copy voice selection criteria */
      memcpy (&g_array_index (cache->ranges, int, voice->range_index),
	      &g_array_index (item_vcache->ranges, int, item_voice->range_index),
	      2 * cache->sel_count * sizeof (int));

      /* modify note range depending on mode */
      if (note_index >= 0)
      {
	note_range = &g_array_index (cache->ranges, int,
				     voice->range_index + note_index);

	if (region->note_range_mode == IPATCH_VBANK_REGION_NOTE_RANGE_MODE_INTERSECT)
	{
	  note_range[0] = MAX (note_low, note_range[0]);
	  note_range[1] = MIN (note_high, note_range[1]);
	}
	else	/* IPATCH_VBANK_REGION_NOTE_RANGE_MODE_OVERRIDE */
	{
	  note_range[0] = note_low;
	  note_range[1] = note_high;
	}
      }

      /* modify root note tuning depending on mode */
      if (region->root_note_mode == IPATCH_VBANK_REGION_ROOT_NOTE_MODE_OFFSET)
      {
	root_note = voice->root_note + region->root_note;
	voice->root_note = CLAMP (root_note, 0, 127);
      }		/* IPATCH_VBANK_REGION_ROOT_NOTE_MODE_OVERRIDE */
      else voice->root_note = region->root_note;
    } /* for voicendx in item_vcache->voices */

    IPATCH_ITEM_RUNLOCK (region);       /* -- unlock region */

    g_object_unref (item_vcache);	/* -- unref item voice cache */
  } /* for p in regions */

  IPATCH_ITEM_RUNLOCK (inst);           /* -- unlock instrument */

  /* if convert object was vbank region, unref parent instrument */
  if ((void *)obj != (void *)inst)
    g_object_unref (inst);	/* -- unref parent instrument */

  return (TRUE);
}

#define _vbank_region_to_sf2_voice_cache_convert \
  _vbank_inst_to_sf2_voice_cache_convert


CONVERTER_CLASS_INIT(vbank_inst_to_sf2_voice_cache);
CONVERTER_CLASS_INIT(vbank_region_to_sf2_voice_cache);

CONVERTER_GET_TYPE(vbank_inst_to_sf2_voice_cache, VBankInstToSF2VoiceCache);
CONVERTER_GET_TYPE(vbank_region_to_sf2_voice_cache, VBankRegionToSF2VoiceCache);
