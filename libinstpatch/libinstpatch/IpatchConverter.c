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
 * SECTION: IpatchConverter
 * @short_description: Base class for object conversion handlers
 * @see_also: 
 * @stability: Stable
 *
 * A base abstract type for object conversion handlers.
 */
#include <stdio.h>
#include <glib.h>
#include <glib-object.h>
#include "misc.h"
#include "IpatchConverter.h"
#include "i18n.h"

enum {
  PROP_0,
  PROP_PROGRESS
};

/* structure used for log entries (the log is a prepend log, newest items
   at the head of the list) */
typedef struct _LogEntry
{
  GObject *item;	   /* item this message applies to or %NULL */
  guint8 type; /* type of message and flags (IpatchConverterLogType) */
  union
  {
    char *msg;			/* LOG_INFO/WARN/CRITICAL/FATAL */
    float rating;		/* LOG_RATING */
  } data;
} LogEntry;


static gint priority_GCompareFunc (gconstpointer a, gconstpointer b);
static const IpatchConverterInfo *convert_lookup_map_U (GType **array, GType conv_type,
                                                        GType src_type, GType dest_type, guint flags);
static void ipatch_converter_class_init (IpatchConverterClass *klass);
static void ipatch_converter_finalize (GObject *gobject);
static void ipatch_converter_set_property (GObject *object, guint property_id,
					   const GValue *value,
					   GParamSpec *pspec);
static void ipatch_converter_get_property (GObject *object, guint property_id,
					   GValue *value, GParamSpec *pspec);

/* lock used for list and hash tables */
G_LOCK_DEFINE_STATIC (conv_maps);
static GList *conv_maps = NULL;	/* list of all registered IpatchConverterInfo */
static gpointer parent_class = NULL;


/**
 * ipatch_convert_objects:
 * @input: Input object
 * @output: Output object
 * @err: Location to store error info or %NULL
 *
 * A convenience function for converting from one object to another.  This
 * function will only work for converters which take exactly one input and
 * output object.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set)
 */
gboolean
ipatch_convert_objects (GObject *input, GObject *output, GError **err)
{
  IpatchConverter *conv;

  conv = ipatch_create_converter_for_objects (input, output, err);      // ++ ref converter
  if (!conv) return FALSE;

  if (!ipatch_converter_convert (conv, err))                            // -- unref converter
  {
    g_object_unref (conv);
    return (FALSE);
  }

  g_object_unref (conv);                                                // -- unref converter

  return (TRUE);
}

/**
 * ipatch_convert_object_to_type:
 * @object: Object to convert from
 * @type: Type of object to convert to
 * @err: Location to store error info or %NULL to ignore
 *
 * A convenience function to convert an object to another object of a given
 * type.  This function will only work for converters which require 1
 * input and one or zero outputs. The output object is created as needed
 * and returned.
 *
 * Returns: (transfer full): The output object or %NULL on error (in which
 * case @err may be set). The returned object has a refcount of 1 which the caller owns.
 */
GObject *
ipatch_convert_object_to_type (GObject *object, GType type, GError **err)
{
  const IpatchConverterInfo *info;
  IpatchConverter *conv;
  GObject *output = NULL;
  GType convtype;

  convtype = ipatch_find_converter (G_OBJECT_TYPE (object), type);
  if (!convtype)
  {
    g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_UNHANDLED_CONVERSION,
                 _("Unsupported conversion of type %s to %s"),
                 G_OBJECT_TYPE_NAME (object), g_type_name (type));
    return (NULL);
  }

  info = ipatch_lookup_converter_info (convtype, G_OBJECT_TYPE (object), type);
  g_return_val_if_fail (info != NULL, NULL);	/* shouldn't happen */

  if (info->dest_count < 0 || info->dest_count > 1)
    {
      g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_UNSUPPORTED,
		   _("Conversion from %s to %s requires %d outputs"),
		   G_OBJECT_TYPE_NAME (object), g_type_name (type),
		   info->dest_count);
      return (NULL);
    }

  conv = IPATCH_CONVERTER (g_object_new (convtype, NULL));	/* ++ ref */

  ipatch_converter_add_input (conv, object);

  if (info->dest_count == 1)	/* if 1 output object expected, create it */
  {
      output = g_object_new (type, NULL);	/* ++ ref */
    ipatch_converter_add_output (conv, output);
  }

  if (!ipatch_converter_convert (conv, err))	/* do the conversion */
  {
      if (output) g_object_unref (output);
      g_object_unref (conv);
    return (NULL);
  }

  if (!output) output = ipatch_converter_get_output (conv);	/* ++ ref */

  g_object_unref (conv);

  return (output);	/* !! caller takes over reference */
}

/**
 * ipatch_convert_object_to_type_multi: (skip)
 * @object: Object to convert from
 * @type: Type of object to convert to
 * @err: Location to store error info or %NULL to ignore
 *
 * A convenience function to convert an object to one or more objects of a given
 * @type.  This function will work for converters which require 1
 * input and any number of outputs.
 *
 * Returns: (transfer full): List of output objects or %NULL on error (in which
 * case @err may be set). The returned object list has a refcount of 1 which the caller owns.
 */
IpatchList *
ipatch_convert_object_to_type_multi (GObject *object, GType type, GError **err)
{
  return (ipatch_convert_object_to_type_multi_set (object, type, err, NULL));
}

/**
 * ipatch_convert_object_to_type_multi_list: (rename-to ipatch_convert_object_to_type_multi)
 * @object: Object to convert from
 * @type: Type of object to convert to
 * @err: Location to store error info or %NULL to ignore
 *
 * A convenience function to convert an object to one or more objects of a given
 * @type.  This function will work for converters which require 1
 * input and any number of outputs.
 *
 * Returns: (element-type GObject.Object) (transfer full): List of output objects or %NULL on error (in which
 * case @err may be set). Free the list with ipatch_glist_unref_free() when finished using it.
 *
 * Since: 1.1.0
 */
GList *
ipatch_convert_object_to_type_multi_list (GObject *object, GType type, GError **err)
{
  return (ipatch_convert_object_to_type_multi_set_vlist (object, type, err, NULL, NULL));
}

/**
 * ipatch_convert_object_to_type_multi_set: (skip)
 * @object: Object to convert from
 * @type: Type of object to convert to
 * @err: Location to store error info or %NULL to ignore
 * @first_property_name: (nullable): Name of first property to assign or %NULL
 * @...: First property value followed by property name/value pairs (as per
 *   g_object_set()) to assign to the resulting converter, terminated with a
 *   %NULL property name.
 *
 * A convenience function to convert an object to one or more objects of a given
 * @type.  This function will work for converters which require 1
 * input and any number of outputs.  Like ipatch_convert_object_to_type_multi()
 * but allows for properties of the converter to be assigned.
 *
 * Returns: (transfer full): List of output objects or %NULL on error (in which
 * case @err may be set). The returned object list has a refcount of 1 which the caller owns.
 */
IpatchList *
ipatch_convert_object_to_type_multi_set (GObject *object, GType type, GError **err,
                                         const char *first_property_name, ...)
{
  IpatchList *list;
  GList *items;
  va_list args;

  va_start (args, first_property_name);
  items = ipatch_convert_object_to_type_multi_set_vlist (object, type,          // ++ alloc items list
                                                         err, first_property_name, args);
  va_end (args);

  if (!items) return NULL;

  list = ipatch_list_new ();            // ++ ref new list
  list->items = items;                  // !! Assign items to list
  return list;                          // !! Caller takes over list
}

/**
 * ipatch_convert_object_to_type_multi_set_vlist: (skip)
 * @object: Object to convert from
 * @type: Type of object to convert to
 * @err: Location to store error info or %NULL to ignore
 * @first_property_name: (nullable): Name of first property to assign or %NULL
 * @args: First property value followed by property name/value pairs (as per
 *   g_object_set()) to assign to the resulting converter, terminated with a
 *   %NULL property name.
 *
 * A convenience function to convert an object to one or more objects of a given
 * @type.  This function will work for converters which require 1
 * input and any number of outputs.  Like ipatch_convert_object_to_type_multi()
 * but allows for properties of the converter to be assigned.
 *
 * Returns: (element-type GObject.Object) (transfer full): List of output objects or %NULL on error (in which
 * case @err may be set). Free the list with ipatch_glist_unref_free() when finished using it.
 *
 * Since: 1.1.0
 */
GList *
ipatch_convert_object_to_type_multi_set_vlist (GObject *object, GType type, GError **err,
                                               const char *first_property_name, va_list args)
{
  IpatchConverter *conv;
  GList *items;

  conv = ipatch_create_converter_for_object_to_type (object, type, err);        // ++ ref converter
  if (!conv) return NULL;

  if (first_property_name)
    g_object_set_valist ((GObject *)conv, first_property_name, args);

  if (!ipatch_converter_convert (conv, err))	/* do the conversion */
  {
    g_object_unref (conv);                      // -- unref converter
    return (NULL);
  }

  items = ipatch_converter_get_outputs_list (conv);     /* ++ alloc object list */

  g_object_unref (conv);        /* -- unref converter */

  return (items);	/* !! caller takes over ownership */
}

/**
 * ipatch_create_converter:
 * @src_type: #GObject derived source type
 * @dest_type: #GObject derived destination type
 *
 * Create a converter object for converting an object of type @src_type to
 * @dest_type. A convenience function, since one could use
 * ipatch_find_converter() and create an instance of the returned type.
 * See ipatch_find_converter() for more details.
 *
 * Returns: (transfer full): The new converter object with a reference count
 *   of 1 which the caller owns, or %NULL if there is no matching conversion
 *   handler type.
 */
IpatchConverter *
ipatch_create_converter (GType src_type, GType dest_type)
{
  GType conv_type;

  g_return_val_if_fail (g_type_is_a (src_type, G_TYPE_OBJECT), NULL);
  g_return_val_if_fail (g_type_is_a (dest_type, G_TYPE_OBJECT), NULL);

  conv_type = ipatch_find_converter (src_type, dest_type);
  if (!conv_type) return (NULL);

  /* ++ ref new object and let the caller have it */
  return (IPATCH_CONVERTER (g_object_new (conv_type, NULL)));
}

/**
 * ipatch_create_converter_for_objects:
 * @input: Input object
 * @output: Output object
 * @err: Location to store error info or %NULL
 *
 * A convenience function for creating a converter for converting from one
 * object to another. This function will only work for converters which take
 * exactly one input and output object.
 *
 * Returns: (transfer full): The new converter or %NULL on error
 *
 * Since: 1.1.0
 */
IpatchConverter *
ipatch_create_converter_for_objects (GObject *input, GObject *output, GError **err)
{
  IpatchConverter *conv;

  g_return_val_if_fail (G_IS_OBJECT (input), FALSE);
  g_return_val_if_fail (G_IS_OBJECT (output), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  /* ++ ref new converter */
  conv = ipatch_create_converter (G_OBJECT_TYPE (input), G_OBJECT_TYPE (output));

  if (!conv)
  {
    g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_UNHANDLED_CONVERSION,
                 _("Unsupported conversion of type %s to %s."),
                 G_OBJECT_TYPE_NAME (input), G_OBJECT_TYPE_NAME (output));
    return (FALSE);
  }

  ipatch_converter_add_input (conv, input);
  ipatch_converter_add_output (conv, output);

  return (conv);        // !! Caller takes over reference
}

/**
 * ipatch_create_converter_for_object_to_type:
 * @object: Object to convert from
 * @dest_type: Type of object to convert to
 * @err: Location to store error info or %NULL to ignore
 *
 * A convenience function to create a converter for converting an object to
 * another object of a given type.
 *
 * Returns: (transfer full): The new converter object with a reference count
 *   of 1 which the caller owns, or %NULL if there is no matching conversion
 *   handler type.
 *
 * Since: 1.1.0
 */
IpatchConverter *
ipatch_create_converter_for_object_to_type (GObject *object, GType dest_type, GError **err)
{
  const IpatchConverterInfo *info;
  IpatchConverter *conv;
  GObject *output = NULL;
  GType convtype;
  int i;

  convtype = ipatch_find_converter (G_OBJECT_TYPE (object), dest_type);

  if (!convtype)
  {
    g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_UNHANDLED_CONVERSION,
                 _("Unsupported conversion of type %s to %s"),
                 G_OBJECT_TYPE_NAME (object), g_type_name (dest_type));
    return (NULL);
  }

  info = ipatch_lookup_converter_info (convtype, G_OBJECT_TYPE (object), dest_type);
  g_return_val_if_fail (info != NULL, NULL);	/* shouldn't happen */

  conv = IPATCH_CONVERTER (g_object_new (convtype, NULL));	/* ++ ref */

  ipatch_converter_add_input (conv, object);


  if (info->dest_count > 0)	/* if outputs are expected, create them */
  {
    for (i = 0; i < info->dest_count; i++)
    {
      output = g_object_new (dest_type, NULL);	/* ++ ref output */
      ipatch_converter_add_output (conv, output);
      g_object_unref (output);  /* -- unref output */
    }
  }

  return (conv);                /* !! caller takes over reference */
}

/**
 * ipatch_register_converter_map:
 * @conv_type: #IpatchConverter derived GType of conversion handler
 * @flags: #IpatchConverterFlags, #IPATCH_CONVERTER_SRC_DERIVED or
 *   #IPATCH_CONVERTER_DEST_DERIVED will match derived types of @src_type
 *   or @dest_type respectively.
 * @priority: Converter map priority (number from 0 to 100, 0 will use the default).
 *   #IpatchConverterPriority defines some common priorities. Used for overriding other
 *   converters for specific types.
 * @src_type: Type for source object (GObject derived type or interface type).
 * @src_match: The furthest parent type of @src_type to match (all types in
 *   between are also matched, 0 or G_TYPE_NONE to only use @src_type).
 * @src_count: Required number of source items (can also be an #IpatchConverterCount value)
 * @dest_type: Type for destination object (GObject derived type or interface type).
 * @dest_match: The furthest parent type of @dest_type to match (all types in
 *   between are also matched, 0, or G_TYPE_NONE to only use @dest_type).
 * @dest_count: Required number of destination items (can also be
 *   an #IpatchConverterCount value).  This value can be 0 in the case where
 *   no objects should be supplied, but will be instead assigned after
 *   conversion.
 *
 * Registers a #IpatchConverter handler to convert objects of @src_type to @dest_type.
 * NOTE: Previous versions of this function combined flags and priority into a single param.
 *
 * Since: 1.1.0
 */
void
ipatch_register_converter_map (GType conv_type, guint8 flags, guint8 priority,
			       GType src_type, GType src_match, gint8 src_count,
			       GType dest_type, GType dest_match, gint8 dest_count)
{
  const IpatchConverterInfo *converter_exists;
  IpatchConverterInfo *map;

  g_return_if_fail (g_type_is_a (conv_type, IPATCH_TYPE_CONVERTER));
  g_return_if_fail (g_type_is_a (src_type, G_TYPE_OBJECT) || G_TYPE_IS_INTERFACE (src_type));
  g_return_if_fail (g_type_is_a (dest_type, G_TYPE_OBJECT) || G_TYPE_IS_INTERFACE (dest_type));

  g_return_if_fail (!src_match || g_type_is_a (src_type, src_match));
  g_return_if_fail (!dest_match || g_type_is_a (dest_type, dest_match));

  converter_exists = ipatch_lookup_converter_info (conv_type, 0, 0);
  g_return_if_fail (!converter_exists);

  priority = flags & 0xFF;
  if (priority == 0) priority = IPATCH_CONVERTER_PRIORITY_DEFAULT;

  // Enable derived flags for interface types

  if (G_TYPE_IS_INTERFACE (src_type))
    flags |= IPATCH_CONVERTER_SRC_DERIVED;

  if (G_TYPE_IS_INTERFACE (dest_type))
    flags |= IPATCH_CONVERTER_DEST_DERIVED;

  map = g_slice_new (IpatchConverterInfo);
  map->conv_type = conv_type;
  map->flags = flags;
  map->priority = priority;
  map->src_type = src_type;
  map->src_match = src_match;
  map->src_count = src_count;
  map->dest_type = dest_type;
  map->dest_match = dest_match;
  map->dest_count = dest_count;

  G_LOCK (conv_maps);
  conv_maps = g_list_insert_sorted (conv_maps, map, priority_GCompareFunc);
  G_UNLOCK (conv_maps);
}

/* GList GCompareFunc to sort list by mapping priority */
static gint
priority_GCompareFunc (gconstpointer a, gconstpointer b)
{
  IpatchConverterInfo *mapa = (IpatchConverterInfo *)a, *mapb = (IpatchConverterInfo *)b;

  /* priority sorts from highest to lowest, so subtract a from b */
  return (mapb->priority - mapa->priority);
}

/**
 * ipatch_find_converter:
 * @src_type: #GObject derived source type (0 or G_TYPE_NONE for wildcard - Since 1.1.0)
 * @dest_type: #GObject derived destination type (0 or G_TYPE_NONE for wildcard - Since 1.1.0)
 *
 * Lookup a conversion handler type for a given @src_type to @dest_type
 * conversion.  In some cases there may be multiple conversion handlers for
 * the given types, this function only returns the highest priority type.
 * To get a list of all available converters use ipatch_find_converters().
 *
 * Returns: An #IpatchConverter derived GType of the matching conversion
 *   handler or 0 if no matches.
 */
GType
ipatch_find_converter (GType src_type, GType dest_type)
{
  const IpatchConverterInfo *info;

  g_return_val_if_fail (g_type_is_a (src_type, G_TYPE_OBJECT) || G_TYPE_IS_INTERFACE (src_type), 0);
  g_return_val_if_fail (g_type_is_a (dest_type, G_TYPE_OBJECT) || G_TYPE_IS_INTERFACE (dest_type), 0);

  G_LOCK (conv_maps);
  info = convert_lookup_map_U (NULL, 0, src_type, dest_type, 0);
  G_UNLOCK (conv_maps);

  return (info ? info->conv_type : 0);
}

/**
 * ipatch_find_converters:
 * @src_type: #GObject derived source type (G_TYPE_NONE for wildcard)
 * @dest_type: #GObject derived destination type (G_TYPE_NONE for wildcard)
 * @flags: #IpatchConverterFlags to modify converter matching behavior (logically OR'd with registered converter flags)
 *
 * Lookup conversion handler types matching search criteria.
 *
 * Returns: (array zero-terminated=1) (transfer full) (nullable): 0 terminated array of #IpatchConverter derived GTypes or %NULL
 * if no matching converters.  Array should be freed with g_free() when finished using it.
 *
 * Since: 1.1.0
 */
GType *
ipatch_find_converters (GType src_type, GType dest_type, guint flags)
{
  GType *types_array;

  G_LOCK (conv_maps);
  convert_lookup_map_U (&types_array, 0, src_type, dest_type, flags);
  G_UNLOCK (conv_maps);

  return types_array;
}

/* Lookup a IpatchConverterInfo in the conv_maps list (caller responsible for LOCK of conv_maps).
 * Pass a pointer for "array" if all matching info is desired or NULL to ignore.
 * Use 0 or G_TYPE_NONE for wildcard types.
 * IpatchConverterInfo structures are considered static (never unregistered and don't change).
 */
static const IpatchConverterInfo *
convert_lookup_map_U (GType **array, GType conv_type, GType src_type, GType dest_type, guint flags)
{
  GArray *types_array = NULL;
  IpatchConverterInfo *info;
  GList *p;

  if (array) *array = NULL;
  if (conv_type == G_TYPE_NONE) conv_type = 0;
  if (src_type == G_TYPE_NONE) src_type = 0;
  if (dest_type == G_TYPE_NONE) dest_type = 0;

  for (p = conv_maps; p; p = p->next)
  {
    info = (IpatchConverterInfo *)(p->data);

    if (conv_type && conv_type != info->conv_type)
      continue;

    if (src_type)
    {
      if ((flags | info->flags) & IPATCH_CONVERTER_SRC_DERIVED)         // If derived flag set (from map or caller) and source type is not a descendant of map type, skip
      { // Derived will automatically be set for interface types as well
        if (!g_type_is_a (info->src_type, src_type))
          continue;
      }
      else if (info->src_match) // If parent source match set and type is outside of map type range of src_match <-> src_type, skip
      {
        if (!g_type_is_a (src_type, info->src_match)
            || (src_type != info->src_type && g_type_is_a (src_type, info->src_type)))
          continue;
      }                 // Neither derived or src_map, just do a direct comparison
      else if (src_type != info->src_type)
        continue;
    }

    if (dest_type)
    {
      if ((flags | info->flags) & IPATCH_CONVERTER_DEST_DERIVED)        // If derived flag set and destination type is not a descendant of map type, skip
      { // Derived will automatically be set for interface types as well
        if (!g_type_is_a (info->dest_type, dest_type))
          continue;
      }
      else if (info->dest_match)        // If parent destination match set and type is outside of map type range of dest_match <-> dest_type, skip
      {
        if (!g_type_is_a (dest_type, info->dest_match)
            || (dest_type != info->dest_type && g_type_is_a (dest_type, info->dest_type)))
          continue;
      }                 // Neither derived or dest_map, just do a direct comparison
      else if (dest_type != info->dest_type)
        continue;
    }

    if (!array)         // Not requesting array? Just return highest priority match
      return info;

    // Create types array if not already created
    if (!types_array)
      types_array = g_array_new (TRUE, FALSE, sizeof (GType));

    g_array_append_val (types_array, info->conv_type);          // Append converter type to array
  }

  /* free type array but not the array itself */
  if (types_array)
    *array = (GType *)g_array_free (types_array, FALSE);

  return NULL;
}

/**
 * ipatch_lookup_converter_info:
 * @conv_type: #IpatchConverter derived GType to lookup info on (or 0 for wildcard, or G_TYPE_NONE - since 1.1.0)
 * @src_type: Source type of conversion map to lookup (or 0 for default map, or G_TYPE_NONE - since 1.1.0)
 * @dest_type: Destination type of conversion map (0 if @src_type is 0, or G_TYPE_NONE - since 1.1.0)
 *
 * Lookup converter map info.
 *
 * Returns: (transfer none) (nullable): Converter info structure or %NULL if no match.
 * The returned pointer is internal and should not be modified or freed.
 */
const IpatchConverterInfo *
ipatch_lookup_converter_info (GType conv_type, GType src_type, GType dest_type)
{
  const IpatchConverterInfo *info;

  G_LOCK (conv_maps);
  info = convert_lookup_map_U (NULL, conv_type, src_type, dest_type, 0);
  G_UNLOCK (conv_maps);

  return (info);
}

/**
 * ipatch_get_converter_info:
 * @conv_type: #IpatchConverter derived GType to lookup info on
 *
 * Get converter info structure for a converter type.
 *
 * Returns: (transfer none) (nullable): Converter info structure or %NULL if no match.
 * The returned pointer is internal and should not be modified or freed.
 *
 * Since: 1.1.0
 */
const IpatchConverterInfo *
ipatch_get_converter_info (GType conv_type)
{
  return ipatch_lookup_converter_info (conv_type, 0, 0);
}


GType
ipatch_converter_get_type (void)
{
  static GType obj_type = 0;

  if (!obj_type) {
    static const GTypeInfo obj_info = {
      sizeof (IpatchConverterClass), NULL, NULL,
      (GClassInitFunc)ipatch_converter_class_init, NULL, NULL,
      sizeof (IpatchConverter), 0,
      (GInstanceInitFunc) NULL,
    };

    obj_type = g_type_register_static (G_TYPE_OBJECT, "IpatchConverter",
				       &obj_info, G_TYPE_FLAG_ABSTRACT);
  }

  return (obj_type);
}

static void
ipatch_converter_class_init (IpatchConverterClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  obj_class->set_property = ipatch_converter_set_property;
  obj_class->get_property = ipatch_converter_get_property;
  obj_class->finalize = ipatch_converter_finalize;

  g_object_class_install_property (obj_class, PROP_PROGRESS,
				g_param_spec_float ("progress", _("Progress"),
						    _("Conversion progress"),
						    0.0, 1.0, 0.0,
						    G_PARAM_READWRITE));
}

/* function called when a patch is being destroyed */
static void
ipatch_converter_finalize (GObject *gobject)
{
  IpatchConverter *converter = IPATCH_CONVERTER (gobject);
  GList *p;

  // Call destroy notify function for link function assignment (if set)
  if (converter->notify_func)
    converter->notify_func (converter->user_data);

  p = converter->inputs;
  while (p)
    {
      g_object_unref (p->data);
      p = g_list_delete_link (p, p);
    }

  p = converter->outputs;
  while (p)
    {
      g_object_unref (p->data);
      p = g_list_delete_link (p, p);
    }

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
ipatch_converter_set_property (GObject *object, guint property_id,
			       const GValue *value, GParamSpec *pspec)
{
  IpatchConverter *converter;

  g_return_if_fail (IPATCH_IS_CONVERTER (object));
  converter = IPATCH_CONVERTER (object);

  switch (property_id)
    {
    case PROP_PROGRESS:
      converter->progress = g_value_get_float (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ipatch_converter_get_property (GObject *object, guint property_id,
			       GValue *value, GParamSpec *pspec)
{
  IpatchConverter *converter;

  g_return_if_fail (IPATCH_IS_CONVERTER (object));
  converter = IPATCH_CONVERTER (object);

  switch (property_id)
    {
    case PROP_PROGRESS:
      g_value_set_float (value, converter->progress);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * ipatch_converter_add_input:
 * @converter: Converter instance
 * @object: Object to add
 *
 * Add an input object to a converter object.
 */
void
ipatch_converter_add_input (IpatchConverter *converter, GObject *object)
{
  g_return_if_fail (IPATCH_IS_CONVERTER (converter));
  g_return_if_fail (G_IS_OBJECT (object));

  converter->inputs = g_list_append (converter->inputs, g_object_ref (object));
}

/**
 * ipatch_converter_add_output:
 * @converter: Converter instance
 * @object: Object to add
 *
 * Add an output object to a converter object.
 */
void
ipatch_converter_add_output (IpatchConverter *converter, GObject *object)
{
  g_return_if_fail (IPATCH_IS_CONVERTER (converter));
  g_return_if_fail (G_IS_OBJECT (object));

  converter->outputs = g_list_append (converter->outputs,
				      g_object_ref (object));
}

/**
 * ipatch_converter_add_inputs:
 * @converter: Converter instance
 * @objects: (element-type GObject.Object) (transfer none): List of objects to add
 *
 * Add a list of input objects to a converter object.
 */
void
ipatch_converter_add_inputs (IpatchConverter *converter, GList *objects)
{
  GList *p;

  g_return_if_fail (IPATCH_IS_CONVERTER (converter));
  g_return_if_fail (objects != NULL);

  p = objects;
  while (p)
    {
      converter->inputs = g_list_append (converter->inputs,
					 g_object_ref (p->data));
      p = g_list_next (p);
    }
}

/**
 * ipatch_converter_add_outputs:
 * @converter: Converter instance
 * @objects: (element-type GObject.Object) (transfer none): List of objects to add
 *
 * Add a list of output objects to a converter object.
 */
void
ipatch_converter_add_outputs (IpatchConverter *converter, GList *objects)
{
  GList *p;

  g_return_if_fail (IPATCH_IS_CONVERTER (converter));
  g_return_if_fail (objects != NULL);

  p = objects;
  while (p)
    {
      converter->outputs = g_list_append (converter->outputs,
					  g_object_ref (p->data));
      p = g_list_next (p);
    }
}

/**
 * ipatch_converter_get_input:
 * @converter: Converter instance
 *
 * Get a single input object from a converter.
 *
 * Returns: (transfer full): The first input object from a converter or %NULL if
 *   no input objects. The caller owns a reference to the returned
 *   object.
 */
GObject *
ipatch_converter_get_input (IpatchConverter *converter)
{
  GObject *obj = NULL;

  g_return_val_if_fail (IPATCH_IS_CONVERTER (converter), NULL);

  if (converter->inputs) obj = (GObject *)(converter->inputs->data);
  if (obj) g_object_ref (obj);
  return (obj);
}

/**
 * ipatch_converter_get_output:
 * @converter: Converter instance
 *
 * Get a single output object from a converter.
 *
 * Returns: (transfer full): The first output object from a converter or %NULL if
 *   no output objects. The caller owns a reference to the returned
 *   object.
 */
GObject *
ipatch_converter_get_output (IpatchConverter *converter)
{
  GObject *obj = NULL;

  g_return_val_if_fail (IPATCH_IS_CONVERTER (converter), NULL);

  if (converter->outputs) obj = (GObject *)(converter->outputs->data);
  if (obj) g_object_ref (obj);
  return (obj);
}

/**
 * ipatch_converter_get_inputs: (skip)
 * @converter: Converter instance
 *
 * Get a list of input objects from a converter.
 *
 * Returns: (transfer full): A newly created input object list from a converter or
 *   %NULL if no input objects. The caller owns a reference to the
 *   returned list.
 */
IpatchList *
ipatch_converter_get_inputs (IpatchConverter *converter)
{
  IpatchList *list;
  GList *items;

  items = ipatch_converter_get_inputs_list (converter);
  if (!items) return NULL;

  list = ipatch_list_new ();	/* ++ ref new */
  list->items = items;          // !! list takes over items
  return (list);		/* !! caller takes over list reference */
}

/**
 * ipatch_converter_get_inputs_list: (rename-to ipatch_converter_get_inputs)
 * @converter: Converter instance
 *
 * Get a list of input objects from a converter.
 *
 * Returns: (element-type GObject.Object) (transfer full): A newly created input
 *   object list from a converter or %NULL if no input objects.
 *   Free the list with ipatch_glist_unref_free() when finished using it.
 *
 * Since: 1.1.0
 */
GList *
ipatch_converter_get_inputs_list (IpatchConverter *converter)
{
  GList *items = NULL, *p;

  g_return_val_if_fail (IPATCH_IS_CONVERTER (converter), NULL);

  if (!converter->inputs) return (NULL);

  for (p = converter->inputs; p; p = g_list_next (p))
    items = g_list_prepend (items, g_object_ref (p->data));

  return g_list_reverse (items);        // !! caller takes over list
}

/**
 * ipatch_converter_get_outputs: (skip)
 * @converter: Converter instance
 *
 * Get a list of output objects from a converter.
 *
 * Returns: (transfer full): A newly created output object list from a converter or
 *   %NULL if no output objects. The caller owns a reference to the
 *   returned list.
 */
IpatchList *
ipatch_converter_get_outputs (IpatchConverter *converter)
{
  IpatchList *list;
  GList *items;

  items = ipatch_converter_get_outputs_list (converter);
  if (!items) return NULL;

  list = ipatch_list_new ();	/* ++ ref new */
  list->items = items;          // !! list takes over items
  return (list);		/* !! caller takes over list reference */
}

/**
 * ipatch_converter_get_outputs_list: (rename-to ipatch_converter_get_outputs)
 * @converter: Converter instance
 *
 * Get a list of output objects from a converter.
 *
 * Returns: (element-type GObject.Object) (transfer full): A newly created output
 *   object list from a converter or %NULL if no output objects.
 *   Free the list with ipatch_glist_unref_free() when finished using it.
 *
 * Since: 1.1.0
 */
GList *
ipatch_converter_get_outputs_list (IpatchConverter *converter)
{
  GList *items = NULL, *p;

  g_return_val_if_fail (IPATCH_IS_CONVERTER (converter), NULL);

  if (!converter->outputs) return (NULL);

  for (p = converter->outputs; p; p = g_list_next (p))
    items = g_list_prepend (items, g_object_ref (p->data));

  return g_list_reverse (items);        // !! caller takes over list
}

/**
 * ipatch_converter_verify:
 * @converter: Converter object
 * @failmsg: (out) (transfer full): Location to store a failure message if @converter fails
 *   verification. The stored message should be freed when no longer needed.
 *
 * Verifies the settings of a converter object. This is automatically called
 * before a conversion is done, so it doesn't usually need to be explicitly
 * called.
 *
 * Returns: %TRUE if @converter passed verification, %FALSE otherwise in which
 *   case an error message may be stored in @failmsg. Remember to free the
 *   message when finished with it.
 */
gboolean
ipatch_converter_verify (IpatchConverter *converter, char **failmsg)
{
  IpatchConverterClass *klass;
  const IpatchConverterInfo *info;
  char *msg = NULL;
  gboolean retval;
  GType type;
  int count;
  GList *p;

  g_return_val_if_fail (IPATCH_IS_CONVERTER (converter), FALSE);

  klass = IPATCH_CONVERTER_GET_CLASS (converter);

  // Verify method set? - use it
  if (klass->verify)
  {
    retval = (klass->verify)(converter, &msg);

    if (failmsg) *failmsg = msg;
    else g_free (msg);

    return (retval);
  }

  // No verify method, check input/output types and counts
  info = ipatch_lookup_converter_info (G_OBJECT_TYPE (converter), 0, 0);

  if (info->src_count == 0 && converter->inputs)
    goto input_failed;

  for (p = converter->inputs, count = 0; p; p = p->next, count++)
  {
    type = G_OBJECT_TYPE (p->data);

    if (info->flags & IPATCH_CONVERTER_SRC_DERIVED)     // If derived flag set and source type is not a descendant of map type, fail
    {
      if (!g_type_is_a (info->src_type, type))
        goto input_failed;
    }
    else if (info->src_match)   // If parent source match set and type is outside of map type range of src_match <-> src_type, fail
    {
      if (!g_type_is_a (type, info->src_match)
          || (type != info->src_type && g_type_is_a (type, info->src_type)))
        goto input_failed;
    }                           // Neither derived or src_map, just do a direct type comparison, if not equal, fail
    else if (type != info->src_type)
      goto input_failed;
  }

  if (info->src_count == IPATCH_CONVERTER_COUNT_ONE_OR_MORE)
  {
    if (count < 1)
      goto input_failed;
  }
  else if (info->src_count != IPATCH_CONVERTER_COUNT_ZERO_OR_MORE && count != info->src_count)
    goto input_failed;


  for (p = converter->outputs, count = 0; p; p = p->next, count++)
  {
    type = G_OBJECT_TYPE (p->data);

    if (info->flags & IPATCH_CONVERTER_DEST_DERIVED)    // If derived flag set and dest type is not a descendant of map type, fail
    {
      if (!g_type_is_a (info->dest_type, type))
        goto input_failed;
    }
    else if (info->dest_match)          // If parent dest match set and type is outside of map type range of dest_match <-> dest_type, fail
    {
      if (!g_type_is_a (type, info->dest_match)
          || (type != info->dest_type && g_type_is_a (type, info->dest_type)))
        goto input_failed;
    }                                   // Neither derived or dest_map, just do a direct type comparison, if not equal, fail
    else if (type != info->dest_type)
      goto input_failed;
  }

  if (info->dest_count == IPATCH_CONVERTER_COUNT_ONE_OR_MORE)
  {
    if (count < 1)
      goto output_failed;
  }
  else if (info->dest_count != IPATCH_CONVERTER_COUNT_ZERO_OR_MORE && count != info->dest_count)
    goto output_failed;

  return TRUE;

input_failed:
  if (failmsg) *failmsg = g_strdup ("Converter inputs failed to verify");
  return (FALSE);

output_failed:
  if (failmsg) *failmsg = g_strdup ("Converter outputs failed to verify");
  return (FALSE);
}

/**
 * ipatch_converter_init:
 * @converter: Converter object
 *
 * Allows a converter type to initialize its parameters based on its input
 * and/or output objects. This function should be called after setting the
 * input and output objects, but before setting object parameters or
 * converting. Calling this function is not required, but certain converters
 * will work more intuitively if it is (an example is an audio sample saver
 * converter which could initialize the output sample file object format
 * based on the input sample object format).
 *
 * NOTE: Verification of converter parameters is not done by this routine
 * so converter types implementing an init method are responsible for their
 * own verification.
 */
void
ipatch_converter_init (IpatchConverter *converter)
{
  IpatchConverterClass *klass;

  g_return_if_fail (IPATCH_IS_CONVERTER (converter));

  klass = IPATCH_CONVERTER_GET_CLASS (converter);
  if (!klass->init) return;

  (klass->init)(converter);
}

/**
 * ipatch_converter_convert:
 * @converter: Converter object
 * @err: Location to store error info or %NULL
 *
 * Runs the conversion method of a converter object. The @converter object's
 * conversion paramters are first verified before the conversion is run.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
ipatch_converter_convert (IpatchConverter *converter, GError **err)
{
  IpatchConverterClass *klass;
  char *failmsg = NULL;

  g_return_val_if_fail (IPATCH_IS_CONVERTER (converter), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  klass = IPATCH_CONVERTER_GET_CLASS (converter);
  g_return_val_if_fail (klass->convert != NULL, FALSE);

  if (!ipatch_converter_verify (converter, &failmsg))
    {
      g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_INVALID,
		   _("Verification of conversion parameters failed: %s"),
		   failmsg ? failmsg : _("<No detailed error message>"));
      return (FALSE);
    }

  return ((klass->convert)(converter, err));
}

/**
 * ipatch_converter_reset:
 * @converter: Converter object
 * 
 * Reset a converter object so it can be re-used.
 */
void
ipatch_converter_reset (IpatchConverter *converter)
{
  GList *p;

  g_return_if_fail (IPATCH_IS_CONVERTER (converter));

  p = converter->inputs;
  while (p)
    {
      g_object_unref (p->data);
      p = g_list_delete_link (p, p);
    }
  converter->inputs = NULL;

  p = converter->outputs;
  while (p)
    {
      g_object_unref (p->data);
      p = g_list_delete_link (p, p);
    }
  converter->outputs = NULL;

  p = converter->log;
  while (p)
    {
      g_free (p->data);
      p = g_list_delete_link (p, p);
    }
  converter->log = NULL;

  converter->min_rate = 0.0;
  converter->max_rate = 0.0;
  converter->avg_rate = 0.0;
  converter->sum_rate = 0.0;
  converter->item_count = 0;
}

/**
 * ipatch_converter_get_notes:
 * @converter: Converter object
 *
 * Get notes about a conversion implementation. These notes could include
 * things such as information about any loss of information in the conversion
 * that may occur, etc.
 *
 * Returns: Newly allocated and possibly multi-line notes and comments
 *   about a given conversion or %NULL if no notes. Meant for display to
 *   the user. This string should be freed when no longer needed.
 */
char *
ipatch_converter_get_notes (IpatchConverter *converter)
{
  IpatchConverterClass *klass;

  g_return_val_if_fail (IPATCH_IS_CONVERTER (converter), NULL);

  klass = IPATCH_CONVERTER_GET_CLASS (converter);
  if (!klass->notes) return (NULL);

  return ((klass->notes)(converter));
}

/* FIXME-GIR: @type consists of multiple flags types?  @msg can be static or dynamic. */

/**
 * ipatch_converter_log: (skip)
 * @converter: Converter object
 * @item: (nullable): Item the log entry pertains to or %NULL if not item specific
 * @type: #IpatchConverterLogType and other flags
 * @msg: Message of the log. If message is dynamically allocated then
 *   the #IPATCH_CONVERTER_LOG_MSG_ALLOC flag should be set in @type
 *
 * Logs an entry to a converter log. Usually only used by converter
 * object handlers.
 */
void
ipatch_converter_log (IpatchConverter *converter, GObject *item,
		      int type, char *msg)
{
  LogEntry *entry;

  g_return_if_fail (IPATCH_IS_CONVERTER (converter));
  g_return_if_fail (!item || G_IS_OBJECT (item));
  g_return_if_fail (msg != NULL);

  entry = g_new0 (LogEntry, 1);
  if (item) entry->item = g_object_ref (item);
  entry->type = type;
  entry->data.msg = msg;

  converter->log = g_list_prepend (converter->log, entry);
}

/* FIXME-GIR: @type consists of multiple flags types? */

/**
 * ipatch_converter_log_printf:
 * @converter: Converter object
 * @item: (nullable): Item the log entry pertains to or %NULL if not item specific
 * @type: #IpatchConverterLogType and other flags
 * @fmt: Printf format style string
 * @...: Arguments to @fmt message string
 *
 * Logs a printf style message to a converter log. Usually only used by converter
 * object handlers.  The #IPATCH_CONVERTER_LOG_MSG_ALLOC flag is automatically
 * set on the log entry, since it is dynamically allocated.
 */
void
ipatch_converter_log_printf (IpatchConverter *converter, GObject *item,
			     int type, const char *fmt, ...)
{
  LogEntry *entry;
  va_list args;

  g_return_if_fail (IPATCH_IS_CONVERTER (converter));
  g_return_if_fail (!item || G_IS_OBJECT (item));
  g_return_if_fail (fmt != NULL);

  entry = g_new0 (LogEntry, 1);
  if (item) entry->item = g_object_ref (item);
  entry->type = type | IPATCH_CONVERTER_LOG_MSG_ALLOC;

  va_start (args, fmt);
  entry->data.msg = g_strdup_vprintf (fmt, args);
  va_end (args);

  converter->log = g_list_prepend (converter->log, entry);
}

/**
 * ipatch_converter_log_next:
 * @converter: Converter object
 * @pos: (out): Opaque current position in log, should be %NULL on first call to
 *   this function to return first log item (oldest item)
 * @item: (out) (transfer none) (optional): Location to store item of the log entry or %NULL,
 *   no reference is added so the item is only guarenteed to exist for as long as the
 *   @converter does
 * @type: (out) (optional): Location to store the type parameter of the log entry or %NULL
 * @msg: (out) (transfer none): Location to store the message of the log entry or %NULL, message
 *   is internal and should not be messed with and is only guarenteed for
 *   the lifetime of the @converter
 *
 * Get the first or next log entry from a converter object.
 *
 * Returns: %TRUE if an entry was returned, %FALSE if no more entries in which
 *   case item, type and msg are all undefined.
 */
gboolean
ipatch_converter_log_next (IpatchConverter *converter, gpointer *pos,
			   GObject **item, int *type, char **msg)
{
  LogEntry *entry;
  GList *p;

  g_return_val_if_fail (IPATCH_IS_CONVERTER (converter), FALSE);
  g_return_val_if_fail (pos != NULL, FALSE);

  if (!*pos) p = g_list_last (converter->log);
  else p = g_list_previous ((GList *)(*pos));

  if (!p) return (FALSE);

  entry = (LogEntry *)(p->data);
  if (item) *item = entry->item;
  if (type) *type = entry->type;
  if (msg) *msg = entry->data.msg;

  return (TRUE);
}

/**
 * ipatch_converter_set_link_funcs: (skip)
 * @converter: Converter object
 * @link_lookup: Set the link lookup callback function
 * @link_notify: Set the link notify callback function
 *
 * This function allows for object link interception by the user of
 * an #IpatchConverter instance.  The callback functions are used by
 * conversion processes which create objects linking other external
 * objects which need to be converted.  For each link object needing
 * conversion @link_lookup will be called.  If @link_lookup returns a valid
 * pointer it is used as the converted link object, if %NULL is returned then
 * the link will be converted and @link_notify will be called with the new
 * converted item.  An example usage of this feature is
 * the #IpatchPaste system, which does object conversions and substitutes
 * already converted objects (a conversion pool).
 */
void
ipatch_converter_set_link_funcs (IpatchConverter *converter,
				 IpatchConverterLinkLookupFunc *link_lookup,
				 IpatchConverterLinkNotifyFunc *link_notify)
{
  g_return_if_fail (IPATCH_IS_CONVERTER (converter));
  ipatch_converter_set_link_funcs_full (converter, link_lookup, link_notify, NULL, NULL);
}

/**
 * ipatch_converter_set_link_funcs_full: (rename-to ipatch_converter_set_link_funcs)
 * @converter: Converter object
 * @link_lookup: (scope notified) (nullable): Set the link lookup callback function
 * @link_notify: (scope notified) (nullable): Set the link notify callback function
 * @notify_func: (scope async) (closure user_data) (nullable): Callback which gets
 *   called when link functions are removed.
 * @user_data: (nullable): User data passed to @notify_func (not @link_lookup or @link_notify)
 *
 * This function allows for object link interception by the user of
 * an #IpatchConverter instance.  The callback functions are used by
 * conversion processes which create objects linking other external
 * objects which need to be converted.  For each link object needing
 * conversion @link_lookup will be called.  If @link_lookup returns a valid
 * pointer it is used as the converted link object, if %NULL is returned then
 * the link will be converted and @link_notify will be called with the new
 * converted item.  An example usage of this feature is
 * the #IpatchPaste system, which does object conversions and substitutes
 * already converted objects (a conversion pool).
 *
 * Since: 1.1.0
 */
void
ipatch_converter_set_link_funcs_full (IpatchConverter *converter,
                                      IpatchConverterLinkLookupFunc *link_lookup,
                                      IpatchConverterLinkNotifyFunc *link_notify,
                                      GDestroyNotify notify_func, gpointer user_data)
{
  g_return_if_fail (IPATCH_IS_CONVERTER (converter));

  if (converter->notify_func)
    converter->notify_func (converter->user_data);

  converter->link_lookup = link_lookup;
  converter->link_notify = link_notify;
  converter->notify_func = notify_func;
  converter->user_data = user_data;
}
