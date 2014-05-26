/*
 * Swami
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
 * SECTION: IpatchRange
 * @short_description: A boxed type which defines a number range
 * @see_also: 
 * @stability: Stable
 *
 * Boxed type used for #GValue and #GParamSpec properties.  Consists of a low
 * and a high integer value defining a range.
 */
#include "config.h"

#include <stdio.h>
#include <glib.h>
#include <glib-object.h>

#include "IpatchRange.h"
#include "IpatchXml.h"
#include "IpatchXmlObject.h"
#include "misc.h"
#include "ipatch_priv.h"

static gboolean
ipatch_range_xml_encode_func (GNode *node, GObject *object, GParamSpec *pspec,
                              GValue *value, GError **err);
static gboolean
ipatch_range_xml_decode_func (GNode *node, GObject *object, GParamSpec *pspec,
                              GValue *value, GError **err);
static void ipatch_param_spec_range_set_default (GParamSpec *pspec,
						GValue *value);
static gboolean ipatch_param_spec_range_validate (GParamSpec *pspec,
						 GValue *value);
static gint ipatch_param_spec_range_values_cmp (GParamSpec *pspec,
						const GValue *value1,
						const GValue *value2);

/**
 * _ipatch_range_init: (skip)
 *
 * Init function to register pickle XML encode/decode functions for ranges
 */
void
_ipatch_range_init (void)
{
  ipatch_xml_register_handler (IPATCH_TYPE_RANGE, NULL,
                               ipatch_range_xml_encode_func,
                               ipatch_range_xml_decode_func);
}

/* XML range value encoding function */
static gboolean
ipatch_range_xml_encode_func (GNode *node, GObject *object, GParamSpec *pspec,
                              GValue *value, GError **err)
{
  IpatchRange *range;

  g_return_val_if_fail (IPATCH_VALUE_HOLDS_RANGE (value), FALSE);

  range = g_value_get_boxed (value);
  if (range) ipatch_xml_set_value_printf (node, "%d-%d", range->low, range->high);
  return (TRUE);
}

static gboolean
ipatch_range_xml_decode_func (GNode *node, GObject *object, GParamSpec *pspec,
                              GValue *value, GError **err)
{
  IpatchRange *range;
  int low, high;
  const char *strval;

  strval = ipatch_xml_get_value (node);
  if (!strval) g_value_set_boxed (value, NULL);

  if (sscanf (strval, "%d-%d", &low, &high) != 2)
  {
    g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_INVALID,
		 _("Invalid XML '%s' for range value"), strval);
    return (FALSE);
  }

  range = ipatch_range_new (low, high);
  g_value_take_boxed (value, range);

  return (TRUE);
}

GType
ipatch_range_get_type (void)
{
  static GType item_type = 0;

  if (!item_type)
  {
    item_type = g_boxed_type_register_static
      ("IpatchRange", (GBoxedCopyFunc) ipatch_range_copy,
       (GBoxedFreeFunc) ipatch_range_free);
  }

  return (item_type);
}

/**
 * ipatch_range_new:
 * @low: Low value to initialize range to
 * @high: High value to initialize range to
 *
 * Create a new value range structure (to store an integer range).
 *
 * Returns: Newly allocated integer range structure.
 */
IpatchRange *
ipatch_range_new (int low, int high)
{
  IpatchRange *range;

  range = g_slice_new (IpatchRange);
  range->low = low;
  range->high = high;

  return (range);
}

/**
 * ipatch_range_copy:
 * @range: Range structure to duplicate
 *
 * Duplicates an integer range structure.
 *
 * Returns: New duplicate range structure.
 */
IpatchRange *
ipatch_range_copy (IpatchRange *range)
{
  g_return_val_if_fail (range != NULL, NULL);
  return (ipatch_range_new (range->low, range->high));
}

/**
 * ipatch_range_free:
 * @range: Integer range structure to free
 *
 * Free a range structure previously allocated with ipatch_range_new().
 */
void
ipatch_range_free (IpatchRange *range)
{
  g_slice_free (IpatchRange, range);
}

/**
 * ipatch_value_set_range:
 * @value: a valid GValue of IPATCH_TYPE_RANGE boxed type
 * @range: Range structure to assign to @value
 *
 * Set the range values of a IPATCH_TYPE_RANGE GValue. The @range
 * structure is copied.
 */
void
ipatch_value_set_range (GValue *value, const IpatchRange *range)
{
  g_return_if_fail (IPATCH_VALUE_HOLDS_RANGE (value));
  g_value_set_boxed (value, range);
}

/**
 * ipatch_value_set_static_range: (skip)
 * @value: A valid GValue of IPATCH_TYPE_RANGE boxed type
 * @range: (transfer full): Range structure to assign to @value
 *
 * Set the range values of a IPATCH_TYPE_RANGE GValue. This function uses
 * @range directly and so it should be static, use ipatch_value_set_range()
 * if the @range value should be duplicated.
 */
void
ipatch_value_set_static_range (GValue *value, IpatchRange *range)
{
  g_return_if_fail (IPATCH_VALUE_HOLDS_RANGE (value));
  g_value_set_static_boxed (value, range);
}

/**
 * ipatch_value_get_range:
 * @value: A valid GValue of IPATCH_TYPE_RANGE boxed type
 *
 * Get the range structure from a IPATCH_TYPE_RANGE GValue.
 *
 * Returns: (transfer none): #IpatchRange structure containing the range values of @value or
 * %NULL if not set. The returned structure is NOT duplicated and is the
 * same pointer used in @value.
 */
IpatchRange *
ipatch_value_get_range (const GValue *value)
{
  g_return_val_if_fail (IPATCH_VALUE_HOLDS_RANGE (value), NULL);
  return ((IpatchRange *)g_value_get_boxed (value));
}


GType
ipatch_param_spec_range_get_type (void)
{
  static GType spec_type = 0;

  if (!spec_type)
    {
      static const GParamSpecTypeInfo spec_info =
	{
	  sizeof (IpatchParamSpecRange),
	  0,
	  NULL,			/* instance_init */
	  G_TYPE_BOXED,		/* value type */
	  NULL,			/* finalize */
	  ipatch_param_spec_range_set_default,
	  ipatch_param_spec_range_validate,
	  ipatch_param_spec_range_values_cmp,
	};

      spec_type = g_param_type_register_static ("IpatchParamSpecRange",
						&spec_info);
    }

  return (spec_type);
}

static void
ipatch_param_spec_range_set_default (GParamSpec *pspec, GValue *value)
{
  IpatchParamSpecRange *range_pspec = IPATCH_PARAM_SPEC_RANGE (pspec);
  IpatchRange *range;

  range = ipatch_value_get_range (value);

  if (!range)
  {
    range = ipatch_range_new (0, 0);
    ipatch_value_set_range (value, range);
  }

  range->low = range_pspec->default_low;
  range->high = range_pspec->default_high;
}

static gboolean
ipatch_param_spec_range_validate (GParamSpec *pspec, GValue *value)
{
  IpatchParamSpecRange *range_pspec = IPATCH_PARAM_SPEC_RANGE (pspec);
  IpatchRange *range, old_range;

  range = ipatch_value_get_range (value);
  if (!range)
    {
      range = ipatch_range_new (0, 0);
      range->low = range_pspec->default_low;
      range->high = range_pspec->default_high;
      return (TRUE);
    }

  old_range = *range;
  range->low = CLAMP (range->low, range_pspec->min, range_pspec->max);
  range->high = CLAMP (range->high, range_pspec->min, range_pspec->max);

  return (old_range.low != range->low || old_range.high != range->high);
}

static gint
ipatch_param_spec_range_values_cmp (GParamSpec *pspec, const GValue *value1,
				    const GValue *value2)
{
  IpatchRange *range1, *range2;

  range1 = ipatch_value_get_range (value1);
  range2 = ipatch_value_get_range (value2);

  /* either one NULL? */
  if (!range1 || !range2)
  {
    if (!range1 && !range2) return 0;
    if (!range1) return -1;
    return 1;
  }

  /* check if low value is not equal */
  if (range1->low < range2->low) return -1;
  if (range1->low > range2->low) return 1;

  /* low values are equal */

  /* check if high values are not equal */
  if (range1->high < range2->high) return -1;
  if (range1->high > range2->high) return 1;

  /* range is equal */
  return 0;
}

/**
 * ipatch_param_spec_range:
 * @name: Property name
 * @nick: Property nick name
 * @blurb: Property description blurb
 * @min: Minimum value for range end points (can be -1 to allow undefined
 *   ranges)
 * @max: Maximum value for range end points
 * @default_low: Default value for low endpoint of range
 * @default_high: Default value for high endpoint of range
 * @flags: Property flags
 *
 * Create a parameter specification for IPATCH_TYPE_RANGE GValues.
 *
 * Returns: (transfer full): New range parameter specification.
 */
GParamSpec *
ipatch_param_spec_range (const char *name, const char *nick, const char *blurb,
			int min, int max, int default_low, int default_high,
			GParamFlags flags)
{
  IpatchParamSpecRange *range_spec;

  g_return_val_if_fail (min >= -1 && min <= max, NULL);
  g_return_val_if_fail (default_low >= min && default_low <= max, NULL);
  g_return_val_if_fail (default_high >= min && default_high <= max, NULL);

  /* FIXME - Whats the proper way to create a boxed GParamSpec? */
  range_spec =  g_param_spec_internal (IPATCH_TYPE_PARAM_RANGE,
				       name, nick, blurb, flags);
  G_PARAM_SPEC (range_spec)->value_type = IPATCH_TYPE_RANGE;

  range_spec->min = min;
  range_spec->max = max;
  range_spec->default_low = default_low;
  range_spec->default_high = default_high;

  return ((GParamSpec *)range_spec);
}
