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
 * SECTION: IpatchVirtualContainer
 * @short_description: Virtual container object
 * @see_also: 
 * @stability: Stable
 *
 * Virtual containers are used in user interfaces to group items in
 * containers that aren't actually present in the hierarchy, such as
 * "Instruments", "Melodic Presets", "Percussion Presets" in SF2 files.
 */
#include <stdio.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchVirtualContainer.h"
#include "IpatchTypeProp.h"

GType
ipatch_virtual_container_get_type (void)
{
  static GType item_type = 0;

  if (!item_type)
    {
      static const GTypeInfo item_info =
	{
	  sizeof (IpatchVirtualContainerClass), NULL, NULL,
	  (GClassInitFunc) NULL, NULL, NULL,
	  sizeof (IpatchVirtualContainer), 0,
	  (GInstanceInitFunc) NULL,
	};

      item_type = g_type_register_static (IPATCH_TYPE_ITEM,
					  "IpatchVirtualContainer",
					  &item_info, G_TYPE_FLAG_ABSTRACT);
    }

  return (item_type);
}
