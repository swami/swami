/*
 * SwamiContainer.c - Root container for instrument patches
 *
 * Swami
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License only.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA or point your web browser to http://www.gnu.org.
 */
#include "SwamiContainer.h"

static void swami_container_class_init (SwamiContainerClass *klass);
static const GType *swami_container_child_types (void);
static gboolean swami_container_init_iter (IpatchContainer *container,
					   IpatchIter *iter, GType type);

static GType container_child_types[2] = { 0 };


GType
swami_container_get_type (void)
{
  static GType item_type = 0;

  if (!item_type)
    {
      static const GTypeInfo item_info =
	{
	  sizeof (SwamiContainerClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) swami_container_class_init, NULL, NULL,
	  sizeof (SwamiContainer), 0,
	  (GInstanceInitFunc) NULL,
	};

      item_type = g_type_register_static (IPATCH_TYPE_CONTAINER,
					  "SwamiContainer", &item_info, 0);
    }

  return (item_type);
}

static void
swami_container_class_init (SwamiContainerClass *klass)
{
  IpatchContainerClass *container_class = IPATCH_CONTAINER_CLASS (klass);

  container_class->child_types = swami_container_child_types;
  container_class->init_iter = swami_container_init_iter;

  container_child_types[0] = IPATCH_TYPE_BASE;
}

static const GType *
swami_container_child_types (void)
{
  return (container_child_types);
}

/* container is locked by caller */
static gboolean
swami_container_init_iter (IpatchContainer *container,
			   IpatchIter *iter, GType type)
{
  SwamiContainer *scontainer = SWAMI_CONTAINER (container);

  if (type != IPATCH_TYPE_BASE)
    {
      g_critical ("Invalid child type '%s' for parent of type '%s'",
		  g_type_name (type), g_type_name (G_OBJECT_TYPE (container)));
      return (FALSE);
    }

  ipatch_iter_GSList_init (iter, &scontainer->patch_list);

  return (TRUE);
}

/**
 * swami_container_new:
 *
 * Create a new Swami container object which is a toplevel container for
 * instrument patches.
 *
 * Returns: New Swami container object
 */
SwamiContainer *
swami_container_new (void)
{
  return SWAMI_CONTAINER (g_object_new (SWAMI_TYPE_CONTAINER, NULL));
}
