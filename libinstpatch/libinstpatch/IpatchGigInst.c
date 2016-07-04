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
 * SECTION: IpatchGigInst
 * @short_description: GigaSampler instrument object
 * @see_also: 
 * @stability: Stable
 *
 * GigaSampler instrument objects are the toplevel instrument objects in a
 * GigaSampler file.
 */
#include <stdarg.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchGigInst.h"
#include "IpatchGigRegion.h"
#include "IpatchGigFile.h"
#include "IpatchGigFile_priv.h"
#include "i18n.h"

enum
{
  PROP_0
};

static void ipatch_gig_inst_class_init (IpatchGigInstClass *klass);
static void ipatch_gig_inst_init (IpatchGigInst *inst);
static void ipatch_gig_inst_set_property (GObject *object,
					  guint property_id,
					  const GValue *value,
					  GParamSpec *pspec);
static void ipatch_gig_inst_get_property (GObject *object,
					  guint property_id,
					  GValue *value,
					  GParamSpec *pspec);
static void ipatch_gig_inst_item_copy (IpatchItem *dest, IpatchItem *src,
				       IpatchItemCopyLinkFunc link_func,
				       gpointer user_data);

static const GType *ipatch_gig_inst_container_child_types (void);
static gboolean
ipatch_gig_inst_container_init_iter (IpatchContainer *container,
				     IpatchIter *iter, GType type);

static gpointer parent_class = NULL;
static GType inst_child_types[2] = { 0 };


GType
ipatch_gig_inst_get_type (void)
{
  static GType item_type = 0;

  if (!item_type) {
    static const GTypeInfo item_info = {
      sizeof (IpatchGigInstClass), NULL, NULL,
      (GClassInitFunc)ipatch_gig_inst_class_init, NULL, NULL,
      sizeof (IpatchGigInst), 0,
      (GInstanceInitFunc)ipatch_gig_inst_init,
    };

    item_type = g_type_register_static (IPATCH_TYPE_DLS2_INST,
					"IpatchGigInst", &item_info, 0);
  }

  return (item_type);
}

static void
ipatch_gig_inst_class_init (IpatchGigInstClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);
  IpatchContainerClass *container_class = IPATCH_CONTAINER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  obj_class->get_property = ipatch_gig_inst_get_property;

  /* we use the IpatchItem item_set_property method */
  item_class->item_set_property = ipatch_gig_inst_set_property;
  item_class->copy = ipatch_gig_inst_item_copy;

  container_class->child_types = ipatch_gig_inst_container_child_types;
  container_class->init_iter = ipatch_gig_inst_container_init_iter;

  inst_child_types[0] = IPATCH_TYPE_GIG_REGION;
}

static void
ipatch_gig_inst_init (IpatchGigInst *inst)
{
  guint8 def_3ewg[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x38, 0x23 };

  /* FIXME - Proper default values? */
  inst->attenuate = 0;
  inst->effect_send = 0;
  inst->fine_tune = 0;
  inst->pitch_bend_range = 2;	/* 2 semitones */
  inst->dim_key_start = 0;
  inst->dim_key_end = 0;

  /* FIXME - What is it for? */
  memcpy (inst->chunk_3ewg, def_3ewg, IPATCH_GIG_3EWG_SIZE);
}

static void
ipatch_gig_inst_set_property (GObject *object, guint property_id,
			      const GValue *value, GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      return;
    }
}

static void
ipatch_gig_inst_get_property (GObject *object, guint property_id,
			      GValue *value, GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


static void
ipatch_gig_inst_item_copy (IpatchItem *dest, IpatchItem *src,
			   IpatchItemCopyLinkFunc link_func,
			   gpointer user_data)
{
  IpatchGigInst *src_inst, *dest_inst;

  src_inst = IPATCH_GIG_INST (src);
  dest_inst = IPATCH_GIG_INST (dest);

  /* call IpatchDLS2Inst class copy function */
  IPATCH_ITEM_CLASS (parent_class)->copy (dest, src, link_func, user_data);

  /* don't need to lock for this stuff */
  dest_inst->attenuate = src_inst->attenuate;
  dest_inst->effect_send = src_inst->effect_send;
  dest_inst->fine_tune = src_inst->fine_tune;
  dest_inst->pitch_bend_range = src_inst->pitch_bend_range;
  dest_inst->dim_key_start = src_inst->dim_key_start;
  dest_inst->dim_key_end = src_inst->dim_key_end;

  IPATCH_ITEM_RLOCK (src_inst);
  memcpy (dest_inst->chunk_3ewg, src_inst->chunk_3ewg, IPATCH_GIG_3EWG_SIZE);
  IPATCH_ITEM_RUNLOCK (src_inst);
}

static const GType *
ipatch_gig_inst_container_child_types (void)
{
  return (inst_child_types);
}

/* container is locked by caller */
static gboolean
ipatch_gig_inst_container_init_iter (IpatchContainer *container,
				     IpatchIter *iter, GType type)
{
  IpatchGigInst *inst = IPATCH_GIG_INST (container);

  if (!g_type_is_a (type, IPATCH_TYPE_GIG_REGION))
    {
      g_critical ("Invalid child type '%s' for parent of type '%s'",
		  g_type_name (type), g_type_name (G_OBJECT_TYPE (container)));
      return (FALSE);
    }

  ipatch_iter_GSList_init (iter, &((IpatchDLS2Inst *)inst)->regions);
  return (TRUE);
}

/**
 * ipatch_gig_inst_new:
 *
 * Create a new GigaSampler instrument object.
 *
 * Returns: New GigaSampler instrument with a reference count of 1. Caller
 * owns the reference and removing it will destroy the item, unless another
 * reference is added (if its parented for example).
 */
IpatchGigInst *
ipatch_gig_inst_new (void)
{
  return (IPATCH_GIG_INST (g_object_new (IPATCH_TYPE_GIG_INST, NULL)));
}

/**
 * ipatch_gig_inst_first: (skip)
 * @iter: Patch item iterator containing #IpatchGigInst items
 *
 * Gets the first item in a GigaSampler instrument iterator. A convenience
 * wrapper for ipatch_iter_first().
 *
 * Returns: The first GigaSampler instrument in @iter or %NULL if empty.
 */
IpatchGigInst *
ipatch_gig_inst_first (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_first (iter);
  if (obj) return (IPATCH_GIG_INST (obj));
  else return (NULL);
}

/**
 * ipatch_gig_inst_next: (skip)
 * @iter: Patch item iterator containing #IpatchGigInst items
 *
 * Gets the next item in a GigaSampler instrument iterator. A convenience
 * wrapper for ipatch_iter_next().
 *
 * Returns: The next GigaSampler instrument in @iter or %NULL if at
 * the end of the list.
 */
IpatchGigInst *
ipatch_gig_inst_next (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_next (iter);
  if (obj) return (IPATCH_GIG_INST (obj));
  else return (NULL);
}
