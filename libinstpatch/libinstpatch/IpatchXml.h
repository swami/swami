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
#ifndef __IPATCH_XML_H__
#define __IPATCH_XML_H__

#include <glib.h>

typedef struct _IpatchXmlNode IpatchXmlNode;
typedef struct _IpatchXmlAttr IpatchXmlAttr;

/**
 * IpatchXmlAttr:
 * @name: Attribute name
 * @value: Attribute value
 *
 * Structure for storing an XML attribute.
 */
struct _IpatchXmlAttr
{
  char *name;
  char *value;
};

/**
 * IpatchXmlNode:
 * @name: XML element name
 * @value: XML text value or %NULL
 * @attributes: Linked list of #IpatchXmlAttr structures
 *
 * An XML element node.  Note that a given node can contain only one text value.
 */
struct _IpatchXmlNode
{
  char *name;		/* XML element name */
  char *value;		/* Text content of element */
  GData *qdata;		/* To associate arbitrary data with XML nodes */
  GList *attributes;	/* List of IpatchXmlAttr structures */
};

GNode *ipatch_xml_new_node (GNode *parent, const char *name, const char *value,
                            const char *attr_name, ...);
GNode *ipatch_xml_new_node_strv (GNode *parent, const char *name, const char *value,
                                 const char **attr_names, const char **attr_values);
gpointer ipatch_xml_get_data (GNode *node, const char *key);
void ipatch_xml_set_data (GNode *node, const char *key, gpointer data);
void ipatch_xml_set_data_full (GNode *node, const char *key, gpointer data,
                               GDestroyNotify destroy_func);
gpointer ipatch_xml_steal_data (GNode *node, const char *key);
gpointer ipatch_xml_get_qdata (GNode *node, GQuark quark);
void ipatch_xml_set_qdata (GNode *node, GQuark quark, gpointer data);
void ipatch_xml_set_qdata_full (GNode *node, GQuark quark, gpointer data,
                                GDestroyNotify destroy_func);
gpointer ipatch_xml_steal_qdata (GNode *node, GQuark quark);
void ipatch_xml_destroy (GNode *node);
GNode *ipatch_xml_copy (GNode *node);
void ipatch_xml_set_name (GNode *node, const char *name);
void ipatch_xml_set_value (GNode *node, const char *value);
void ipatch_xml_set_value_printf (GNode *node, const char *format, ...);
void ipatch_xml_take_name (GNode *node, char *name);
void ipatch_xml_take_value (GNode *node, char *value);
G_CONST_RETURN char *ipatch_xml_get_name (GNode *node);
gboolean ipatch_xml_test_name (GNode *node, const char *cmpname);
G_CONST_RETURN char *ipatch_xml_get_value (GNode *node);
char *ipatch_xml_dup_value (GNode *node);
gboolean ipatch_xml_test_value (GNode *node, const char *cmpvalue);
void ipatch_xml_set_attribute (GNode *node, const char *attr_name,
                               const char *attr_value);
void ipatch_xml_set_attributes (GNode *node, const char *attr_name,
                                const char *attr_value, const char *attr2_name, ...);
G_CONST_RETURN char *ipatch_xml_get_attribute (GNode *node, const char *attr_name);
gboolean ipatch_xml_test_attribute (GNode *node, const char *attr_name,
                                    const char *cmpval);
GNode *ipatch_xml_find_child (GNode *node, const char *name);
GNode *ipatch_xml_find_by_path (GNode *node, const char *path);
char *ipatch_xml_to_str (GNode *node, guint indent);
gboolean ipatch_xml_save_to_file (GNode *node, guint indent, const char *filename,
                                  GError **err);
GNode *ipatch_xml_from_str (const char *str, GError **err);
GNode *ipatch_xml_load_from_file (const char *filename, GError **err);

IpatchXmlNode *ipatch_xml_node_new (void);
void ipatch_xml_node_free (IpatchXmlNode *xmlnode);
IpatchXmlNode *ipatch_xml_node_duplicate (const IpatchXmlNode *xmlnode);
IpatchXmlAttr *ipatch_xml_attr_new (void);
void ipatch_xml_attr_free (IpatchXmlAttr *attr);
IpatchXmlAttr *ipatch_xml_attr_duplicate (const IpatchXmlAttr *attr);

#endif
