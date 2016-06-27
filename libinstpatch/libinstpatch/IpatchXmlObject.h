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
#ifndef __IPATCH_XML_OBJECT_H__
#define __IPATCH_XML_OBJECT_H__

#include <glib.h>
#include <glib-object.h>

/**
 * IpatchXmlEncodeFunc:
 * @node: XML node to encode XML to
 * @object: Object being encoded (object and property encoders)
 * @pspec: Spec of property being encoded (property encoders only)
 * @value: Value to encode (property and GValue encoders only)
 * @err: Location to store error value (or %NULL if ignoring)
 *
 * Function type for encoding objects, properties or GValue types to XML trees.
 * Forms the basis of serializing GObject and GValues to XML.  The caller
 * handles creating an XML node element to contain the given object, property or
 * value XML encoding.
 *
 * Returns: Should return %TRUE on success and %FALSE on error (in which
 *   case @err should be set).
 */
typedef gboolean (*IpatchXmlEncodeFunc)(GNode *node, GObject *object,
                                        GParamSpec *pspec, GValue *value,
                                        GError **err);
/**
 * IpatchXmlDecodeFunc:
 * @node: XML node to be decoded
 * @object: Object being decoded to (object and property decoders, %NULL otherwise)
 * @pspec: Spec of property being decoded (property decoders only, %NULL otherwise)
 * @value: Value to decode to (property and GValue decoders only, %NULL otherwise)
 * @err: Location to store error value (or %NULL if ignoring)
 *
 * Function type for decoding objects, properties or GValue types from XML trees to
 * their original instance values in memory.  For object decoders, only @object
 * will be set and the decoded XML content should be assigned to the object;
 * for property decoders @object, @pspec and @value will be set, @value is
 * initialized to the property value type and the decoded value should be
 * assigned to it; for GValue decoders, only the @value will be initialized
 * which the decoded value should be assigned to.
 *
 * Returns: Should return TRUE on success, FALSE otherwise (in which case @err
 *   should be set)
 */
typedef gboolean (*IpatchXmlDecodeFunc)(GNode *node, GObject *object,
                                        GParamSpec *pspec, GValue *value,
                                        GError **err);

void ipatch_xml_register_handler (GType type, const char *prop_name,
                                  IpatchXmlEncodeFunc encode_func,
                                  IpatchXmlDecodeFunc decode_func);
void
ipatch_xml_register_handler_full (GType type, const char *prop_name,
                                  IpatchXmlEncodeFunc encode_func,
                                  IpatchXmlDecodeFunc decode_func,
                                  GDestroyNotify notify_func, gpointer user_data);
gboolean ipatch_xml_lookup_handler (GType type, GParamSpec *pspec,
                                    IpatchXmlEncodeFunc *encode_func,
                                    IpatchXmlDecodeFunc *decode_func);
gboolean ipatch_xml_lookup_handler_by_prop_name (GType type, const char *prop_name,
                                                 IpatchXmlEncodeFunc *encode_func,
                                                 IpatchXmlDecodeFunc *decode_func);

gboolean ipatch_xml_encode_object (GNode *node, GObject *object,
			           gboolean create_element, GError **err);
gboolean ipatch_xml_encode_property (GNode *node, GObject *object, GParamSpec *pspec,
                                     gboolean create_element, GError **err);
gboolean ipatch_xml_encode_property_by_name (GNode *node, GObject *object,
                                             const char *propname,
                                             gboolean create_element, GError **err);
gboolean ipatch_xml_encode_value (GNode *node, GValue *value, GError **err);

gboolean ipatch_xml_decode_object (GNode *node, GObject *object, GError **err);
gboolean ipatch_xml_decode_property (GNode *node, GObject *object, GParamSpec *pspec,
                                     GError **err);
gboolean ipatch_xml_decode_property_by_name (GNode *node, GObject *object,
                                             const char *propname, GError **err);
gboolean ipatch_xml_decode_value (GNode *node, GValue *value, GError **err);

gboolean ipatch_xml_default_encode_object_func (GNode *node, GObject *object,
                                                GParamSpec *pspec, GValue *value,
                                                GError **err);
gboolean ipatch_xml_default_encode_property_func (GNode *node, GObject *object,
                                                  GParamSpec *pspec, GValue *value,
                                                  GError **err);
gboolean ipatch_xml_default_encode_value_func (GNode *node, GObject *object,
                                               GParamSpec *pspec, GValue *value,
                                               GError **err);
gboolean ipatch_xml_default_decode_object_func (GNode *node, GObject *object,
                                                GParamSpec *pspec, GValue *value,
                                                GError **err);
gboolean ipatch_xml_default_decode_property_func (GNode *node, GObject *object,
                                                  GParamSpec *pspec, GValue *value,
                                                  GError **err);
gboolean ipatch_xml_default_decode_value_func (GNode *node, GObject *object,
                                               GParamSpec *pspec, GValue *value,
                                               GError **err);
#endif
