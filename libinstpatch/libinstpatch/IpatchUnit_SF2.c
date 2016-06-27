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
 * SECTION: IpatchUnit_SF2
 * @short_description: Unit types and conversions for SoundFont
 * @see_also: 
 * @stability: Stable
 */
#include <stdio.h>
#include <math.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchUnit_SF2.h"
#include "IpatchUnit.h"
#include "i18n.h"


static void
ipatch_unit_sf2_abs_pitch_to_dls_abs_pitch_value (const GValue *src_val,
						  GValue *dest_val);
static void
ipatch_unit_dls_abs_pitch_to_sf2_abs_pitch_value (const GValue *src_val,
						  GValue *dest_val);
static void
ipatch_unit_sf2_abs_pitch_to_hertz_value (const GValue *src_val,
					  GValue *dest_val);
static void
ipatch_unit_hertz_to_sf2_abs_pitch_value (const GValue *src_val,
					  GValue *dest_val);
static void
ipatch_unit_sf2_abs_time_to_dls_abs_time_value (const GValue *src_val,
						GValue *dest_val);
static void
ipatch_unit_dls_abs_time_to_sf2_abs_time_value (const GValue *src_val,
						GValue *dest_val);
static void
ipatch_unit_sf2_abs_time_to_seconds_value (const GValue *src_val,
					   GValue *dest_val);
static void
ipatch_unit_seconds_to_sf2_abs_time_value (const GValue *src_val,
					   GValue *dest_val);
static void
ipatch_unit_centibels_to_dls_gain_value (const GValue *src_val,
					 GValue *dest_val);
static void
ipatch_unit_dls_gain_to_centibels_value (const GValue *src_val,
					 GValue *dest_val);
static void
ipatch_unit_centibels_to_decibels_value (const GValue *src_val,
				         GValue *dest_val);
static void
ipatch_unit_decibels_to_centibels_value (const GValue *src_val,
				         GValue *dest_val);
static void
ipatch_unit_tenth_percent_to_percent_value (const GValue *src_val,
					    GValue *dest_val);
static void
ipatch_unit_percent_to_tenth_percent_value (const GValue *src_val,
					    GValue *dest_val);


/**
 * _ipatch_unit_sf2_init: (skip)
 */
void
_ipatch_unit_sf2_init (void)
{
  IpatchUnitInfo *info;

  info = ipatch_unit_info_new ();
  info->digits = 0;
  info->label = NULL;
  info->descr = NULL;
  info->value_type = G_TYPE_INT;

  /* FIXME - SoundFont absolute pitch is the same as cents.. */
  info->id = IPATCH_UNIT_TYPE_SF2_ABS_PITCH;
  info->name = "SF2AbsPitch";
  ipatch_unit_register (info);

  info->id = IPATCH_UNIT_TYPE_SF2_OFS_PITCH;
  info->name = "SF2OfsPitch";
  ipatch_unit_register (info);

  info->id = IPATCH_UNIT_TYPE_SF2_ABS_TIME;
  info->name = "SF2AbsTime";
  ipatch_unit_register (info);

  info->id = IPATCH_UNIT_TYPE_SF2_OFS_TIME;
  info->name = "SF2OfsTime";
  ipatch_unit_register (info);

  info->id = IPATCH_UNIT_TYPE_CENTIBELS;
  info->flags = IPATCH_UNIT_LOGARITHMIC;
  info->name = "Centibels";
  ipatch_unit_register (info);
  info->flags = 0;

  info->id = IPATCH_UNIT_TYPE_32K_SAMPLES;
  info->name = "32kSamples";
  ipatch_unit_register (info);

  info->id = IPATCH_UNIT_TYPE_TENTH_PERCENT;
  info->name = "TenthPercent";
  ipatch_unit_register (info);

  ipatch_unit_info_free (info);	/* done with info structure, free it */

  /* SF2 absolute pitch <==> DLS absolute pitch */
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_SF2_ABS_PITCH, IPATCH_UNIT_TYPE_DLS_ABS_PITCH,
     ipatch_unit_sf2_abs_pitch_to_dls_abs_pitch_value);
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_DLS_ABS_PITCH, IPATCH_UNIT_TYPE_SF2_ABS_PITCH,
     ipatch_unit_dls_abs_pitch_to_sf2_abs_pitch_value);

  /* SF2 absolute pitch <==> Hertz */
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_SF2_ABS_PITCH, IPATCH_UNIT_TYPE_HERTZ,
     ipatch_unit_sf2_abs_pitch_to_hertz_value);
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_HERTZ, IPATCH_UNIT_TYPE_SF2_ABS_PITCH,
     ipatch_unit_hertz_to_sf2_abs_pitch_value);

  /* SF2 offset pitch <==> multiplier (reuse ABS time to seconds - same equation) */
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_SF2_OFS_PITCH, IPATCH_UNIT_TYPE_MULTIPLIER,
     ipatch_unit_sf2_abs_time_to_seconds_value);
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_MULTIPLIER, IPATCH_UNIT_TYPE_SF2_OFS_PITCH,
     ipatch_unit_seconds_to_sf2_abs_time_value);

  /* SF2 absolute time <==> DLS absolute time */
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_SF2_ABS_TIME, IPATCH_UNIT_TYPE_DLS_ABS_TIME,
     ipatch_unit_sf2_abs_time_to_dls_abs_time_value);
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_DLS_ABS_TIME, IPATCH_UNIT_TYPE_SF2_ABS_TIME,
     ipatch_unit_dls_abs_time_to_sf2_abs_time_value);

  /* SF2 absolute time <==> Seconds */
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_SF2_ABS_TIME, IPATCH_UNIT_TYPE_SECONDS,
     ipatch_unit_sf2_abs_time_to_seconds_value);
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_SECONDS, IPATCH_UNIT_TYPE_SF2_ABS_TIME,
     ipatch_unit_seconds_to_sf2_abs_time_value);

  /* SF2 offset time <==> multiplier (reuse ABS time to seconds - same equation)  */
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_SF2_OFS_TIME, IPATCH_UNIT_TYPE_MULTIPLIER,
     ipatch_unit_sf2_abs_time_to_seconds_value);
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_MULTIPLIER, IPATCH_UNIT_TYPE_SF2_OFS_TIME,
     ipatch_unit_seconds_to_sf2_abs_time_value);

  /* Centibels <==> DLS gain */
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_CENTIBELS, IPATCH_UNIT_TYPE_DLS_GAIN,
     ipatch_unit_centibels_to_dls_gain_value);
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_DLS_GAIN, IPATCH_UNIT_TYPE_CENTIBELS,
     ipatch_unit_dls_gain_to_centibels_value);

  /* Centibels <==> Decibels */
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_CENTIBELS, IPATCH_UNIT_TYPE_DECIBELS,
     ipatch_unit_centibels_to_decibels_value);
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_DECIBELS, IPATCH_UNIT_TYPE_CENTIBELS,
     ipatch_unit_decibels_to_centibels_value);

  /* TenthPercent <==> Percent */
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_TENTH_PERCENT, IPATCH_UNIT_TYPE_PERCENT,
     ipatch_unit_tenth_percent_to_percent_value);
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_PERCENT, IPATCH_UNIT_TYPE_TENTH_PERCENT,
     ipatch_unit_percent_to_tenth_percent_value);


  ipatch_unit_class_register_map (IPATCH_UNIT_CLASS_USER,
				  IPATCH_UNIT_TYPE_SF2_ABS_PITCH,
				  IPATCH_UNIT_TYPE_HERTZ);
  ipatch_unit_class_register_map (IPATCH_UNIT_CLASS_DLS,
				  IPATCH_UNIT_TYPE_SF2_ABS_PITCH,
				  IPATCH_UNIT_TYPE_DLS_ABS_PITCH);
  ipatch_unit_class_register_map (IPATCH_UNIT_CLASS_USER,
				  IPATCH_UNIT_TYPE_SF2_OFS_PITCH,
				  IPATCH_UNIT_TYPE_MULTIPLIER);

  ipatch_unit_class_register_map (IPATCH_UNIT_CLASS_USER,
				  IPATCH_UNIT_TYPE_SF2_ABS_TIME,
				  IPATCH_UNIT_TYPE_SECONDS);
  ipatch_unit_class_register_map (IPATCH_UNIT_CLASS_DLS,
				  IPATCH_UNIT_TYPE_SF2_ABS_TIME,
				  IPATCH_UNIT_TYPE_DLS_ABS_TIME);
  ipatch_unit_class_register_map (IPATCH_UNIT_CLASS_USER,
				  IPATCH_UNIT_TYPE_SF2_OFS_TIME,
				  IPATCH_UNIT_TYPE_MULTIPLIER);

  ipatch_unit_class_register_map (IPATCH_UNIT_CLASS_USER,
				  IPATCH_UNIT_TYPE_CENTIBELS,
				  IPATCH_UNIT_TYPE_DECIBELS);
  ipatch_unit_class_register_map (IPATCH_UNIT_CLASS_DLS,
				  IPATCH_UNIT_TYPE_CENTIBELS,
				  IPATCH_UNIT_TYPE_DLS_GAIN);

  ipatch_unit_class_register_map (IPATCH_UNIT_CLASS_USER,
				  IPATCH_UNIT_TYPE_TENTH_PERCENT,
				  IPATCH_UNIT_TYPE_PERCENT);
}

/**
 * ipatch_unit_sf2_abs_pitch_to_dls_abs_pitch:
 * @sf2_abs_pitch: Value in SF2 absolute pitch
 *
 * Converts a value from SF2 absolute pitch to DLS absolute pitch.
 *
 * sf2_abs_pitch = 1200 * log2(f/8.176)
 * f = 8.176 * 2^(sf2_abs_pitch/1200)
 *
 * dls_abs_pitch = (1200 * log2(f/440) + 6900) * 65536
 * f = 440 * 2^((dls_abs_pitch / 65536 - 6900) / 1200)
 *
 * Returns: Value converted to DLS absolute pitch
 */
int
ipatch_unit_sf2_abs_pitch_to_dls_abs_pitch (int sf2_abs_pitch)
{
  double hz;

  hz = 8.176 * pow (2.0, ((double)sf2_abs_pitch) / 1200.0);
  return ((1200.0 * (log (hz / 440.0) / log (2.0)) + 6900.0)
	  * 65536.0 + 0.5);	/* +0.5 for rounding */
}

/**
 * ipatch_unit_dls_abs_pitch_to_sf2_abs_pitch:
 * @dls_abs_pitch: Value in DLS absolute pitch
 *
 * Converts a value from DLS absolute pitch to SF2 absolute pitch.
 * See ipatch_unit_sf2_abs_pitch_to_dls_abs_pitch()
 *
 * Returns: Value converted to SF2 absolute pitch
 */
int
ipatch_unit_dls_abs_pitch_to_sf2_abs_pitch (int dls_abs_pitch)
{
  double hz;

  hz = 440.0 * pow (2.0, (((double)dls_abs_pitch / 65536.0 - 6900.0)
			  / 1200.0));
  return (1200.0 * (log (hz / 8.176) / log (2.0)) + 0.5); /* +0.5 to round */
}

/**
 * ipatch_unit_sf2_abs_pitch_to_hertz:
 * @sf2_abs_pitch: Value in SoundFont absolute pitch
 *
 * Convert SoundFont absolute pitch to frequency in Hertz.
 *
 * Returns: Value in Hertz (cycles per second)
 */
double
ipatch_unit_sf2_abs_pitch_to_hertz (int sf2_abs_pitch)
{
  return (8.176 * pow (2.0, ((double)sf2_abs_pitch) / 1200.0));
}

/**
 * ipatch_unit_hertz_to_sf2_abs_pitch:
 * @hz: Hertz (cycles per second) value
 *
 * Convert frequency in Hertz to SoundFont absolute pitch.
 *
 * Returns: Converted value in SoundFont absolute pitch.
 */
int
ipatch_unit_hertz_to_sf2_abs_pitch (double hz)
{
  return (log (hz / 8.176) / log (2) * 1200 + 0.5); /* +0.5 for rounding */
}

/**
 * ipatch_unit_sf2_ofs_pitch_to_multiplier:
 * @sf2_ofs_pitch: Value in SoundFont offset pitch
 *
 * Convert SoundFont offset pitch (cents) to multiplier factor.
 *
 * Returns: Multiplier factor
 */
double
ipatch_unit_sf2_ofs_pitch_to_multiplier (int sf2_ofs_pitch)
{
  return (pow (2.0, ((double)sf2_ofs_pitch) / 1200.0));
}

/**
 * ipatch_unit_multiplier_to_sf2_ofs_pitch:
 * @multiplier: Multiplier factor
 *
 * Convert multiplier factor to SoundFont offset pitch (cents).
 *
 * Returns: Converted value in SoundFont offset pitch (cents).
 */
int
ipatch_unit_multiplier_to_sf2_ofs_pitch (double multiplier)
{
  return (log (multiplier) / log (2) * 1200 + 0.5); /* +0.5 for rounding */
}

/**
 * ipatch_unit_sf2_abs_time_to_dls_abs_time:
 * @sf2_abs_time: Value in SF2 absolute time (timecents)
 *
 * Convert a value from SF2 absolute time to DLS absolute time.
 *
 * sf2_abs_time = 1200 * log2 (seconds)
 * seconds = 2^(sf2_abs_time / 1200)
 *
 * dls_abs_time = 1200 * log2 (seconds) * 65536
 * seconds = 2^(dls_abs_time / (1200 * 65536))
 *
 * Returns: Value converted to DLS absolute time.
 */
int
ipatch_unit_sf2_abs_time_to_dls_abs_time (int sf2_abs_time)
{
  return (sf2_abs_time * 65536);
}

/**
 * ipatch_unit_dls_abs_time_to_sf2_abs_time:
 * @dls_abs_time: Value in DLS absolute time
 *
 * Convert a value from DLS absolute time to SF2 absolute time.
 * See ipatch_unit_sf2_abs_time_to_dls_abs_time()
 *
 * Returns: Value converted to SF2 absolute time
 */
int
ipatch_unit_dls_abs_time_to_sf2_abs_time (int dls_abs_time)
{
  return ((dls_abs_time + 32768) / 65536); /* +32768 for rounding */
}

/**
 * ipatch_unit_sf2_abs_time_to_seconds:
 * @sf2_abs_time: Value in SoundFont absolute time
 *
 * Convert a value from SoundFont absolute time (timecents) to seconds.
 *
 * Returns: Value in seconds
 */
double
ipatch_unit_sf2_abs_time_to_seconds (int sf2_abs_time)
{
  return (pow (2.0, (double)sf2_abs_time / 1200.0));
}

/**
 * ipatch_unit_seconds_to_sf2_abs_time:
 * @sec: Value in seconds
 *
 * Convert value from seconds to SoundFont absolute time (timecents).
 *
 * Returns: Value in SoundFont absolute time
 */
int
ipatch_unit_seconds_to_sf2_abs_time (double sec)
{
  return (log (sec) / log (2) * 1200 + 0.5); /* +0.5 for rounding */
}

/**
 * ipatch_unit_sf2_ofs_time_to_multiplier:
 * @sf2_ofs_time: Value in SoundFont offset time (timecents)
 *
 * Convert a value from SoundFont offset time (timecents) to a multiplier.
 *
 * Returns: Multiplier factor
 */
double
ipatch_unit_sf2_ofs_time_to_multiplier (int sf2_ofs_time)
{
  return (pow (2.0, (double)sf2_ofs_time / 1200.0));
}

/**
 * ipatch_unit_multiplier_to_sf2_ofs_time:
 * @multiplier: Multiplier factor
 *
 * Convert value from a multiplier to SoundFont offset time (timecents).
 *
 * Returns: Value in SoundFont offset time (timecents)
 */
int
ipatch_unit_multiplier_to_sf2_ofs_time (double multiplier)
{
  return (log (multiplier) / log (2) * 1200 + 0.5); /* +0.5 for rounding */
}

/**
 * ipatch_unit_centibels_to_dls_gain:
 * @centibel: Value in centibels (10th of a Decibel)
 *
 * Convert a value from centibels to DLS gain (1/655360th of a dB).
 *
 * V = target amplitude, v = original amplitude
 * centibel = 200 * log10 (v / V)
 * dls_gain = 200 * 65536 * log10 (V / v)
 *
 * Returns: Value converted to DLS gain
 */
int
ipatch_unit_centibels_to_dls_gain (int centibel)
{
  return (centibel * 65536);
}

/**
 * ipatch_unit_dls_gain_to_centibels:
 * @dls_gain: Value in DLS gain (1/655360th of a dB)
 *
 * Convert a value from DLS gain to centibels.
 *
 * Returns: Value converted to centibels.
 */
int
ipatch_unit_dls_gain_to_centibels (int dls_gain)
{
  return ((dls_gain + 32768) / 65536); /* +32768 for rounding */
}

/**
 * ipatch_unit_centibels_to_decibels:
 * @cb: Value in Centibels (10th of a Decibel)
 *
 * Convert a value from Centibels to Decibels.
 *
 * Returns: Value in Decibels
 */
double
ipatch_unit_centibels_to_decibels (int cb)
{
  return ((double) cb / 10.0);
}

/**
 * ipatch_unit_decibels_to_centibels:
 * @db: Value in Decibels
 *
 * Convert Decibels to Centibels (10ths of a dB)
 *
 * Returns: Converted value in Centibels (10ths of a Decibel)
 */
int
ipatch_unit_decibels_to_centibels (double db)
{
  return (db * 10.0 + 0.5);	/* +0.5 for rounding */
}

/**
 * ipatch_unit_tenth_percent_to_percent:
 * @tenth_percent: Value in 10ths of a Percent (Percent * 10)
 *
 * Convert a value from 10ths of a Percent to Percent.
 *
 * Returns: Value in Percent
 */
double
ipatch_unit_tenth_percent_to_percent (int tenth_percent)
{
  return ((double) tenth_percent / 10.0);
}

/**
 * ipatch_unit_percent_to_tenth_percent:
 * @percent: Value in Percent
 *
 * Convert Percent to 10ths of a Percent (Percent * 10)
 *
 * Returns: Converted value in 10ths of a Percent
 */
int
ipatch_unit_percent_to_tenth_percent (double percent)
{
  return (percent * 10.0 + 0.5);	/* +0.5 for rounding */
}

/* =================================================
   GValue conversion functions, duplicated for speed
   ================================================= */


static void
ipatch_unit_sf2_abs_pitch_to_dls_abs_pitch_value (const GValue *src_val,
						  GValue *dest_val)
{
  int sf2_abs_pitch = g_value_get_int (src_val);
  int dls_abs_pitch;
  double hz;

  hz = 8.176 * pow (2.0, ((double)sf2_abs_pitch) / 1200.0);
  dls_abs_pitch = (int)((1200.0 * (log (hz / 440.0) / log (2.0)) + 6900.0)
			* 65536.0 + 0.5);
  g_value_set_int (dest_val, dls_abs_pitch);
}

static void
ipatch_unit_dls_abs_pitch_to_sf2_abs_pitch_value (const GValue *src_val,
						  GValue *dest_val)
{
  int dls_abs_pitch = g_value_get_int (src_val);
  int sf2_abs_pitch;
  double hz;

  hz = 440.0 * pow (2.0, (((double)dls_abs_pitch / 65536.0 - 6900.0)
			  / 1200.0));
  sf2_abs_pitch = (int)(1200.0 * (log (hz / 8.176) / log (2.0)) + 0.5);
  g_value_set_int (dest_val, sf2_abs_pitch);
}

static void
ipatch_unit_sf2_abs_pitch_to_hertz_value (const GValue *src_val,
					  GValue *dest_val)
{
  int sf2_abs_pitch = g_value_get_int (src_val);
  g_value_set_double (dest_val, 8.176 * pow (2.0, ((double) sf2_abs_pitch)
					    / 1200.0));
}

static void
ipatch_unit_hertz_to_sf2_abs_pitch_value (const GValue *src_val,
					  GValue *dest_val)
{
  double hz = g_value_get_double (src_val);
  g_value_set_int (dest_val, log (hz / 8.176) / log (2) * 1200 + 0.5);
}

static void
ipatch_unit_sf2_abs_time_to_dls_abs_time_value (const GValue *src_val,
						GValue *dest_val)
{
  int sf2_abs_time = g_value_get_int (src_val);
  g_value_set_int (dest_val, sf2_abs_time * 65536);
}

static void
ipatch_unit_dls_abs_time_to_sf2_abs_time_value (const GValue *src_val,
						GValue *dest_val)
{
  int dls_abs_time = g_value_get_int (src_val);
  g_value_set_int (dest_val, (dls_abs_time + 32768) / 65536);
}

static void
ipatch_unit_sf2_abs_time_to_seconds_value (const GValue *src_val,
					   GValue *dest_val)
{
  int sf2_abs_time = g_value_get_int (src_val);
  g_value_set_double (dest_val, pow (2.0, (double)sf2_abs_time / 1200.0));
}

static void
ipatch_unit_seconds_to_sf2_abs_time_value (const GValue *src_val,
					   GValue *dest_val)
{
  double sec = g_value_get_double (src_val);
  g_value_set_int (dest_val, log (sec) / log (2) * 1200 + 0.5);
}

static void
ipatch_unit_centibels_to_dls_gain_value (const GValue *src_val,
					 GValue *dest_val)
{
  int centibels = g_value_get_int (src_val);
  g_value_set_int (dest_val, centibels * 65536);
}

static void
ipatch_unit_dls_gain_to_centibels_value (const GValue *src_val,
					 GValue *dest_val)
{
  int dls_gain = g_value_get_int (src_val);
  g_value_set_int (dest_val, (dls_gain + 32768) / 65536);
}

static void
ipatch_unit_centibels_to_decibels_value (const GValue *src_val,
					 GValue *dest_val)
{
  int cb = g_value_get_int (src_val);
  g_value_set_double (dest_val, (double)cb / 10.0);
}

static void
ipatch_unit_decibels_to_centibels_value (const GValue *src_val,
					 GValue *dest_val)
{
  double db = g_value_get_double (src_val);
  g_value_set_int (dest_val, db * 10.0 + 0.5);
}

static void
ipatch_unit_tenth_percent_to_percent_value (const GValue *src_val,
					    GValue *dest_val)
{
  int tenthperc = g_value_get_int (src_val);
  g_value_set_double (dest_val, (double)tenthperc / 10.0);
}

static void
ipatch_unit_percent_to_tenth_percent_value (const GValue *src_val,
					    GValue *dest_val)
{
  double percent = g_value_get_double (src_val);
  g_value_set_int (dest_val, percent * 10.0 + 0.5);
}
