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
 * SECTION: IpatchUnit_generic
 * @short_description: Generic unit types and conversions
 * @see_also: 
 * @stability: Stable
 */
#include <stdio.h>
#include <math.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchUnit_generic.h"
#include "IpatchUnit.h"
#include "IpatchRange.h"
#include "i18n.h"

/* used to convert cents (100ths of a semitone) to hertz */
#define HERTZ_CENTS_FACTOR  8.175798915643707


static void ipatch_unit_cents_to_hertz_value (const GValue *src_val,
					      GValue *dest_val);
static void ipatch_unit_hertz_to_cents_value (const GValue *src_val,
					      GValue *dest_val);

/**
 * _ipatch_unit_generic_init: (skip)
 */
void
_ipatch_unit_generic_init (void)
{
  IpatchUnitInfo *info;

  /* register generic unit types */
  info = ipatch_unit_info_new ();

  info->name = "Int";
  info->id = IPATCH_UNIT_TYPE_INT;
  info->label = NULL;
  info->descr = NULL;
  info->value_type = G_TYPE_INT;
  info->flags = 0;
  info->digits = 0;
  ipatch_unit_register (info);

  info->name = "UInt";
  info->id = IPATCH_UNIT_TYPE_UINT;
  info->value_type = G_TYPE_UINT;
  ipatch_unit_register (info);

  info->name = "Range";
  info->id = IPATCH_UNIT_TYPE_RANGE;
  info->value_type = IPATCH_TYPE_RANGE;
  ipatch_unit_register (info);

  info->name = "Decibels";
  info->id = IPATCH_UNIT_TYPE_DECIBELS;
  info->label = _("dB");	/* abbreviation for decibel */
  info->descr = _("Decibels");
  info->value_type = G_TYPE_DOUBLE;
  info->flags = IPATCH_UNIT_LOGARITHMIC | IPATCH_UNIT_USER;
  info->digits = 3;
  ipatch_unit_register (info);

  info->name = "Percent";
  info->id = IPATCH_UNIT_TYPE_PERCENT;
  info->label = _("%");		/* abbreviation for percent */
  info->descr = _("Percent");
  info->value_type = G_TYPE_DOUBLE;
  info->flags = IPATCH_UNIT_USER;
  info->digits = 1;
  ipatch_unit_register (info);

  info->name = "Semitones";
  info->id = IPATCH_UNIT_TYPE_SEMITONES;
  info->label = _("Notes");
  info->descr = _("Unit of pitch ratio (one note)");
  info->value_type = G_TYPE_DOUBLE;
  info->flags = IPATCH_UNIT_USER;
  info->digits = 0;
  ipatch_unit_register (info);

  info->name = "Cents";
  info->id = IPATCH_UNIT_TYPE_CENTS;
  info->label = _("Cents");
  info->descr = _("Unit of pitch ratio (100th of a semitone)");
  info->value_type = G_TYPE_DOUBLE;
  info->flags = IPATCH_UNIT_USER;
  info->digits = 0;
  ipatch_unit_register (info);

  info->name = "TimeCents";
  info->id = IPATCH_UNIT_TYPE_TIME_CENTS;
  info->label = _("T-Cents");
  info->descr = _("Time ratio in cents (1200 cents = 2x)");
  info->value_type = G_TYPE_DOUBLE;
  info->flags = IPATCH_UNIT_USER;
  info->digits = 3;
  ipatch_unit_register (info);

  info->name = "SampleRate";
  info->id = IPATCH_UNIT_TYPE_SAMPLE_RATE;
  info->label = _("Rate");	/* abbreviation for sample rate */
  info->descr = _("Audio sampling rate");
  info->value_type = G_TYPE_DOUBLE;
  info->flags = IPATCH_UNIT_USER;
  info->digits = 0;
  ipatch_unit_register (info);

  info->name = "Samples";
  info->id = IPATCH_UNIT_TYPE_SAMPLES;
  info->label = _("Samples");
  info->descr = _("Number of sample points");
  info->value_type = G_TYPE_INT;
  info->flags = IPATCH_UNIT_USER;
  info->digits = 0;
  ipatch_unit_register (info);

  info->name = "Hertz";
  info->id = IPATCH_UNIT_TYPE_HERTZ;
  info->label = _("Hz");	/* abbreviation for Hertz */
  info->descr = _("Frequency in Hertz (cycles per second)");
  info->value_type = G_TYPE_DOUBLE;
  info->flags = IPATCH_UNIT_USER;
  info->digits = 3;
  ipatch_unit_register (info);

  info->name = "Seconds";
  info->id = IPATCH_UNIT_TYPE_SECONDS;
  info->label = _("Sec");	/* abbreviation for seconds */
  info->descr = _("Amount of time in seconds");
  info->value_type = G_TYPE_DOUBLE;
  info->flags = IPATCH_UNIT_USER;
  info->digits = 3;
  ipatch_unit_register (info);

  info->name = "Multiplier";
  info->id = IPATCH_UNIT_TYPE_MULTIPLIER;
  info->label = _("X");	/* abbreviation for multiplier */
  info->descr = _("Multiplier");
  info->value_type = G_TYPE_DOUBLE;
  info->flags = IPATCH_UNIT_USER;
  info->digits = 3;
  ipatch_unit_register (info);

  ipatch_unit_info_free (info);	/* done with unit info structure, free it */


  /* conversion functions */


  /* Hertz <==> Cents */
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_CENTS, IPATCH_UNIT_TYPE_HERTZ,
     ipatch_unit_cents_to_hertz_value);
  ipatch_unit_conversion_register
    (IPATCH_UNIT_TYPE_HERTZ, IPATCH_UNIT_TYPE_CENTS,
     ipatch_unit_hertz_to_cents_value);
}

/**
 * ipatch_unit_cents_to_hertz:
 * @cents: Value in cents
 *
 * Convert cents to relative frequency in Hertz.
 *
 * Returns: Value in relative Hertz (cycles per second)
 */
double
ipatch_unit_cents_to_hertz (double cents)
{
  return (HERTZ_CENTS_FACTOR * pow (2.0, cents / 1200.0));
}

/**
 * ipatch_unit_hertz_to_cents:
 * @hz: Hertz (cycles per second) value
 *
 * Convert frequency in Hertz to relative cents.
 *
 * Returns: Converted value in relative cents.
 */
double
ipatch_unit_hertz_to_cents (double hz)
{
  return (log (hz / HERTZ_CENTS_FACTOR) / log (2) * 1200);
}


/* =================================================
   GValue conversion functions, duplicated for speed
   ================================================= */


static void
ipatch_unit_cents_to_hertz_value (const GValue *src_val, GValue *dest_val)
{
  double cents = g_value_get_double (src_val);
  g_value_set_double (dest_val, HERTZ_CENTS_FACTOR
		     * pow (2.0, cents / 1200.0));
}

static void
ipatch_unit_hertz_to_cents_value (const GValue *src_val, GValue *dest_val)
{
  double hz = g_value_get_double (src_val);
  g_value_set_double (dest_val, log (hz / HERTZ_CENTS_FACTOR) / log (2) * 1200);
}
