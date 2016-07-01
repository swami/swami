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
 * SECTION: IpatchXmlObject
 * @short_description: GObject related XML tree functions
 * @see_also: IpatchXml
 * @stability: Stable
 *
 * Functions for saving/loading GObject, GValue and GObject properties
 * to/from XML trees.  Includes a system for registering custom encoding and
 * decoding handlers for objects, properties and value types.
 */
#include <string.h>
#include "IpatchXmlObject.h"
#include "IpatchXml.h"
#include "IpatchParamProp.h"
#include "misc.h"
#include "i18n.h"

/* Number of decimal places of precision for floating point numbers stored to XML */
#define XML_FLOAT_PRECISION	6

/* structure used as hash key for xml_handlers */
typedef struct
{
  GType type;
  GParamSpec *pspec;
} HandlerHashKey;

/* structure used as hash value for xml_handlers */
typedef struct
{
  IpatchXmlEncodeFunc encode_func;
  IpatchXmlDecodeFunc decode_func;
  GDestroyNotify notify_func;
  gpointer user_data;
} HandlerHashValue;


static guint xml_handlers_hash_func (gconstpointer key);
static gboolean xml_handlers_key_equal_func (gconstpointer a, gconstpointer b);
static void xml_handlers_key_destroy_func (gpointer data);
static void xml_handlers_value_destroy_func (gpointer data);

/* Lock for xml_handlers */
G_LOCK_DEFINE_STATIC (xml_handlers);

/* hash of XML handlers (HandlerHashKey -> HandlerHashValue) */
static GHashTable *xml_handlers = NULL;


void
_ipatch_xml_object_init (void)
{
  xml_handlers = g_hash_table_new_full (xml_handlers_hash_func,
                                        xml_handlers_key_equal_func,
                                        xml_handlers_key_destroy_func,
                                        xml_handlers_value_destroy_func);
}

static guint
xml_handlers_hash_func (gconstpointer key)
{
  HandlerHashKey *hkey = (HandlerHashKey *)key;
  return (hkey->type + GPOINTER_TO_UINT (hkey->pspec));
}

static gboolean
xml_handlers_key_equal_func (gconstpointer a, gconstpointer b)
{
  HandlerHashKey *akey = (HandlerHashKey *)a, *bkey = (HandlerHashKey *)b;
  return (akey->type == bkey->type && akey->pspec == bkey->pspec);
}

static void
xml_handlers_key_destroy_func (gpointer data)
{
  g_slice_free (HandlerHashKey, data);
}

static void
xml_handlers_value_destroy_func (gpointer data)
{
  g_slice_free (HandlerHashValue, data);
}

/**
 * ipatch_xml_register_handler: (skip)
 * @type: GType to register handler functions for (GObject type if an object or
 *   object property handler, GValue type for value handlers).
 * @prop_name: GObject property name (or %NULL if not a GObject property handler)
 * @encode_func: Function to handle encoding (object/property/value -> XML)
 * @decode_func: Function to handle decoding (XML -> object/property/value)
 *
 * Registers XML encoding/decoding handlers for a GObject type, GObject property or
 * GValue type.
 */
void
ipatch_xml_register_handler (GType type, const char *prop_name,
                             IpatchXmlEncodeFunc encode_func,
                             IpatchXmlDecodeFunc decode_func)
{
  ipatch_xml_register_handler_full (type, prop_name, encode_func, decode_func, NULL, NULL);
}

/**
 * ipatch_xml_register_handler_full: (rename-to ipatch_xml_register_handler)
 * @type: GType to register handler functions for (GObject type if an object or
 *   object property handler, GValue type for value handlers).
 * @prop_name: (nullable): GObject property name (or %NULL if not a GObject property handler)
 * @encode_func: (scope notified): Function to handle encoding (object/property/value -> XML)
 * @decode_func: (scope notified): Function to handle decoding (XML -> object/property/value)
 * @notify_func: (nullable) (scope async) (closure user_data): Callback when handlers are removed.
 * @user_data: (nullable): Data passed to @notify_func
 *
 * Registers XML encoding/decoding handlers for a GObject type, GObject property or
 * GValue type.
 *
 * Since: 1.1.0
 */
void
ipatch_xml_register_handler_full (GType type, const char *prop_name,
                                  IpatchXmlEncodeFunc encode_func,
                                  IpatchXmlDecodeFunc decode_func,
                                  GDestroyNotify notify_func, gpointer user_data)
{
  HandlerHashKey *key;
  HandlerHashValue *val;
  GParamSpec *pspec = NULL;
  GObjectClass *obj_class;

  g_return_if_fail (type != 0);
  g_return_if_fail (encode_func != NULL);
  g_return_if_fail (decode_func != NULL);

  if (prop_name)
  {
    obj_class = g_type_class_peek (type);
    g_return_if_fail (obj_class != NULL);

    pspec = g_object_class_find_property (obj_class, prop_name);
    g_return_if_fail (pspec != NULL);
  }

  key = g_slice_new (HandlerHashKey);
  key->type = type;
  key->pspec = pspec;

  val = g_slice_new (HandlerHashValue);
  val->encode_func = encode_func;
  val->decode_func = decode_func;
  val->notify_func = notify_func;
  val->user_data = user_data;

  G_LOCK (xml_handlers);
  g_hash_table_insert (xml_handlers, key, val);
  G_UNLOCK (xml_handlers);
}

/**
 * ipatch_xml_lookup_handler: (skip)
 * @type: GObject or GValue type of handler to lookup
 * @pspec: (nullable): GObject property spec (or %NULL if not a GObject property handler)
 * @encode_func: (out) (optional): Location to store encoding function (or %NULL to ignore)
 * @decode_func: (out) (optional): Location to store decoding function (or %NULL to ignore)
 *
 * Looks up handlers for a given GObject type, GObject property or GValue
 * type previously registered with ipatch_xml_register_handler().
 * These functions are used for encoding/decoding to/from XML.
 *
 * Returns: %TRUE if handler found, %FALSE otherwise
 */
gboolean
ipatch_xml_lookup_handler (GType type, GParamSpec *pspec,
                           IpatchXmlEncodeFunc *encode_func,
                           IpatchXmlDecodeFunc *decode_func)
{
  HandlerHashValue *val;
  HandlerHashKey key;

  g_return_val_if_fail (type != 0, FALSE);

  key.type = type;
  key.pspec = pspec;

  G_LOCK (xml_handlers);
  val = g_hash_table_lookup (xml_handlers, &key);
  G_UNLOCK (xml_handlers);

  if (encode_func) *encode_func = val ? val->encode_func : NULL;
  if (decode_func) *decode_func = val ? val->decode_func : NULL;

  return (val != NULL);
}

/**
 * ipatch_xml_lookup_handler_by_prop_name: (skip)
 * @type: GObject or GValue type of handler to lookup
 * @prop_name: (nullable): GObject property name (or %NULL if not a GObject property handler)
 * @encode_func: (out) (optional): Location to store encoding function (or %NULL to ignore)
 * @decode_func: (out) (optional): Location to store decoding function (or %NULL to ignore)
 *
 * Like ipatch_xml_lookup_handler() but takes a @prop_name string to indicate which
 * GObject property to lookup instead of a GParamSpec.
 *
 * Returns: %TRUE if handler found, %FALSE otherwise
 */
gboolean
ipatch_xml_lookup_handler_by_prop_name (GType type, const char *prop_name,
                                        IpatchXmlEncodeFunc *encode_func,
                                        IpatchXmlDecodeFunc *decode_func)
{
  GParamSpec *pspec = NULL;
  GObjectClass *obj_class;

  g_return_val_if_fail (type != 0, FALSE);

  if (prop_name)
  {
    obj_class = g_type_class_peek (type);
    g_return_val_if_fail (obj_class != NULL, FALSE);

    pspec = g_object_class_find_property (obj_class, prop_name);
    g_return_val_if_fail (pspec != NULL, FALSE);
  }

  return (ipatch_xml_lookup_handler (type, pspec, encode_func, decode_func));
}

/**
 * ipatch_xml_encode_object:
 * @node: XML node to encode to
 * @object: Object to encode to XML
 * @create_element: %TRUE to create a &lt;obj&gt; element, %FALSE to add object
 *   properties to current open element
 * @err: Location to store error info or %NULL to ignore
 *
 * Encodes an object to XML.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_xml_encode_object (GNode *node, GObject *object,
			  gboolean create_element, GError **err)
{
  IpatchXmlEncodeFunc encode_func;
  GType type;

  g_return_val_if_fail (node != NULL, FALSE);
  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  type = G_OBJECT_TYPE (object);

  /* search through type ancestry for Object handler */
  do
  {
    if (ipatch_xml_lookup_handler (type, NULL, &encode_func, NULL)) break;
  } while ((type = g_type_parent (type)));

  /* not found? Use default Object encoder */
  if (!type) encode_func = ipatch_xml_default_encode_object_func;

  if (create_element)
    node = ipatch_xml_new_node (node, "obj", NULL,
                                "type", g_type_name (type),
                                NULL);

  return (encode_func (node, object, NULL, NULL, err));
}

/**
 * ipatch_xml_encode_property:
 * @node: XML node to encode to
 * @object: GObject to encode property of
 * @pspec: Parameter specification of property to encode
 * @create_element: %TRUE to create a &lt;prop name="PROPNAME"&gt; element, %FALSE to
 *   assign object property value to node
 * @err: Location to store error info or %NULL to ignore
 *
 * Encode an object property to an XML node.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
ipatch_xml_encode_property (GNode *node, GObject *object, GParamSpec *pspec,
                            gboolean create_element, GError **err)
{
  IpatchXmlEncodeFunc encode_func;
  GValue value = { 0 };
  gboolean retval;

  g_return_val_if_fail (node != NULL, FALSE);
  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  /* ++ alloc value */
  g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  g_object_get_property (object, g_param_spec_get_name (pspec), &value);

  if (create_element)
    node = ipatch_xml_new_node (node, "prop", NULL, "name", pspec->name, NULL);

  if (!ipatch_xml_lookup_handler (pspec->owner_type, pspec, &encode_func, NULL))
    retval = ipatch_xml_encode_value (node, &value, err);
  else retval = encode_func (node, object, pspec, &value, err);

  g_value_unset (&value);	/* -- free value */

  if (!retval && create_element)
    ipatch_xml_destroy (node);	/* Cleanup after error (if create_element) */

  return (retval);
}

/**
 * ipatch_xml_encode_property_by_name:
 * @node: XML node to encode to
 * @object: GObject to encode property of
 * @propname: Name of object property to encode
 * @create_element: %TRUE to create a &lt;prop name="PROPNAME"&gt; element, %FALSE to
 *   assign object property value to node
 * @err: Location to store error info or %NULL to ignore
 *
 * Encode an object property by name to an XML node.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
ipatch_xml_encode_property_by_name (GNode *node, GObject *object,
                                    const char *propname,
                                    gboolean create_element, GError **err)
{
  GParamSpec *pspec;

  g_return_val_if_fail (node != NULL, FALSE);
  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);
  g_return_val_if_fail (propname != NULL, FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), propname);
  g_return_val_if_fail (pspec != NULL, FALSE);

  return (ipatch_xml_encode_property (node, object, pspec, create_element, err));
}

/**
 * ipatch_xml_encode_value:
 * @node: XML node to encode to
 * @value: Value to encode
 * @err: Location to store error info or %NULL to ignore
 *
 * Encodes a GValue to an XML node text value.
 *
 * Returns: TRUE on success, FALSE on error (@err may be set)
 */
gboolean
ipatch_xml_encode_value (GNode *node, GValue *value, GError **err)
{
  IpatchXmlEncodeFunc encode_func;

  g_return_val_if_fail (node != NULL, FALSE);
  g_return_val_if_fail (G_IS_VALUE (value), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  if (!ipatch_xml_lookup_handler (G_VALUE_TYPE (value), NULL, &encode_func, NULL))
    encode_func = ipatch_xml_default_encode_value_func;

  return (encode_func (node, NULL, NULL, value, err));
}

/**
 * ipatch_xml_decode_object:
 * @node: XML node to decode from
 * @object: Object to decode to from XML
 * @err: Location to store error info or %NULL to ignore
 *
 * Decodes XML to an object.  The default GObject decoder will only decode
 * those properties which don't have the #IPATCH_PARAM_NO_SAVE flag set.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_xml_decode_object (GNode *node, GObject *object, GError **err)
{
  IpatchXmlDecodeFunc decode_func;
  GType type;

  g_return_val_if_fail (node != NULL, FALSE);
  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  type = G_OBJECT_TYPE (object);

  /* search through type ancestry for Object handler */
  do
  {
    if (ipatch_xml_lookup_handler (type, NULL, NULL, &decode_func)) break;
  } while ((type = g_type_parent (type)));

  /* not found? Use default Object decoder */
  if (!type) decode_func = ipatch_xml_default_decode_object_func;

  return (decode_func (node, object, NULL, NULL, err));
}

/**
 * ipatch_xml_decode_property:
 * @node: XML node to decode from
 * @object: GObject to decode property of
 * @pspec: Parameter specification of property to decode
 * @err: Location to store error info or %NULL to ignore
 *
 * Decode an object property from an XML node value to an object.  Note that
 * the property is NOT checked for the #IPATCH_PARAM_NO_SAVE flag.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
ipatch_xml_decode_property (GNode *node, GObject *object, GParamSpec *pspec,
                            GError **err)
{
  IpatchXmlDecodeFunc decode_func;
  GValue value = { 0 };
  gboolean retval;

  g_return_val_if_fail (node != NULL, FALSE);
  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  /* ++ alloc value */
  g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));

  if (!ipatch_xml_lookup_handler (pspec->owner_type, pspec, NULL, &decode_func))
    retval = ipatch_xml_decode_value (node, &value, err);
  else retval = decode_func (node, object, pspec, &value, err);

  if (retval) g_object_set_property (object, pspec->name, &value);

  g_value_unset (&value);	/* -- free value */

  return (retval);
}

/**
 * ipatch_xml_decode_property_by_name:
 * @node: XML node to decode from
 * @object: GObject to decode property of
 * @propname: Name of object property to decode
 * @err: Location to store error info or %NULL to ignore
 *
 * Decode an object property from an XML node value to an object by property name.
 * Note that the property is NOT checked for the #IPATCH_PARAM_NO_SAVE flag.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
ipatch_xml_decode_property_by_name (GNode *node, GObject *object,
                                    const char *propname, GError **err)
{
  GParamSpec *pspec;

  g_return_val_if_fail (node != NULL, FALSE);
  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);
  g_return_val_if_fail (propname != NULL, FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), propname);
  g_return_val_if_fail (pspec != NULL, FALSE);

  return (ipatch_xml_decode_property (node, object, pspec, err));
}

/**
 * ipatch_xml_decode_value:
 * @node: XML node to decode from
 * @value: Value to decode to
 * @err: Location to store error info or %NULL to ignore
 *
 * Decodes a GValue from an XML node text value.
 *
 * Returns: TRUE on success, FALSE on error (@err may be set)
 */
gboolean
ipatch_xml_decode_value (GNode *node, GValue *value, GError **err)
{
  IpatchXmlDecodeFunc decode_func;

  g_return_val_if_fail (node != NULL, FALSE);
  g_return_val_if_fail (G_IS_VALUE (value), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  if (!ipatch_xml_lookup_handler (G_VALUE_TYPE (value), NULL, NULL, &decode_func))
    decode_func = ipatch_xml_default_decode_value_func;

  return (decode_func (node, NULL, NULL, value, err));
}

/**
 * ipatch_xml_default_encode_object_func: (type IpatchXmlEncodeFunc)
 * @node: XML node to encode XML to
 * @object: Object to encode
 * @pspec: Will be %NULL
 * @value: Will be %NULL
 * @err: Location to store error value (or %NULL if ignoring)
 *
 * Default GObject encode handler.  Useful for custom handlers to chain to
 * the default if needed.
 *
 * Returns: TRUE on success, FALSE on error (@err may be set)
 */
gboolean
ipatch_xml_default_encode_object_func (GNode *node, GObject *object,
                                       GParamSpec *pspec, GValue *value,
                                       GError **err)
{
  GParamSpec **pspecs;
  GError *local_err = NULL;
  guint n_props;
  guint i;

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (object),	 /* ++ alloc */
					   &n_props);
  for (i = 0; i < n_props; i++)
  { /* Skip parameters marked as no save or not read/write */
    if ((pspecs[i]->flags & IPATCH_PARAM_NO_SAVE)
        || (pspecs[i]->flags & G_PARAM_READWRITE) != G_PARAM_READWRITE)
      continue;

    if (!ipatch_xml_encode_property (node, object, pspecs[i], TRUE, &local_err))  /* ++ alloc */
    {
      g_warning ("Failed to store property '%s' for object of type '%s': %s",
		 pspecs[i]->name, g_type_name (G_OBJECT_TYPE (object)),
		 ipatch_gerror_message (local_err));
      g_clear_error (&local_err);
    }
  }

  g_free (pspecs);	/* -- free */

  return (TRUE);
}

/**
 * ipatch_xml_default_encode_property_func: (type IpatchXmlEncodeFunc)
 * @node: XML node to encode XML to
 * @object: Object to encode
 * @pspec: Parameter spec of property to encode
 * @value: Value of the property
 * @err: Location to store error value (or %NULL if ignoring)
 *
 * Default GObject property encode handler.  Useful for custom handlers to chain
 * to the default if needed.
 *
 * Returns: TRUE on success, FALSE on error (@err may be set)
 */
gboolean
ipatch_xml_default_encode_property_func (GNode *node, GObject *object,
                                         GParamSpec *pspec, GValue *value,
                                         GError **err)
{
  return (ipatch_xml_encode_value (node, value, err));
}

/**
 * ipatch_xml_default_encode_value_func: (type IpatchXmlEncodeFunc)
 * @node: XML node to encode XML to
 * @object: Will be %NULL
 * @pspec: Will be %NULL
 * @value: Value to encode
 * @err: Location to store error value (or %NULL if ignoring)
 *
 * Default GValue encode handler.  Useful for custom handlers to chain to
 * the default if needed.
 *
 * Returns: TRUE on success, FALSE on error (@err may be set)
 */
gboolean
ipatch_xml_default_encode_value_func (GNode *node, GObject *object,
                                      GParamSpec *pspec, GValue *value,
                                      GError **err)
{
  GType valtype;
  const char *s;

  g_return_val_if_fail (node != NULL, FALSE);
  g_return_val_if_fail (G_IS_VALUE (value), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  valtype = G_VALUE_TYPE (value);

  switch (G_TYPE_FUNDAMENTAL (valtype))
  {
    case G_TYPE_CHAR:
      ipatch_xml_set_value_printf (node, "%d", g_value_get_char (value));
      return (TRUE);
    case G_TYPE_UCHAR:
      ipatch_xml_set_value_printf (node, "%u", g_value_get_uchar (value));
      return (TRUE);
    case G_TYPE_BOOLEAN:
      ipatch_xml_set_value_printf (node, "%u", g_value_get_boolean (value) != 0);
      return (TRUE);
    case G_TYPE_INT:
      ipatch_xml_set_value_printf (node, "%d", g_value_get_int (value));
      return (TRUE);
    case G_TYPE_UINT:
      ipatch_xml_set_value_printf (node, "%u", g_value_get_uint (value));
      return (TRUE);
    case G_TYPE_LONG:
      ipatch_xml_set_value_printf (node, "%ld", g_value_get_long (value));
      return (TRUE);
    case G_TYPE_ULONG:
      ipatch_xml_set_value_printf (node, "%lu", g_value_get_ulong (value));
      return (TRUE);
    case G_TYPE_INT64:
      ipatch_xml_set_value_printf (node, "%" G_GINT64_FORMAT,
                                   g_value_get_int64 (value));
      return (TRUE);
    case G_TYPE_UINT64:
      ipatch_xml_set_value_printf (node, "%" G_GUINT64_FORMAT,
                                   g_value_get_uint64 (value));
      return (TRUE);
    case G_TYPE_ENUM:
      ipatch_xml_set_value_printf (node, "%d", g_value_get_enum (value));
      return (TRUE);
    case G_TYPE_FLAGS:
      ipatch_xml_set_value_printf (node, "%u", g_value_get_flags (value));
      return (TRUE);
    case G_TYPE_FLOAT:
      ipatch_xml_set_value_printf (node, "%.*f", XML_FLOAT_PRECISION,
                                   (double)g_value_get_float (value));
      return (TRUE);
    case G_TYPE_DOUBLE:
      ipatch_xml_set_value_printf (node, "%.*f", XML_FLOAT_PRECISION,
                                   g_value_get_double (value));
      return (TRUE);
    case G_TYPE_STRING:
      s = g_value_get_string (value);

      if (s) ipatch_xml_take_value (node, g_markup_escape_text (s, -1));
      else ipatch_xml_set_value (node, NULL);

      return (TRUE);
    default:
      if (valtype == G_TYPE_GTYPE)
      {
        ipatch_xml_set_value (node, g_type_name (g_value_get_gtype (value)));
	return (TRUE);
      }
      else
      {
	g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_UNHANDLED_CONVERSION,
		     "Unhandled GValue to XML conversion for type '%s'",
		     g_type_name (valtype));
        return (FALSE);
      }
  }

  return (TRUE);
}

/**
 * ipatch_xml_default_decode_object_func: (type IpatchXmlDecodeFunc)
 * @node: XML node to decode XML from
 * @object: Object to decode to
 * @pspec: Will be %NULL
 * @value: Will be %NULL
 * @err: Location to store error value (or %NULL if ignoring)
 *
 * Default GObject decode handler.  Useful for custom handlers to chain to
 * the default if needed.
 *
 * Returns: TRUE on success, FALSE on error (@err may be set)
 */
gboolean
ipatch_xml_default_decode_object_func (GNode *node, GObject *object,
                                       GParamSpec *pspec, GValue *value,
                                       GError **err)
{
  IpatchXmlNode *xmlnode;
  GObjectClass *klass;
  GParamSpec *prop;
  const char *propname;
  GError *local_err = NULL;
  GNode *n;

  klass = G_OBJECT_GET_CLASS (object);

  /* Look for property child nodes */
  for (n = node->children; n; n = n->next)
  {
    xmlnode = (IpatchXmlNode *)(n->data);
    if (strcmp (xmlnode->name, "prop") != 0) continue;

    propname = ipatch_xml_get_attribute (n, "name");
    if (!propname) continue;

    prop = g_object_class_find_property (klass, propname);

    if (prop)
    {
      if (prop->flags & IPATCH_PARAM_NO_SAVE)
      {
	g_warning (_("Ignoring non storeable XML object property '%s' for object type '%s'"),
	           prop->name, g_type_name (G_OBJECT_TYPE (object)));
	continue;
      }

      if (!ipatch_xml_decode_property (n, object, prop, &local_err))
      {
	g_warning (_("Failed to decode object property: %s"),
	           ipatch_gerror_message (local_err));
	g_clear_error (&local_err);
      }
    }
    else g_warning (_("XML object property '%s' not valid for object type '%s'"),
                    propname, g_type_name (G_OBJECT_TYPE (object)));
  }

  return (TRUE);
}

/**
 * ipatch_xml_default_decode_property_func: (type IpatchXmlDecodeFunc)
 * @node: XML node to decode XML from
 * @object: Object whose property is being decoded
 * @pspec: Parameter spec of property to decode
 * @value: Initialized value to decode to
 * @err: Location to store error value (or %NULL if ignoring)
 *
 * Default GObject property decode handler.  Useful for custom handlers to chain
 * to the default if needed.
 *
 * Returns: TRUE on success, FALSE on error (@err may be set)
 */
gboolean
ipatch_xml_default_decode_property_func (GNode *node, GObject *object,
                                         GParamSpec *pspec, GValue *value,
                                         GError **err)
{
  return (ipatch_xml_decode_value (node, value, err));
}

/**
 * ipatch_xml_default_decode_value_func: (type IpatchXmlDecodeFunc)
 * @node: XML node to decode XML from
 * @object: Will be %NULL
 * @pspec: Will be %NULL
 * @value: Value to decode to
 * @err: Location to store error value (or %NULL if ignoring)
 *
 * Default GValue decode handler.  Useful for custom handlers to chain to
 * the default if needed.
 *
 * Returns: TRUE on success, FALSE on error (@err may be set)
 */
gboolean
ipatch_xml_default_decode_value_func (GNode *node, GObject *object,
                                      GParamSpec *pspec, GValue *value,
                                      GError **err)
{
  GType valtype;
  guint64 u64;
  gint64 i64;
  gulong lu;
  glong li;
  guint u;
  int i;
  float f;
  double d;
  const char *xml;
  char *endptr;

  valtype = G_VALUE_TYPE (value);

  xml = ipatch_xml_get_value (node);
  if (!xml) xml = "";

  switch (G_TYPE_FUNDAMENTAL (valtype))
  {
    case G_TYPE_CHAR:
      if (sscanf (xml, "%d", &i) != 1) goto malformed_err;
      if (i < G_MININT8 || i > G_MAXINT8) goto range_err;
      g_value_set_char (value, i);
      break;
    case G_TYPE_UCHAR:
      if (sscanf (xml, "%u", &u) != 1) goto malformed_err;
      if (u > G_MAXUINT8) goto range_err;
      g_value_set_uchar (value, u);
      break;
    case G_TYPE_BOOLEAN:
      if (sscanf (xml, "%u", &u) != 1) goto malformed_err;
      if (u > 1) goto range_err;
      g_value_set_boolean (value, u);
      break;
    case G_TYPE_INT:
      if (sscanf (xml, "%d", &i) != 1) goto malformed_err;
      g_value_set_int (value, i);
      break;
    case G_TYPE_UINT:
      if (sscanf (xml, "%u", &u) != 1) goto malformed_err;
      g_value_set_uint (value, u);
      break;
    case G_TYPE_LONG:
      if (sscanf (xml, "%ld", &li) != 1) goto malformed_err;
      g_value_set_long (value, li);
      break;
    case G_TYPE_ULONG:
      if (sscanf (xml, "%lu", &lu) != 1) goto malformed_err;
      g_value_set_ulong (value, lu);
      break;
    case G_TYPE_INT64:
      i64 = g_ascii_strtoll (xml, &endptr, 10);
      if (endptr == xml) goto malformed_err;
      g_value_set_int64 (value, i64);
      break;
    case G_TYPE_UINT64:
      u64 = g_ascii_strtoull (xml, &endptr, 10);
      if (endptr == xml) goto malformed_err;
      g_value_set_uint64 (value, u64);
      break;
    case G_TYPE_ENUM:
      if (sscanf (xml, "%d", &i) != 1) goto malformed_err;
      g_value_set_enum (value, i);
      break;
    case G_TYPE_FLAGS:
      if (sscanf (xml, "%u", &u) != 1) goto malformed_err;
      g_value_set_flags (value, u);
      break;
    case G_TYPE_FLOAT:
      if (sscanf (xml, "%f", &f) != 1) goto malformed_err;
      g_value_set_float (value, f);
      break;
    case G_TYPE_DOUBLE:
      if (sscanf (xml, "%lf", &d) != 1) goto malformed_err;
      g_value_set_double (value, d);
      break;
    case G_TYPE_STRING:
      g_value_set_string (value, xml);
      break;
    default:
      if (valtype == G_TYPE_GTYPE)
      {
        g_value_set_gtype (value, g_type_from_name (xml));
        return (TRUE);
      }
      else
      {
        g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_UNHANDLED_CONVERSION,
		     "Unhandled XML to GValue conversion for type '%s'",
		     g_type_name (valtype));
        return (FALSE);
      }
  }

  return (TRUE);

malformed_err:
  g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_INVALID,
	       "Invalid XML GValue '%s' for type '%s'", xml,
	       g_type_name (valtype));
  return (FALSE);

range_err:
  g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_INVALID,
	       "Out of range XML GValue '%s' for type '%s'", xml,
	       g_type_name (valtype));
  return (FALSE);
}
