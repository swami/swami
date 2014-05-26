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
#ifndef __IPATCH_VIRTUAL_CONTAINER_H__
#define __IPATCH_VIRTUAL_CONTAINER_H__

#include <glib.h>
#include <glib-object.h>

/* forward type declarations */

typedef struct _IpatchVirtualContainer IpatchVirtualContainer;
typedef struct _IpatchVirtualContainerClass IpatchVirtualContainerClass;

#include <libinstpatch/IpatchItem.h>

#define IPATCH_TYPE_VIRTUAL_CONTAINER   (ipatch_virtual_container_get_type ())
#define IPATCH_VIRTUAL_CONTAINER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_VIRTUAL_CONTAINER, \
   IpatchVirtualContainer))
#define IPATCH_VIRTUAL_CONTAINER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_VIRTUAL_CONTAINER, \
   IpatchVirtualContainerClass))
#define IPATCH_IS_VIRTUAL_CONTAINER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_VIRTUAL_CONTAINER))

/**
 * IPATCH_VIRTUAL_CONTAINER_CREATE:
 * @type_under: Type string in the form "type_name"
 * @TypeCase: Type string in the form "TypeName"
 * @name: Name type property
 * @blurb: Blurb type property (more detailed description)
 * @childtype: Child type of this virtual container type
 */
#define IPATCH_VIRTUAL_CONTAINER_CREATE(type_under, TypeCase, name, blurb, childtype) \
GType type_under##_get_type (void) \
{ \
  static GType obj_type = 0; \
 \
  if (!obj_type) { \
    static const GTypeInfo obj_info = { \
      sizeof (IpatchVirtualContainerClass), NULL, NULL, \
      (GClassInitFunc) NULL, NULL, NULL, \
      sizeof (IpatchVirtualContainer), 0, \
      (GInstanceInitFunc) NULL, \
    }; \
 \
    obj_type = g_type_register_static (IPATCH_TYPE_VIRTUAL_CONTAINER, \
				       #TypeCase, &obj_info, 0); \
    ipatch_type_set (obj_type, \
		     "name", name, \
		     "blurb", blurb, \
		     "virtual-child-type", childtype, NULL); \
  } \
 \
  return (obj_type); \
}

/* virtual container object */
struct _IpatchVirtualContainer
{
  IpatchItem parent_instance; /* derived from IpatchItem */
};

/* virtual container class */
struct _IpatchVirtualContainerClass
{
  IpatchItemClass parent_class;
};


/**
 * IpatchVirtualContainerConformFunc:
 * @object: Object to conform to a virtual container criteria.
 *
 * A function type used to make an item conform to the criteria of a virtual
 * container (force a SoundFont preset to be a percussion preset for example).
 */
typedef void (*IpatchVirtualContainerConformFunc)(GObject *object);


GType ipatch_virtual_container_get_type (void);

#endif
