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
 * SECTION: IpatchSF2Gen
 * @short_description: SoundFont generator functions and definitions
 * @see_also: 
 * @stability: Stable
 *
 * SoundFont generators are synthesis parameters used by #IpatchSF2Preset,
 * #IpatchSF2Inst, #IpatchSF2PZone and #IpatchSF2IZone objects.
 */
#include <stdio.h>
#include <stdarg.h>
#include <math.h>		/* for conversion functions */
#include <string.h>
#include <glib.h>
#include "IpatchSF2Gen.h"
#include "IpatchParamProp.h"
#include "IpatchRange.h"
#include "IpatchUnit.h"
#include "ipatch_priv.h"
#include "builtin_enums.h"
#include "util.h"


/* default arrays for offset and absolute (preset/instrument zones).
   For fast initialization of generator arrays. */
IpatchSF2GenArray *ipatch_sf2_gen_ofs_array = NULL;
IpatchSF2GenArray *ipatch_sf2_gen_abs_array = NULL;

/* masks of valid generators for offset or absolute arrays */
guint64 ipatch_sf2_gen_ofs_valid_mask;
guint64 ipatch_sf2_gen_abs_valid_mask;

/* a mask for generators that can be added together (ofs_mask ! ranges) */
guint64 ipatch_sf2_gen_add_mask;

/* array of property names by generator ID */
static const char **gen_property_names = NULL; /* names [IPATCH_SF2_GEN_COUNT] */

/**
 * _ipatch_sf2_gen_init: (skip)
 *
 * Library internal init function for SoundFont generator subsystem.
 */
void
_ipatch_sf2_gen_init (void)
{
  GEnumClass *enum_class;
  GEnumValue *enum_val;
  guint64 v;
  int i;

  /* initialize valid generator masks */
  for (i = 0, v = 0x1; i < IPATCH_SF2_GEN_COUNT; i++, v <<= 1)
    {
      switch (i)
	{
	case IPATCH_SF2_GEN_SAMPLE_START:
	case IPATCH_SF2_GEN_SAMPLE_END:
	case IPATCH_SF2_GEN_SAMPLE_LOOP_START:
	case IPATCH_SF2_GEN_SAMPLE_LOOP_END:
	case IPATCH_SF2_GEN_SAMPLE_COARSE_START:
	case IPATCH_SF2_GEN_SAMPLE_COARSE_END:
	case IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_START:
	case IPATCH_SF2_GEN_FIXED_NOTE:
	case IPATCH_SF2_GEN_FIXED_VELOCITY:
	case IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_END:
	case IPATCH_SF2_GEN_SAMPLE_MODES:
	case IPATCH_SF2_GEN_EXCLUSIVE_CLASS:
	case IPATCH_SF2_GEN_ROOT_NOTE_OVERRIDE:
	  ipatch_sf2_gen_abs_valid_mask |= v; /* OK for absolute generators */
	  break;
	case IPATCH_SF2_GEN_UNUSED1:
	case IPATCH_SF2_GEN_UNUSED2:
	case IPATCH_SF2_GEN_UNUSED3:
	case IPATCH_SF2_GEN_UNUSED4:
	case IPATCH_SF2_GEN_RESERVED1:
	case IPATCH_SF2_GEN_RESERVED2:
	case IPATCH_SF2_GEN_RESERVED3:
	case IPATCH_SF2_GEN_INSTRUMENT_ID: /* we don't use these in the API */
	case IPATCH_SF2_GEN_SAMPLE_ID: /* they get saved to files though */
	  break;		/* not valid for any generator type */
	default:		/* valid for either generator type */
	  ipatch_sf2_gen_ofs_valid_mask |= v;
	  ipatch_sf2_gen_abs_valid_mask |= v;
	  break;
	}
    }

  /* create generator add mask (gens that can be directly summed) */
  ipatch_sf2_gen_add_mask = ipatch_sf2_gen_ofs_valid_mask;

  /* I don't trust constants to be 64 bit (LL perhaps?), so .. */
  v = 1;
  v <<= IPATCH_SF2_GEN_NOTE_RANGE;
  ipatch_sf2_gen_add_mask &= ~v;
  
  v = 1;
  v <<= IPATCH_SF2_GEN_VELOCITY_RANGE;
  ipatch_sf2_gen_add_mask &= ~v;

  /* initialize default offset array values */
  ipatch_sf2_gen_ofs_array = ipatch_sf2_gen_array_new (TRUE);
  ipatch_sf2_gen_ofs_array->values[IPATCH_SF2_GEN_NOTE_RANGE].range.low = 0;
  ipatch_sf2_gen_ofs_array->values[IPATCH_SF2_GEN_NOTE_RANGE].range.high = 127;
  ipatch_sf2_gen_ofs_array->values[IPATCH_SF2_GEN_VELOCITY_RANGE].range.low =0;
  ipatch_sf2_gen_ofs_array->values[IPATCH_SF2_GEN_VELOCITY_RANGE].range.high = 127;

  /* initialize absolute generator default values */
  ipatch_sf2_gen_abs_array = ipatch_sf2_gen_array_new (TRUE);
  for (i = 0; i < IPATCH_SF2_GEN_COUNT; i++)
    ipatch_sf2_gen_abs_array->values[i] = ipatch_sf2_gen_info[i].def;

  /* init flags to all valid generators for the given type */
  ipatch_sf2_gen_ofs_array->flags = ipatch_sf2_gen_ofs_valid_mask;
  ipatch_sf2_gen_abs_array->flags = ipatch_sf2_gen_abs_valid_mask;


  /* initialize array mapping generator IDs to property names */

  gen_property_names = g_malloc (sizeof (char *) * IPATCH_SF2_GEN_COUNT);

  enum_class = g_type_class_ref (IPATCH_TYPE_SF2_GEN_TYPE);
  if (log_if_fail (enum_class != NULL))	/* shouldn't happen.. just in case */
    {
      for (i = 0; i < IPATCH_SF2_GEN_COUNT; i++)
	gen_property_names[i] = NULL;
      return;
    }

  for (i = 0; i < IPATCH_SF2_GEN_COUNT; i++)
    {
      enum_val = g_enum_get_value (enum_class, i);
      gen_property_names[i] = enum_val ? enum_val->value_nick : NULL;
    }
}

/**
 * ipatch_sf2_gen_is_valid:
 * @genid: Generator ID to check
 * @propstype: Generator property type (instrument/preset + global)
 *
 * Checks if a generator is valid for the given @propstype.
 *
 * Returns: %TRUE if valid, %FALSE otherwise
 */
gboolean
ipatch_sf2_gen_is_valid (guint genid, IpatchSF2GenPropsType propstype)
{
  if (genid == IPATCH_SF2_GEN_SAMPLE_MODES
      && propstype == IPATCH_SF2_GEN_PROPS_INST_GLOBAL)
    return (FALSE);
  else if ((propstype & 0x1) == IPATCH_SF2_GEN_PROPS_INST)
    return ((ipatch_sf2_gen_abs_valid_mask & ((guint64)0x1 << genid)) != 0);
  else return ((ipatch_sf2_gen_ofs_valid_mask & ((guint64)0x1 << genid)) != 0);
}

/* IpatchSF2GenArray boxed type */
GType
ipatch_sf2_gen_array_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_boxed_type_register_static ("IpatchSF2GenArray",
				(GBoxedCopyFunc)ipatch_sf2_gen_array_duplicate,
				(GBoxedFreeFunc)ipatch_sf2_gen_array_free);
  return (type);
}

/**
 * ipatch_sf2_gen_array_new:
 * @clear: If %TRUE then array will be cleared, %FALSE will not initalize it
 *   and it is up to the caller to do so.
 *
 * Create a new generator array object. A convenience function really,
 * because one could just allocate an IpatchSF2GenArray structure instead.
 *
 * Returns: New generator array
 */
IpatchSF2GenArray *
ipatch_sf2_gen_array_new (gboolean clear)
{
  if (!clear) return (g_new (IpatchSF2GenArray, 1));
  else return (g_new0 (IpatchSF2GenArray, 1));
}

/**
 * ipatch_sf2_gen_free:
 * @array: Generator array to free
 *
 * Frees a generator array structure.
 */
void
ipatch_sf2_gen_array_free (IpatchSF2GenArray *array)
{
  g_return_if_fail (array != NULL);
  g_free (array);
}

/**
 * ipatch_sf2_gen_array_duplicate:
 * @array: Generator array to duplicate
 *
 * Duplicates a generator array structure.
 *
 * Returns: A newly allocated generator array structure which is a duplicate
 *   of @array.
 */
IpatchSF2GenArray *
ipatch_sf2_gen_array_duplicate (const IpatchSF2GenArray *array)
{
  IpatchSF2GenArray *new;

  g_return_val_if_fail (array != NULL, NULL);

  new = g_new (IpatchSF2GenArray, 1);
  memcpy (new, array, sizeof (IpatchSF2GenArray));
  return (new);
}

/**
 * ipatch_sf2_gen_array_init:
 * @array: Generator array
 * @offset: %TRUE = initialize to Preset offset (zero) values,
 *   %FALSE = initialize to instrument default values
 * @set: %TRUE to set flags indicating generator values are set, %FALSE to
 *   clear all flag "set" bits
 *
 * Initialize a generator array to default values.
 */
void
ipatch_sf2_gen_array_init (IpatchSF2GenArray *array, gboolean offset,
			   gboolean set)
{
  IpatchSF2GenArray *duparray;

  g_return_if_fail (array != NULL);

  duparray = offset ? ipatch_sf2_gen_ofs_array : ipatch_sf2_gen_abs_array;
  memcpy (array->values, duparray->values, sizeof (array->values));
  if (set) array->flags = duparray->flags;
  else array->flags = 0;
}

/**
 * ipatch_sf2_gen_array_offset:
 * @abs_array: Destination generator amount array that contains absolute
 *   (Instrument) generator values
 * @ofs_array: Source generator amount array that contains offset (Preset)
 *   generator values
 *
 * Offsets the generators amount array in @abs_array by adding the
 * values in @ofs_array to it. Values are clamped to their valid ranges.
 *
 * Returns: %FALSE if note or velocity range does not intersect (in which case
 *   the non-intersecting ranges are left unassigned), %TRUE otherwise
 */
gboolean
ipatch_sf2_gen_array_offset (IpatchSF2GenArray *abs_array,
			     const IpatchSF2GenArray *ofs_array)
{
  IpatchSF2GenAmount *abs_vals;
  const IpatchSF2GenAmount *ofs_vals;
  gboolean retval;
  guint64 v;
  gint32 temp;
  int i;

  abs_vals = abs_array->values;
  ofs_vals = ofs_array->values;

  for (i = 0, v = 0x1; i < IPATCH_SF2_GEN_COUNT; i++, v <<= 1)
    { /* generator in add_mask and offset value set? */
      if ((ipatch_sf2_gen_add_mask & v) && (ofs_array->flags & v))
	{
	  temp = (gint32)(abs_vals[i].sword) + (gint32)(ofs_vals[i].sword);
	  if (temp < (gint32)ipatch_sf2_gen_info[i].min.sword)
	    temp = ipatch_sf2_gen_info[i].min.sword;
	  else if (temp > (gint32)ipatch_sf2_gen_info[i].max.sword)
	    temp = ipatch_sf2_gen_info[i].max.sword;
	  abs_vals[i].sword = (gint16)temp;
	  abs_array->flags |= v; /* generator now set */
	}
    }

  retval = ipatch_sf2_gen_range_intersect (&abs_vals[IPATCH_SF2_GEN_NOTE_RANGE],
                                           &ofs_vals[IPATCH_SF2_GEN_NOTE_RANGE]);
  return retval
    && ipatch_sf2_gen_range_intersect (&abs_vals[IPATCH_SF2_GEN_VELOCITY_RANGE],
                                       &ofs_vals[IPATCH_SF2_GEN_VELOCITY_RANGE]);
}

/**
 * ipatch_sf2_gen_array_intersect_test:
 * @array1: First generator amount array
 * @array2: Second generator amount array
 *
 * Checks if the note and velocity ranges in two generator arrays intersect.
 *
 * Returns: %TRUE if both ranges intersect, %FALSE if one or both do not.
 */
gboolean
ipatch_sf2_gen_array_intersect_test (const IpatchSF2GenArray *array1,
                                     const IpatchSF2GenArray *array2)
{
  if (!ipatch_sf2_gen_range_intersect_test (&array1->values[IPATCH_SF2_GEN_NOTE_RANGE],
                                            &array2->values[IPATCH_SF2_GEN_NOTE_RANGE]))
    return (FALSE);

  return (ipatch_sf2_gen_range_intersect_test (&array1->values[IPATCH_SF2_GEN_VELOCITY_RANGE],
                                               &array2->values[IPATCH_SF2_GEN_VELOCITY_RANGE]));
}

/**
 * ipatch_sf2_gen_array_count_set:
 * @array: Generator array
 *
 * Get count of "set" generators in a generator array.
 *
 * Returns: Count of "set" generators.
 */
guint
ipatch_sf2_gen_array_count_set (IpatchSF2GenArray *array)
{
  guint count = 0;
  guint64 v;

  g_return_val_if_fail (array != NULL, 0);

  for (v = array->flags; v; v >>= 1) 
    if (v & 0x1) count++;

  return (count);
}

/**
 * ipatch_sf2_gen_amount_to_value:
 * @genid: Generator ID
 * @amt: Generator amount for given @genid
 * @value: Uninitialized GValue to set to @amt
 *
 * Converts a generator amount to a GValue. Value will be initialized to
 * one of two types: G_TYPE_INT for signed/unsigned integers or
 * IPATCH_TYPE_RANGE for velocity or note split ranges.
 */
void
ipatch_sf2_gen_amount_to_value (guint genid, const IpatchSF2GenAmount *amt,
				GValue *value)
{
  g_return_if_fail (genid < IPATCH_SF2_GEN_COUNT);
  g_return_if_fail (amt != NULL);
  g_return_if_fail (value != NULL);

  if (ipatch_sf2_gen_info [genid].unit != IPATCH_UNIT_TYPE_RANGE)
    {
      g_value_init (value, G_TYPE_INT);
      g_value_set_int (value, amt->sword);
    }
  else
    {
      IpatchRange range;

      range.low = amt->range.low;
      range.high = amt->range.high;
      g_value_init (value, IPATCH_TYPE_RANGE);
      ipatch_value_set_range (value, &range);
    }
}

/**
 * ipatch_sf2_gen_default_value:
 * @genid: Generator ID
 * @ispreset: TRUE for preset generators, FALSE for instrument
 * @out_amt: (out): A pointer to store the default amount into
 *
 * Get default value for a generator ID for the specified (@ispreset) zone
 * type.
 */
void
ipatch_sf2_gen_default_value (guint genid, gboolean ispreset,
			      IpatchSF2GenAmount *out_amt)
{
  g_return_if_fail (out_amt != NULL);

  out_amt->sword = 0;		/* in case we fail, user gets 0 amount */

  g_return_if_fail (ipatch_sf2_gen_is_valid (genid, ispreset));

  if (ispreset)
    {
      if (ipatch_sf2_gen_info[genid].unit == IPATCH_UNIT_TYPE_RANGE)
	{
	  out_amt->range.low = 0;
	  out_amt->range.high = 127;
	}
      /* else: Amount already set to 0, which is default for preset gens */
    }
  else *out_amt = ipatch_sf2_gen_info[genid].def;
}

/**
 * ipatch_sf2_gen_offset:
 * @genid: ID of Generator to offset. Must be a valid preset generator.
 * @dst: (inout): Pointer to the initial amount to offset, result is stored back
 *   into this parameter.
 * @ofs: Pointer to offset amount.
 *
 * Offsets a generator amount. Result of offset is clamped to maximum and
 *   minimum values for the given generator ID.  In the case of note or
 *   velocity ranges a return value of %TRUE (clamped) means that the ranges
 *   don't intersect (contrary return value to other range related functions).
 *
 * Returns: %TRUE if value was clamped, %FALSE otherwise.
 */
gboolean
ipatch_sf2_gen_offset (guint genid, IpatchSF2GenAmount *dst,
		       const IpatchSF2GenAmount *ofs)
{
  gint32 temp;
  gboolean clamped = FALSE;

  g_return_val_if_fail (dst != NULL, FALSE);
  g_return_val_if_fail (ofs != NULL, FALSE);
  g_return_val_if_fail (ipatch_sf2_gen_is_valid (genid, TRUE), FALSE);

  if (genid != IPATCH_SF2_GEN_NOTE_RANGE && genid != IPATCH_SF2_GEN_VELOCITY_RANGE)
    {
      temp = (gint32)(dst->sword) + (gint32)(ofs->sword);
      if (temp < (gint32)ipatch_sf2_gen_info[genid].min.sword)
	{
	  temp = ipatch_sf2_gen_info[genid].min.sword;
	  clamped = TRUE;
	}
      else if (temp > (gint32)ipatch_sf2_gen_info[genid].max.sword)
	{
	  temp = ipatch_sf2_gen_info[genid].max.sword;
	  clamped = TRUE;
	}
      dst->sword = (gint16)temp;
    }
  else clamped = !ipatch_sf2_gen_range_intersect (dst, ofs);

  return (clamped);
}

/**
 * ipatch_sf2_gen_clamp:
 * @genid: Generator ID (#IpatchSF2GenType)
 * @sfval: (inout): Generator value to clamp (changed in place)
 * @ispreset: TRUE if its a Preset generator, FALSE if Instrument
 *
 * Clamp a generators value to its valid range.
 */
void
ipatch_sf2_gen_clamp (guint genid, int *sfval, gboolean ispreset)
{
  int ofsrange;			/* used only for offset gens, range of value */

  g_return_if_fail (ipatch_sf2_gen_is_valid (genid, ispreset));

  if (ispreset)
    {
      ofsrange = ipatch_sf2_gen_info[genid].max.sword
	- ipatch_sf2_gen_info[genid].min.sword;
      if (*sfval < -ofsrange) *sfval = -ofsrange;
      else if (*sfval > ofsrange) *sfval = ofsrange;
    }
  else
    {
      if (*sfval < ipatch_sf2_gen_info[genid].min.sword)
	*sfval = ipatch_sf2_gen_info[genid].min.sword;
      else if (*sfval > ipatch_sf2_gen_info[genid].max.sword)
	*sfval = ipatch_sf2_gen_info[genid].max.sword;
    }
}

/**
 * ipatch_sf2_gen_range_intersect:
 * @dst: (inout): First generator amount range, result is also stored here
 * @src: Second generator amount range
 *
 * Find intersection of two generator ranges (common shared range).
 * If ranges don't share anything in common @dst is not assigned.
 *
 * Returns: %FALSE if ranges don't share any range in common.
 */
gboolean
ipatch_sf2_gen_range_intersect (IpatchSF2GenAmount *dst,
				const IpatchSF2GenAmount *src)
{
  guint8 dl, dh, sl, sh;

  dl = dst->range.low;
  dh = dst->range.high;
  sl = src->range.low;
  sh = src->range.high;

  /* Nothing in common? */
  if (dh < sl || sh < dl) return (FALSE);

  dst->range.low = MAX (dl, sl);
  dst->range.high = MIN (dh, sh);

  return (TRUE);
}

/**
 * ipatch_sf2_gen_range_intersect_test:
 * @amt1: First generator amount range
 * @amt2: Second generator amount range
 *
 * Test if two ranges intersect.
 *
 * Returns: %FALSE if ranges don't share any range in common, %TRUE otherwise
 */
gboolean
ipatch_sf2_gen_range_intersect_test (const IpatchSF2GenAmount *amt1,
                                     const IpatchSF2GenAmount *amt2)
{
  return (!(amt1->range.high < amt2->range.low || amt2->range.high < amt1->range.low));
}

/**
 * ipatch_sf2_gen_get_prop_name:
 * @genid: Generator ID
 *
 * Get the GObject property name for a given generator ID.
 *
 * Returns: Property name or %NULL if no property name for @genid.
 * The returned string is internal and should not be modified or freed.
 */
G_CONST_RETURN char *
ipatch_sf2_gen_get_prop_name (guint genid)
{
  g_return_val_if_fail (genid < IPATCH_SF2_GEN_COUNT, NULL);
  return (gen_property_names[genid]);
}
