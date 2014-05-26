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
 * SECTION: IpatchGigDimension
 * @short_description: GigaSampler dimension object
 * @see_also: #IpatchGigInst
 * @stability: Stable
 *
 * Defines a GigaSampler dimension object which are the children of
 * #IpatchGigInst objects.
 */
#include <glib.h>
#include <glib-object.h>
#include "IpatchGigDimension.h"
#include "IpatchRange.h"
#include "IpatchTypeProp.h"
#include "ipatch_priv.h"
#include "builtin_enums.h"

enum
{
  PROP_0,

  PROP_NAME,
  PROP_TYPE,
  PROP_SPLIT_COUNT
};

static void ipatch_gig_dimension_class_init (IpatchGigDimensionClass *klass);
static void ipatch_gig_dimension_set_property (GObject *object,
					       guint property_id,
					       const GValue *value,
					       GParamSpec *pspec);
static void ipatch_gig_dimension_get_property (GObject *object,
					       guint property_id,
					       GValue *value,
					       GParamSpec *pspec);

static void ipatch_gig_dimension_init (IpatchGigDimension *gig_dimension);
static void ipatch_gig_dimension_finalize (GObject *gobject);
static void ipatch_gig_dimension_item_copy (IpatchItem *dest, IpatchItem *src,
					    IpatchItemCopyLinkFunc link_func,
					    gpointer user_data);

static GObjectClass *parent_class = NULL;


GType
ipatch_gig_dimension_get_type (void)
{
  static GType item_type = 0;

  if (!item_type) {
    static const GTypeInfo item_info = {
      sizeof (IpatchGigDimensionClass), NULL, NULL,
      (GClassInitFunc)ipatch_gig_dimension_class_init, NULL, NULL,
      sizeof (IpatchGigDimension), 0,
      (GInstanceInitFunc)ipatch_gig_dimension_init,
    };

    item_type = g_type_register_static (IPATCH_TYPE_ITEM,
					"IpatchGigDimension", &item_info, 0);
  }

  return (item_type);
}

static void
ipatch_gig_dimension_class_init (IpatchGigDimensionClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  obj_class->finalize = ipatch_gig_dimension_finalize;
  obj_class->get_property = ipatch_gig_dimension_get_property;

  item_class->item_set_property = ipatch_gig_dimension_set_property;
  item_class->copy = ipatch_gig_dimension_item_copy;

  /* use parent's mutex (IpatchGigRegion) */
  item_class->mutex_slave = TRUE;

  g_object_class_override_property (obj_class, PROP_NAME, "title");

  g_object_class_install_property (obj_class, PROP_NAME,
	    g_param_spec_string ("name", _("Name"),
				 _("Dimension name"),
				 NULL, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_TYPE,
	    g_param_spec_enum ("type", _("Type"),
			       _("Dimension type"),
			       IPATCH_TYPE_GIG_DIMENSION_TYPE,
			       IPATCH_GIG_DIMENSION_NONE,
			       G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_TYPE,
	    g_param_spec_int ("split-count", _("Split count"),
			      _("Number of split bits"),
			      1, 5, 1, G_PARAM_READWRITE));
}

static void
ipatch_gig_dimension_set_property (GObject *object, guint property_id,
				   const GValue *value, GParamSpec *pspec)
{
  IpatchGigDimension *dimension = IPATCH_GIG_DIMENSION (object);

  switch (property_id)
    {
    case PROP_NAME:
      IPATCH_ITEM_WLOCK (dimension);
      g_free (dimension->name);
      dimension->name = g_value_dup_string (value);
      IPATCH_ITEM_WUNLOCK (dimension);

      /* title property notify */
      ipatch_item_prop_notify ((IpatchItem *)dimension, ipatch_item_pspec_title,
			       value, NULL);
      break;
    case PROP_TYPE:
      dimension->type = g_value_get_enum (value);
      break;
    case PROP_SPLIT_COUNT:
      dimension->split_count = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      return;
    }
}

static void
ipatch_gig_dimension_get_property (GObject *object, guint property_id,
				   GValue *value, GParamSpec *pspec)
{
  IpatchGigDimension *dimension = IPATCH_GIG_DIMENSION (object);

  switch (property_id)
    {
    case PROP_NAME:
      IPATCH_ITEM_RLOCK (dimension);
      g_value_set_string (value, dimension->name);
      IPATCH_ITEM_RUNLOCK (dimension);
      break;
    case PROP_TYPE:
      g_value_set_enum (value, dimension->type);
      break;
    case PROP_SPLIT_COUNT:
      g_value_set_int (value, dimension->split_count);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ipatch_gig_dimension_init (IpatchGigDimension *dimension)
{
}

static void
ipatch_gig_dimension_finalize (GObject *gobject)
{
  IpatchGigDimension *dimension = IPATCH_GIG_DIMENSION (gobject);

  IPATCH_ITEM_WLOCK (dimension);
  g_free (dimension->name);
  dimension->name = NULL;
  IPATCH_ITEM_WUNLOCK (dimension);

  if (parent_class->finalize)
    parent_class->finalize (gobject);
}

static void
ipatch_gig_dimension_item_copy (IpatchItem *dest, IpatchItem *src,
				IpatchItemCopyLinkFunc link_func,
				gpointer user_data)
{
  IpatchGigDimension *src_dim, *dest_dim;

  src_dim = IPATCH_GIG_DIMENSION (src);
  dest_dim = IPATCH_GIG_DIMENSION (dest);

  IPATCH_ITEM_RLOCK (src_dim);

  dest_dim->name = g_strdup (src_dim->name);
  dest_dim->type = src_dim->type;
  dest_dim->split_count = src_dim->split_count;
  dest_dim->split_mask = src_dim->split_mask;
  dest_dim->split_shift = src_dim->split_shift;

  IPATCH_ITEM_RUNLOCK (src_dim);
}

/**
 * ipatch_gig_dimension_new:
 *
 * Create a new GigaSampler instrument dimension.
 *
 * Returns: New GigaSampler dimension with a ref count of 1 which the caller
 * owns.
 */
IpatchGigDimension *
ipatch_gig_dimension_new (void)
{
  return (IPATCH_GIG_DIMENSION (g_object_new (IPATCH_TYPE_GIG_DIMENSION, NULL)));
}

/**
 * ipatch_gig_dimension_first: (skip)
 * @iter: Patch item iterator containing #IpatchGigDimension items
 *
 * Gets the first item in a dimension iterator. A convenience
 * wrapper for ipatch_iter_first().
 *
 * Returns: The first dimension in @iter or %NULL if empty.
 */
IpatchGigDimension *
ipatch_gig_dimension_first (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_first (iter);
  if (obj) return (IPATCH_GIG_DIMENSION (obj));
  else return (NULL);
}

/**
 * ipatch_gig_dimension_next: (skip)
 * @iter: Patch item iterator containing #IpatchGigDimension items
 *
 * Gets the next item in a dimension iterator. A convenience wrapper
 * for ipatch_iter_next().
 *
 * Returns: The next dimension in @iter or %NULL if at the end of
 *   the list.
 */
IpatchGigDimension *
ipatch_gig_dimension_next (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_next (iter);
  if (obj) return (IPATCH_GIG_DIMENSION (obj));
  else return (NULL);
}
