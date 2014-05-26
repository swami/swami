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
#ifndef __IPATCH_GIG_EFFECTS_H__
#define __IPATCH_GIG_EFFECTS_H__

#include <glib.h>
#include <glib-object.h>

typedef struct _IpatchGigEffects IpatchGigEffects;

#include <libinstpatch/IpatchFile.h>
#include <libinstpatch/IpatchSF2Gen.h>

/* GigaSampler envelope/lfo/filter settings (3ewa chunk) */
struct _IpatchGigEffects
{
  guint32 unknown1;		/* byte 0-3, always 0x0000008C? */
  guint32 unknown7;		/* byte 44-47 */
  guint32 unknown14;		/* byte 100-103 */

  guint16 unknown2;		/* byte 12-13 */
  guint16 unknown3;		/* byte 16-17 */
  guint16 unknown4;		/* byte 20-21 */
  guint16 unknown5;		/* byte 24-25 */
  guint16 unknown6;		/* byte 36-37 */
  guint16 unknown8;		/* byte 60-61 */
  guint16 unknown9;		/* byte 68-69 */
  guint16 unknown10;		/* byte 76-77 */
  guint16 unknown11;		/* byte 84-85 */
  guint16 unknown13;		/* byte 98-99 */
  guint16 unknown15;		/* byte 106-107 */
  guint16 unknown17;		/* byte 116-117 */
  guint16 unknown18;		/* byte 122-123 */
  guint16 unknown20;		/* byte 126-127 */

  guint8 unknown12;		/* byte 93 */
  guint8 unknown16;		/* byte 111 */
  guint8 unknown19;		/* byte 125 */
  guint8 unknown21;		/* byte 129 */
  guint8 unknown22;		/* byte 130 */
  guint8 unknown23;		/* byte 135 */

  /* EG1 - Volume envelope */
  guint16 eg1_pre_attack;	/* 10th percent */
  guint16 eg1_sustain;		/* 10th percent */
  guint32 eg1_attack;		/* timecents */
  guint32 eg1_decay;		/* timecents */
  guint32 eg1_decay2;		/* timecents (where is the "inf" flag?) */
  guint32 eg1_release;		/* timecents */
  guint8 eg1_hold;		/* bit 8=1:true */

  /* EG2 - Filter envelope */
  guint16 eg2_pre_attack;	/* 10th percent */
  guint16 eg2_sustain;		/* 10th percent */
  guint32 eg2_attack;		/* timecents */
  guint32 eg2_decay;		/* timecents */
  guint32 eg2_decay2;		/* timecents */
  guint32 eg2_release;		/* timecents */

  /* EG3 - Pitch envelope */
  guint32 eg3_attack;		/* timecents */
  guint16 eg3_depth;		/* 12 bit signed (cents) */

  /* LFO1 - Volume LFO */
  guint16 lfo1_internal_depth;	/* 0-1200 */
  guint32 lfo1_freq;		/* pitch cents */
  guint16 lfo1_ctrl_depth;	/* 0-1200 */
  guint8 lfo1_ctrl;
  /* 0=internal, 1=mod wheel, 2=breath ctrl, 3=internal/mod wheel,
     4=internal/breath ctrl */
  /* bit 8=1:flip phase */
  /* bit 7=1:synch */
  /* bits 5 en 6: Res midictrl: 0=18, 1=19, 2=80, 3=81 */

  /* LFO2 - Filter LFO */
  guint8 lfo2_ctrl;
  /* 0=internal, 1=mod wheel, 2=breath ctrl, 3=internal/mod wheel,
     4=internal/breath ctrl */
  /* bit 6=1:synch */
  /* bit 8=1:flip phase */
  /* bit 7=1:Resonance midi ctrl */

  guint32 lfo2_freq;		/* pitch cents */
  guint16 lfo2_internal_depth;	/* 0-1200 */
  guint16 lfo2_ctrl_depth;	/* 0-1200 */

  /* LFO3 - Pitch LFO */
  guint32 lfo3_freq;		/* pitch cents */
  guint16 lfo3_internal_depth;	/* cents */
  guint16 lfo3_ctrl_depth;	/* cents */
  guint8 lfo3_ctrl; /* bit 6: LFO3 synch  bit 8:invert attentuation ctrl */


  /* filter parameters */
  guint8 filter_type; /* 0=lowpass, 1=bandpass, 2=highpass, 3=bandreject */
  guint8 turbo_lowpass; /* bit 7=0: on */
  guint8 filter_cutoff; /* bit 8=1:on */
  guint8 filter_midi_ctrl; /* bit 8=1:use ctrl  rest 0:aftertouch */
  guint8 filter_vel_scale;
  guint8 filter_resonance; /* bit 8=0:dynamic */
  guint8 filter_breakpoint; /* bit 8=1:keyboard tracking */

  /* velocity parameters */
  guint8 vel_response; /* 0-4 = nonlinear, 5-9 = linear, 10-14 = special */
  guint8 vel_dyn_range;

  /* release velocity paramaters */
  guint8 release_vel_response; /* 0-4 = nonlinear, 5-9 = linear, 10-14 = special */
  /* release velocity dynamic range? */

  guint8 release_trigger_decay;

  guint8 attn_ctrl;	      /* bit 1:on, rest=ctrl, 0xFF=velocity */
  guint8 max_velocity;		/* Used for velocity split */

  guint16 sample_offset;
  guint8 pitch_track_dim_bypass; /* bit 0=0: pitch track */
				/* 0x10/0x20=dim bypass ctrl 94/95 */
  guint8 layer_pan;		/* 7-bit signed */
  guint8 self_mask;		/* 1=true */
  guint8 channel_offset;	/* (*4) */
  guint8 sust_defeat;		/* 2=on */
};

typedef enum
{
  IPATCH_GIG_FILTER_LOWPASS = 0,
  IPATCH_GIG_FILTER_BANDPASS = 1,
  IPATCH_GIG_FILTER_HIGHPASS = 2,
  IPATCH_GIG_FILTER_BANDREJECT = 3
} IpatchGigFilterType;

/* MIDI controllers used in GigaSampler files */
typedef enum
{
  IPATCH_GIG_CTRL_MOD_WHEEL       = 0x01,
  IPATCH_GIG_CTRL_BREATH          = 0x02,
  IPATCH_GIG_CTRL_FOOT            = 0x04,
  IPATCH_GIG_CTRL_PORTAMENTO_TIME = 0x05,
  IPATCH_GIG_CTRL_EFFECT_1        = 0x0C,
  IPATCH_GIG_CTRL_EFFECT_2        = 0x0D,
  IPATCH_GIG_CTRL_GEN_PURPOSE_1   = 0x10,
  IPATCH_GIG_CTRL_GEN_PURPOSE_2   = 0x11,
  IPATCH_GIG_CTRL_GEN_PURPOSE_3   = 0x12,
  IPATCH_GIG_CTRL_GEN_PURPOSE_4   = 0x13,
  IPATCH_GIG_CTRL_SUSTAIN_PEDAL   = 0x40,
  IPATCH_GIG_CTRL_PORTAMENTO      = 0x41,
  IPATCH_GIG_CTRL_SOSTENUTO       = 0x42,
  IPATCH_GIG_CTRL_SOFT_PEDAL      = 0x43,
  IPATCH_GIG_CTRL_GEN_PURPOSE_5   = 0x50,
  IPATCH_GIG_CTRL_GEN_PURPOSE_6   = 0x51,
  IPATCH_GIG_CTRL_GEN_PURPOSE_7   = 0x52,
  IPATCH_GIG_CTRL_GEN_PURPOSE_8   = 0x53,
  IPATCH_GIG_CTRL_EFFECT_DEPTH_1  = 0x5B,
  IPATCH_GIG_CTRL_EFFECT_DEPTH_2  = 0x5C,
  IPATCH_GIG_CTRL_EFFECT_DEPTH_3  = 0x5D,
  IPATCH_GIG_CTRL_EFFECT_DEPTH_4  = 0x5E,
  IPATCH_GIG_CTRL_EFFECT_DEPTH_5  = 0x5F
} IpatchGigControlType;

void ipatch_gig_parse_effects (IpatchFileHandle *handle,
			       IpatchGigEffects *effects);
void ipatch_gig_store_effects (IpatchFileHandle *handle,
			       IpatchGigEffects *effects);
void ipatch_gig_effects_init (IpatchGigEffects *effects);

void ipatch_gig_effects_to_gen_array (IpatchGigEffects *effects,
				      IpatchSF2GenArray *array);
guint16 ipatch_gig_to_sf2_timecents (gint32 gig_tc);
guint16 ipatch_gig_volsust_to_sf2_centibels (guint gig_tperc);

#endif
