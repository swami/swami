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
/*
 * IpatchSF2Gen_tables.c - SoundFont generator table structures
 */
#include <glib.h>
#include "IpatchSF2Gen.h"
#include "IpatchUnit.h"
#include "ipatch_priv.h"

/* Default range value stored in 2 byte form with correct host byte order */
#define DEFRANGE	(GUINT16_FROM_LE (0x7F00))
#define MAXNEG -32768
#define MAXPOS 32767
#define UMAX 65535


/* generator info */
const IpatchSF2GenInfo ipatch_sf2_gen_info[] = {
  /* StartAddrOfs */
  { {0}, {MAXPOS}, {0}, IPATCH_UNIT_TYPE_SAMPLES, N_("Sample Start Ofs"), NULL},
  /* EndAddrOfs */
  { {MAXNEG}, {0}, {0}, IPATCH_UNIT_TYPE_SAMPLES, N_("Sample End Ofs"), NULL},
  /* StartLoopAddrOfs */
  { {MAXNEG}, {MAXPOS}, {0}, IPATCH_UNIT_TYPE_SAMPLES,
    N_("Sample Loop Start Ofs"), NULL},
  /* EndLoopAddrOfs */
  { {MAXNEG}, {MAXPOS}, {0}, IPATCH_UNIT_TYPE_SAMPLES,
    N_("Sample Loop End Ofs"), NULL},
  /* StartAddrCoarseOfs */
  { {0}, {MAXPOS}, {0}, IPATCH_UNIT_TYPE_32K_SAMPLES,
    N_("Sample Start Coarse Ofs"), NULL},
  /* ModLFO2Pitch */
  { {-12000}, {12000}, {0}, IPATCH_UNIT_TYPE_CENTS, N_("To Pitch"),
    N_("Modulation oscillator to pitch") },
  /* VibLFO2Pitch */
  { {-12000}, {12000}, {0}, IPATCH_UNIT_TYPE_CENTS, N_("To Pitch"),
    N_("Vibrato oscillator to pitch") },
  /* ModEnv2Pitch */
  { {-12000}, {12000}, {0}, IPATCH_UNIT_TYPE_CENTS, N_("To Pitch"),
    N_("Modulation envelope to pitch") },
  /* FilterFc */
  { {1500}, {13500}, {13500}, IPATCH_UNIT_TYPE_SF2_ABS_PITCH, N_("Filter cutoff"),
    N_("Low pass filter cutoff frequency") },
  /* FilterQ */
  { {0}, {960}, {0}, IPATCH_UNIT_TYPE_CENTIBELS, N_("Filter Q"),
    N_("Low pass filter Q factor") },
  /* ModLFO2FilterFc */
  { {-12000}, {12000}, {0}, IPATCH_UNIT_TYPE_CENTS, N_("To Filter Cutoff"),
    N_("Modulation oscillator to filter cutoff") },
  /* ModEnv2FilterFc */
  { {-12000}, {12000}, {0}, IPATCH_UNIT_TYPE_CENTS, N_("To Filter Cutoff"),
    N_("Modulation envelope to filter cutoff") },
  /* EndAddrCoarseOfs */
  { {MAXNEG}, {0}, {0}, IPATCH_UNIT_TYPE_32K_SAMPLES,
    N_("Sample End Coarse Ofs"), NULL},
  /* ModLFO2Vol */
  { {-960}, {960}, {0}, IPATCH_UNIT_TYPE_CENTIBELS, N_("To Volume"),
    N_("Modulation oscillator to volume") },
  /* Unused1 */
  { {0}, {0}, {0}, IPATCH_UNIT_TYPE_NONE, NULL, NULL},
  /* ChorusSend */
  { {0}, {1000}, {0}, IPATCH_UNIT_TYPE_TENTH_PERCENT, N_("Chorus"), NULL},
  /* ReverbSend */
  { {0}, {1000}, {0}, IPATCH_UNIT_TYPE_TENTH_PERCENT, N_("Reverb"), NULL},
  /* Pan */
  { {-500}, {500}, {0}, IPATCH_UNIT_TYPE_TENTH_PERCENT, N_("Pan"), N_("Panning") },
  /* Unused2 */
  { {0}, {0}, {0}, IPATCH_UNIT_TYPE_NONE, NULL, NULL},
  /* Unused3 */
  { {0}, {0}, {0}, IPATCH_UNIT_TYPE_NONE, NULL, NULL},
  /* Unused4 */
  { {0}, {0}, {0}, IPATCH_UNIT_TYPE_NONE, NULL, NULL},
  /* ModLFODelay */
  { {-12000}, {5000}, {-12000}, IPATCH_UNIT_TYPE_SF2_ABS_TIME, N_("Delay"),
    N_("Modulation oscillator delay") },
  /* ModLFOFreq */
  { {-16000}, {4500}, {0}, IPATCH_UNIT_TYPE_SF2_ABS_PITCH, N_("Frequency"),
    N_("Modulation oscillator frequency") },
  /* VibLFODelay */
  { {-12000}, {5000}, {-12000}, IPATCH_UNIT_TYPE_SF2_ABS_TIME, N_("Delay"),
    N_("Vibrato oscillator delay") },
  /* VibLFOFreq */
  { {-16000}, {4500}, {0}, IPATCH_UNIT_TYPE_SF2_ABS_PITCH, N_("Frequency"),
    N_("Vibrato oscillator frequency") },
  /* ModEnvDelay */
  { {-12000}, {5000}, {-12000}, IPATCH_UNIT_TYPE_SF2_ABS_TIME, N_("Delay"),
    N_("Modulation envelope delay") },
  /* ModEnvAttack */
  { {-12000}, {8000}, {-12000}, IPATCH_UNIT_TYPE_SF2_ABS_TIME, N_("Attack"),
    N_("Modulation envelope attack") },
  /* ModEnvHold */
  { {-12000}, {5000}, {-12000}, IPATCH_UNIT_TYPE_SF2_ABS_TIME, N_("Hold"),
    N_("Modulation envelope hold") },
  /* ModEnvDecay */
  { {-12000}, {8000}, {-12000}, IPATCH_UNIT_TYPE_SF2_ABS_TIME, N_("Decay"),
    N_("Modulation envelope decay") },
  /* ModEnvSustain */
  { {0}, {1000}, {0}, IPATCH_UNIT_TYPE_TENTH_PERCENT, N_("Sustain"),
    N_("Modulation envelope sustain") },
  /* ModEnvRelease */
  { {-12000}, {8000}, {-12000}, IPATCH_UNIT_TYPE_SF2_ABS_TIME, N_("Release"),
    N_("Modulation envelope release") },
  /* Note2ModEnvHold */
  { {-1200}, {1200}, {0}, IPATCH_UNIT_TYPE_CENTS, N_("Note to Hold"),
    N_("MIDI note to modulation envelope hold") },
  /* Note2ModEnvDecay */
  { {-1200}, {1200}, {0}, IPATCH_UNIT_TYPE_CENTS, N_("Note to Decay"),
    N_("MIDI note to modulation envelope decay") },
  /* VolEnvDelay */
  { {-12000}, {5000}, {-12000}, IPATCH_UNIT_TYPE_SF2_ABS_TIME, N_("Delay"),
    N_("Volume envelope delay") },
  /* VolEnvAttack */
  { {-12000}, {8000}, {-12000}, IPATCH_UNIT_TYPE_SF2_ABS_TIME, N_("Attack"),
    N_("Volume envelope attack") },
  /* VolEnvHold */
  { {-12000}, {5000}, {-12000}, IPATCH_UNIT_TYPE_SF2_ABS_TIME, N_("Hold"),
    N_("Volume envelope hold") },
  /* VolEnvDecay */
  { {-12000}, {8000}, {-12000}, IPATCH_UNIT_TYPE_SF2_ABS_TIME, N_("Decay"),
    N_("Volume envelope decay") },
  /* VolEnvSustain */
  { {0}, {1440}, {0}, IPATCH_UNIT_TYPE_CENTIBELS, N_("Sustain"),
    N_("Volume envelope sustain") },
  /* VolEnvRelease */
  { {-12000}, {8000}, {-12000}, IPATCH_UNIT_TYPE_SF2_ABS_TIME, N_("Release"),
    N_("Volume envelope release") },
  /* Note2VolEnvHold */
  { {-1200}, {1200}, {0}, IPATCH_UNIT_TYPE_CENTS, N_("Note to Hold"),
    N_("MIDI note to volume envelope hold") },
  /* Note2VolEnvDecay */
  { {-1200}, {1200}, {0}, IPATCH_UNIT_TYPE_CENTS, N_("Note to Decay"),
    N_("MIDI note to volume envelope decay") },
  /* Instrument */
  { {0}, {uword:UMAX}, {0}, IPATCH_UNIT_TYPE_UINT, N_("Instrument ID"), NULL},
  /* Reserved1 */
  { {0}, {0}, {0}, IPATCH_UNIT_TYPE_NONE, NULL, NULL},
  /* NoteRange */
  { {0}, {127}, {DEFRANGE}, IPATCH_UNIT_TYPE_RANGE, N_("Note Range"), NULL},
  /* VelRange */
  { {0}, {127}, {DEFRANGE}, IPATCH_UNIT_TYPE_RANGE,
    N_("Velocity Range"), NULL},
  /* StartLoopAddrCoarseOfs */
  { {MAXNEG}, {MAXPOS}, {0}, IPATCH_UNIT_TYPE_32K_SAMPLES,
   N_("Sample Loop Start Coarse Ofs"), NULL},
  /* FixedNote */
  { {-1}, {127}, {-1}, IPATCH_UNIT_TYPE_INT, N_("Fixed Note"), NULL},
  /* Velocity */
  { {-1}, {127}, {-1}, IPATCH_UNIT_TYPE_INT, N_("Fixed Velocity"), NULL},
  /* InitAttenuation */
  { {0}, {1440}, {0}, IPATCH_UNIT_TYPE_CENTIBELS, N_("Attenuation"),
    N_("Volume attenuation") },
  /* Reserved2 */
  { {0}, {0}, {0}, IPATCH_UNIT_TYPE_NONE, NULL, NULL},
  /* EndLoopAddrCoarseOfs */
  { {MAXNEG}, {MAXPOS}, {0}, IPATCH_UNIT_TYPE_32K_SAMPLES,
   N_("Sample Loop End Coarse Ofs"), NULL},
  /* CourseTune */
  { {-120}, {120}, {0}, IPATCH_UNIT_TYPE_SEMITONES, N_("Coarse Tune"), NULL},
  /* FineTune */
  { {-99}, {99}, {0}, IPATCH_UNIT_TYPE_CENTS, N_("Fine Tune"), NULL},
  /* sampleId */
  { {0}, {uword:UMAX}, {0}, IPATCH_UNIT_TYPE_UINT, N_("Sample ID"), NULL},
  /* SampleModes */
  { {0}, {3}, {0}, IPATCH_UNIT_TYPE_UINT, N_("Sample Modes"), NULL},
  /* Reserved3 */
  { {0}, {0}, {0}, IPATCH_UNIT_TYPE_NONE, NULL, NULL},
  /* ScaleTuning */
  { {0}, {1200}, {100}, IPATCH_UNIT_TYPE_CENTS, N_("Scale Tune"), NULL},
  /* ExclusiveClass */
  { {0}, {127}, {0}, IPATCH_UNIT_TYPE_INT, N_("Exclusive Class"), NULL},
  /* RootNote */
  { {-1}, {127}, {-1}, IPATCH_UNIT_TYPE_INT, N_("Root Note"), NULL}
};
