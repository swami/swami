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
#ifndef __IPATCH_UNIT_SF2_H__
#define __IPATCH_UNIT_SF2_H__

#include <glib.h>
#include <glib-object.h>

int ipatch_unit_sf2_abs_pitch_to_dls_abs_pitch (int sf2_abs_pitch);
int ipatch_unit_dls_abs_pitch_to_sf2_abs_pitch (int dls_abs_pitch);
double ipatch_unit_sf2_abs_pitch_to_hertz (int sf2_abs_pitch);
int ipatch_unit_hertz_to_sf2_abs_pitch (double hz);
double ipatch_unit_sf2_ofs_pitch_to_multiplier (int sf2_ofs_pitch);
int ipatch_unit_multiplier_to_sf2_ofs_pitch (double multiplier);

int ipatch_unit_sf2_abs_time_to_dls_abs_time (int sf2_abs_time);
int ipatch_unit_dls_abs_time_to_sf2_abs_time (int dls_abs_time);
double ipatch_unit_sf2_abs_time_to_seconds (int sf2_abs_time);
int ipatch_unit_seconds_to_sf2_abs_time (double sec);
double ipatch_unit_sf2_ofs_time_to_multiplier (int sf2_ofs_time);
int ipatch_unit_multiplier_to_sf2_ofs_time (double multiplier);

int ipatch_unit_centibels_to_dls_gain (int centibel);
int ipatch_unit_dls_gain_to_centibels (int dls_gain);
double ipatch_unit_centibels_to_decibels (int cb);
int ipatch_unit_decibels_to_centibels (double db);

double ipatch_unit_tenth_percent_to_percent (int tenth_percent);
int ipatch_unit_percent_to_tenth_percent (double percent);

#endif
