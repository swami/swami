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
 * SECTION: IpatchUnit
 * @short_description: Unit conversion system
 * @see_also: 
 * @stability: Stable
 *
 * System for registering unit types and conversion functions.
 */
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchUnit.h"
#include "IpatchRange.h"
#include "ipatch_priv.h"
#include "i18n.h"

/* first dynamic unit type ID */
#define IPATCH_UNIT_TYPE_FIRST_DYNAMIC_ID	1024

typedef struct
{
  IpatchValueTransform func;
  GDestroyNotify notify_func;
  gpointer user_data;
} ConversionHashVal;

static void ipatch_unit_conversion_hash_val_destroy (gpointer data);
static ConversionHashVal *ipatch_unit_conversion_hash_val_new (void);
static void ipatch_unit_conversion_hash_val_free (ConversionHashVal *val);

G_LOCK_DEFINE_STATIC (unit_info);

static GHashTable *unit_id_hash = NULL; /* unit registry (id) */
static GHashTable *unit_name_hash = NULL; /* secondary key (name -> id) */

/* unit class mappings (srcId:classType -> destInfo) */
static GHashTable *class_map_hash = NULL;

static GHashTable *conversion_hash = NULL; /* conversion function hash */

/* next dynamic unit type ID (increment) */
static guint16 last_unit_id = IPATCH_UNIT_TYPE_FIRST_DYNAMIC_ID;

void _ipatch_unit_generic_init (void);
void _ipatch_unit_dls_init (void);
void _ipatch_unit_sf2_init (void);


/**
 * _ipatch_unit_init: (skip)
 *
 * Initialize unit system
 */
void
_ipatch_unit_init (void)
{
  unit_id_hash = g_hash_table_new (NULL, NULL);
  unit_name_hash = g_hash_table_new (g_str_hash, g_str_equal);
  class_map_hash = g_hash_table_new (NULL, NULL);
  conversion_hash = g_hash_table_new_full (NULL, NULL, NULL,
                                           ipatch_unit_conversion_hash_val_destroy);

  /* initialize unit types and conversion handlers */

  _ipatch_unit_generic_init ();
  _ipatch_unit_dls_init ();
  _ipatch_unit_sf2_init ();
}

static void
ipatch_unit_conversion_hash_val_destroy (gpointer data)
{
  ConversionHashVal *val = data;

  if (val->notify_func) val->notify_func (val->user_data);
  ipatch_unit_conversion_hash_val_free (val);
}

static ConversionHashVal *
ipatch_unit_conversion_hash_val_new (void)
{
  return (g_slice_new (ConversionHashVal));
}

static void
ipatch_unit_conversion_hash_val_free (ConversionHashVal *val)
{
  g_slice_free (ConversionHashVal, val);
}

GType
ipatch_unit_info_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_boxed_type_register_static ("IpatchUnitInfo",
				(GBoxedCopyFunc)ipatch_unit_info_duplicate,
				(GBoxedFreeFunc)ipatch_unit_info_free);

  return (type);
}

/**
 * ipatch_unit_info_new: (skip)
 *
 * Allocate a unit info structure for registering unit types with
 * ipatch_unit_register(). Using this function should minimize API changes
 * if additional fields are added to #IpatchUnitInfo. Free the returned
 * structure with ipatch_unit_free() when finished registering unit types.
 *
 * Returns: The newly allocated unit info structure.
 */
IpatchUnitInfo *
ipatch_unit_info_new (void)
{
  return (g_slice_new0 (IpatchUnitInfo));
}

/**
 * ipatch_unit_info_free: (skip)
 * @info: Unit info to free
 *
 * Free a unit info structure that was created with ipatch_unit_info_new().
 */
void
ipatch_unit_info_free (IpatchUnitInfo *info)
{
  g_slice_free (IpatchUnitInfo, info);
}

/**
 * ipatch_unit_info_duplicate: (skip)
 * @info: Unit info to duplicate
 *
 * Duplicate a unit info structure.
 *
 * Returns: Newly allocated duplicate unit info structure
 *
 * Since: 1.1.0
 */
IpatchUnitInfo *
ipatch_unit_info_duplicate (const IpatchUnitInfo *info)
{
  IpatchUnitInfo *new;

  new = ipatch_unit_info_new ();
  memcpy (new, info, sizeof (IpatchUnitInfo));
  return (new);
}

/**
 * ipatch_unit_register:
 * @info: (transfer none): Unit info (shallow copied)
 *
 * Add a new unit type to the unit registry. Note that the @info structure
 * is shallow copied, so strings should be constant or guaranteed to not
 * be freed.  If the <structfield>id</structfield> field is already set
 * (non-zero) in @info, then it is used (should be 0 for dynamic unit
 * types).  If the <structfield>label</structfield> field of the @info
 * structure is %NULL then it is set to the i18n translated string for
 * <structfield>name</structfield>.  Unit types can not be un-registered.
 * Unit IDs of dynamic (non built-in types) should not be relied apon to
 * always be the same between program executions.
 *
 * Returns: New unit ID
 */
guint16
ipatch_unit_register (const IpatchUnitInfo *info)
{
  IpatchUnitInfo *newinfo;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (info->name != NULL, 0);

  /* allocate the new unit info, so that the pointer is constant and can be
   * returned to the user out of lock (not safe if it was in a GArray) */
  newinfo = ipatch_unit_info_new ();
  *newinfo = *info;

  /* if label not set, use i18n translated name */
  if (!info->label) newinfo->label = _(info->name);

  G_LOCK (unit_info);

  if (!newinfo->id) newinfo->id = last_unit_id++;

  g_hash_table_insert (unit_id_hash, /* hash by id */
		       GUINT_TO_POINTER ((guint32)(newinfo->id)), newinfo);
  g_hash_table_insert (unit_name_hash, newinfo->name, newinfo);	/* hash by name */

  G_UNLOCK (unit_info);

  return (newinfo->id);
}

/**
 * ipatch_unit_lookup:
 * @id: Unit ID
 *
 * Looks up unit info by ID.
 *
 * Returns: (transfer none): Unit info structure with @id or %NULL if not found,
 * returned structure is internal and should not be modified or freed
 */
IpatchUnitInfo *
ipatch_unit_lookup (guint16 id)
{
  IpatchUnitInfo *info;

  G_LOCK (unit_info);
  info = g_hash_table_lookup (unit_id_hash, GUINT_TO_POINTER ((guint32)id));
  G_UNLOCK (unit_info);

  return (info);
}

/**
 * ipatch_unit_lookup_by_name:
 * @name: Unit name identifier
 *
 * Looks up unit info by name.
 *
 * Returns: (transfer none): Unit info structure with @name or %NULL if not found,
 * returned structure is internal and should not be modified or freed
 */
IpatchUnitInfo *
ipatch_unit_lookup_by_name (const char *name)
{
  IpatchUnitInfo *info;

  G_LOCK (unit_info);
  info = g_hash_table_lookup (unit_name_hash, name);
  G_UNLOCK (unit_info);

  return (info);
}

/**
 * ipatch_unit_class_register_map:
 * @class_type: (type IpatchUnitClassType): Class type (see #IpatchUnitClassType)
 * @src_units: Source unit type of mapping
 * @dest_units: Destination unit type for this map
 * 
 * Register a unit class mapping.  Unit class types define domains of
 * conversion, an example is the "user" unit class
 * (#IPATCH_UNIT_CLASS_USER) which is used to convert values to units
 * digestable by a human.  A conversion class is essentially a mapping
 * between unit types, which can then be used to lookup conversion
 * functions.
 */
void
ipatch_unit_class_register_map (guint16 class_type, guint16 src_units,
				guint16 dest_units)
{
  IpatchUnitInfo *src_info, *dest_info;
  guint32 hashval;

  g_return_if_fail (class_type > IPATCH_UNIT_CLASS_NONE);
  g_return_if_fail (class_type < IPATCH_UNIT_CLASS_COUNT);

  hashval = class_type | (src_units << 16);

  G_LOCK (unit_info);

  src_info = g_hash_table_lookup (unit_id_hash,
				  GUINT_TO_POINTER ((guint32)src_units));
  dest_info = g_hash_table_lookup (unit_id_hash,
				   GUINT_TO_POINTER ((guint32)dest_units));

  /* only insert map if unit types are valid */
  if (src_info != NULL && dest_info != NULL)
    g_hash_table_insert (class_map_hash, GUINT_TO_POINTER (hashval), dest_info);

  G_UNLOCK (unit_info);

  g_return_if_fail (src_info != NULL);
  g_return_if_fail (dest_info != NULL);
}

/**
 * ipatch_unit_class_lookup_map:
 * @class_type: (type IpatchUnitClassType): Class type (see #IpatchUnitClassType)
 * @src_units: Source unit type of mapping to lookup
 * 
 * Lookup a unit class mapping (see ipatch_unit_class_register_map ()).
 * 
 * Returns: (transfer none): Pointer to destination unit info structure,
 *   or %NULL if not found.  Returned structure is internal and should not
 *   be modified or freed.
 */
IpatchUnitInfo *
ipatch_unit_class_lookup_map (guint16 class_type, guint16 src_units)
{
  IpatchUnitInfo *dest_info;
  guint32 hashval;

  g_return_val_if_fail (class_type > IPATCH_UNIT_CLASS_NONE, 0);
  g_return_val_if_fail (class_type < IPATCH_UNIT_CLASS_COUNT, 0);

  hashval = class_type | (src_units << 16);

  G_LOCK (unit_info);
  dest_info = g_hash_table_lookup (class_map_hash, GUINT_TO_POINTER (hashval));
  G_UNLOCK (unit_info);

  return (dest_info);
}

/**
 * ipatch_unit_conversion_register: (skip)
 * @src_units: Source unit type
 * @dest_units: Destination unit type
 * @func: Conversion function handler or %NULL for unity conversion (the
 *   value type will be converted but not the actual value, example:
 *   float -> int).
 *
 * Register a parameter unit conversion function.
 */
void
ipatch_unit_conversion_register (guint16 src_units, guint16 dest_units,
				 IpatchValueTransform func)
{
  ipatch_unit_conversion_register_full (src_units, dest_units, func, NULL, NULL);
}

/**
 * ipatch_unit_conversion_register_full: (rename-to ipatch_unit_conversion_register)
 * @src_units: Source unit type
 * @dest_units: Destination unit type
 * @func: Conversion function handler or %NULL for unity conversion (the
 *   value type will be converted but not the actual value, example: float to int).
 * @notify_func: (nullable) (scope async) (closure user_data): Destroy notification
 *   when conversion function is removed
 * @user_data: (nullable): Data to pass to @notify_func callback
 *
 * Register a parameter unit conversion function.  Like ipatch_unit_conversion_register()
 * but friendlier to GObject Introspection.
 *
 * Since: 1.1.0
 */
void
ipatch_unit_conversion_register_full (guint16 src_units, guint16 dest_units,
				      IpatchValueTransform func, GDestroyNotify notify_func,
                                      gpointer user_data)
{
  ConversionHashVal *val;
  guint32 hashkey;

  hashkey = src_units | (dest_units << 16);

  val = ipatch_unit_conversion_hash_val_new ();         // ++ alloc
  val->func = func;
  val->notify_func = notify_func;
  val->user_data = user_data;

  G_LOCK (unit_info);   // !! hash takes over value
  g_hash_table_insert (conversion_hash, GUINT_TO_POINTER (hashkey), val);
  G_UNLOCK (unit_info);
}


/**
 * ipatch_unit_conversion_lookup: (skip)
 * @src_units: Source unit type
 * @dest_units: Destination unit type
 * @set: Location to store a boolean value indicating if the conversion is
 *   set, to differentiate between a %NULL conversion function and an invalid
 *   conversion. Can be %NULL in which case this parameter is ignored.
 *
 * Lookup a conversion function by source and destination unit types.
 *
 * Returns: Conversion function pointer or %NULL if a unity conversion or
 * no matching handlers (use @set to determine which).
 */
IpatchValueTransform
ipatch_unit_conversion_lookup (guint16 src_units, guint16 dest_units,
			       gboolean *set)
{
  ConversionHashVal *hashval;
  IpatchValueTransform func;
  guint32 hashkey;

  hashkey = src_units | (dest_units << 16);

  G_LOCK (unit_info);
  hashval = g_hash_table_lookup (conversion_hash, GUINT_TO_POINTER (hashkey));
  if (hashval) func = hashval->func;
  G_UNLOCK (unit_info);

  if (set) *set = hashval != NULL;
  return (hashval ? func : NULL);
}

/**
 * ipatch_unit_convert:
 * @src_units: Source unit type ID
 * @dest_units: Destination unit type ID
 * @src_val: Source value (type should be compatible with the source unit's
 *   value type)
 * @dest_val: (transfer none): Destination value (value should be initialized to a type that is
 *   compatible with the destination unit's value type)
 *
 * Convert a value from one unit type to another.
 *
 * Returns: %TRUE if value was successfully converted, %FALSE
 * otherwise (the only reasons for failure are invalid function
 * parameters, no conversion function for the given unit types, or
 * incompatible GValue types in conversion, therefore the return value
 * can be safely ignored if the caller is sure the parameters and
 * types are OK).
 */
gboolean
ipatch_unit_convert (guint16 src_units, guint16 dest_units,
		     const GValue *src_val, GValue *dest_val)
{
  IpatchValueTransform convert_func;
  IpatchUnitInfo *src_info, *dest_info;
  ConversionHashVal *unit_converter;            // More descriptive for g_return_val_if_fail
  const GValue *sv;
  GValue tmpsv = { 0 }, tmpdv = { 0 }, *dv;
  guint32 hashkey;
  gboolean retval;

  hashkey = src_units | (dest_units << 16);

  G_LOCK (unit_info);

  src_info = g_hash_table_lookup (unit_id_hash,
				  GUINT_TO_POINTER ((guint32)src_units));
  dest_info = g_hash_table_lookup (unit_id_hash,
				   GUINT_TO_POINTER ((guint32)dest_units));
  unit_converter = g_hash_table_lookup (conversion_hash, GUINT_TO_POINTER (hashkey));
  convert_func = unit_converter ? unit_converter->func : NULL;

  G_UNLOCK (unit_info);

  g_return_val_if_fail (src_info != NULL, FALSE);
  g_return_val_if_fail (dest_info != NULL, FALSE);
  g_return_val_if_fail (unit_converter != NULL, FALSE);

  if (G_UNLIKELY (!convert_func))	/* unity conversion? */
    {
      retval = g_value_transform (src_val, dest_val);

      if (G_UNLIKELY (!retval))
	{
	  g_critical ("%s: Failed to transform value type '%s' to type '%s'",
		      G_STRLOC, g_type_name (G_VALUE_TYPE (src_val)),
		      g_type_name (G_VALUE_TYPE (dest_val)));
	  return (FALSE);
	}

      return (TRUE);
    }

  /* source value needs to be transformed? */
  if (G_UNLIKELY (G_VALUE_TYPE (src_val) != src_info->value_type))
    {
      g_value_init (&tmpsv, src_info->value_type);
      retval = g_value_transform (src_val, &tmpsv);
      sv = &tmpsv;

      if (G_UNLIKELY (!retval))
	{
	  g_value_unset (&tmpsv);
	  g_critical ("%s: Failed to transform value type '%s' to type '%s'",
		      G_STRLOC, g_type_name (G_VALUE_TYPE (src_val)),
		      g_type_name (src_info->value_type));
	  return (FALSE);
	}
    }
  else sv = src_val;		/* same type, just use it */

  /* destination value needs to be transformed? */
  if (G_LIKELY (G_VALUE_TYPE (dest_val) == dest_info->value_type))
    {			  /* same type, just reset value and use it */
      g_value_reset (dest_val);
      dv = dest_val;
    }
  else if (!g_value_type_transformable (dest_info->value_type,
					G_VALUE_TYPE (dest_val)))
    {
      g_critical ("%s: Failed to transform value type '%s' to type '%s'",
		  G_STRLOC, g_type_name (dest_info->value_type),
		  g_type_name (G_VALUE_TYPE (dest_val)));
      return (FALSE);
    }
  else	/* initialize temp value to native type (transformed later) */
    {
      g_value_init (&tmpdv, dest_info->value_type);
      dv = &tmpdv;
    }

  /* do the conversion */
  (*convert_func)(sv, dv);

  /* free the converted source value if needed */
  if (G_UNLIKELY (sv == &tmpsv)) g_value_unset (&tmpsv);

  if (G_UNLIKELY (dv == &tmpdv)) /* transform the destination value if needed */
    {
      g_value_transform (dv, dest_val);
      g_value_unset (&tmpdv);
    }

  return (TRUE);
}

/**
 * ipatch_unit_user_class_convert:
 * @src_units: Source unit type ID
 * @src_val: Source value (type should be compatible with the source unit's
 *   value type)
 * 
 * Converts a value to "user" units.  User units are unit types that
 * are adequate for human consumption.  The #IPATCH_UNIT_CLASS_USER
 * map is used to lookup the corresponding user type to convert to.
 * Not all unit types have an associated user type or the @src_units
 * type can itself be a user type; in either of these cases the
 * @src_val is converted as is (possibly converted from another value
 * type to double).
 *
 * Returns: The value converted to user units.
 */
double
ipatch_unit_user_class_convert (guint16 src_units, const GValue *src_val)
{
  IpatchUnitInfo *dest_info;
  guint16 dest_units;
  GValue v = { 0 };
  double retval;

  g_return_val_if_fail (src_val != NULL, 0.0);

  dest_info = ipatch_unit_class_lookup_map (IPATCH_UNIT_CLASS_USER, src_units);
  dest_units = dest_info ? dest_info->id : src_units;

  g_value_init (&v, G_TYPE_DOUBLE);
  ipatch_unit_convert (src_units, dest_units, src_val, &v);

  retval = g_value_get_double (&v);
  g_value_unset (&v);	/* probably not necessary, but its the right way (TM) */

  return (retval);
}
