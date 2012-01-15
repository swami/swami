/*
 * libInstPatch
 * Copyright (C) 1999-2010 Joshua "Element" Green <jgreen@users.sourceforge.net>
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
static IpatchConverterInfo *convert_lookup_map_U (IpatchConverterInfo ***array,
						  GType conv_type,
						  GType src_type,
						  GType dest_type);
static IpatchConverterInfo *
find_match_from_lists (GSList *src_list, GSList *dest_list, GType conv_type);
static void ipatch_converter_class_init (IpatchConverterClass *klass);
static void ipatch_converter_finalize (GObject *gobject);
static void ipatch_converter_set_property (GObject *object, guint property_id,
					   const GValue *value,
					   GParamSpec *pspec);
static void ipatch_converter_get_property (GObject *object, guint property_id,
					   GValue *value, GParamSpec *pspec);
static gboolean ipatch_converter_test_object_list (GList *list, GType type,
						   int count);

/* lock used for list and hash tables */
G_LOCK_DEFINE_STATIC (conv_maps);
static GList *conv_maps = NULL;	/* list of all registered IpatchConverterInfo */

/* hash of source and destination type to info lists.
 * type -> GSList (IpatchConverterInfo) */
static GHashTable *conv_src_types = NULL;	/* source type matching lists */
static GHashTable *conv_derived_types = NULL; /* derived src type match lists */
static GHashTable *conv_dest_types = NULL;	/* dest type matching lists */

/* conv_type -> GSList (IpatchConverterInfo) */
static GHashTable *conv_types = NULL;

static gpointer parent_class = NULL;


/* converter system init function */
void
_ipatch_converter_init (void)
{
  conv_src_types = g_hash_table_new (NULL, NULL);
  conv_derived_types = g_hash_table_new (NULL, NULL);
  conv_dest_types = g_hash_table_new (NULL, NULL);
  conv_types = g_hash_table_new (NULL, NULL);
}

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

  if (!ipatch_converter_convert (conv, err))
    {
      g_object_unref (conv);
      return (FALSE);
    }

  g_object_unref (conv);

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
 * Returns: The output object or %NULL on error (in which case @err may be set).
 * The returned object has a refcount of 1 which the caller owns.
 */
GObject *
ipatch_convert_object_to_type (GObject *object, GType type, GError **err)
{
  IpatchConverterInfo *info;
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
 * ipatch_convert_object_to_type_multi:
 * @object: Object to convert from
 * @type: Type of object to convert to
 * @err: Location to store error info or %NULL to ignore
 *
 * A convenience function to convert an object to one or more objects of a given
 * @type.  This function will work for converters which require 1
 * input and any number of outputs.
 *
 * Returns: List of output objects or %NULL on error (in which case @err may be set).
 * The returned object list has a refcount of 1 which the caller owns.
 */
IpatchList *
ipatch_convert_object_to_type_multi (GObject *object, GType type, GError **err)
{
  return (ipatch_convert_object_to_type_multi_set (object, type, err, NULL));
}

/**
 * ipatch_convert_object_to_type_multi_set:
 * @object: Object to convert from
 * @type: Type of object to convert to
 * @err: Location to store error info or %NULL to ignore
 * @first_property_name: Name of first property to assign or %NULL
 * @...: First property value followed by property name/value pairs (as per
 *   g_object_set()) to assign to the resulting converter, terminated with a
 *   %NULL property name.
 *
 * A convenience function to convert an object to one or more objects of a given
 * @type.  This function will work for converters which require 1
 * input and any number of outputs.  Like ipatch_convert_object_to_type_multi()
 * but allows for properties of the converter to be assigned.
 *
 * Returns: List of output objects or %NULL on error (in which case @err may be set).
 * The returned object list has a refcount of 1 which the caller owns.
 */
IpatchList *
ipatch_convert_object_to_type_multi_set (GObject *object, GType type, GError **err,
                                         const char *first_property_name, ...)
{
  IpatchConverterInfo *info;
  IpatchConverter *conv;
  GObject *output = NULL;
  IpatchList *list;
  GType convtype;
  va_list args;
  int i;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (!err || !*err, NULL);

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

  conv = IPATCH_CONVERTER (g_object_new (convtype, NULL));	/* ++ ref */

  ipatch_converter_add_input (conv, object);

  if (info->dest_count > 0)	/* if outputs are expected, create them */
  {
    for (i = 0; i < info->dest_count; i++)
    {
      output = g_object_new (type, NULL);	/* ++ ref */
      ipatch_converter_add_output (conv, output);
      g_object_unref (output);  /* -- unref */
    }
  }

  if (first_property_name)
  {
    va_start (args, first_property_name);
    g_object_set_valist ((GObject *)conv, first_property_name, args);
    va_end (args);
  }

  if (!ipatch_converter_convert (conv, err))	/* do the conversion */
  {
    if (output) g_object_unref (output);
    g_object_unref (conv);
    return (NULL);
  }

  list = ipatch_converter_get_outputs (conv);     /* ++ ref object list */

  g_object_unref (conv);        /* -- unref converter */

  return (list);	/* !! caller takes over reference */
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
 * Returns: The new converter object with a reference count of 1 which the
 *   caller owns, or %NULL if there is no matching conversion handler type.
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
 * ipatch_register_converter_map:
 * @conv_type: #IpatchConverter derived GType of conversion handler
 * @flags: #IpatchConverterFlags logically ORed with a priority (number from
 *    0 to 100, 0 will use the default).  #IpatchConverterPriority defines some
 *    common priorities.  Used for overriding other converters for specific types.
 *    If #IPATCH_CONVERTER_FLAG_SRC_DERIVED is specified then types which are
 *    derived (children) of @src_type will also match.
 * @src_type: Type for source object (GObject derived type or interface type).
 * @src_match: The furthest parent type of @src_type to match (all types in
 *   between are also matched, 0 to match only @src_type).
 * @src_count: Required number of source items (can also be
 *   an #IpatchConverterCount value)
 * @dest_type: Type for destination object (GObject derived type or interface type).
 * @dest_match: The furthest parent type of @dest_type to match (all types in
 *   between are also matched, 0 to match only @dest_type).
 * @dest_count: Required number of destination items (can also be
 *   an #IpatchConverterCount value).  This value can be 0 in the case where
 *   no objects should be supplied, but will be instead assigned after
 *   conversion.
 *
 * Registers a #IpatchConverter handler to convert objects of @src_type
 * to @dest_type.
 */
void
ipatch_register_converter_map (GType conv_type, guint flags,
			       GType src_type, GType src_match, gint8 src_count,
			       GType dest_type, GType dest_match, gint8 dest_count)
{
  IpatchConverterInfo *map;
  GSList *list, *p;
  GType type;
  guint8 priority;

  g_return_if_fail (g_type_is_a (conv_type, IPATCH_TYPE_CONVERTER));
  g_return_if_fail (g_type_is_a (src_type, G_TYPE_OBJECT) || G_TYPE_IS_INTERFACE (src_type));
  g_return_if_fail (g_type_is_a (dest_type, G_TYPE_OBJECT) || G_TYPE_IS_INTERFACE (dest_type));

  g_return_if_fail (!src_match || g_type_is_a (src_type, src_match));
  g_return_if_fail (!dest_match || g_type_is_a (dest_type, dest_match));

  priority = flags & 0xFF;
  if (priority == 0) priority = IPATCH_CONVERTER_PRIORITY_DEFAULT;

  map = g_new (IpatchConverterInfo, 1);
  map->conv_type = conv_type;
  map->flags = flags >> 8;
  map->priority = priority;
  map->src_type = src_type;
  map->src_match = src_match;
  map->src_count = src_count;
  map->dest_type = dest_type;
  map->dest_match = dest_match;
  map->dest_count = dest_count;

  G_LOCK (conv_maps);

  conv_maps = g_list_insert_sorted (conv_maps, map, priority_GCompareFunc);

  /* hash all source matching types to new map */
  for (type = src_type; type; type = g_type_parent (type))
    { /* lookup the list of maps for this source type */
      list = g_hash_table_lookup (conv_src_types, GUINT_TO_POINTER (type));

      /* insert the new map sorted by priority */
      p = g_slist_insert_sorted (list, map, priority_GCompareFunc);

      if (p != list)	/* update hash list pointer if head item changed */
	g_hash_table_insert (conv_src_types, GUINT_TO_POINTER (type), p);

      if (!src_match || type == src_match) break;
    }

  /* hash all dest matching types to new map */
  for (type = dest_type; type; type = g_type_parent (type))
    { /* lookup the list of maps for this dest type */
      list = g_hash_table_lookup (conv_dest_types, GUINT_TO_POINTER (type));

      /* insert the new map sorted by priority */
      p = g_slist_insert_sorted (list, map, priority_GCompareFunc);

      if (p != list)	/* update hash list pointer if head item changed */
	g_hash_table_insert (conv_dest_types, GUINT_TO_POINTER (type), p);

      if (!dest_match || type == dest_match) break;
    }

  /* add to conv_derived_types hash if child derived types flag specified */
  if (flags & IPATCH_CONVERTER_FLAG_SRC_DERIVED)
    { /* lookup any existing type list for this src_type */
      list = g_hash_table_lookup (conv_derived_types,
				  GUINT_TO_POINTER (src_type));

      /* insert the map into the list sorted by priority */
      p = g_slist_insert_sorted (list, map, priority_GCompareFunc);

      if (p != list)	/* update hash list pointer if head item changed */
	g_hash_table_insert (conv_derived_types, GUINT_TO_POINTER (src_type), p);
    }

  /* add info to converter type hash */
  list = g_hash_table_lookup (conv_types, GUINT_TO_POINTER (conv_type));

  /* insert sorted by priority */
  p = g_slist_insert_sorted (list, map, priority_GCompareFunc);

  if (p != list)	/* update hash list pointer if head item changed */
    g_hash_table_insert (conv_types, GUINT_TO_POINTER (conv_type), p);

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
 * @src_type: #GObject derived source type
 * @dest_type: #GObject derived destination type
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
  IpatchConverterInfo *info;

  g_return_val_if_fail (g_type_is_a (src_type, G_TYPE_OBJECT) || G_TYPE_IS_INTERFACE (src_type), 0);
  g_return_val_if_fail (g_type_is_a (dest_type, G_TYPE_OBJECT) || G_TYPE_IS_INTERFACE (dest_type), 0);

  G_LOCK (conv_maps);
  info = convert_lookup_map_U (NULL, 0, src_type, dest_type);
  G_UNLOCK (conv_maps);

  return (info ? info->conv_type : 0);
}

/**
 * ipatch_find_converters:
 * @src_type: #GObject derived source type
 * @dest_type: #GObject derived destination type
 *
 * Like ipatch_find_converter() but returns all matching converter types.
 *
 * Returns: 0 terminated array of #IpatchConverter derived GTypes or %NULL
 * if no matching converters.  Array should be freed when finished using it.
 */
GType *
ipatch_find_converters (GType src_type, GType dest_type)
{
  IpatchConverterInfo **info_array;
  GType *type_array;
  int count, i;

  g_array_new (TRUE, FALSE, sizeof (GType));

  G_LOCK (conv_maps);
  convert_lookup_map_U (&info_array, 0, src_type, dest_type);
  G_UNLOCK (conv_maps);

  if (!info_array) return (NULL);	/* no matching converters? */

  for (count = 0; info_array[count]; count++);	/* count them */
  type_array = g_new (GType, count + 1);	/* alloc type array */

  for (i = 0; i < count; i++)	/* copy converter types */
    type_array[i] = info_array[i]->conv_type;

  type_array[count] = 0;	/* 0 terminate */
  g_free (info_array);		/* free info array */

  return (type_array);
}

/* Lookup a IpatchConverterInfo in the conv_maps list (not LOCKED!).
 * Pass a pointer for "array" if all matching info is desired or NULL to ignore.
 * Use 0 for conv_type for wild card. */
static IpatchConverterInfo *
convert_lookup_map_U (IpatchConverterInfo ***array, GType conv_type,
		      GType src_type, GType dest_type)
{
  GArray *info_array = NULL;
  IpatchConverterInfo *info;
  GSList *src_lists = NULL, *dest_lists = NULL;	/* list of type lists to test */
  GSList *sp, *dp;
  GType *ifaces, type;
  GSList *p;
  int i;

  if (array) *array = NULL;

  g_return_val_if_fail (conv_type != 0 || (src_type && dest_type), NULL);

  /* src_type or dest_type not specified but conv_type is? */
  if (!src_type || !dest_type)
    { /* lookup map info list for this converter type */
      sp = g_hash_table_lookup (conv_types, GUINT_TO_POINTER (conv_type));

      if (!sp) return (NULL);

      /* don't want all info?  - Return highest priority mapping */
      if (!array) return ((IpatchConverterInfo *)(sp->data));

      /* create array for all mappings for this converter type */
      info_array = g_array_new (TRUE, FALSE, sizeof (IpatchConverterInfo *));

      /* add all the mappings to the array */
      for (p = sp; p; p = p->next)
	{
	  info = (IpatchConverterInfo *)(p->data);
	  g_array_append_val (info_array, info);
	}

      *array = (IpatchConverterInfo **)g_array_free (info_array, FALSE);
      return ((*array)[0]);
    }

  /* see if source type conforms to any hashed interface types */
  ifaces = g_type_interfaces (src_type, NULL);
  if (ifaces)
    {
      for (i = 0; ifaces[i]; i++)
	{
	  type = ifaces[i];
	  p = g_hash_table_lookup (conv_src_types, GUINT_TO_POINTER (type));
	  if (p) src_lists = g_slist_prepend (src_lists, p);  /* add list to list */
	}
      g_free (ifaces);
    }

  /* check conv_derived_types for any matches in src_type's ancestry */
  for (type = g_type_parent (src_type); type; type = g_type_parent (type))
    {
      p = g_hash_table_lookup (conv_derived_types, GUINT_TO_POINTER (type));
      if (p)
	{
	  src_lists = g_slist_prepend (src_lists, p);
	  break;
	}
    }

  /* add the src_type converter info list if any */
  p = g_hash_table_lookup (conv_src_types, GUINT_TO_POINTER (src_type));
  if (p) src_lists = g_slist_prepend (src_lists, p);

  /* we prepend and then reverse for added performance */
  src_lists = g_slist_reverse (src_lists);

  /* see if dest type conforms to any hashed interface types */
  ifaces = g_type_interfaces (dest_type, NULL);
  if (ifaces)
    {
      for (i = 0; ifaces[i]; i++)
	{
	  type = ifaces[i];
	  p = g_hash_table_lookup (conv_dest_types, GUINT_TO_POINTER (type));
	  if (p) dest_lists = g_slist_prepend (dest_lists, p);  /* add list to list */
	}
      g_free (ifaces);
    }

  /* add the dest_type converter info list if any */
  p = g_hash_table_lookup (conv_dest_types, GUINT_TO_POINTER (dest_type));
  if (p) dest_lists = g_slist_prepend (dest_lists, p);

  /* we prepend and then reverse for added performance */
  dest_lists = g_slist_reverse (dest_lists);

  /* look for match amongst all combinations of types */
  for (sp = src_lists; sp; sp = sp->next)
    {
      for (dp = dest_lists; dp; dp = dp->next)
	{
	  info = find_match_from_lists ((GSList *)(sp->data),
					(GSList *)(dp->data), conv_type);
	  if (info)
	    {
	      if (!array)
		{
		  g_slist_free (src_lists);
		  g_slist_free (dest_lists);
		  return (info);
		}

	      if (!info_array)
		info_array = g_array_new (TRUE, FALSE,
					  sizeof (IpatchConverterInfo *));

	      for (i = 0; i < info_array->len; i++)	/* already in array? */
		if (((IpatchConverterInfo **)(info_array->data))[i] == info)
		  break;

	      if (i < info_array->len)	/* only add if not already in array */
		g_array_append_val (info_array, info);
	    }
	}
    }

  if (src_lists) g_slist_free (src_lists);
  if (dest_lists) g_slist_free (dest_lists);

  /* free type array but not the array itself */
  if (info_array)
    {
      *array = (IpatchConverterInfo **)g_array_free (info_array, FALSE);
      return ((*array)[0]);
    }

  return (NULL);
}

/* finds the highest priority IpatchConverterInfo that is present in both
 * src_list and dest_list, if conv_type is not 0 then it must also match. */
static IpatchConverterInfo *
find_match_from_lists (GSList *src_list, GSList *dest_list, GType conv_type)
{
  IpatchConverterInfo *sinfo, *dinfo;
  GSList *sp, *dp;

  if (!src_list || !dest_list) return (NULL);

  for (sp = src_list; sp; sp = sp->next)	/* loop over source list */
    {
      sinfo = (IpatchConverterInfo *)(sp->data);
      if (conv_type && sinfo->conv_type != conv_type) continue;

      /* loop on dest list until the priority is less than source priority
	 (lists are sorted by priority) */
      for (dp = dest_list; dp; dp = dp->next)
	{
	  dinfo = (IpatchConverterInfo *)(dp->data);

	  if (dinfo->priority < sinfo->priority) break;

	  if (sinfo == dinfo) return (sinfo);	/* match found? */
	}
    }

  return (NULL);
}

/**
 * ipatch_lookup_converter_info:
 * @conv_type: #IpatchConverter derived GType to lookup info on (or 0 for wildcard)
 * @src_type: Source type of conversion map to lookup (or 0 for default map)
 * @dest_type: Destination type of conversion map (0 if @src_type is 0)
 *
 * Lookup converter map info.
 *
 * Returns: Converter info structure or %NULL if no match.  The returned
 * pointer is internal and should not be modified or freed.
 */
IpatchConverterInfo *
ipatch_lookup_converter_info (GType conv_type, GType src_type, GType dest_type)
{
  IpatchConverterInfo *info;

  G_LOCK (conv_maps);
  info = convert_lookup_map_U (NULL, conv_type, src_type, dest_type);
  G_UNLOCK (conv_maps);

  return (info);
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
 * @objects: (element-type GObject): List of objects to add
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
 * @objects: (element-type GObject): List of objects to add
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
 * Returns: The first input object from a converter or %NULL if
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
 * Returns: The first output object from a converter or %NULL if
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
 * ipatch_converter_get_inputs:
 * @converter: Converter instance
 *
 * Get a list of input objects from a converter.
 *
 * Returns: A newly created input object list from a converter or
 *   %NULL if no input objects. The caller owns a reference to the
 *   returned list.
 */
IpatchList *
ipatch_converter_get_inputs (IpatchConverter *converter)
{
  IpatchList *list;
  GList *p;

  g_return_val_if_fail (IPATCH_IS_CONVERTER (converter), NULL);

  if (!converter->inputs) return (NULL);

  list = ipatch_list_new ();	/* ++ ref new */

  p = converter->inputs;
  while (p)
    {
      list->items = g_list_prepend (list->items, g_object_ref (p->data));
      p = g_list_next (p);
    }

  list->items = g_list_reverse (list->items);
  return (list);		/* !! caller takes over list reference */
}

/**
 * ipatch_converter_get_outputs:
 * @converter: Converter instance
 *
 * Get a list of output objects from a converter.
 *
 * Returns: A newly created output object list from a converter or
 *   %NULL if no output objects. The caller owns a reference to the
 *   returned list.
 */
IpatchList *
ipatch_converter_get_outputs (IpatchConverter *converter)
{
  IpatchList *list;
  GList *p;

  g_return_val_if_fail (IPATCH_IS_CONVERTER (converter), NULL);

  if (!converter->outputs) return (NULL);

  list = ipatch_list_new ();	/* ++ ref new */

  p = converter->outputs;
  while (p)
    {
      list->items = g_list_prepend (list->items, g_object_ref (p->data));
      p = g_list_next (p);
    }

  list->items = g_list_reverse (list->items);
  return (list);		/* !! caller takes over list reference */
}

/**
 * ipatch_converter_verify:
 * @converter: Converter object
 * @failmsg: Location to store a failure message if @converter fails
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
  IpatchConverterInfo *info;
  char *msg = NULL;
  gboolean retval;
  GSList *list, *p;

  g_return_val_if_fail (IPATCH_IS_CONVERTER (converter), FALSE);

  klass = IPATCH_CONVERTER_GET_CLASS (converter);

  /* if no verify method, check input/output types and counts */
  if (!klass->verify)
    {
      G_LOCK (conv_maps);
      list = g_hash_table_lookup (conv_types,
				  GUINT_TO_POINTER (G_OBJECT_TYPE (converter)));
      list = g_slist_copy (list);  /* duplicate so we can use outside of lock */
      G_UNLOCK (conv_maps);

      for (p = list; p; p = p->next)
	{
	  info = (IpatchConverterInfo *)(p->data);

	  if (ipatch_converter_test_object_list (converter->inputs,
					    info->src_type, info->src_count)
	      && ipatch_converter_test_object_list (converter->outputs,
					    info->dest_type, info->dest_count))
	    break;
	}

      g_slist_free (list);

      if (!p)	/* found a map which is valid for the converter object lists */
	{
	  if (failmsg)
	    *failmsg = g_strdup ("Converter objects failed verify");

	  return (FALSE);
	}
      else return (TRUE);
    }
  else
    {
      retval = (klass->verify)(converter, &msg);

      if (failmsg) *failmsg = msg;
      else g_free (msg);

      return (retval);
    }

  return (TRUE);
}

/* verifies that a GList of objects matches the @type and @count criteria. */
static gboolean
ipatch_converter_test_object_list (GList *list, GType type, int count)
{
  GList *p;
  int i;

  switch (count)
    {
    case 1:
      if (!list || list->next || !G_TYPE_CHECK_INSTANCE_TYPE (list->data, type))
	return (FALSE);
      break;
    case 0:
      if (list) return (FALSE);
      break;
    case IPATCH_CONVERTER_COUNT_ONE_OR_MORE:
      if (!list) return (FALSE);

      /* fall through */
    case IPATCH_CONVERTER_COUNT_ZERO_OR_MORE:
      for (p = list; p; p = p->next)
	if (!G_TYPE_CHECK_INSTANCE_TYPE (p->data, type)) return (FALSE);
      break;
    default:
      for (p = list, i = 0; p; p = p->next, i++)
	if (!G_TYPE_CHECK_INSTANCE_TYPE (p->data, type)) return (FALSE);

      if (i != count) return (FALSE);
      break;
    }

  return (TRUE);
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

/**
 * ipatch_converter_log:
 * @converter: Converter object
 * @item: Item the log entry pertains to or %NULL if not item specific
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

/**
 * ipatch_converter_log_printf:
 * @converter: Converter object
 * @item: Item the log entry pertains to or %NULL if not item specific
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
 * @pos: Opaque current position in log, should be %NULL on first call to
 *   this function to return first log item (oldest item)
 * @item: Location to store item of the log entry or %NULL, no reference is
 *   added so the item is only guarenteed to exist for as long as the
 *   @converter does
 * @type: Location to store the type parameter of the log entry or %NULL
 * @msg: Location to store the message of the log entry or %NULL, message
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
 * ipatch_converter_set_link_funcs:
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

  converter->link_lookup = link_lookup;
  converter->link_notify = link_notify;
}
