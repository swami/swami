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
 * SECTION: IpatchUnit_DLS
 * @short_description: Unit types and conversions for DLS
 * @see_also: 
 * @stability: Stable
 */
#include <stdio.h>
#include <math.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchUnit_DLS.h"
#include "IpatchUnit.h"
#include "i18n.h"


static void
ipatch_unit_dls_percent_to_percent_value (const GValue *src_val,
					  GValue *dest_val);
static void
ipatch_unit_percent_to_dls_percent_value (const GValue *src_val,
					  GValue *dest_val);
static void ipatch_unit_dls_gain_to_decibels_value (const GValue *src_val,
						    GValue *dest_val);
static void ipatch_unit_decibels_to_dls_gain_value (const GValue *src_val,
						    GValue *dest_val);
static void
ipatch_unit_dls_abs_time_to_seconds_value (const GValue *src_val,
					   GValue *dest_val);
static void
ipatch_unit_seconds_to_dls_abs_time_value (const GValue *src_val,
					   GValue *dest_val);
static void
ipatch_unit_dls_rel_time_to_time_cents_value (const GValue *src_val,
					      GValue *dest_val);
static void
ipatch_unit_time_cents_to_dls_rel_time_value (const GValue *src_val,
					      GValue *dest_val);
static void
ipatch_unit_dls_abs_pitch_to_hertz_value (const GValue *src_val,
					  GValue *dest_val);
static void
ipatch_unit_hertz_to_dls_abs_pitch_value (const GValue *src_val,
					  GValue *dest_val);
static void
ipatch_unit_dls_rel_pitch_to_cents_value (const GValue *src_val,
					  GValue *dest_val);
static void
ipatch_unit_cents_to_dls_rel_pitch_value (const GValue *src_val,
					  GValue *dest_val);

/**
 * _ipatch_unit_dls_init: (skip)
 */
void
_ipatch_unit_dls_init (void)
{
  IpatchUnitInfo *info;

  info = ipatch_unit_info_new ();
  info->label = NULL;
  info->descr = NULL;
  info->value_type = G_TYPE_INT;
  info->digits = 0;

  info->id = IPATCH_UNIT_TYPE_DLS_PERCENT;
  info->name = "DLSPercent";
  ipatch_unit_register (info);

  info->id = IPATCH_UNIT_TYPE_DLS_GAIN;
  info->name = "DLSGain";
  ipatch_unit_register (info);

  info->id = IPATCH_UNIT_TYPE_DLS_ABS_TIME;
  info->name = "DLSAbsTime";
  ipatch_unit_register (info);

  info->id = IPATCH_UNIT_TYPE_DLS_REL_TIME;
  info->name = "DLSRelTime";
  ipatch_unit_register (info);

  info->id = IPATCH_UNIT_TYPE_DLS_ABS_PITCH;
  info->name = "DLSAbsPitch";
  ipatch_unit_register (info);

  info->id = IPATCH_UNIT_TYPE_DLS_REL_PITCH;
  info->name = "DLSRelPitch";
  ipatch_unit_register (info);

  ipatch_unit_info_free (info);	/* done with unit info structure, free it */

  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_DLS_PERCENT, IPATCH_UNIT_TYPE_PERCENT,
     ipatch_unit_dls_percent_to_percent_value);
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_PERCENT, IPATCH_UNIT_TYPE_DLS_PERCENT,
     ipatch_unit_percent_to_dls_percent_value);

  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_DLS_GAIN, IPATCH_UNIT_TYPE_DECIBELS,
     ipatch_unit_dls_gain_to_decibels_value);
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_DECIBELS, IPATCH_UNIT_TYPE_DLS_GAIN,
     ipatch_unit_decibels_to_dls_gain_value);

  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_DLS_ABS_TIME, IPATCH_UNIT_TYPE_SECONDS,
     ipatch_unit_dls_abs_time_to_seconds_value);
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_SECONDS, IPATCH_UNIT_TYPE_DLS_ABS_TIME,
     ipatch_unit_seconds_to_dls_abs_time_value);

  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_DLS_REL_TIME, IPATCH_UNIT_TYPE_TIME_CENTS,
     ipatch_unit_dls_rel_time_to_time_cents_value);
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_TIME_CENTS, IPATCH_UNIT_TYPE_DLS_REL_TIME,
     ipatch_unit_time_cents_to_dls_rel_time_value);

  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_DLS_ABS_PITCH, IPATCH_UNIT_TYPE_HERTZ,
     ipatch_unit_dls_abs_pitch_to_hertz_value);
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_HERTZ, IPATCH_UNIT_TYPE_DLS_ABS_PITCH,
     ipatch_unit_hertz_to_dls_abs_pitch_value);

  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_DLS_REL_PITCH, IPATCH_UNIT_TYPE_CENTS,
     ipatch_unit_dls_rel_pitch_to_cents_value);
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_CENTS, IPATCH_UNIT_TYPE_DLS_REL_PITCH,
     ipatch_unit_cents_to_dls_rel_pitch_value);


  ipatch_unit_class_register_map (IPATCH_UNIT_CLASS_USER,
				  IPATCH_UNIT_TYPE_DLS_PERCENT,
				  IPATCH_UNIT_TYPE_PERCENT);
  ipatch_unit_class_register_map (IPATCH_UNIT_CLASS_USER,
				  IPATCH_UNIT_TYPE_DLS_GAIN,
				  IPATCH_UNIT_TYPE_DECIBELS);
  ipatch_unit_class_register_map (IPATCH_UNIT_CLASS_USER,
				  IPATCH_UNIT_TYPE_DLS_ABS_TIME,
				  IPATCH_UNIT_TYPE_SECONDS);
  ipatch_unit_class_register_map (IPATCH_UNIT_CLASS_USER,
				  IPATCH_UNIT_TYPE_DLS_REL_TIME,
				  IPATCH_UNIT_TYPE_TIME_CENTS);
  ipatch_unit_class_register_map (IPATCH_UNIT_CLASS_USER,
				  IPATCH_UNIT_TYPE_DLS_ABS_PITCH,
				  IPATCH_UNIT_TYPE_HERTZ);
  ipatch_unit_class_register_map (IPATCH_UNIT_CLASS_USER,
				  IPATCH_UNIT_TYPE_DLS_REL_PITCH,
				  IPATCH_UNIT_TYPE_CENTS);
}

/**
 * ipatch_unit_dls_class_convert:
 * @src_units: Source unit type ID
 * @src_val: Source value (type should be compatible with the source unit's
 *   value type)
 * 
 * Converts a value to "DLS" units.  DLS units are unit types that
 * are used by DLS (Downloadable Sounds) patches.  The #IPATCH_UNIT_CLASS_DLS
 * map is used to lookup the corresponding type to convert to.
 * Only some types have an associated DLS type.  It is an error to pass a
 * @src_units type that has no DLS mapping (note that this is contrary to the
 * behavior of ipatch_unit_user_class_convert()).
 *
 * Returns: The value converted to DLS units.
 */
int
ipatch_unit_dls_class_convert (guint16 src_units, const GValue *src_val)
{
  IpatchUnitInfo *dest_info;
  GValue v = { 0 };
  int retval;

  g_return_val_if_fail (src_val != NULL, 0);

  dest_info = ipatch_unit_class_lookup_map (IPATCH_UNIT_CLASS_DLS, src_units);
  g_return_val_if_fail (dest_info != NULL, 0);

  g_value_init (&v, G_TYPE_INT);
  ipatch_unit_convert (src_units, dest_info->id, src_val, &v);

  retval = g_value_get_int (&v);
  g_value_unset (&v);	/* probably not needed, for the sake of extra paranoia (TM) */

  return (retval);
}

/**
 * ipatch_unit_dls_percent_to_percent:
 * @dls_percent: Value in DLS percent units
 *
 * Convert value in DLS percent units to percent.
 *
 * percent = dls_percent / (10 * 65536)
 *
 * Returns: Value in percent
 */
double
ipatch_unit_dls_percent_to_percent (int dls_percent)
{
  return ((double)dls_percent / 655360.0);
}

/**
 * ipatch_unit_percent_to_dls_percent:
 * @percent: Value in percent
 *
 * Convert percent to DLS percent.
 *
 * dls_percent = percent * 10 * 65536
 *
 * Returns: Converted integer in DLS percent
 */
int
ipatch_unit_percent_to_dls_percent (double percent)
{
  return (percent * 655360.0 + 0.5);	/* +0.5 for rounding */
}

/**
 * ipatch_unit_dls_gain_to_decibels:
 * @dls_gain: Value in DLS gain units
 *
 * Converts a value from DLS gain to decibels.
 *
 * dls_gain = 200 * 65536 * log10 (V / v)
 * decibels = 20 * log10 (V / v)
 *
 * Returns: Value converted to decibels
 */
double
ipatch_unit_dls_gain_to_decibels (int dls_gain)
{
  return ((double)dls_gain / 655360.0);
}

/**
 * ipatch_unit_decibels_to_dls_gain:
 * @db: Value in decibels
 *
 * Converts a value from decibels to DLS gain.
 * See ipatch_unit_dls_gain_to_decibel()
 *
 * Returns: Value converted to DLS gain
 */
int
ipatch_unit_decibels_to_dls_gain (double db)
{
  return (db * 655360.0 + 0.5);
}

/**
 * ipatch_unit_dls_abs_time_to_seconds:
 * @dls_abs_time: Value in DLS absolute time
 * 
 * Converts a value from DLS absolute time to seconds.
 * seconds = 2^(dls_abs_time / (1200 * 65536))
 *
 * 0x80000000 is used as a 0 value.
 *
 * Returns: Value converted to seconds
 */
double
ipatch_unit_dls_abs_time_to_seconds (gint32 dls_abs_time)
{
  if (dls_abs_time == IPATCH_UNIT_DLS_ABS_TIME_0SECS)
    return (0.0);

  return (pow (2.0, (double)dls_abs_time / (1200 * 65536)));
}

/**
 * ipatch_unit_seconds_to_dls_abs_time:
 * @seconds: Value in seconds
 *
 * Converts a value from seconds to DLS absolute time.
 * dls_rel_time = 1200 * log2 (seconds) * 65536
 *
 * Returns: Value converted to DLS relative time
 */
gint32
ipatch_unit_seconds_to_dls_abs_time (double seconds)
{
  if (seconds == 0.0) return (IPATCH_UNIT_DLS_ABS_TIME_0SECS);

  return ((double)1200.0 * (log (seconds) / log (2.0)) * 65536.0 + 0.5);
}

/**
 * ipatch_unit_dls_rel_time_to_time_cents:
 * @dls_rel_time: Value in DLS relative time
 * 
 * Converts a value from DLS relative time to time cents.
 * time_cents = dls_rel_time / 65536
 *
 * Returns: Value converted to time cents
 */
double
ipatch_unit_dls_rel_time_to_time_cents (int dls_rel_time)
{
  return (dls_rel_time / 65536.0);
}

/**
 * ipatch_unit_time_cents_to_dls_rel_time:
 * @time_cents: Value in time cents
 * 
 * Converts a value from time_cents to DLS relative time.
 * dls_rel_time = time_cents * 65536
 *
 * Returns: Value converted to DLS relative time
 */
int
ipatch_unit_time_cents_to_dls_rel_time (double time_cents)
{
  return (time_cents * 65536.0 + 0.5);
}

/**
 * ipatch_unit_dls_abs_pitch_to_hertz:
 * @dls_abs_pitch: Value in DLS absolute pitch
 * 
 * Converts a value from DLS absolute pitch to hertz.
 * hertz = 440 * 2^((dls_abs_pitch / 65536 - 6900) / 1200)
 *
 * Returns: Value converted to hertz
 */
double
ipatch_unit_dls_abs_pitch_to_hertz (int dls_abs_pitch)
{
  return ((double)440.0 * pow (2.0, (dls_abs_pitch / 65536.0 - 6900.0) / 1200.0));
}

/**
 * ipatch_unit_hertz_to_dls_abs_pitch:
 * @hertz: Value in hertz
 * 
 * Converts a value from hertz to DLS absolute pitch.
 * dls_abs_pitch = (1200 * log2(hertz/440) + 6900) * 65536
 *
 * Returns: Value converted to DLS absolute pitch
 */
int
ipatch_unit_hertz_to_dls_abs_pitch (double hertz)
{
  return (((double)1200.0 * (log (hertz / 440.0) / log (2)) + 6900.0) * 65536.0 + 0.5);
}

/**
 * ipatch_unit_dls_rel_pitch_to_cents:
 * @dls_rel_pitch: Value in DLS relative pitch
 * 
 * Converts a value from DLS relative pitch to cents.
 * cents = dls_rel_pitch / 65536
 *
 * Returns: Value converted to cents
 */
double
ipatch_unit_dls_rel_pitch_to_cents (int dls_rel_pitch)
{
  return ((double)dls_rel_pitch / 65536.0);
}

/**
 * ipatch_unit_cents_to_dls_rel_pitch:
 * @cents: Value in cents
 * 
 * Converts a value from cents to DLS relative pitch.
 * dls_rel_pitch = cents * 65536
 *
 * Returns: Value converted to DLS relative pitch
 */
int
ipatch_unit_cents_to_dls_rel_pitch (double cents)
{
  return (cents * 65536.0 + 0.5);
}


/* =================================================
   GValue conversion functions, duplicated for speed
   ================================================= */


static void
ipatch_unit_dls_percent_to_percent_value (const GValue *src_val,
					  GValue *dest_val)
{
  int dls_percent = g_value_get_int (src_val);
  g_value_set_double (dest_val, (double)dls_percent / 655360.0);
}

static void
ipatch_unit_percent_to_dls_percent_value (const GValue *src_val,
					  GValue *dest_val)
{
  double percent = g_value_get_double (src_val);
  g_value_set_int (dest_val, percent * 655360.0 + 0.5);
}

static void
ipatch_unit_dls_gain_to_decibels_value (const GValue *src_val, GValue *dest_val)
{
  int dls_gain = g_value_get_int (src_val);
  g_value_set_double (dest_val, (double)dls_gain / 655360.0);
}

static void
ipatch_unit_decibels_to_dls_gain_value (const GValue *src_val, GValue *dest_val)
{
  double db = g_value_get_double (src_val);
  g_value_set_int (dest_val, db * 655360.0 + 0.5);
}

static void
ipatch_unit_dls_abs_time_to_seconds_value (const GValue *src_val,
					   GValue *dest_val)
{
  int dls_abs_time = g_value_get_int (src_val);
  double secs;

  if (dls_abs_time != IPATCH_UNIT_DLS_ABS_TIME_0SECS)
    secs = pow (2.0, (double)dls_abs_time / (1200 * 65536));
  else secs = 0.0;

  g_value_set_double (dest_val, secs);
}

static void
ipatch_unit_seconds_to_dls_abs_time_value (const GValue *src_val,
					   GValue *dest_val)
{
  double secs = g_value_get_double (src_val);
  int dls_abs_time;

  if (secs != 0.0)
    dls_abs_time = (double)1200.0 * (log (secs) / log (2.0)) * 65536.0 + 0.5;
  else dls_abs_time = IPATCH_UNIT_DLS_ABS_TIME_0SECS;

  g_value_set_int (dest_val, dls_abs_time);
}

static void
ipatch_unit_dls_rel_time_to_time_cents_value (const GValue *src_val,
					      GValue *dest_val)
{
  int dls_rel_time = g_value_get_int (src_val);
  g_value_set_double (dest_val, (double)dls_rel_time / 65536.0);
}

static void
ipatch_unit_time_cents_to_dls_rel_time_value (const GValue *src_val,
					      GValue *dest_val)
{
  double time_cents = g_value_get_double (src_val);
  g_value_set_int (dest_val, time_cents * 65536.0 + 0.5);
}

static void
ipatch_unit_dls_abs_pitch_to_hertz_value (const GValue *src_val,
					  GValue *dest_val)
{
  int dls_abs_pitch = g_value_get_int (src_val);
  g_value_set_double (dest_val, 440.0 * pow (2.0, ((double)dls_abs_pitch
						   / 65536.0 - 6900.0) / 1200.0));
}

static void
ipatch_unit_hertz_to_dls_abs_pitch_value (const GValue *src_val,
					  GValue *dest_val)
{
  double hertz = g_value_get_double (src_val);
  g_value_set_int (dest_val, ((double)1200.0 * (log (hertz / 440.0) / log (2))
			      + 6900.0) * 65536.0 + 0.5);
}

static void
ipatch_unit_dls_rel_pitch_to_cents_value (const GValue *src_val,
					  GValue *dest_val)
{
  int dls_rel_pitch = g_value_get_int (src_val);
  g_value_set_double (dest_val, (double)dls_rel_pitch / 65536.0);
}

static void
ipatch_unit_cents_to_dls_rel_pitch_value (const GValue *src_val,
					  GValue *dest_val)
{
  double cents = g_value_get_double (src_val);
  g_value_set_int (dest_val, cents * 65536.0 + 0.5);
}
