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
#ifndef __IPATCH_TYPE_PROP_H__
#define __IPATCH_TYPE_PROP_H__

#include <glib.h>
#include <glib-object.h>

/* some built in type categories for "category" property */
enum
{
  IPATCH_CATEGORY_NONE,			/* a NULL value */
  IPATCH_CATEGORY_BASE,			/* patch file IpatchBase type */
  IPATCH_CATEGORY_PROGRAM,		/* a MIDI program type (MIDI locale) */
  IPATCH_CATEGORY_INSTRUMENT,		/* an instrument type (no MIDI locale) */
  IPATCH_CATEGORY_INSTRUMENT_REF,	/* a type referencing an Instrument */
  IPATCH_CATEGORY_SAMPLE,		/* sample type */
  IPATCH_CATEGORY_SAMPLE_REF		/* a type referencing a sample */
};

/* enum for "splits-type" to indicate that a type has
   key-range or velocity-range parameters or its children do */
typedef enum
{
  IPATCH_SPLITS_NONE,			/* type does not have splits */
  IPATCH_SPLITS_NORMAL,			/* normal splits */
  IPATCH_SPLITS_NO_OVERLAP		/* splits do not overlap */
} IpatchSplitsType;

/**
 * IpatchTypePropGetFunc:
 * @type: The GType of the type property
 * @spec: The parameter specification
 * @value: Initialized value to store the type prop value in
 * @object: Object instance (might be %NULL)
 *
 * A function type used for active type property callbacks.
 * Allows for dynamic type properties that can return values depending
 * on an object's state.
 */
typedef void (*IpatchTypePropGetFunc)(GType type, GParamSpec *spec,
				      GValue *value, GObject *object);

void ipatch_type_install_property (GParamSpec *prop_spec);
GParamSpec *ipatch_type_find_property (const char *name);
GParamSpec **ipatch_type_list_properties (guint *n_properties);
GType *ipatch_type_find_types_with_property (const char *name, const GValue *value,
                                             guint *n_types);
void ipatch_type_set (GType type, const char *first_property_name, ...);
void ipatch_type_set_valist (GType type, const char *first_property_name,
			     va_list args);
void ipatch_type_set_property (GType type, const char *property_name,
			       const GValue *value);
void ipatch_type_unset_property (GType type, const char *property_name);
void ipatch_type_get (GType type, const char *first_property_name, ...);
void ipatch_type_get_valist (GType type, const char *first_property_name,
			     va_list args);
void ipatch_type_get_property (GType type, const char *property_name,
			       GValue *value);
void ipatch_type_object_get (GObject *object, const char *first_property_name,
			     ...);
void ipatch_type_object_get_valist (GObject *object,
				    const char *first_property_name,
				    va_list args);
void ipatch_type_object_get_property (GObject *object,
				      const char *property_name, GValue *value);
void ipatch_type_set_dynamic_func (GType type, const char *property_name,
				   IpatchTypePropGetFunc func);
void ipatch_type_set_dynamic_func_full (GType type, const char *property_name,
			                IpatchTypePropGetFunc func,
                                        GDestroyNotify notify_func, gpointer user_data);
IpatchTypePropGetFunc ipatch_type_get_dynamic_func (GType type,
						    const char *property_name);
#endif
