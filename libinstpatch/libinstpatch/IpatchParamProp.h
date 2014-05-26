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
#ifndef __IPATCH_PARAM_PROP_H__
#define __IPATCH_PARAM_PROP_H__

#include <glib.h>
#include <glib-object.h>

/* libInstPatch GParamSpec->flags */
enum 
{
/* Parameter should be unique amongst siblings */
  IPATCH_PARAM_UNIQUE = (1 << G_PARAM_USER_SHIFT),

/* Hint that a property should be hidden in user interfaces */
  IPATCH_PARAM_HIDE = (1 << (G_PARAM_USER_SHIFT + 1)),

/* Indicates that property affects audio synthesis */
  IPATCH_PARAM_SYNTH = (1 << (G_PARAM_USER_SHIFT + 2)),

/* Indicates that property can be a real time synthesis parameter */
  IPATCH_PARAM_SYNTH_REALTIME = (1 << (G_PARAM_USER_SHIFT + 3)),

/* Used for properties which don't modify the saveable state of an object.
 * The object's base object save dirty flag wont get set. */
  IPATCH_PARAM_NO_SAVE_CHANGE = (1 << (G_PARAM_USER_SHIFT + 4)),

  /* Indicates that property should not be saved as object state (XML for example) */
  IPATCH_PARAM_NO_SAVE = (1 << (G_PARAM_USER_SHIFT + 5))
};


/* next shift value usable by libInstPatch user in GParamSpec->flags */
#define IPATCH_PARAM_USER_SHIFT  (G_PARAM_USER_SHIFT + 12)


void ipatch_param_install_property (GParamSpec *prop_spec);
GParamSpec *ipatch_param_find_property (const char *name);
GParamSpec **ipatch_param_list_properties (guint *n_properties);
GParamSpec *ipatch_param_set (GParamSpec *spec,
			      const char *first_property_name, ...);
void ipatch_param_set_valist (GParamSpec *spec, const char *first_property_name,
			      va_list args);
void ipatch_param_set_property (GParamSpec *spec, const char *property_name,
				const GValue *value);
void ipatch_param_get (GParamSpec *spec, const char *first_property_name, ...);
void ipatch_param_get_valist (GParamSpec *spec, const char *first_property_name,
			      va_list args);
gboolean ipatch_param_get_property (GParamSpec *spec, const char *property_name,
				    GValue *value);

#endif
