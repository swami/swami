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
#ifndef __IPATCH_GIG_DIMENSION_H__
#define __IPATCH_GIG_DIMENSION_H__

#include <glib.h>
#include <glib-object.h>

/* forward type declarations */
typedef struct _IpatchGigDimension IpatchGigDimension;
typedef struct _IpatchGigDimensionClass IpatchGigDimensionClass;

/* GigaSampler dimension type */
typedef enum
{
  IPATCH_GIG_DIMENSION_NONE            = 0x00, /* not in use (is this in files?) */

  /* MIDI controllers - see IpatchGigControlType (IpatchGigEffects.h) */

  IPATCH_GIG_DIMENSION_MOD_WHEEL       = 0x01,
  IPATCH_GIG_DIMENSION_BREATH          = 0x02,
  IPATCH_GIG_DIMENSION_FOOT            = 0x04,
  IPATCH_GIG_DIMENSION_PORTAMENTO_TIME = 0x05,
  IPATCH_GIG_DIMENSION_EFFECT_1        = 0x0C,
  IPATCH_GIG_DIMENSION_EFFECT_2        = 0x0D,
  IPATCH_GIG_DIMENSION_GEN_PURPOSE_1   = 0x10,
  IPATCH_GIG_DIMENSION_GEN_PURPOSE_2   = 0x11,
  IPATCH_GIG_DIMENSION_GEN_PURPOSE_3   = 0x12,
  IPATCH_GIG_DIMENSION_GEN_PURPOSE_4   = 0x13,
  IPATCH_GIG_DIMENSION_SUSTAIN_PEDAL   = 0x40,
  IPATCH_GIG_DIMENSION_PORTAMENTO      = 0x41,
  IPATCH_GIG_DIMENSION_SOSTENUTO       = 0x42,
  IPATCH_GIG_DIMENSION_SOFT_PEDAL      = 0x43,
  IPATCH_GIG_DIMENSION_GEN_PURPOSE_5   = 0x50,
  IPATCH_GIG_DIMENSION_GEN_PURPOSE_6   = 0x51,
  IPATCH_GIG_DIMENSION_GEN_PURPOSE_7   = 0x52,
  IPATCH_GIG_DIMENSION_GEN_PURPOSE_8   = 0x53,
  IPATCH_GIG_DIMENSION_EFFECT_DEPTH_1  = 0x5B,
  IPATCH_GIG_DIMENSION_EFFECT_DEPTH_2  = 0x5C,
  IPATCH_GIG_DIMENSION_EFFECT_DEPTH_3  = 0x5D,
  IPATCH_GIG_DIMENSION_EFFECT_DEPTH_4  = 0x5E,
  IPATCH_GIG_DIMENSION_EFFECT_DEPTH_5  = 0x5F,

  IPATCH_GIG_DIMENSION_CHANNEL         = 0x80, /* sample has more than 1 channel */
  IPATCH_GIG_DIMENSION_LAYER           = 0x81, /* layer up to 8 zones (cross fade 2 or 4) */
  IPATCH_GIG_DIMENSION_VELOCITY        = 0x82, /* key velocity (only type that allows specific ranges) */
  IPATCH_GIG_DIMENSION_AFTER_TOUCH     = 0x83, /* channel MIDI after touch */
  IPATCH_GIG_DIMENSION_RELEASE_TRIG    = 0x84, /* trigger on key release */
  IPATCH_GIG_DIMENSION_KEYBOARD        = 0x85, /* key switching (FIXME WTF?) */
  IPATCH_GIG_DIMENSION_ROUND_ROBIN     = 0x86, /* selects zones in sequence */
  IPATCH_GIG_DIMENSION_RANDOM          = 0x87  /* selects random zone */
} IpatchGigDimensionType;

/* maximum value for dimension type */
#define IPATCH_GIG_DIMENSION_TYPE_MAX   IPATCH_GIG_DIMENSION_RANDOM

#include <libinstpatch/IpatchItem.h>
#include <libinstpatch/IpatchGigRegion.h>

#define IPATCH_TYPE_GIG_DIMENSION   (ipatch_gig_dimension_get_type ())
#define IPATCH_GIG_DIMENSION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_GIG_DIMENSION, \
  IpatchGigDimension))
#define IPATCH_GIG_DIMENSION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_GIG_DIMENSION, \
  IpatchGigDimensionClass))
#define IPATCH_IS_GIG_DIMENSION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_GIG_DIMENSION))
#define IPATCH_IS_GIG_DIMENSION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_GIG_DIMENSION))
#define IPATCH_GIG_DIMENSION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_GIG_DIMENSION, \
  IpatchGigDimensionClass))

/* GigaSampler dimension (up to 5 per IpatchGigRegion) */
struct _IpatchGigDimension
{
  IpatchItem parent_instance;

  char *name;			/* name of dimension or NULL */
  guint8 type;			/* dimension type (IpatchGigDimensionType) */
  guint8 split_count;		/* count of split bits for this dimension */

  /* convenience variables (derivable from other info) */
  guint8 split_mask;		/* sub region index mask */
  guint8 split_shift;		/* bit shift to first set bit in mask */
};

/* GigaSampler dimension class */
struct _IpatchGigDimensionClass
{
  IpatchItemClass parent_class;
};

GType ipatch_gig_dimension_get_type (void);
IpatchGigDimension *ipatch_gig_dimension_new (void);

IpatchGigDimension *ipatch_gig_dimension_first (IpatchIter *iter);
IpatchGigDimension *ipatch_gig_dimension_next (IpatchIter *iter);

#endif
