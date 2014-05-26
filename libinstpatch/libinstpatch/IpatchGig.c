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
 * SECTION: IpatchGig
 * @short_description: GigaSampler instrument file object
 * @see_also: 
 * @stability: Stable
 *
 * Defines a GigaSampler instrument file object.
 */
#include <glib.h>
#include <glib-object.h>
#include "IpatchGig.h"
#include "IpatchGigInst.h"
#include "IpatchGigSample.h"
#include "IpatchContainer.h"
#include "IpatchVirtualContainer_types.h"
#include "i18n.h"

static void ipatch_gig_class_init (IpatchGigClass *klass);
static void ipatch_gig_finalize (GObject *object);
static void ipatch_gig_item_copy (IpatchItem *dest, IpatchItem *src,
				  IpatchItemCopyLinkFunc link_func,
				  gpointer user_data);
static const GType *ipatch_gig_container_child_types (void);
static const GType *ipatch_gig_container_virtual_types (void);

static GObjectClass *parent_class = NULL;
static GType gig_child_types[3] = { 0 };
static GType gig_virt_types[4] = { 0 };


GType
ipatch_gig_get_type (void)
{
  static GType item_type = 0;

  if (!item_type) {
    static const GTypeInfo item_info = {
      sizeof (IpatchGigClass), NULL, NULL,
      (GClassInitFunc) ipatch_gig_class_init, NULL, NULL,
      sizeof (IpatchGig), 0,
      (GInstanceInitFunc) NULL,
    };

    item_type = g_type_register_static (IPATCH_TYPE_DLS2,
					"IpatchGig", &item_info, 0);
  }

  return (item_type);
}

static void
ipatch_gig_class_init (IpatchGigClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);
  IpatchContainerClass *container_class = IPATCH_CONTAINER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  obj_class->finalize = ipatch_gig_finalize;

  item_class->copy = ipatch_gig_item_copy;

  container_class->child_types = ipatch_gig_container_child_types;
  container_class->virtual_types = ipatch_gig_container_virtual_types;

  gig_child_types[0] = IPATCH_TYPE_GIG_INST;
  gig_child_types[1] = IPATCH_TYPE_GIG_SAMPLE;

  gig_virt_types[0] = IPATCH_TYPE_VIRTUAL_GIG_MELODIC;
  gig_virt_types[1] = IPATCH_TYPE_VIRTUAL_GIG_PERCUSSION;
  gig_virt_types[2] = IPATCH_TYPE_VIRTUAL_GIG_SAMPLES;
}

static void
ipatch_gig_finalize (GObject *object)
{
  IpatchGig *gig = IPATCH_GIG (object);
  GSList *p;

  IPATCH_ITEM_WLOCK (object);

  /* free sample group names */
  p = gig->group_names;
  while (p)
    {
      g_free (p->data);
      p = g_slist_delete_link (p, p);
    }

  IPATCH_ITEM_WUNLOCK (object);

  parent_class->finalize (object);
}

static void
ipatch_gig_item_copy (IpatchItem *dest, IpatchItem *src,
		      IpatchItemCopyLinkFunc link_func,
		      gpointer user_data)
{
  IpatchGig *src_gig, *dest_gig;
  GSList *p;

  src_gig = IPATCH_GIG (src);
  dest_gig = IPATCH_GIG (dest);

  IPATCH_ITEM_CLASS (parent_class)->copy (dest, src, link_func, user_data);

  IPATCH_ITEM_RLOCK (src_gig);

  /* duplicate group names */
  p = src_gig->group_names;
  while (p)
    {
      dest_gig->group_names = g_slist_append (dest_gig->group_names,
					      g_strdup ((char *)(p->data)));
      p = g_slist_next (p);
    }

  IPATCH_ITEM_RUNLOCK (src_gig);
}

static const GType *
ipatch_gig_container_child_types (void)
{
  return (gig_child_types);
}

static const GType *
ipatch_gig_container_virtual_types (void)
{
  return (gig_virt_types);
}

/**
 * ipatch_gig_new:
 *
 * Create a new GigaSampler object.
 *
 * Returns: New GigaSampler object with a ref count of 1 which the caller
 * owns.
 */
IpatchGig *
ipatch_gig_new (void)
{
  return (IPATCH_GIG (g_object_new (IPATCH_TYPE_GIG, NULL)));
}
