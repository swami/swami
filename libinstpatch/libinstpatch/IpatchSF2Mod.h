/*
 * libInstPatch
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Moderal Public License
 * as published by the Free Software Foundation; version 2.1
 * of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Moderal Public License for more details.
 *
 * You should have received a copy of the GNU Moderal Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA or on the web at http://www.gnu.org.
 */
#ifndef __IPATCH_SF2_MOD_H__
#define __IPATCH_SF2_MOD_H__

#include <glib.h>
#include <glib-object.h>

#include <libinstpatch/IpatchItem.h>

/* forward type declarations */

typedef struct _IpatchSF2Mod IpatchSF2Mod;

/* IpatchSF2Mod has a glib boxed type */
#define IPATCH_TYPE_SF2_MOD   (ipatch_sf2_mod_get_type ())

/* modulator structure */
struct _IpatchSF2Mod
{
  guint16 src;			/* source modulator (MIDI controller, etc) */
  guint16 dest;			/* destination generator */
  gint16 amount;		/* degree of modulation */
  guint16 amtsrc;		/* second source controls amount of first */
  guint16 trans;		/* transform function applied to source */
};

#include <libinstpatch/IpatchSF2ModList.h>      // For backwards compatibility where IpatchSF2Mod and IpatchSF2ModList were combined

/* Compare two modulators to see if they are identical and
   can be combined (all fields except amount must be identical).
   Returns TRUE if identical, FALSE otherwise */
#define IPATCH_SF2_MOD_ARE_IDENTICAL(a, b) \
   ((a)->src == (b)->src && (a)->dest == (b)->dest \
    && (a)->amtsrc == (b)->amtsrc && (a)->trans == (b)->trans)

/* like IPATCH_SF2_MOD_ARE_IDENTICAL but also checks if amounts are
   identical */
#define IPATCH_SF2_MOD_ARE_IDENTICAL_AMOUNT(a, b) \
   ((a)->src == (b)->src && (a)->dest == (b)->dest \
    && (a)->amtsrc == (b)->amtsrc && (a)->trans == (b)->trans \
    && (a)->amount == (b)->amount)

/* modulator flags */

typedef enum
{
  IPATCH_SF2_MOD_MASK_CONTROL = 0x007F,
  IPATCH_SF2_MOD_MASK_CC = 0x0080,
  IPATCH_SF2_MOD_MASK_DIRECTION = 0x0100,
  IPATCH_SF2_MOD_MASK_POLARITY = 0x0200,
  IPATCH_SF2_MOD_MASK_TYPE = 0xFC00
} IpatchSF2ModFieldMasks;

typedef enum
{
  IPATCH_SF2_MOD_SHIFT_CONTROL = 0,
  IPATCH_SF2_MOD_SHIFT_CC = 7,
  IPATCH_SF2_MOD_SHIFT_DIRECTION = 8,
  IPATCH_SF2_MOD_SHIFT_POLARITY = 9,
  IPATCH_SF2_MOD_SHIFT_TYPE = 10
} IpatchSF2ModFieldShifts;

typedef enum
{
  IPATCH_SF2_MOD_CONTROL_NONE = 0,
  IPATCH_SF2_MOD_CONTROL_NOTE_ON_VELOCITY = 2,
  IPATCH_SF2_MOD_CONTROL_NOTE_NUMBER = 3,
  IPATCH_SF2_MOD_CONTROL_POLY_PRESSURE = 10,
  IPATCH_SF2_MOD_CONTROL_CHAN_PRESSURE = 13,
  IPATCH_SF2_MOD_CONTROL_PITCH_WHEEL = 14,
  IPATCH_SF2_MOD_CONTROL_BEND_RANGE = 16
} IpatchSF2ModControl;

typedef enum
{
  IPATCH_SF2_MOD_CC_GENERAL = (0 << IPATCH_SF2_MOD_SHIFT_CC),
  IPATCH_SF2_MOD_CC_MIDI = (1 << IPATCH_SF2_MOD_SHIFT_CC)
} IpatchSF2ModControlPalette;

typedef enum
{
  IPATCH_SF2_MOD_DIRECTION_POSITIVE = (0 << IPATCH_SF2_MOD_SHIFT_DIRECTION),
  IPATCH_SF2_MOD_DIRECTION_NEGATIVE = (1 << IPATCH_SF2_MOD_SHIFT_DIRECTION)
} IpatchSF2ModDirection;

typedef enum
{
  IPATCH_SF2_MOD_POLARITY_UNIPOLAR = (0 << IPATCH_SF2_MOD_SHIFT_POLARITY),
  IPATCH_SF2_MOD_POLARITY_BIPOLAR = (1 << IPATCH_SF2_MOD_SHIFT_POLARITY)
} IpatchSF2ModPolarity;

typedef enum
{
  IPATCH_SF2_MOD_TYPE_LINEAR = (0 << IPATCH_SF2_MOD_SHIFT_TYPE),
  IPATCH_SF2_MOD_TYPE_CONCAVE = (1 << IPATCH_SF2_MOD_SHIFT_TYPE),
  IPATCH_SF2_MOD_TYPE_CONVEX = (2 << IPATCH_SF2_MOD_SHIFT_TYPE),
  IPATCH_SF2_MOD_TYPE_SWITCH = (3 << IPATCH_SF2_MOD_SHIFT_TYPE)
} IpatchSF2ModType;

typedef enum
{
  IPATCH_SF2_MOD_TRANSFORM_LINEAR = 0
} IpatchSF2ModTransform;


/* flags for ipatch_sf2_mod_class_set_mods() */
typedef enum
{
  IPATCH_SF2_MOD_NO_DUPLICATE	= 1 << 0,  /* don't duplicate mod list (owned!) */
  IPATCH_SF2_MOD_NO_NOTIFY	= 1 << 1   /* don't do item property notify */
} IpatchSF2ModFlags;

GType ipatch_sf2_mod_get_type (void);

IpatchSF2Mod *ipatch_sf2_mod_new (void);
void ipatch_sf2_mod_free (IpatchSF2Mod *mod);
IpatchSF2Mod *ipatch_sf2_mod_duplicate (const IpatchSF2Mod *mod);

#endif
