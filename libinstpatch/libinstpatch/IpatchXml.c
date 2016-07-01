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
 * SECTION: IpatchXml
 * @short_description: XML tree functions
 * @see_also: IpatchXmlObject
 * @stability: Stable
 *
 * Functions for manipulating XML node trees and saving/loading to/from
 * XML content in strings or files.  XML node trees use the glib GNode N-ary
 * tree data type for added flexibility.
 */
#include <string.h>
#include "IpatchXml.h"
#include "misc.h"

static gboolean xml_destroy_traverse_func (GNode *node, gpointer data);
static GNode *ipatch_xml_find_by_path_recurse (GNode *node, const char *path);
static void ipatch_xml_to_str_recurse (GString *str, GNode *node, guint indent,
                                       guint inc);
static void
xml_start_element (GMarkupParseContext *context, const gchar *element_name,
                   const gchar **attribute_names, const gchar **attribute_values,
                   gpointer user_data, GError **error);
static void
xml_end_element (GMarkupParseContext *context, const gchar *element_name,
                 gpointer user_data, GError **error);
static void
xml_text (GMarkupParseContext *context, const gchar *text, gsize text_len,  
          gpointer user_data, GError **error);

/**
 * ipatch_xml_new_node: (skip)
 * @parent: (nullable): Parent node to add new
 *   node to as a child, or %NULL to create new root node
 * @name: Name of the new XML node
 * @value: (nullable): Text value to assign to the new node or %NULL
 * @attr_name: (nullable): First attribute name to assign or %NULL
 * @...: (type char*): If @attr_name was supplied first string value to be assigned should be
 *   the first parameter, additional name/value attribute string pairs may
 *   follow terminated by a %NULL name.
 *
 * Create a new XML tree node and append it to the given @parent, if supplied.
 * Note that the returned GNode can be used with other N-Array glib operations.
 *
 * Returns: New XML tree node
 */
GNode *
ipatch_xml_new_node (GNode *parent, const char *name, const char *value,
                     const char *attr_name, ...)
{
  IpatchXmlNode *xmlnode;
  IpatchXmlAttr *attr;
  va_list var_args;
  char *vname, *vvalue;

  g_return_val_if_fail (name != NULL, NULL);

  xmlnode = ipatch_xml_node_new ();
  xmlnode->name = g_strdup (name);
  xmlnode->value = g_strdup (value);
  xmlnode->attributes = NULL;

  if (attr_name)
  {
    va_start (var_args, attr_name);

    attr = ipatch_xml_attr_new ();
    attr->name = g_strdup (attr_name);
    attr->value = g_strdup (va_arg (var_args, char *));
    xmlnode->attributes = g_list_append (xmlnode->attributes, attr);

    while ((vname = va_arg (var_args, char *)))
    {
      vvalue = va_arg (var_args, char *);
      if (!vvalue) continue;

      attr = ipatch_xml_attr_new ();
      attr->name = g_strdup (vname);
      attr->value = g_strdup (vvalue);
      xmlnode->attributes = g_list_append (xmlnode->attributes, attr);
    }

    va_end (var_args);
  }

  return (parent ? g_node_append_data (parent, xmlnode) : g_node_new (xmlnode));
}

/**
 * ipatch_xml_new_node_strv: (skip)
 * @parent: (nullable): Parent node to add
 *   new node to as a child, or %NULL to create new root node
 * @name: Name of the new XML node
 * @value: (nullable): Text value to assign to the new node or %NULL
 * @attr_names: (array zero-terminated=1) (nullable): %NULL terminated
 *   array of attribute names or %NULL
 * @attr_values: (array zero-terminated=1) (nullable): %NULL terminated
 *   array of attribute values or %NULL
 *
 * Like ipatch_xml_new_node() but takes attribute name/values as separate strv
 * arrays.
 *
 * Returns: New XML tree node
 */
GNode *
ipatch_xml_new_node_strv (GNode *parent, const char *name, const char *value,
                          const char **attr_names, const char **attr_values)
{
  IpatchXmlNode *xmlnode;
  IpatchXmlAttr *attr;
  int i;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (!attr_names == !attr_values, NULL);

  xmlnode = ipatch_xml_node_new ();
  xmlnode->name = g_strdup (name);
  xmlnode->value = g_strdup (value);
  xmlnode->attributes = NULL;

  if (attr_names)
  {
    for (i = 0; attr_names[i] && attr_values[i]; i++)
    {
      if (!attr_values[i]) continue;

      attr = ipatch_xml_attr_new ();
      attr->name = g_strdup (attr_names[i]);
      attr->value = g_strdup (attr_values[i]);
      xmlnode->attributes = g_list_append (xmlnode->attributes, attr);
    }
  }

  return (parent ? g_node_append_data (parent, xmlnode) : g_node_new (xmlnode));
}

#define QDATA(node)	&(((IpatchXmlNode *)(node->data))->qdata)

/**
 * ipatch_xml_get_data: (skip)
 * @node: XML node
 * @key: Name of the key
 *
 * Lookup data assigned to an XML node.
 *
 * Returns: The data pointer or %NULL if not set
 */
gpointer
ipatch_xml_get_data (GNode *node, const char *key)
{
  g_return_val_if_fail (node != NULL, NULL);
  return (g_datalist_get_data (QDATA (node), key));
}

/**
 * ipatch_xml_set_data: (skip)
 * @node: XML node
 * @key: Name of the key
 * @data: Data to associate with the key
 *
 * Assigns arbitrary data to an XML node specified by a @key.
 */
void
ipatch_xml_set_data (GNode *node, const char *key, gpointer data)
{
  g_return_if_fail (node != NULL);
  return (g_datalist_set_data (QDATA (node), key, data));
}

/**
 * ipatch_xml_set_data_full: (skip)
 * @node: XML node
 * @key: Name of the key
 * @data: Data to associate with the key
 * @destroy_func: (nullable): Destroy function or %NULL
 *
 * Assigns arbitrary data to an XML node specified by a @key.  Also assigns a
 * @destroy_func callback to destroy the data when it is removed.
 */
void
ipatch_xml_set_data_full (GNode *node, const char *key, gpointer data,
                          GDestroyNotify destroy_func)
{
  g_return_if_fail (node != NULL);
  return (g_datalist_set_data_full (QDATA (node), key, data, destroy_func));
}

/**
 * ipatch_xml_steal_data: (skip)
 * @node: XML node
 * @key: Name of the key
 *
 * Remove keyed data from an XML node, but don't call the data's destroy notify.
 * Caller is thus given the ownership of the data.
 *
 * Returns: (transfer none): The data pointer or %NULL if not set
 */
gpointer
ipatch_xml_steal_data (GNode *node, const char *key)
{
  gpointer data;
  GQuark quark;

  g_return_val_if_fail (node != NULL, NULL);

  quark = g_quark_try_string (key);
  if (!quark) return (NULL);

  data = g_datalist_id_get_data (QDATA (node), quark);
  if (data) g_datalist_id_remove_no_notify (QDATA (node), quark);

  return (data);
}

/**
 * ipatch_xml_get_qdata: (skip)
 * @node: XML node
 * @quark: Quark key
 *
 * Lookup data assigned to an XML node using a quark.  This is faster than
 * ipatch_xml_get_data() since the key must be converted to a quark anyways.
 *
 * Returns: (transfer none): The data pointer or %NULL if not set
 */
gpointer
ipatch_xml_get_qdata (GNode *node, GQuark quark)
{
  g_return_val_if_fail (node != NULL, NULL);
  return (g_datalist_id_get_data (QDATA (node), quark));
}

/**
 * ipatch_xml_set_qdata: (skip)
 * @node: XML node
 * @quark: Quark key
 * @data: Data to associate with the key
 *
 * Assigns arbitrary data to an XML node specified by a @quark key.  This is
 * faster than ipatch_xml_set_data() since the key must be converted to a quark
 * anyways.
 */
void
ipatch_xml_set_qdata (GNode *node, GQuark quark, gpointer data)
{
  g_return_if_fail (node != NULL);
  return (g_datalist_id_set_data (QDATA (node), quark, data));
}

/**
 * ipatch_xml_set_qdata_full: (skip)
 * @node: XML node
 * @quark: Quark key
 * @data: Data to associate with the key
 * @destroy_func: (nullable): Destroy function or %NULL
 *
 * Assigns arbitrary data to an XML node specified by a @key.  Also assigns a
 * @destroy_func callback to destroy the data when it is removed.  This is
 * faster than ipatch_xml_set_data_full() since the key must be converted to a quark
 * anyways.
 */
void
ipatch_xml_set_qdata_full (GNode *node, GQuark quark, gpointer data,
                           GDestroyNotify destroy_func)
{
  g_return_if_fail (node != NULL);
  return (g_datalist_id_set_data_full (QDATA (node), quark, data, destroy_func));
}

/**
 * ipatch_xml_steal_qdata: (skip)
 * @node: XML node
 * @quark: Quark key
 *
 * Remove keyed data from an XML node, but don't call the data's destroy notify.
 * Caller is thus given the ownership of the data.  This is faster than
 * ipatch_xml_steal_data() since the key must be converted to a quark
 * anyways.
 *
 * Returns: (transfer none): The data pointer or %NULL if not set
 */
gpointer
ipatch_xml_steal_qdata (GNode *node, GQuark quark)
{
  gpointer data;

  g_return_val_if_fail (node != NULL, NULL);

  data = g_datalist_id_get_data (QDATA (node), quark);
  if (data) g_datalist_id_remove_no_notify (QDATA (node), quark);

  return (data);
}

/**
 * ipatch_xml_destroy: (skip)
 * @node: Root of XML tree/sub tree to destroy
 *
 * Free an XML tree (a root @node and all its children).  Does not need to be
 * the actual root of a tree, i.e., can remove a sub tree.
 */
void
ipatch_xml_destroy (GNode *node)
{
  g_node_traverse (node, G_PRE_ORDER, G_TRAVERSE_ALL, -1, xml_destroy_traverse_func, NULL);
  g_node_destroy (node);
}

static gboolean
xml_destroy_traverse_func (GNode *node, gpointer data)
{
  ipatch_xml_node_free (node->data);
  return (FALSE);
}

/**
 * ipatch_xml_copy: (skip)
 * @node: XML tree to copy
 *
 * Perform a deep copy on an XML tree.
 *
 * Returns: New duplicate XML tree.
 */
GNode *
ipatch_xml_copy (GNode *node)
{
  g_return_val_if_fail (node != NULL, NULL);

  return (g_node_copy_deep (node, (GCopyFunc)ipatch_xml_node_duplicate, NULL));
}

/**
 * ipatch_xml_set_name: (skip)
 * @node: XML node
 * @name: Name to assign
 *
 * Set the name of an XML node.
 */
void
ipatch_xml_set_name (GNode *node, const char *name)
{
  IpatchXmlNode *xmlnode;

  g_return_if_fail (node != NULL);
  g_return_if_fail (name != NULL);

  xmlnode = node->data;
  g_free (xmlnode->name);
  xmlnode->name = g_strdup (name);
}

/**
 * ipatch_xml_set_value: (skip)
 * @node: XML node
 * @value: Text value to assign or %NULL to clear it
 *
 * Set the text value of an XML node.
 */
void
ipatch_xml_set_value (GNode *node, const char *value)
{
  IpatchXmlNode *xmlnode;

  g_return_if_fail (node != NULL);

  xmlnode = node->data;
  g_free (xmlnode->value);
  xmlnode->value = g_strdup (value);
}

/**
 * ipatch_xml_set_value_printf: (skip)
 * @node: XML node
 * @format: Printf format
 * @...: Printf arguments
 *
 * Assign a value to an XML node using a printf format and arguments.
 */
void
ipatch_xml_set_value_printf (GNode *node, const char *format, ...)
{
  va_list var_args;
  char *value;

  g_return_if_fail (node != NULL);
  g_return_if_fail (format != NULL);

  va_start (var_args, format);
  value = g_strdup_vprintf (format, var_args);
  va_end (var_args);

  ipatch_xml_take_value (node, value);
}

/**
 * ipatch_xml_take_name: (skip)
 * @node: XML node
 * @name: (nullable) (transfer full): Name to assign or %NULL to clear it
 *
 * Like ipatch_xml_set_name() but takes over the allocation of @name rather than
 * duplicating it.
 */
void
ipatch_xml_take_name (GNode *node, char *name)
{
  IpatchXmlNode *xmlnode;

  g_return_if_fail (node != NULL);
  g_return_if_fail (name != NULL);

  xmlnode = node->data;
  g_free (xmlnode->name);
  xmlnode->name = name;
}

/**
 * ipatch_xml_take_value: (skip)
 * @node: XML node
 * @value: (transfer full): Text value to assign
 *
 * Like ipatch_xml_set_value() but takes over the allocation of @value rather than
 * duplicating it.
 */
void
ipatch_xml_take_value (GNode *node, char *value)
{
  IpatchXmlNode *xmlnode;

  g_return_if_fail (node != NULL);

  xmlnode = node->data;
  g_free (xmlnode->value);
  xmlnode->value = value;
}

/**
 * ipatch_xml_get_name: (skip)
 * @node: XML node to get name of
 *
 * Get the name of an XML node.
 *
 * Returns: Name of XML node which is internal and should not be modified or freed.
 */
G_CONST_RETURN char *
ipatch_xml_get_name (GNode *node)
{
  g_return_val_if_fail (node != NULL, NULL);
  return (((IpatchXmlNode *)(node->data))->name);
}

/**
 * ipatch_xml_test_name: (skip)
 * @node: XML node to get name of
 * @cmpname: Name to compare to
 *
 * Test if the node has the given name.
 *
 * Returns: %TRUE if the node has the given name, %FALSE otherwise
 */
gboolean
ipatch_xml_test_name (GNode *node, const char *cmpname)
{
  const char *name;

  g_return_val_if_fail (node != NULL, FALSE);
  g_return_val_if_fail (cmpname != NULL, FALSE);

  name = ipatch_xml_get_name (node);

  return (name && strcmp (name, cmpname) == 0);
}

/**
 * ipatch_xml_get_value: (skip)
 * @node: XML node to get value of
 *
 * Get the text value of an XML node.
 *
 * Returns: Value of XML node or %NULL, which is internal and should not be
 *   modified or freed.
 */
G_CONST_RETURN char *
ipatch_xml_get_value (GNode *node)
{
  g_return_val_if_fail (node != NULL, NULL);
  return (((IpatchXmlNode *)(node->data))->value);
}

/**
 * ipatch_xml_dup_value: (skip)
 * @node: XML node to duplicate value of
 *
 * Duplicate the text value of an XML node.  Like ipatch_xml_get_value() but
 * makes a copy of the value which the caller owns.
 *
 * Returns: Newly allocated duplicate value of XML node or %NULL.
 */
char *
ipatch_xml_dup_value (GNode *node)
{
  g_return_val_if_fail (node != NULL, NULL);
  return (g_strdup (((IpatchXmlNode *)(node->data))->value));
}

/**
 * ipatch_xml_test_value: (skip)
 * @node: XML node to get name of
 * @cmpvalue: Value to compare to
 *
 * Test if the node has the given value.
 *
 * Returns: %TRUE if the node has the given value, %FALSE otherwise
 */
gboolean
ipatch_xml_test_value (GNode *node, const char *cmpvalue)
{
  const char *value;

  g_return_val_if_fail (node != NULL, FALSE);
  g_return_val_if_fail (cmpvalue != NULL, FALSE);

  value = ipatch_xml_get_value (node);

  return (value && strcmp (value, cmpvalue) == 0);
}

/**
 * ipatch_xml_set_attribute: (skip)
 * @node: XML node
 * @attr_name: Attribute name to assign to
 * @attr_value: (nullable): Attribute value to assign or %NULL to unset
 *
 * Set or unset an attribute of an XML node.  If there is already an existing
 * attribute with the given @attr_name, its value will be replaced.
 */
void
ipatch_xml_set_attribute (GNode *node, const char *attr_name,
                          const char *attr_value)
{
  IpatchXmlNode *xmlnode;
  IpatchXmlAttr *attr;
  GList *p;

  g_return_if_fail (node != NULL);
  g_return_if_fail (attr_name != NULL);

  xmlnode = (IpatchXmlNode *)(node->data);

  for (p = xmlnode->attributes; p; p = p->next)
  {
    attr = (IpatchXmlAttr *)(p->data);

    if (strcmp (attr->name, attr_name) == 0)
    {
      if (attr_value)
      {
        g_free (attr->value);
        attr->value = g_strdup (attr_value);
      }
      else
      {
        ipatch_xml_attr_free (attr);
        xmlnode->attributes = g_list_delete_link (xmlnode->attributes, p);
      }

      return;
    }
  }

  attr = ipatch_xml_attr_new ();
  attr->name = g_strdup (attr_name);
  attr->value = g_strdup (attr_value);
  xmlnode->attributes = g_list_append (xmlnode->attributes, attr);
}

/**
 * ipatch_xml_set_attributes: (skip)
 * @node: XML node
 * @attr_name: First attribute name
 * @attr_value: First attribute value
 * @...: Additional name/value attribute strings, terminated by a %NULL name
 *
 * Set one or more attributes of an XML node.
 */
void
ipatch_xml_set_attributes (GNode *node, const char *attr_name,
                           const char *attr_value, const char *attr2_name, ...)
{
  va_list var_args;
  char *vname;
  
  g_return_if_fail (node != NULL);
  g_return_if_fail (attr_name != NULL);

  ipatch_xml_set_attribute (node, attr_name, attr_value);
  if (!attr2_name) return;

  va_start (var_args, attr2_name);

  ipatch_xml_set_attribute (node, attr2_name, va_arg (var_args, char *));

  while ((vname = va_arg (var_args, char *)))
    ipatch_xml_set_attribute (node, vname, va_arg (var_args, char *));

  va_end (var_args);
}

/**
 * ipatch_xml_get_attribute: (skip)
 * @node: XML node
 * @attr_name: Name of attribute
 *
 * Get the value of an attribute of an XML node.
 *
 * Returns: Value of the named attribute or %NULL if not found, value is internal
 *   and should not be modified or freed.
 */
G_CONST_RETURN char *
ipatch_xml_get_attribute (GNode *node, const char *attr_name)
{
  IpatchXmlAttr *attr;
  GList *p;

  g_return_val_if_fail (node != NULL, NULL);
  g_return_val_if_fail (attr_name != NULL, NULL);

  for (p = ((IpatchXmlNode *)(node->data))->attributes; p; p = p->next)
  {
    attr = (IpatchXmlAttr *)(p->data);

    if (strcmp (attr->name, attr_name) == 0)
      return (attr->value);
  }

  return (NULL);
}

/**
 * ipatch_xml_test_attribute: (skip)
 * @node: XML node
 * @attr_name: Name of attribute
 * @cmpval: Value to compare attribute to (%NULL to just check existence).
 *
 * Test if an attribute of an XML node is a given value or exists (@cmpval = %NULL).
 *
 * Returns: %TRUE if attribute exists and matches @cmpval (if set).
 */
gboolean
ipatch_xml_test_attribute (GNode *node, const char *attr_name, const char *cmpval)
{
  const char *attr_val;

  g_return_val_if_fail (node != NULL, FALSE);
  g_return_val_if_fail (attr_name != NULL, FALSE);

  attr_val = ipatch_xml_get_attribute (node, attr_name);

  return (attr_val && (!cmpval || strcmp (attr_val, cmpval) == 0));
}

/**
 * ipatch_xml_find_child: (skip)
 * @node: XML node
 * @name: Node name of child to find
 *
 * Find a child node with the given @name.  Only searches the children of @node
 * and does not search recursively.
 *
 * Returns: (transfer none): Matching node or %NULL if not found.
 */
GNode *
ipatch_xml_find_child (GNode *node, const char *name)
{
  IpatchXmlNode *xmlnode;
  GNode *n;

  g_return_val_if_fail (node != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (n = node->children; n; n = n->next)
  {
    xmlnode = (IpatchXmlNode *)(n->data);

    if (strcmp (xmlnode->name, name) == 0)
      return (n);
  }

  return (NULL);
}

/**
 * ipatch_xml_find_by_path: (skip)
 * @node: XML node
 * @path: Path specification in the form "name1.name2.name3" where each child
 *   of a node is separated by a '.' character.
 *
 * Get a node in a tree from a path string.
 *
 * Returns: (transfer none): Matching node or %NULL if not found.
 */
GNode *
ipatch_xml_find_by_path (GNode *node, const char *path)
{
  g_return_val_if_fail (node != NULL, NULL);
  g_return_val_if_fail (path != NULL, NULL);

  return (ipatch_xml_find_by_path_recurse (node, path));
}

static GNode *
ipatch_xml_find_by_path_recurse (GNode *node, const char *path)
{
  IpatchXmlNode *xmlnode;
  char *dot;
  int len;
  GNode *n;

  dot = strchr (path, '.');
  len = dot ? dot - path : strlen (path);

  for (n = node->children; n; n = n->next)
  {
    xmlnode = (IpatchXmlNode *)(n->data);

    if (strncmp (xmlnode->name, path, len) == 0)
    {
      if (!dot) return (n);
      else return (ipatch_xml_find_by_path_recurse (n, dot + 1));
    }
  }

  return (NULL);
}

/**
 * ipatch_xml_to_str: (skip)
 * @node: XML node
 * @indent: Number of spaces of indent per level (0 for no indent)
 *
 * Render an XML tree to a string.
 *
 * Returns: Newly allocated string of XML content representing @node, free with
 *   g_free() when done using it.
 */
char *
ipatch_xml_to_str (GNode *node, guint indent)
{
  GString *str;

  g_return_val_if_fail (node != NULL, NULL);

  str = g_string_new ("");

  ipatch_xml_to_str_recurse (str, node, 0, indent);

  return (g_string_free (str, FALSE));
}

static void
ipatch_xml_to_str_recurse (GString *str, GNode *node, guint indent, guint inc)
{
  IpatchXmlNode *xmlnode;
  char *esc;
  GNode *n;
  int i;

  xmlnode = (IpatchXmlNode *)(node->data);

  for (i = 0; i < indent; i++)
    g_string_append_c (str, ' ');

  g_string_append_printf (str, "<%s", xmlnode->name);

  if (xmlnode->attributes)
  {
    IpatchXmlAttr *attr;
    GList *p;

    for (p = xmlnode->attributes; p; p = p->next)
    {
      attr = (IpatchXmlAttr *)(p->data);
      esc = g_markup_escape_text (attr->value, -1);	/* ++ alloc */
      g_string_append_printf (str, " %s=\"%s\"", attr->name, esc);
      g_free (esc);	/* -- free */
    }
  }

  if (!xmlnode->value && !node->children)
  {
    g_string_append (str, "/>\n");
    return;
  }
  else g_string_append (str, ">");

  if (xmlnode->value)
  {
    esc = g_markup_escape_text (xmlnode->value, -1);	/* ++ alloc */
    g_string_append (str, esc);
    g_free (esc);	/* -- free */
  }

  if (node->children)
  {
    g_string_append_c (str, '\n');

    for (n = node->children; n; n = n->next)
      ipatch_xml_to_str_recurse (str, n, indent + inc, inc);

    for (i = 0; i < indent; i++)
      g_string_append_c (str, ' ');
  }

  g_string_append_printf (str, "</%s>\n", xmlnode->name);
}

/**
 * ipatch_xml_save_to_file: (skip)
 * @node: XML tree to save
 * @indent: Number of spaces to indent per level
 * @filename: File name to save to
 * @err: Location to store error info or %NULL to ignore
 *
 * Store an XML tree to a file.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set)
 */
gboolean
ipatch_xml_save_to_file (GNode *node, guint indent, const char *filename,
                         GError **err)
{
  gboolean retval;
  char *s;

  s = ipatch_xml_to_str (node, indent);		/* ++ alloc */
  if (!s) return (FALSE);

  retval = g_file_set_contents (filename, s, -1, err);

  g_free (s);	/* -- free */

  return (retval);
}

/**
 * ipatch_xml_from_str: (skip)
 * @str: XML content to parse
 * @err: Location to store error info or %NULL to ignore
 *
 * Parse XML content into an XML node tree.
 *
 * Returns: Newly allocated XML node tree or
 *   %NULL on error (@err may be set), can be freed with ipatch_xml_destroy().
 */
GNode *
ipatch_xml_from_str (const char *str, GError **err)
{
  GMarkupParseContext *ctx;

  GMarkupParser parser = {
    start_element: xml_start_element,
    end_element: xml_end_element,
    text: xml_text
  };

  GNode *root = NULL;

  ctx = g_markup_parse_context_new (&parser, 0, &root, NULL);

  if (!g_markup_parse_context_parse (ctx, str, -1, err)
      || !g_markup_parse_context_end_parse (ctx, err))
  {
    g_markup_parse_context_free (ctx);

    if (root)
    {
      root = g_node_get_root (root);
      ipatch_xml_destroy (root);
    }

    return (NULL);
  }

  g_markup_parse_context_free (ctx);

  return (root);
}

static void
xml_start_element (GMarkupParseContext *context, const gchar *element_name,
                   const gchar **attribute_names, const gchar **attribute_values,
                   gpointer user_data, GError **error)
{
  GNode **node = (GNode **)user_data;
  *node = ipatch_xml_new_node_strv (*node, element_name, NULL,
                                    attribute_names, attribute_values);
}

static void
xml_end_element (GMarkupParseContext *context, const gchar *element_name,
                 gpointer user_data, GError **error)
{
  GNode **node = (GNode **)user_data;

  if ((*node)->parent)
    *node = (*node)->parent;
}

static void
xml_text (GMarkupParseContext *context, const gchar *text, gsize text_len,  
          gpointer user_data, GError **error)
{
  GNode **node = (GNode **)user_data;
  IpatchXmlNode *xmlnode;

  xmlnode = (IpatchXmlNode *)((*node)->data);
  g_free (xmlnode->value);
  xmlnode->value = g_strdup (text);
}

/**
 * ipatch_xml_load_from_file: (skip)
 * @filename: File name containing XML content to parse
 * @err: Location to store error info or %NULL to ignore
 *
 * Parse an XML file into an XML node tree.
 *
 * Returns: Newly allocated XML node tree
 *   or %NULL on error (@err may be set), can be freed with ipatch_xml_destroy().
 */
GNode *
ipatch_xml_load_from_file (const char *filename, GError **err)
{
  GNode *node;
  char *str;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (!err || !*err, NULL);

  if (!g_file_get_contents (filename, &str, NULL, err))		/* ++ alloc */
    return (NULL);

  node = ipatch_xml_from_str (str, err);

  g_free (str);

  return (node);
}

/**
 * ipatch_xml_node_new: (skip)
 *
 * Create a new XML node structure.  Not normally used.
 *
 * Returns: New XML node structure, which should be added to a GNode.
 */
IpatchXmlNode *
ipatch_xml_node_new (void)
{
  IpatchXmlNode *xmlnode;

  xmlnode = g_slice_new0 (IpatchXmlNode);
  g_datalist_init (&xmlnode->qdata);

  return (xmlnode);
}

/**
 * ipatch_xml_node_free: (skip)
 * @xmlnode: XML node structure to free
 *
 * Free an XML node structure and its contents.  Not normally used.
 */
void
ipatch_xml_node_free (IpatchXmlNode *xmlnode)
{
  GList *p;

  g_return_if_fail (xmlnode != NULL);

  g_free (xmlnode->name);
  g_free (xmlnode->value);

  g_datalist_clear (&xmlnode->qdata);

  for (p = xmlnode->attributes; p; p = g_list_delete_link (p, p))
    ipatch_xml_attr_free (p->data);

  g_slice_free (IpatchXmlNode, xmlnode);
}

/**
 * ipatch_xml_node_duplicate: (skip)
 * @xmlnode: XML node structure to duplicate
 *
 * Duplicate an XML node structure and its contents.  Not normally used.
 * Note that arbitrary user data assigned to the XML node will not be duplicated.
 *
 * Returns: New duplicate of @xmlnode.
 */
IpatchXmlNode *
ipatch_xml_node_duplicate (const IpatchXmlNode *xmlnode)
{
  IpatchXmlNode *dupnode;
  IpatchXmlAttr *dupattr;
  GList *p;

  g_return_val_if_fail (xmlnode != NULL, NULL);

  dupnode = ipatch_xml_node_new ();
  dupnode->name = g_strdup (xmlnode->name);
  dupnode->value = g_strdup (xmlnode->value);

  for (p = xmlnode->attributes; p; p = p->next)
  {
    dupattr = ipatch_xml_attr_duplicate (p->data);
    dupnode->attributes = g_list_prepend (dupnode->attributes, dupattr);
  }

  dupnode->attributes = g_list_reverse (dupnode->attributes);

  return (dupnode);
}

/**
 * ipatch_xml_attr_new: (skip)
 *
 * Create a new XML attribute structure.  Not normally used.
 *
 * Returns: New XML attribute structure which should be added to an #IpatchXmlNode
 *   attributes list.
 */
IpatchXmlAttr *
ipatch_xml_attr_new (void)
{
  return (g_slice_new0 (IpatchXmlAttr));
}

/**
 * ipatch_xml_attr_free: (skip)
 * @attr: Attribute structure to free
 *
 * Free an XML attribute structure.  Not normally used.
 */
void
ipatch_xml_attr_free (IpatchXmlAttr *attr)
{
  g_return_if_fail (attr != NULL);
  g_free (attr->name);
  g_free (attr->value);
  g_slice_free (IpatchXmlAttr, attr);
}

/**
 * ipatch_xml_attr_duplicate: (skip)
 * @attr: Attribute structure to duplicate
 *
 * Duplicate an XML attribute structure.  Not normally used.
 *
 * Returns: New duplicate attribute structure
 */
IpatchXmlAttr *
ipatch_xml_attr_duplicate (const IpatchXmlAttr *attr)
{
  IpatchXmlAttr *dupattr;
  
  g_return_val_if_fail (attr != NULL, NULL);

  dupattr = ipatch_xml_attr_new ();
  dupattr->name = g_strdup (attr->name);
  dupattr->value = g_strdup (attr->value);

  return (dupattr);
}
