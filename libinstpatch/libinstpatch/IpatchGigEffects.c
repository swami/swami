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
 * SECTION: IpatchGigEffects
 * @short_description: GigaSampler instrument parameters and effects
 * @see_also: 
 * @stability: Stable
 *
 * Functions and types for GigaSampler instrument parameters and effects.
 */
#include <glib.h>
#include <glib-object.h>
#include <string.h>

#include "IpatchGigEffects.h"

/**
 * ipatch_gig_parse_effects:
 * @handle: File handle containing buffered 3ewa data
 * @effects: (out): Pointer to a user supplied GigaSampler effects structure to fill
 *
 * Parse an 3ewa GigaSampler effects chunk into a structure.
 */
void
ipatch_gig_parse_effects (IpatchFileHandle *handle, IpatchGigEffects *effects)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (effects != NULL);

  effects->unknown1 = ipatch_file_buf_read_u32 (handle); /* 0-4 */

  effects->lfo3_freq = ipatch_file_buf_read_u32 (handle);
  effects->eg3_attack = ipatch_file_buf_read_u32 (handle);

  effects->unknown2 = ipatch_file_buf_read_u16 (handle); /* 12-13 */

  effects->lfo1_internal_depth = ipatch_file_buf_read_u16 (handle);

  effects->unknown3 = ipatch_file_buf_read_u16 (handle); /* 16-17 */

  effects->lfo3_internal_depth = ipatch_file_buf_read_u16 (handle);

  effects->unknown4 = ipatch_file_buf_read_u16 (handle); /* 20-21 */

  effects->lfo1_ctrl_depth = ipatch_file_buf_read_u16 (handle);

  effects->unknown5 = ipatch_file_buf_read_u16 (handle); /* 22-23 */

  effects->lfo3_ctrl_depth = ipatch_file_buf_read_u16 (handle);
  effects->eg1_attack = ipatch_file_buf_read_u32 (handle);
  effects->eg1_decay = ipatch_file_buf_read_u32 (handle);

  effects->unknown6 = ipatch_file_buf_read_u16 (handle); /* 36-37 */

  effects->eg1_sustain = ipatch_file_buf_read_u16 (handle);
  effects->eg1_release = ipatch_file_buf_read_u32 (handle);

  effects->unknown7 = ipatch_file_buf_read_u32 (handle); /* 44-47 */

  effects->lfo1_freq = ipatch_file_buf_read_u32 (handle);
  effects->eg2_attack = ipatch_file_buf_read_u32 (handle);
  effects->eg2_decay = ipatch_file_buf_read_u32 (handle);

  effects->unknown8 = ipatch_file_buf_read_u16 (handle); /* 60-61 */

  effects->eg2_sustain = ipatch_file_buf_read_u16 (handle);
  effects->eg2_release = ipatch_file_buf_read_u32 (handle);

  effects->unknown9 = ipatch_file_buf_read_u16 (handle); /* 68-69 */

  effects->lfo2_ctrl_depth = ipatch_file_buf_read_u16 (handle);
  effects->lfo2_freq = ipatch_file_buf_read_u32 (handle);

  effects->unknown10 = ipatch_file_buf_read_u16 (handle); /* 76-77 */

  effects->lfo2_internal_depth = ipatch_file_buf_read_u16 (handle);
  effects->eg1_decay2 = ipatch_file_buf_read_u32 (handle);

  effects->unknown11 = ipatch_file_buf_read_u16 (handle); /* 84-85 */

  effects->eg1_pre_attack = ipatch_file_buf_read_u16 (handle);
  effects->eg2_decay2 = ipatch_file_buf_read_u32 (handle);
  effects->turbo_lowpass = ipatch_file_buf_read_u8 (handle);

  effects->unknown12 = ipatch_file_buf_read_u8 (handle); /* 93 */

  effects->eg2_pre_attack = ipatch_file_buf_read_u16 (handle);
  effects->vel_response = ipatch_file_buf_read_u8 (handle);
  effects->release_vel_response = ipatch_file_buf_read_u8 (handle);

  effects->unknown13 = ipatch_file_buf_read_u16 (handle); /* 98-99 */
  effects->unknown14 = ipatch_file_buf_read_u32 (handle); /* 100-103 */

  effects->sample_offset = ipatch_file_buf_read_u16 (handle);

  effects->unknown15 = ipatch_file_buf_read_u16 (handle); /* 106-107 */

  effects->pitch_track_dim_bypass = ipatch_file_buf_read_u8 (handle);
  effects->layer_pan = ipatch_file_buf_read_u8 (handle);
  effects->self_mask = ipatch_file_buf_read_u8 (handle);

  effects->unknown16 = ipatch_file_buf_read_u8 (handle); /* 111 */

  effects->lfo3_ctrl = ipatch_file_buf_read_u8 (handle);
  effects->attn_ctrl = ipatch_file_buf_read_u8 (handle);
  effects->lfo2_ctrl = ipatch_file_buf_read_u8 (handle);
  effects->lfo1_ctrl = ipatch_file_buf_read_u8 (handle);

  effects->unknown17 = ipatch_file_buf_read_u16 (handle); /* 116-117 */

  effects->lfo3_internal_depth = ipatch_file_buf_read_u16 (handle);
  effects->channel_offset = ipatch_file_buf_read_u8 (handle);
  effects->sust_defeat = ipatch_file_buf_read_u8 (handle);

  effects->unknown18 = ipatch_file_buf_read_u16 (handle); /* 122-123 */

  effects->max_velocity = ipatch_file_buf_read_u8 (handle);

  effects->unknown19 = ipatch_file_buf_read_u8 (handle); /* 125 */
  effects->unknown20 = ipatch_file_buf_read_u16 (handle); /* 126-127 */

  effects->release_trigger_decay = ipatch_file_buf_read_u8 (handle);

  effects->unknown21 = ipatch_file_buf_read_u8 (handle); /* 129 */
  effects->unknown22 = ipatch_file_buf_read_u8 (handle); /* 130 */

  effects->eg1_hold = ipatch_file_buf_read_u8 (handle);
  effects->filter_cutoff = ipatch_file_buf_read_u8 (handle);
  effects->filter_midi_ctrl = ipatch_file_buf_read_u8 (handle);
  effects->filter_vel_scale = ipatch_file_buf_read_u8 (handle);

  effects->unknown23 = ipatch_file_buf_read_u8 (handle); /* 135 */

  effects->filter_resonance = ipatch_file_buf_read_u8 (handle);
  effects->filter_breakpoint = ipatch_file_buf_read_u8 (handle);
  effects->vel_dyn_range = ipatch_file_buf_read_u8 (handle);
  effects->filter_type = ipatch_file_buf_read_u8 (handle);
}

/**
 * ipatch_gig_write_effects:
 * @handle: File handle to buffer writes to, should be committed after this call
 * @effects: Pointer to GigaSampler effects structure to store
 *
 * Store a 3ewa GigaSampler effects chunk into a file buffer.  The file
 * buffer should be at least #IPATCH_GIG_3EWA_SIZE bytes.
 */
void
ipatch_gig_store_effects (IpatchFileHandle *handle, IpatchGigEffects *effects)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (effects != NULL);

  ipatch_file_buf_write_u32 (handle, effects->unknown1); /* 0-3 */

  ipatch_file_buf_write_u32 (handle, effects->lfo3_freq);
  ipatch_file_buf_write_u32 (handle, effects->eg3_attack);

  ipatch_file_buf_write_u16 (handle, effects->unknown2); /* 12-13 */

  ipatch_file_buf_write_u16 (handle, effects->lfo1_internal_depth);

  ipatch_file_buf_write_u16 (handle, effects->unknown3); /* 16-17 unknown */

  ipatch_file_buf_write_u16 (handle, effects->lfo3_internal_depth);

  ipatch_file_buf_write_u16 (handle, effects->unknown4); /* 20-21 unknown */

  ipatch_file_buf_write_u16 (handle, effects->lfo1_ctrl_depth);

  ipatch_file_buf_write_u16 (handle, effects->unknown5); /* 24-25 unknown */

  ipatch_file_buf_write_u16 (handle, effects->lfo3_ctrl_depth);
  ipatch_file_buf_write_u32 (handle, effects->eg1_attack);
  ipatch_file_buf_write_u32 (handle, effects->eg1_decay);

  ipatch_file_buf_write_u16 (handle, effects->unknown6); /* 36-37 unknown */

  ipatch_file_buf_write_u16 (handle, effects->eg1_sustain);
  ipatch_file_buf_write_u32 (handle, effects->eg1_release);

  ipatch_file_buf_write_u32 (handle, effects->unknown7); /* 44-47 unknown */

  ipatch_file_buf_write_u32 (handle, effects->lfo1_freq);
  ipatch_file_buf_write_u32 (handle, effects->eg2_attack);
  ipatch_file_buf_write_u32 (handle, effects->eg2_decay);

  ipatch_file_buf_write_u16 (handle, effects->unknown8); /* 60-61 unknown */

  ipatch_file_buf_write_u16 (handle, effects->eg2_sustain);
  ipatch_file_buf_write_u32 (handle, effects->eg2_release);

  ipatch_file_buf_write_u16 (handle, effects->unknown9); /* 68-69 unknown */

  ipatch_file_buf_write_u16 (handle, effects->lfo2_ctrl_depth);
  ipatch_file_buf_write_u32 (handle, effects->lfo2_freq);

  ipatch_file_buf_write_u16 (handle, effects->unknown10); /* 76-77 unknown */

  ipatch_file_buf_write_u16 (handle, effects->lfo2_internal_depth);
  ipatch_file_buf_write_u32 (handle, effects->eg1_decay2);

  ipatch_file_buf_write_u16 (handle, effects->unknown11); /* 84-85 unknown */

  ipatch_file_buf_write_u16 (handle, effects->eg1_pre_attack);
  ipatch_file_buf_write_u32 (handle, effects->eg2_decay2);
  ipatch_file_buf_write_u8 (handle, effects->turbo_lowpass);

  ipatch_file_buf_write_u8 (handle, effects->unknown12); /* 93 unknown */

  ipatch_file_buf_write_u16 (handle, effects->eg2_pre_attack);
  ipatch_file_buf_write_u8 (handle, effects->vel_response);
  ipatch_file_buf_write_u8 (handle, effects->release_vel_response);

  ipatch_file_buf_write_u16 (handle, effects->unknown13); /* 98-99 unknown */
  ipatch_file_buf_write_u32 (handle, effects->unknown14);/* 100-103 unknown */

  ipatch_file_buf_write_u16 (handle, effects->sample_offset);

  ipatch_file_buf_write_u16 (handle, effects->unknown15);/* 106-107 unknown */

  ipatch_file_buf_write_u8 (handle, effects->pitch_track_dim_bypass);
  ipatch_file_buf_write_u8 (handle, effects->layer_pan);
  ipatch_file_buf_write_u8 (handle, effects->self_mask);

  ipatch_file_buf_write_u8 (handle, effects->unknown16); /* 111 unknown */

  ipatch_file_buf_write_u8 (handle, effects->lfo3_ctrl);
  ipatch_file_buf_write_u8 (handle, effects->attn_ctrl);
  ipatch_file_buf_write_u8 (handle, effects->lfo2_ctrl);
  ipatch_file_buf_write_u8 (handle, effects->lfo1_ctrl);

  ipatch_file_buf_write_u16 (handle, effects->unknown17); /* 116-117 unknown */

  ipatch_file_buf_write_u16 (handle, effects->lfo3_internal_depth);
  ipatch_file_buf_write_u8 (handle, effects->channel_offset);
  ipatch_file_buf_write_u8 (handle, effects->sust_defeat);

  ipatch_file_buf_write_u16 (handle, effects->unknown18);/* 122-123 unknown */

  ipatch_file_buf_write_u8 (handle, effects->max_velocity);

  ipatch_file_buf_write_u8 (handle, effects->unknown19); /* 125 unknown */
  ipatch_file_buf_write_u16 (handle, effects->unknown20);/* 126-127 unknown */

  ipatch_file_buf_write_u8 (handle, effects->release_trigger_decay);

  ipatch_file_buf_write_u8 (handle, effects->unknown21); /* 129 unknown */
  ipatch_file_buf_write_u8 (handle, effects->unknown22); /* 130 unknown */

  ipatch_file_buf_write_u8 (handle, effects->eg1_hold);
  ipatch_file_buf_write_u8 (handle, effects->filter_cutoff);
  ipatch_file_buf_write_u8 (handle, effects->filter_midi_ctrl);
  ipatch_file_buf_write_u8 (handle, effects->filter_vel_scale);

  ipatch_file_buf_write_u8 (handle, effects->unknown23); /* 135 unknown */

  ipatch_file_buf_write_u8 (handle, effects->filter_resonance);
  ipatch_file_buf_write_u8 (handle, effects->filter_breakpoint);
  ipatch_file_buf_write_u8 (handle, effects->vel_dyn_range);
  ipatch_file_buf_write_u8 (handle, effects->filter_type);
}

/**
 * ipatch_gig_effects_init:
 * @effects: GigaSampler effects structure
 *
 * Initialize GigaSampler effects structure to default values.
 */
void
ipatch_gig_effects_init (IpatchGigEffects *effects)
{
  memset (effects, 0, sizeof (IpatchGigEffects));
  /* FIXME - initialize to defaults */
}

/*
 * GigaSampler has independent Volume/Pitch/Filter envelopes and LFOs where
 * as SoundFont has a Volume Envelope and combined Pitch/Filter envelope,
 * and a combined Volume/Filter/Pitch LFO and a second Pitch LFO.
 *
 * - Filter parameters are only activated if the filter is of type lowpass.
 * - Filter envelope parameters take precedence over Pitch envelope
 * - Second SoundFont pitch LFO is always used for Gig pitch LFO
 * - Volume LFO parameters take precedence over filter parameters
 */

/**
 * ipatch_gig_effects_to_gen_array:
 * @effects: GigaSampler effects structure
 * @array: SoundFont generator array
 *
 * Convert GigaSampler effects structure to SoundFont generator array
 */
void
ipatch_gig_effects_to_gen_array (IpatchGigEffects *effects,
				 IpatchSF2GenArray *array)
{
  guint64 set_vals =
    IPATCH_SF2_GENID_SET (IPATCH_SF2_GEN_VOL_ENV_ATTACK)
    | IPATCH_SF2_GENID_SET (IPATCH_SF2_GEN_VOL_ENV_DECAY)
    | IPATCH_SF2_GENID_SET (IPATCH_SF2_GEN_VOL_ENV_RELEASE);

  IpatchSF2GenAmount *vals = array->values;

  array->flags |= set_vals;

  /* volume envelope */

  /* can we do something with the pre-attack level? */

  vals[IPATCH_SF2_GEN_VOL_ENV_ATTACK].sword =
    ipatch_gig_to_sf2_timecents (effects->eg1_attack);

  /* GigaSampler doesn't have a hold stage, only a hold until loop toggle */
  /* FIXME - we could calculate time until loop */

  vals[IPATCH_SF2_GEN_VOL_ENV_DECAY].sword =
    ipatch_gig_to_sf2_timecents (effects->eg1_decay);

  /* can we do something with decay2? */

  /*
  vals[IPATCH_SF2_GEN_VOL_ENV_SUSTAIN].sword =
    ipatch_gig_volsust_to_sf2_centibels (effects->eg1_sustain);
  */

  vals[IPATCH_SF2_GEN_VOL_ENV_RELEASE].sword =
    ipatch_gig_to_sf2_timecents (effects->eg1_release);


  /* filter envelope */

#if 0
  /* only use filter parameters if its of lowpass type */
  if (effects->filter_type == IPATCH_GIG_FILTER_LOWPASS)
    {
      vals[IPATCH_SF2_GEN_MOD_ENV_ATTACK].sword =
	ipatch_gig_to_sf2_timecents (effects->eg2_attack);

      vals[IPATCH_SF2_GEN_MOD_ENV_DECAY].sword =
	ipatch_gig_to_sf2_timecents (effects->eg2_decay);

      /* can we do something with decay2? */

      vals[IPATCH_SF2_GEN_VOL_ENV_SUSTAIN].sword = effects->eg1_sustain;
      vals[IPATCH_SF2_GEN_VOL_ENV_RELEASE].sword =
	ipatch_gig_to_sf2_timecents (effects->eg1_release);


      /* filter LFO */


    }
#endif
}

/**
 * ipatch_gig_to_sf2_timecents:
 * @gig_tc: Amount in GigaSampler timecents
 *
 * Convert GigaSampler timecents to SoundFont timecents.
 *
 * Returns: SoundFont timecent value
 */
guint16
ipatch_gig_to_sf2_timecents (gint32 gig_tc)
{
  return (gig_tc >> 16);	/* divide by 65536 */
}

/**
 * ipatch_gig_volsust_to_sf2_centibels:
 * @gig_tperc: Tenth percent sustain level
 *
 * Convert GigaSampler volume sustain (tenth percent units) to
 * centibels (10th of a decibel).
 *
 * Returns: SoundFont centibels value
 */
guint16
ipatch_gig_volsust_to_sf2_centibels (guint gig_tperc)
{
  gig_tperc = CLAMP (gig_tperc, 0, 1000);
  return (1000 - gig_tperc);	/* FIXME - probably wrong! */
}
