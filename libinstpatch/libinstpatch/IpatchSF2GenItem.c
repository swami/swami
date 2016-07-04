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
 * SECTION: IpatchSF2GenItem
 * @short_description: SoundFont generator item interface
 * @see_also: #IpatchSF2Preset, #IpatchSF2Inst, #IpatchSF2PZone, #IpatchSF2IZone
 * @stability: Stable
 *
 * Provides an interface for items which have SoundFont generator properties.
 * SoundFont generators are synthesis parameters used by #IpatchSF2Preset,
 * #IpatchSF2Inst, #IpatchSF2PZone and #IpatchSF2IZone objects.
 */
#include <stdio.h>
#include <stdarg.h>
#include <math.h>		/* for conversion functions */
#include <string.h>
#include <glib.h>
#include "IpatchSF2GenItem.h"
#include "IpatchSF2Gen.h"
#include "IpatchParamProp.h"
#include "IpatchRange.h"
#include "IpatchUnit.h"
#include "ipatch_priv.h"
#include "builtin_enums.h"
#include "util.h"


static gboolean
ipatch_sf2_gen_item_set_gen_flag_no_notify (IpatchSF2GenItem *item, guint genid,
					    gboolean setflag);

/* non realtime synthesis parameters */
static const guint8 non_realtime[] =
{
  IPATCH_SF2_GEN_SAMPLE_START,
  IPATCH_SF2_GEN_SAMPLE_END,
  IPATCH_SF2_GEN_SAMPLE_COARSE_START,
  IPATCH_SF2_GEN_SAMPLE_COARSE_END,
  IPATCH_SF2_GEN_NOTE_TO_MOD_ENV_HOLD,
  IPATCH_SF2_GEN_NOTE_TO_MOD_ENV_DECAY,
  IPATCH_SF2_GEN_NOTE_TO_VOL_ENV_HOLD,
  IPATCH_SF2_GEN_NOTE_TO_VOL_ENV_DECAY,
  IPATCH_SF2_GEN_INSTRUMENT_ID,
  IPATCH_SF2_GEN_NOTE_RANGE,
  IPATCH_SF2_GEN_VELOCITY_RANGE,
  IPATCH_SF2_GEN_FIXED_NOTE,
  IPATCH_SF2_GEN_FIXED_VELOCITY,
  IPATCH_SF2_GEN_SAMPLE_ID,
  IPATCH_SF2_GEN_SAMPLE_MODES,
  IPATCH_SF2_GEN_EXCLUSIVE_CLASS,
  IPATCH_SF2_GEN_ROOT_NOTE_OVERRIDE
};


GType
ipatch_sf2_gen_item_get_type (void)
{
  static GType itype = 0;

  if (!itype)
    {
      static const GTypeInfo info =
	{
	  sizeof (IpatchSF2GenItemIface),
	  NULL,			/* base_init */
	  NULL,			/* base_finalize */
	  (GClassInitFunc) NULL,
	  (GClassFinalizeFunc) NULL
	};

      itype = g_type_register_static (G_TYPE_INTERFACE, "IpatchSF2GenItem",
				      &info, 0);

      /* IpatchSF2GenItemIface types must be IpatchItem objects (for locking) */
      g_type_interface_add_prerequisite (itype, IPATCH_TYPE_ITEM);
    }

  return (itype);
}

/**
 * ipatch_sf2_gen_item_get_amount:
 * @item: Item with generators to get value from
 * @genid: Generator ID (#IpatchSF2GenType) of value to get
 * @out_amt: (out): Pointer to store generator amount to
 *
 * Get a generator amount from an item with generator properties.
 *
 * Returns: %TRUE if generator value is set, %FALSE if not set, in which case
 * the value stored to output_amt is the default value for the given generator
 * ID.
 */
gboolean
ipatch_sf2_gen_item_get_amount (IpatchSF2GenItem *item, guint genid,
				IpatchSF2GenAmount *out_amt)
{
  IpatchSF2GenItemIface *iface;
  IpatchSF2GenArray *genarray;
  gboolean set;

  g_return_val_if_fail (IPATCH_IS_SF2_GEN_ITEM (item), FALSE);
  g_return_val_if_fail (genid < IPATCH_SF2_GEN_COUNT, FALSE);
  g_return_val_if_fail (out_amt != NULL, FALSE);

  /* get pointer to genarray from IpatchSF2GenItemIface->genarray_ofs */
  iface = IPATCH_SF2_GEN_ITEM_GET_IFACE (item);
  g_return_val_if_fail (iface->genarray_ofs != 0, FALSE);
  genarray = (IpatchSF2GenArray *)G_STRUCT_MEMBER_P (item, iface->genarray_ofs);

  IPATCH_ITEM_RLOCK (item);
  *out_amt = genarray->values[genid];
  set = IPATCH_SF2_GEN_ARRAY_TEST_FLAG (genarray, genid);
  IPATCH_ITEM_RUNLOCK (item);

  return (set);
}

/**
 * ipatch_sf2_gen_item_set_amount:
 * @item: Item with generators to set value in
 * @genid: Generator ID (#IpatchSF2GenType) of generator to set
 * @amt: Value to set generator to
 *
 * Set a generator amount for an item with generators.
 *
 * #IpatchItem property notify is done for the property and possibly the "-set"
 * property if it was unset before.
 */
void
ipatch_sf2_gen_item_set_amount (IpatchSF2GenItem *item, guint genid,
				IpatchSF2GenAmount *amt)
{
  IpatchSF2GenItemIface *iface;
  IpatchSF2GenArray *genarray;
  IpatchSF2GenType propstype;
  GParamSpec *pspec;
  IpatchSF2GenAmount oldamt;
  GValue oldval = { 0 }, newval = { 0 };
  gboolean valchanged = FALSE, oldset;

  g_return_if_fail (IPATCH_IS_ITEM (item));
  g_return_if_fail (amt != NULL);

  iface = IPATCH_SF2_GEN_ITEM_GET_IFACE (item);
  propstype = iface->propstype;	/* propstype for this class */

  g_return_if_fail (ipatch_sf2_gen_is_valid (genid, propstype));

  /* get pointer to genarray from IpatchSF2GenItemIface->genarray_ofs */
  g_return_if_fail (iface->genarray_ofs != 0);
  genarray = (IpatchSF2GenArray *)G_STRUCT_MEMBER_P (item, iface->genarray_ofs);

  IPATCH_ITEM_WLOCK (item);

  /* has different value? */
  if (genarray->values[genid].sword != amt->sword)
    {
      oldamt = genarray->values[genid];	/* store old val for notify */
      genarray->values[genid] = *amt;
      valchanged = TRUE;
    }

  oldset = IPATCH_SF2_GEN_ARRAY_TEST_FLAG (genarray, genid);
  IPATCH_SF2_GEN_ARRAY_SET_FLAG (genarray, genid); /* value is set */

  IPATCH_ITEM_WUNLOCK (item);

  if (valchanged)  /* do the property change notify if it actually changed */
    {
      pspec = iface->specs[genid];
      ipatch_sf2_gen_amount_to_value (genid, amt, &newval);
      ipatch_sf2_gen_amount_to_value (genid, &oldamt, &oldval);
      ipatch_item_prop_notify (IPATCH_ITEM (item), pspec, &newval, &oldval);
      g_value_unset (&newval);
      g_value_unset (&oldval);
    }

  if (oldset != TRUE)	/* "set" state of property changed? */
    {
      pspec = iface->setspecs[genid];
      ipatch_item_prop_notify (IPATCH_ITEM (item), pspec,
			       ipatch_util_value_bool_true,
			       ipatch_util_value_bool_false);
    }
}

/**
 * ipatch_sf2_gen_item_set_gen_flag:
 * @item: Item with generator properties to set value of a gen "set" flag of
 * @genid: Generator ID (#IpatchSF2GenType) of generator to set "set" flag value of
 * @setflag: If %TRUE then generator amount is assigned, FALSE will cause the
 *   amount to be unset (and revert to its default value)
 *
 * Sets the value of a generator "set" flag in an item with generators.
 *
 * #IpatchItem property notify is done for the property and possibly the "-set"
 * property if it was set before.
 */
void
ipatch_sf2_gen_item_set_gen_flag (IpatchSF2GenItem *item, guint genid,
				  gboolean setflag)
{
  IpatchSF2GenItemIface *iface;
  GParamSpec *pspec;

  if (!ipatch_sf2_gen_item_set_gen_flag_no_notify (item, genid, setflag))
    return;

  iface = IPATCH_SF2_GEN_ITEM_GET_IFACE (item);
  g_return_if_fail (iface != NULL);

  /* do "-set" property notify */
  pspec = iface->setspecs[genid];

  ipatch_item_prop_notify (IPATCH_ITEM (item), pspec,
			   IPATCH_UTIL_VALUE_BOOL (setflag),
			   IPATCH_UTIL_VALUE_BOOL (!setflag));
}

/* Like ipatch_sf2_gen_item_set_gen_flag() but doesn't do "-set" property notify.
 * A regular property notify may occur though,
 * if the effective amount has changed. Caller can check if "-set" parameter
 * changed from return value (TRUE if changed from old value). */
static gboolean
ipatch_sf2_gen_item_set_gen_flag_no_notify (IpatchSF2GenItem *item, guint genid,
					    gboolean setflag)
{
  IpatchSF2GenItemIface *iface;
  IpatchSF2GenArray *genarray;
  IpatchSF2GenType propstype;
  GParamSpec *pspec;
  IpatchSF2GenAmount oldamt, defamt;
  GValue newval = { 0 }, oldval = { 0 };
  gboolean valchanged = FALSE, oldset;

  g_return_val_if_fail (IPATCH_IS_SF2_GEN_ITEM (item), FALSE);

  iface = IPATCH_SF2_GEN_ITEM_GET_IFACE (item);
  propstype = iface->propstype;	/* propstype for this class */

  g_return_val_if_fail (ipatch_sf2_gen_is_valid (genid, propstype), FALSE);

  /* get pointer to genarray from IpatchSF2GenItemIface->genarray_ofs */
  g_return_val_if_fail (iface->genarray_ofs != 0, FALSE);
  genarray = (IpatchSF2GenArray *)G_STRUCT_MEMBER_P (item, iface->genarray_ofs);

  /* grab default val from gen info table if absolute instrument gen or a range
     offset preset gen, otherwise offset gens are 0 by default */
  if (!setflag)
    {
      if ((propstype & 0x1) == IPATCH_SF2_GEN_PROPS_INST
	  || ipatch_sf2_gen_info[genid].unit == IPATCH_UNIT_TYPE_RANGE)
	defamt = ipatch_sf2_gen_info[genid].def;
      else defamt.sword = 0;
    }

  IPATCH_ITEM_WLOCK (item);

  /* unsetting flag and amount has different value than default? */
  if (!setflag && genarray->values[genid].sword != defamt.sword)
    {
      oldamt = genarray->values[genid];
      genarray->values[genid] = defamt;
      valchanged = TRUE;
    }

  oldset = IPATCH_SF2_GEN_ARRAY_TEST_FLAG (genarray, genid);

  /* set/unset flag as requested */
  if (setflag)
    IPATCH_SF2_GEN_ARRAY_SET_FLAG (genarray, genid);
  else IPATCH_SF2_GEN_ARRAY_CLEAR_FLAG (genarray, genid);

  IPATCH_ITEM_WUNLOCK (item);

  /* do the property change notify if it actually changed */
  if (valchanged)
    {
      pspec = iface->specs[genid];
      ipatch_sf2_gen_amount_to_value (genid, &defamt, &newval);
      ipatch_sf2_gen_amount_to_value (genid, &oldamt, &oldval);
      ipatch_item_prop_notify (IPATCH_ITEM (item), pspec, &newval, &oldval);
      g_value_unset (&newval);
      g_value_unset (&oldval);
    }

  return (setflag != oldset);
}

/**
 * ipatch_sf2_gen_item_count_set:
 * @item: Item with generators
 *
 * Get count of "set" generators in an item with generators.
 *
 * Returns: Count of "set" generators.
 */
guint
ipatch_sf2_gen_item_count_set (IpatchSF2GenItem *item)
{
  IpatchSF2GenItemIface *iface;
  IpatchSF2GenArray *genarray;
  guint count = 0;
  guint64 v;

  g_return_val_if_fail (IPATCH_IS_SF2_GEN_ITEM (item), 0);

  /* get pointer to genarray from IpatchSF2GenItemIface->genarray_ofs */
  iface = IPATCH_SF2_GEN_ITEM_GET_IFACE (item);
  g_return_val_if_fail (iface->genarray_ofs != 0, 0);
  genarray = (IpatchSF2GenArray *)G_STRUCT_MEMBER_P (item, iface->genarray_ofs);

  IPATCH_ITEM_RLOCK (item);
  for (v = genarray->flags; v; v >>= 1) 
    if (v & 0x1) count++;
  IPATCH_ITEM_RUNLOCK (item);

  return (count);
}

/**
 * ipatch_sf2_gen_item_copy_all:
 * @item: Item with generators
 * @array: (out): Destination generator array to store to
 *
 * Copies an item's generators to a caller supplied generator array.
 */
void
ipatch_sf2_gen_item_copy_all (IpatchSF2GenItem *item, IpatchSF2GenArray *array)
{
  IpatchSF2GenItemIface *iface;
  IpatchSF2GenArray *genarray;

  g_return_if_fail (IPATCH_IS_SF2_GEN_ITEM (item));
  g_return_if_fail (array != NULL);

  /* get pointer to genarray from IpatchSF2GenItemIface->genarray_ofs */
  iface = IPATCH_SF2_GEN_ITEM_GET_IFACE (item);
  g_return_if_fail (iface->genarray_ofs != 0);
  genarray = (IpatchSF2GenArray *)G_STRUCT_MEMBER_P (item, iface->genarray_ofs);

  IPATCH_ITEM_RLOCK (item);
  memcpy (array, genarray, sizeof (IpatchSF2GenArray));
  IPATCH_ITEM_RUNLOCK (item);
}

/**
 * ipatch_sf2_gen_item_copy_set:
 * @item: Item with generators
 * @array: (out): Destination generator array to store to
 *
 * Copies a item's "set" generators to a caller supplied generator array.
 * This function differs from ipatch_sf2_gen_item_copy_all() in that it
 * only copies generators that are set. It can be used to override values
 * in one array with set values in another. Note that this doesn't change
 * any generators in @item, despite "set" being in the name.
 */
void
ipatch_sf2_gen_item_copy_set (IpatchSF2GenItem *item, IpatchSF2GenArray *array)
{
  IpatchSF2GenItemIface *iface;
  IpatchSF2GenArray *genarray;
  IpatchSF2GenAmount *vals;
  guint64 v;
  int i;

  g_return_if_fail (IPATCH_IS_SF2_GEN_ITEM (item));
  g_return_if_fail (array != NULL);

  /* get pointer to genarray from IpatchSF2GenItemIface->genarray_ofs */
  iface = IPATCH_SF2_GEN_ITEM_GET_IFACE (item);
  g_return_if_fail (iface->genarray_ofs != 0);
  genarray = (IpatchSF2GenArray *)G_STRUCT_MEMBER_P (item, iface->genarray_ofs);

  IPATCH_ITEM_RLOCK (item);

  vals = genarray->values;
  v = genarray->flags;
  array->flags |= v;	  /* set destination array bits from source */

  for (i = 0; v != 0; i++, v >>= 1)
    if (v & 0x1) array->values[i] = vals[i]; /* only copy set values */

  IPATCH_ITEM_RUNLOCK (item);
}

/**
 * ipatch_sf2_gen_item_set_note_range:
 * @item: Item with generators
 * @low: Low value of range (MIDI note # between 0 and 127)
 * @high: High value of range (MIDI note # between 0 and 127)
 *
 * Set the MIDI note range that an item with generators is active on.
 * Only a convenience function really.
 */
void
ipatch_sf2_gen_item_set_note_range (IpatchSF2GenItem *item, int low, int high)
{
  IpatchSF2GenAmount amt;

  g_return_if_fail (IPATCH_IS_SF2_GEN_ITEM (item));
  g_return_if_fail (low >= 0 && low <= 127);
  g_return_if_fail (high >= 0 && high <= 127);

  if (low > high)		/* swap if backwards */
    {
      int temp = low;
      low = high;
      high = temp;
    }

  amt.range.low = low;
  amt.range.high = high;

  ipatch_sf2_gen_item_set_amount (item, IPATCH_SF2_GEN_NOTE_RANGE, &amt);
}

/**
 * ipatch_sf2_gen_item_set_velocity_range:
 * @item: Item with generators
 * @low: Low value of range (MIDI velocity # between 0 and 127)
 * @high: High value of range (MIDI velocity # between 0 and 127)
 *
 * Set the MIDI velocity range that an item with generators is active on.
 * Only a convenience function really.
 */
void
ipatch_sf2_gen_item_set_velocity_range (IpatchSF2GenItem *item, int low, int high)
{
  IpatchSF2GenAmount amt;

  g_return_if_fail (IPATCH_IS_SF2_GEN_ITEM (item));
  g_return_if_fail (low >= 0 && low <= 127);
  g_return_if_fail (high >= 0 && high <= 127);

  if (low > high)		/* swap if backwards */
    {
      int temp = low;
      low = high;
      high = temp;
    }

  amt.range.low = low;
  amt.range.high = high;

  ipatch_sf2_gen_item_set_amount (item, IPATCH_SF2_GEN_VELOCITY_RANGE, &amt);
}

/**
 * ipatch_sf2_gen_item_in_range:
 * @item: Item with generators
 * @note: MIDI note number or -1 for wildcard
 * @velocity: MIDI velocity or -1 for wildcard
 *
 * Check if a note and velocity falls in the ranges of an item with generators
 *
 * Returns: %TRUE if @item is in @note and @velocity range, %FALSE otherwise
 */
gboolean
ipatch_sf2_gen_item_in_range (IpatchSF2GenItem *item, int note, int velocity)
{
  IpatchSF2GenAmount *noteamt, *velamt;
  IpatchSF2GenItemIface *iface;
  IpatchSF2GenArray *genarray;
  gboolean in_range;

  g_return_val_if_fail (IPATCH_IS_SF2_GEN_ITEM (item), FALSE);

  /* get pointer to genarray from IpatchSF2GenItemIface->genarray_ofs */
  iface = IPATCH_SF2_GEN_ITEM_GET_IFACE (item);
  g_return_val_if_fail (iface->genarray_ofs != 0, 0);
  genarray = (IpatchSF2GenArray *)G_STRUCT_MEMBER_P (item, iface->genarray_ofs);

  IPATCH_ITEM_RLOCK (item);
  noteamt = &genarray->values[IPATCH_SF2_GEN_NOTE_RANGE];
  velamt = &genarray->values[IPATCH_SF2_GEN_VELOCITY_RANGE];

  in_range = ((note == -1) || (note >= noteamt->range.low
			       && note <= noteamt->range.high))
    && ((velocity == -1) || (velocity >= velamt->range.low
			     && velocity <= velamt->range.high));
  IPATCH_ITEM_RUNLOCK (item);

  return (in_range);
}

/**
 * ipatch_sf2_gen_item_intersect_test:
 * @item: Item with generators
 * @genarray: Generator array to test note and velocity ranges against
 *
 * Check if a given item's note and velocity ranges intersect with those in a
 * generator array.
 *
 * Returns: %TRUE if both note and velocity ranges intersect, %FALSE if one or
 *   both do not.
 */
gboolean
ipatch_sf2_gen_item_intersect_test (IpatchSF2GenItem *item,
                                    const IpatchSF2GenArray *genarray)
{
  IpatchSF2GenAmount noteamt, velamt;
  IpatchSF2GenItemIface *iface;
  IpatchSF2GenArray *itemgenarray;

  g_return_val_if_fail (IPATCH_IS_SF2_GEN_ITEM (item), FALSE);

  /* get pointer to genarray from IpatchSF2GenItemIface->genarray_ofs */
  iface = IPATCH_SF2_GEN_ITEM_GET_IFACE (item);
  g_return_val_if_fail (iface->genarray_ofs != 0, 0);
  itemgenarray = (IpatchSF2GenArray *)G_STRUCT_MEMBER_P (item, iface->genarray_ofs);

  IPATCH_ITEM_RLOCK (item);
  noteamt = itemgenarray->values[IPATCH_SF2_GEN_NOTE_RANGE];
  velamt = itemgenarray->values[IPATCH_SF2_GEN_VELOCITY_RANGE];
  IPATCH_ITEM_RUNLOCK (item);

  return ipatch_sf2_gen_range_intersect_test (&noteamt, &genarray->values[IPATCH_SF2_GEN_NOTE_RANGE])
    && ipatch_sf2_gen_range_intersect_test (&velamt, &genarray->values[IPATCH_SF2_GEN_VELOCITY_RANGE]);
}

/**
 * ipatch_sf2_gen_item_class_get_pspec: (skip)
 * @genid: Generator ID
 * @klass: Class with an #IpatchSF2GenItem interface
 *
 * Get the parameter specification for a given generator ID and object class.
 *
 * Returns: (transfer none): The parameter specification for the generator or %NULL if
 *   the given @genid for @klass is not valid.
 */
GParamSpec *
ipatch_sf2_gen_item_class_get_pspec (GObjectClass *klass, guint genid)
{
  IpatchSF2GenItemIface *gen_item_iface;

  g_return_val_if_fail (genid < IPATCH_SF2_GEN_COUNT, NULL);
  g_return_val_if_fail (klass != NULL, NULL);

  gen_item_iface = g_type_interface_peek (klass, IPATCH_TYPE_SF2_GEN_ITEM);
  g_return_val_if_fail (gen_item_iface != NULL, NULL);

  return (gen_item_iface->specs[genid]);
}

/**
 * ipatch_sf2_gen_item_class_get_pspec_set: (skip)
 * @genid: Generator ID
 * @klass: Class with an #IpatchSF2GenItem interface
 *
 * Get a "-set" property parameter specification for a given generator ID and
 * object class.
 *
 * Returns: (transfer none): The "-set" property parameter specification for the generator or
 *   %NULL if the given @genid or @klass are not valid.
 */
GParamSpec *
ipatch_sf2_gen_item_class_get_pspec_set (GObjectClass *klass, guint genid)
{
  IpatchSF2GenItemIface *gen_item_iface;

  g_return_val_if_fail (genid < IPATCH_SF2_GEN_COUNT, NULL);
  g_return_val_if_fail (klass != NULL, NULL);

  gen_item_iface = g_type_interface_peek (klass, IPATCH_TYPE_SF2_GEN_ITEM);
  g_return_val_if_fail (gen_item_iface != NULL, NULL);

  return (gen_item_iface->setspecs[genid]);
}

/**
 * ipatch_sf2_gen_item_iface_install_properties: (skip)
 * @klass: Object class to install properties on
 * @propstype: Type of properties to install (instrument/preset)
 * @specs: Location to store a pointer to an allocated array of parameter
 *   specs which should get copied to the interface's specs field and then freed
 * @setspecs: Location to store a pointer to an allocated array of parameter
 *   specs which should get copied to the interface's setspecs field and then freed
 *
 * Installs generator item properties on the provided @klass.
 * Used internally in IpatchSF2GenItemIface init functions.
 */

/* This function is complicated by the fact that GObject properties are supposed
 * to be installed in the class init function, but the gen item interface has
 * not yet been initialized, so values need to be passed to the interface init
 * function. */
void
ipatch_sf2_gen_item_iface_install_properties (GObjectClass *klass,
                                              IpatchSF2GenPropsType propstype,
                                              GParamSpec ***specs,
                                              GParamSpec ***setspecs)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  GParamSpec *pspec;
  char *set_name;
  const IpatchSF2GenInfo *gen_info;
  int nonrt_index = 0;		/* non realtime generator array index */
  gboolean ispreset;
  int i, diff, unit;

  ispreset = propstype & 1;

  /* get generator type GObject enum */
  enum_class = g_type_class_ref (IPATCH_TYPE_SF2_GEN_TYPE);
  g_return_if_fail (enum_class != NULL);

  *specs = g_new (GParamSpec *, IPATCH_SF2_GEN_COUNT);
  *setspecs = g_new (GParamSpec *, IPATCH_SF2_GEN_COUNT);

  /* install generator properties */
  for (i = 0; i < IPATCH_SF2_GEN_COUNT; i++)
    { /* gen is valid for zone type? */
      if (!ipatch_sf2_gen_is_valid (i, propstype)) continue;

      gen_info = &ipatch_sf2_gen_info[i];
      enum_value = g_enum_get_value (enum_class, i);
      if (gen_info->unit == IPATCH_UNIT_TYPE_RANGE)
	pspec = ipatch_param_spec_range (enum_value->value_nick,
		    _(gen_info->label),
		    _(gen_info->descr ? gen_info->descr : gen_info->label),
		    0, 127, 0, 127, G_PARAM_READWRITE);
      /* allow 30 bit number which stores fine and coarse (32k) values */
      else if (gen_info->unit == IPATCH_UNIT_TYPE_SAMPLES)
	pspec = g_param_spec_int (enum_value->value_nick,
		    _(gen_info->label),
		    _(gen_info->descr ? gen_info->descr : gen_info->label),
		    ispreset ? -0x03FFFFFFF : 0, 0x03FFFFFFF, 0,
		    G_PARAM_READWRITE);
      else if (!ispreset)	/* integer absolute property */
	pspec = g_param_spec_int (enum_value->value_nick,
		    _(gen_info->label),
		    _(gen_info->descr ? gen_info->descr : gen_info->label),
		    gen_info->min.sword, gen_info->max.sword,
		    gen_info->def.sword, G_PARAM_READWRITE);
      else			/* integer offset property */
	{
	  diff = (int)gen_info->max.sword - gen_info->min.sword;
	  pspec = g_param_spec_int (enum_value->value_nick,
		      _(gen_info->label),
		      _(gen_info->descr ? gen_info->descr : gen_info->label),
		      -diff, diff, 0, G_PARAM_READWRITE);
	}

      /* all generators affect synthesis */
      pspec->flags |= IPATCH_PARAM_SYNTH;

      /* if generator is not in non_realtime generator array.. */
      if (nonrt_index >= G_N_ELEMENTS (non_realtime)
	  || non_realtime[nonrt_index] != i)
	pspec->flags |= IPATCH_PARAM_SYNTH_REALTIME; /* set realtime flag */
      else if (non_realtime[nonrt_index] == i)
	nonrt_index++;  /* current gen is non realtime, adv to next index */

      /* install the property */
      g_object_class_install_property (klass, i + IPATCH_SF2_GEN_ITEM_FIRST_PROP_ID, pspec);

      unit = gen_info->unit;

      /* set parameter unit type extended property */
      if (ispreset)
      {
        if (unit == IPATCH_UNIT_TYPE_SF2_ABS_PITCH)
          unit = IPATCH_UNIT_TYPE_SF2_OFS_PITCH;
        else if (unit == IPATCH_UNIT_TYPE_SF2_ABS_TIME)
          unit = IPATCH_UNIT_TYPE_SF2_OFS_PITCH;
      }

      ipatch_param_set (pspec, "unit-type", unit, NULL);

      (*specs)[i] = g_param_spec_ref (pspec);  /* add to parameter spec array */

      /* create prop-set property and add to setspecs array */
      set_name = g_strconcat (enum_value->value_nick, "-set", NULL);
      pspec = g_param_spec_boolean (set_name, NULL, NULL, FALSE, G_PARAM_READWRITE);
      g_free (set_name);

      (*setspecs)[i] = g_param_spec_ref (pspec);      /* add to set spec array */

      /* install "-set" property */
      g_object_class_install_property (klass, i + IPATCH_SF2_GEN_ITEM_FIRST_PROP_SET_ID, pspec);
    } /* for loop */
}

/**
 * ipatch_sf2_gen_item_iface_set_property: (skip)
 * @item: IpatchItem instance with generator properties
 * @property_id: Property id to set
 * @value: Value to set property to
 *
 * Used internally for classes with generators, to set values thereof.
 *
 * Returns: %TRUE if @property_id handled, %FALSE otherwise
 */
gboolean
ipatch_sf2_gen_item_iface_set_property (IpatchSF2GenItem *item,
				        guint property_id, const GValue *value)
{
  IpatchSF2GenItemIface *iface;
  IpatchSF2GenArray *genarray;
  const IpatchSF2GenInfo *gen_info;
  IpatchSF2GenAmount amt;
  IpatchRange *range;
  int genid, coarse, val;
  gboolean oldset, oldcoarseset = 0, newcoarseset = 0;
  IpatchSF2GenAmount oldcoarseamt, newcoarseamt;
  gboolean coarsevalchanged = FALSE;
  GParamSpec *pspec;
  GValue newval = { 0 }, oldval = { 0 };
  gboolean setflag;

  iface = IPATCH_SF2_GEN_ITEM_GET_IFACE (item);

  /* a "-set" property? */
  if (property_id >= IPATCH_SF2_GEN_ITEM_FIRST_PROP_SET_ID
      && property_id < IPATCH_SF2_GEN_ITEM_FIRST_PROP_SET_ID + IPATCH_SF2_GEN_COUNT)
    {
      genid = property_id - IPATCH_SF2_GEN_ITEM_FIRST_PROP_SET_ID;

      /* generator valid for zone type? */
      if (!ipatch_sf2_gen_is_valid (genid, iface->propstype)) return (FALSE);

      setflag = g_value_get_boolean (value);
      ipatch_sf2_gen_item_set_gen_flag_no_notify (item, genid, setflag);

      return (TRUE);
    }

  /* regular generator property? */
  if (property_id < IPATCH_SF2_GEN_ITEM_FIRST_PROP_ID
      || property_id >= IPATCH_SF2_GEN_ITEM_FIRST_PROP_ID + IPATCH_SF2_GEN_COUNT)
    return (FALSE);

  genid = property_id - IPATCH_SF2_GEN_ITEM_FIRST_PROP_ID;

  /* generator valid for zone type? */
  if (!ipatch_sf2_gen_is_valid (genid, iface->propstype)) return (FALSE);

  /* get pointer to genarray from IpatchSF2GenItemIface->genarray_ofs */
  g_return_val_if_fail (iface->genarray_ofs != 0, FALSE);
  genarray = (IpatchSF2GenArray *)G_STRUCT_MEMBER_P (item, iface->genarray_ofs);

  gen_info = &ipatch_sf2_gen_info [genid];

  if (gen_info->unit == IPATCH_UNIT_TYPE_SAMPLES)
    { /* set 2 generators - fine and coarse (32k) sample values */
      if (genid == IPATCH_SF2_GEN_SAMPLE_START)
	coarse = IPATCH_SF2_GEN_SAMPLE_COARSE_START;
      else if (genid == IPATCH_SF2_GEN_SAMPLE_END)
	coarse = IPATCH_SF2_GEN_SAMPLE_COARSE_END;
      else if (genid == IPATCH_SF2_GEN_SAMPLE_LOOP_START)
	coarse = IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_START;
      else if (genid == IPATCH_SF2_GEN_SAMPLE_LOOP_END)
	coarse = IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_END;
      else g_return_val_if_fail (NOT_REACHED, FALSE);

      val = g_value_get_int (value);
      newcoarseamt.uword = val >> 15;

      IPATCH_ITEM_WLOCK (item); /* atomically set both gens */

      /* prop notify done by IpatchItem methods, so just set value */
      genarray->values[genid].uword = val & 0x7FFF;

      oldset = IPATCH_SF2_GEN_ARRAY_TEST_FLAG (genarray, genid);
      IPATCH_SF2_GEN_ARRAY_SET_FLAG (genarray, genid); /* value is set */

      /* only set coarse value if it has changed */
      if (genarray->values[coarse].uword != newcoarseamt.uword)
	{
	  oldcoarseamt = genarray->values[coarse];
	  genarray->values[coarse] = newcoarseamt;
	  coarsevalchanged = TRUE;

	  oldcoarseset = IPATCH_SF2_GEN_ARRAY_TEST_FLAG (genarray, genid);

	  if (val != 0)
	    {
	      IPATCH_SF2_GEN_ARRAY_SET_FLAG (genarray, genid); /* value is set */
	      newcoarseset = 1;
	    }
	  else
	    {
	      IPATCH_SF2_GEN_ARRAY_CLEAR_FLAG (genarray, genid); /* value is unset */
	      newcoarseset = 0;
	    }
	}

      IPATCH_ITEM_WUNLOCK (item);

      if (oldset != TRUE)	/* "set" state of property changed? */
	{
	  pspec = iface->setspecs[genid];
	  ipatch_item_prop_notify (IPATCH_ITEM (item), pspec, ipatch_util_value_bool_true,
				   ipatch_util_value_bool_false);
	}

      if (coarsevalchanged)  /* do the property change notify if it actually changed */
	{
	  pspec = iface->specs[coarse];
	  ipatch_sf2_gen_amount_to_value (genid, &newcoarseamt, &newval);
	  ipatch_sf2_gen_amount_to_value (genid, &oldcoarseamt, &oldval);
	  ipatch_item_prop_notify (IPATCH_ITEM (item), pspec, &newval, &oldval);
	  g_value_unset (&newval);
	  g_value_unset (&oldval);
	}
    
      if (oldcoarseset != newcoarseset)	/* "set" state of property changed? */
	{
	  pspec = iface->setspecs[coarse];
	  ipatch_item_prop_notify (IPATCH_ITEM (item), pspec,
				   IPATCH_UTIL_VALUE_BOOL (newcoarseset),
				   IPATCH_UTIL_VALUE_BOOL (oldcoarseset));
	}
    }
  else
    {
      if (gen_info->unit == IPATCH_UNIT_TYPE_RANGE) /* range property? */
      {
	range = ipatch_value_get_range (value);
	amt.range.low = range->low;
	amt.range.high = range->high;
      }
      else
      {
	val = g_value_get_int (value);
	amt.sword = val;
      }

      IPATCH_ITEM_WLOCK (item);

      /* prop notify done by IpatchItem methods, so just set value */
      genarray->values[genid] = amt;

      /* get old value of set flag, and then set it (if no already) */
      oldset = IPATCH_SF2_GEN_ARRAY_TEST_FLAG (genarray, genid);
      IPATCH_SF2_GEN_ARRAY_SET_FLAG (genarray, genid); /* value is set */

      IPATCH_ITEM_WUNLOCK (item);

      if (oldset != TRUE)	/* "set" state of property changed? */
	{
	  pspec = iface->setspecs[genid];
	  ipatch_item_prop_notify (IPATCH_ITEM (item), pspec, ipatch_util_value_bool_true,
				   ipatch_util_value_bool_false);
	}
    }

  return (TRUE);
}

/**
 * ipatch_sf2_gen_item_iface_get_property: (skip)
 * @item: IpatchItem instance with generators
 * @property_id: Property id to set
 * @value: Value to set property to
 *
 * Used internally for classes with generator properties, to get values thereof.
 *
 * Returns: %TRUE if @property_id handled, %FALSE otherwise
 */
gboolean
ipatch_sf2_gen_item_iface_get_property (IpatchSF2GenItem *item, guint property_id,
					GValue *value)
{
  IpatchSF2GenItemIface *iface;
  IpatchSF2GenArray *genarray;
  const IpatchSF2GenInfo *gen_info;
  IpatchSF2GenAmount amt;
  IpatchRange range;
  int genid, coarse, val;
  gboolean setflag;

  iface = IPATCH_SF2_GEN_ITEM_GET_IFACE (item);

  /* get pointer to genarray from IpatchSF2GenItemIface->genarray_ofs */
  g_return_val_if_fail (iface->genarray_ofs != 0, FALSE);
  genarray = (IpatchSF2GenArray *)G_STRUCT_MEMBER_P (item, iface->genarray_ofs);

  /* a "-set" property? */
  if (property_id >= IPATCH_SF2_GEN_ITEM_FIRST_PROP_SET_ID
      && property_id < IPATCH_SF2_GEN_ITEM_FIRST_PROP_SET_ID + IPATCH_SF2_GEN_COUNT)
    {
      genid = property_id - IPATCH_SF2_GEN_ITEM_FIRST_PROP_SET_ID;

      /* generator valid for zone type? */
      if (!ipatch_sf2_gen_is_valid (genid, iface->propstype)) return (FALSE);

      IPATCH_ITEM_RLOCK (item);
      setflag = IPATCH_SF2_GEN_ARRAY_TEST_FLAG (genarray, genid);
      IPATCH_ITEM_RUNLOCK (item);

      g_value_set_boolean (value, setflag);
      return (TRUE);
    }

  /* regular generator property? */
  if (property_id < IPATCH_SF2_GEN_ITEM_FIRST_PROP_ID
      || property_id >= IPATCH_SF2_GEN_ITEM_FIRST_PROP_ID + IPATCH_SF2_GEN_COUNT)
    return (FALSE);

  genid = property_id - IPATCH_SF2_GEN_ITEM_FIRST_PROP_ID;

  /* generator valid for propstype? */
  if (!ipatch_sf2_gen_is_valid (genid, iface->propstype)) return (FALSE);

  gen_info = &ipatch_sf2_gen_info [genid];

  if (gen_info->unit == IPATCH_UNIT_TYPE_RANGE) /* range property? */
    {
      IPATCH_ITEM_WLOCK (item); /* OPTME - lock might not be necessary? */
      amt = genarray->values[genid];
      IPATCH_ITEM_WUNLOCK (item);

      range.low = amt.range.low;
      range.high = amt.range.high;
      ipatch_value_set_range (value, &range);
    }
  else if (gen_info->unit == IPATCH_UNIT_TYPE_SAMPLES)
    { /* get 2 generators - fine and coarse (32k) sample values */
      if (genid == IPATCH_SF2_GEN_SAMPLE_START)
	coarse = IPATCH_SF2_GEN_SAMPLE_COARSE_START;
      else if (genid == IPATCH_SF2_GEN_SAMPLE_END)
	coarse = IPATCH_SF2_GEN_SAMPLE_COARSE_END;
      else if (genid == IPATCH_SF2_GEN_SAMPLE_LOOP_START)
	coarse = IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_START;
      else if (genid == IPATCH_SF2_GEN_SAMPLE_LOOP_END)
	coarse = IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_END;
      else g_return_val_if_fail (NOT_REACHED, FALSE);

      IPATCH_ITEM_WLOCK (item); /* atomically get both gens */
      val = genarray->values[genid].uword;
      val |= genarray->values[coarse].uword << 15;
      IPATCH_ITEM_WUNLOCK (item);

      g_value_set_int (value, val);
    }	/* sword read is atomic */
  else g_value_set_int (value, genarray->values[genid].sword);

  return (TRUE);
}
