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
#ifndef __IPATCH_UNIT_DLS_H__
#define __IPATCH_UNIT_DLS_H__

#include <glib.h>
#include <glib-object.h>

/* Value for 0 seconds in DLS absolute time (a degenerate case) */
#define IPATCH_UNIT_DLS_ABS_TIME_0SECS ((gint32)0x80000000L)

int ipatch_unit_dls_class_convert (guint16 src_units, const GValue *src_val);

double ipatch_unit_dls_percent_to_percent (int dls_percent);
int ipatch_unit_percent_to_dls_percent (double percent);
double ipatch_unit_dls_gain_to_decibels (int dls_gain);
int ipatch_unit_decibels_to_dls_gain (double db);
double ipatch_unit_dls_abs_time_to_seconds (int dls_abs_time);
int ipatch_unit_seconds_to_dls_abs_time (double seconds);
double ipatch_unit_dls_rel_time_to_time_cents (int dls_rel_time);
int ipatch_unit_time_cents_to_dls_rel_time (double time_cents);
double ipatch_unit_dls_abs_pitch_to_hertz (int dls_abs_pitch);
int ipatch_unit_hertz_to_dls_abs_pitch (double hertz);
double ipatch_unit_dls_rel_pitch_to_cents (int dls_rel_pitch);
int ipatch_unit_cents_to_dls_rel_pitch (double cents);

#endif
