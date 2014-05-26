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

#include "IpatchSLIInst.h"
#include "ipatch_priv.h"

/* Category strings */
const char *ipatch_sli_inst_cat_strings[] = { "80s", "A-Synth", "Acid",
  "Attack", "Bass", "Bell", "Big Beat", "Block", "Bongo", "Brass", "Chime",
  "China", "Clap", "Clave", "Conga", "Crash", "Cuica", "Cymbal", "D-Synth",
  "Disco", "Drum-Loop", "Effects", "Electro", "Ethnic", "Ext-In", "FB-Loop",
  "FX-Loop", "Funk", "Gong", "Guiro", "HiHat", "HipHop", "House", "Human",
  "Industry", "Jazz", "Kick", "Lead", "March", "Marimba", "Multi", "Natural",
  "Oldie", "Organ", "Other", "Pad", "Perc-Loop", "Percussion", "Percussive",
  "Piano", "Plug", "Pop", "Release", "Ride", "Rock", "Scratch", "Sequencer",
  "Shaker", "Snare", "Splash", "String", "Synth-Bass", "TR-alike", "Techno",
  "Texture", "Timbale", "Tom", "Tonal-Loop", "Triangle", "Voice", "Whistle",
  "Wind", "World" };

static const IpatchSLIInstCatMapEntry ipatch_sli_inst_subcat_map_default[] = {
  { 'T', IPATCH_SLI_INST_CAT_TR_ALIKE, NULL },
  { 'B', IPATCH_SLI_INST_CAT_BIG_BEAT, NULL },
  { 'H', IPATCH_SLI_INST_CAT_HIPHOP, NULL },
  { 'E', IPATCH_SLI_INST_CAT_ELECTRO, NULL },
  { 'A', IPATCH_SLI_INST_CAT_ACID, NULL },
  { 'C', IPATCH_SLI_INST_CAT_TECHNO, NULL },
  { 'U', IPATCH_SLI_INST_CAT_HOUSE, NULL },
  { 'F', IPATCH_SLI_INST_CAT_FUNK, NULL },
  { 'D', IPATCH_SLI_INST_CAT_DISCO, NULL },
  { 'P', IPATCH_SLI_INST_CAT_POP, NULL },
  { '8', IPATCH_SLI_INST_CAT_80S, NULL },
  { 'N', IPATCH_SLI_INST_CAT_NATURAL, NULL },
  { 'R', IPATCH_SLI_INST_CAT_ROCK, NULL },
  { 'J', IPATCH_SLI_INST_CAT_JAZZ, NULL },
  { 'O', IPATCH_SLI_INST_CAT_OLDIE, NULL },
  { 'W', IPATCH_SLI_INST_CAT_WORLD, NULL },
  { 'X', IPATCH_SLI_INST_CAT_EFFECTS, NULL },
  { '@', IPATCH_SLI_INST_CAT_OTHER, NULL }
};

static const IpatchSLIInstCatMapEntry ipatch_sli_inst_subcat_map_cymbal[] = {
  { 'H', IPATCH_SLI_INST_CAT_CHINA, NULL },
  { 'R', IPATCH_SLI_INST_CAT_RIDE, NULL },
  { 'G', IPATCH_SLI_INST_CAT_GONG, NULL },
  { 'E', IPATCH_SLI_INST_CAT_ETHNIC, NULL },
  { 'C', IPATCH_SLI_INST_CAT_CRASH, NULL },
  { 'S', IPATCH_SLI_INST_CAT_SPLASH, NULL },
  { 'M', IPATCH_SLI_INST_CAT_MARCH, NULL },
  { 'X', IPATCH_SLI_INST_CAT_EFFECTS, NULL },
  { '@', IPATCH_SLI_INST_CAT_OTHER, NULL }
};

static const IpatchSLIInstCatMapEntry ipatch_sli_inst_subcat_map_percussion[] = {
  { 'L', IPATCH_SLI_INST_CAT_BELL, NULL },
  { 'K', IPATCH_SLI_INST_CAT_BLOCK, NULL },
  { 'B', IPATCH_SLI_INST_CAT_BONGO, NULL },
  { 'J', IPATCH_SLI_INST_CAT_CHIME, NULL },
  { 'C', IPATCH_SLI_INST_CAT_CLAP, NULL },
  { 'V', IPATCH_SLI_INST_CAT_CLAVE, NULL },
  { 'O', IPATCH_SLI_INST_CAT_CONGA, NULL },
  { 'I', IPATCH_SLI_INST_CAT_CUICA, NULL },
  { 'X', IPATCH_SLI_INST_CAT_EFFECTS, NULL },
  { 'E', IPATCH_SLI_INST_CAT_ETHNIC, NULL },
  { 'G', IPATCH_SLI_INST_CAT_GUIRO, NULL },
  { 'H', IPATCH_SLI_INST_CAT_HUMAN, NULL },
  { 'Y', IPATCH_SLI_INST_CAT_INDUSTRY, NULL },
  { 'M', IPATCH_SLI_INST_CAT_MARIMBA, NULL },
  { 'R', IPATCH_SLI_INST_CAT_SCRATCH, NULL },
  { 'S', IPATCH_SLI_INST_CAT_SHAKER, NULL },
  { 'T', IPATCH_SLI_INST_CAT_TIMBALE, NULL },
  { 'N', IPATCH_SLI_INST_CAT_TRIANGLE, NULL },
  { 'W', IPATCH_SLI_INST_CAT_WHISTLE, NULL },
  { '@', IPATCH_SLI_INST_CAT_OTHER, NULL }
};

static const IpatchSLIInstCatMapEntry ipatch_sli_inst_subcat_map_asynth[] = {
  { 'S', IPATCH_SLI_INST_CAT_SYNTH_BASS, NULL },
  { 'B', IPATCH_SLI_INST_CAT_BASS, NULL },
  { 'Q', IPATCH_SLI_INST_CAT_SEQUENCER, NULL },
  { 'U', IPATCH_SLI_INST_CAT_PLUG, NULL },
  { 'A', IPATCH_SLI_INST_CAT_ATTACK, NULL },
  { 'R', IPATCH_SLI_INST_CAT_RELEASE, NULL },
  { 'L', IPATCH_SLI_INST_CAT_LEAD, NULL },
  { 'P', IPATCH_SLI_INST_CAT_PAD, NULL },
  { 'G', IPATCH_SLI_INST_CAT_STRING, NULL },
  { 'F', IPATCH_SLI_INST_CAT_BRASS, NULL },
  { 'V', IPATCH_SLI_INST_CAT_VOICE, NULL },
  { 'O', IPATCH_SLI_INST_CAT_ORGAN, NULL },
  { 'W', IPATCH_SLI_INST_CAT_WIND, NULL },
  { 'C', IPATCH_SLI_INST_CAT_PERCUSSIVE, NULL },
  { 'E', IPATCH_SLI_INST_CAT_ETHNIC, NULL },
  { 'T', IPATCH_SLI_INST_CAT_TEXTURE, NULL },
  { 'X', IPATCH_SLI_INST_CAT_EFFECTS, NULL },
  { 'M', IPATCH_SLI_INST_CAT_MULTI, NULL },
  { 'N', IPATCH_SLI_INST_CAT_FB_LOOP, NULL },
  { 'I', IPATCH_SLI_INST_CAT_EXT_IN, NULL },
  { '@', IPATCH_SLI_INST_CAT_OTHER, NULL }
};

static const IpatchSLIInstCatMapEntry ipatch_sli_inst_subcat_map_dsynth[] = {
  { 'S', IPATCH_SLI_INST_CAT_SYNTH_BASS, NULL },
  { 'B', IPATCH_SLI_INST_CAT_BASS, NULL },
  { 'Q', IPATCH_SLI_INST_CAT_SEQUENCER, NULL },
  { 'U', IPATCH_SLI_INST_CAT_PLUG, NULL },
  { 'A', IPATCH_SLI_INST_CAT_ATTACK, NULL },
  { 'R', IPATCH_SLI_INST_CAT_RELEASE, NULL },
  { 'L', IPATCH_SLI_INST_CAT_LEAD, NULL },
  { 'P', IPATCH_SLI_INST_CAT_PAD, NULL },
  { 'G', IPATCH_SLI_INST_CAT_STRING, NULL },
  { 'F', IPATCH_SLI_INST_CAT_BRASS, NULL },
  { 'V', IPATCH_SLI_INST_CAT_VOICE, NULL },
  { 'O', IPATCH_SLI_INST_CAT_ORGAN, NULL },
  { 'W', IPATCH_SLI_INST_CAT_WIND, NULL },
  { 'I', IPATCH_SLI_INST_CAT_PIANO, NULL },
  { 'C', IPATCH_SLI_INST_CAT_PERCUSSIVE, NULL },
  { 'E', IPATCH_SLI_INST_CAT_ETHNIC, NULL },
  { 'T', IPATCH_SLI_INST_CAT_TEXTURE, NULL },
  { 'X', IPATCH_SLI_INST_CAT_EFFECTS, NULL },
  { '@', IPATCH_SLI_INST_CAT_OTHER, NULL }
};

const IpatchSLIInstCatMapEntry ipatch_sli_inst_cat_map[] = {
  { 'K', IPATCH_SLI_INST_CAT_KICK, ipatch_sli_inst_subcat_map_default },
  { 'S', IPATCH_SLI_INST_CAT_SNARE, ipatch_sli_inst_subcat_map_default },
  { 'H', IPATCH_SLI_INST_CAT_HIHAT, ipatch_sli_inst_subcat_map_default },
  { 'O', IPATCH_SLI_INST_CAT_TOM, ipatch_sli_inst_subcat_map_default },
  { 'Y', IPATCH_SLI_INST_CAT_CYMBAL, ipatch_sli_inst_subcat_map_cymbal },
  { 'P', IPATCH_SLI_INST_CAT_PERCUSSION, ipatch_sli_inst_subcat_map_percussion },
  { 'd', IPATCH_SLI_INST_CAT_DRUM_LOOP, ipatch_sli_inst_subcat_map_default },
  { 'p', IPATCH_SLI_INST_CAT_PERC_LOOP, ipatch_sli_inst_subcat_map_default },
  { 't', IPATCH_SLI_INST_CAT_TONAL_LOOP, ipatch_sli_inst_subcat_map_default },
  { 'x', IPATCH_SLI_INST_CAT_FX_LOOP, NULL },
  { 'A', IPATCH_SLI_INST_CAT_A_SYNTH, ipatch_sli_inst_subcat_map_asynth },
  { 'D', IPATCH_SLI_INST_CAT_D_SYNTH, ipatch_sli_inst_subcat_map_dsynth },
  { '@', IPATCH_SLI_INST_CAT_OTHER, NULL }
};
