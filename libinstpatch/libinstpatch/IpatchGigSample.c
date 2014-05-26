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
 * SECTION: IpatchGigSample
 * @short_description: GigaSampler sample object
 * @see_also: #IpatchGig
 * @stability: Stable
 *
 * Object defining a GigaSampler sample object.  Child of #IpatchGig objects
 * and referenced by #IpatchGigSubRegion objects.
 */
#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchGigSample.h"
#include "i18n.h"

enum
{
  PROP_0,
  PROP_GROUP_NUMBER
};

static void ipatch_gig_sample_class_init (IpatchGigSampleClass *klass);
static void ipatch_gig_sample_init (IpatchGigSample *sample);
static void ipatch_gig_sample_set_property (GObject *object,
					    guint property_id,
					    const GValue *value,
					    GParamSpec *pspec);
static void ipatch_gig_sample_get_property (GObject *object,
					    guint property_id,
					    GValue *value,
					    GParamSpec *pspec);
static void ipatch_gig_sample_item_copy (IpatchItem *dest, IpatchItem *src,
					 IpatchItemCopyLinkFunc link_func,
					 gpointer user_data);

static gpointer parent_class = NULL;


GType
ipatch_gig_sample_get_type (void)
{
  static GType item_type = 0;

  if (!item_type) {
    static const GTypeInfo item_info = {
      sizeof (IpatchGigSampleClass), NULL, NULL,
      (GClassInitFunc)ipatch_gig_sample_class_init, NULL, NULL,
      sizeof (IpatchGigSample), 0,
      (GInstanceInitFunc)ipatch_gig_sample_init,
    };

    item_type = g_type_register_static (IPATCH_TYPE_DLS2_SAMPLE,
					"IpatchGigSample", &item_info, 0);
  }

  return (item_type);
}

static void
ipatch_gig_sample_class_init (IpatchGigSampleClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  obj_class->get_property = ipatch_gig_sample_get_property;

  /* we use the IpatchItem item_set_property method */
  item_class->item_set_property = ipatch_gig_sample_set_property;
  item_class->copy = ipatch_gig_sample_item_copy;

  g_object_class_install_property (obj_class, PROP_GROUP_NUMBER,
			g_param_spec_uint ("group-number", _("Group number"),
					   _("Sample group index"),
					   0, G_MAXUINT, 0,
					   G_PARAM_READWRITE));
}

static void
ipatch_gig_sample_init (IpatchGigSample *sample)
{
  sample->group_number = 0;
}

static void
ipatch_gig_sample_set_property (GObject *object, guint property_id,
				const GValue *value, GParamSpec *pspec)
{
  IpatchGigSample *sample = IPATCH_GIG_SAMPLE (object);

  switch (property_id)
    {
    case PROP_GROUP_NUMBER:
      sample->group_number = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      return;
    }
}

static void
ipatch_gig_sample_get_property (GObject *object, guint property_id,
				GValue *value, GParamSpec *pspec)
{
  IpatchGigSample *sample = IPATCH_GIG_SAMPLE (object);

  switch (property_id)
    {
    case PROP_GROUP_NUMBER:
      g_value_set_uint (value, sample->group_number);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ipatch_gig_sample_item_copy (IpatchItem *dest, IpatchItem *src,
			     IpatchItemCopyLinkFunc link_func,
			     gpointer user_data)
{
  IpatchGigSample *src_sample, *dest_sample;

  src_sample = IPATCH_GIG_SAMPLE (src);
  dest_sample = IPATCH_GIG_SAMPLE (dest);

  /* call IpatchDLS2Sample class copy function */
  IPATCH_ITEM_CLASS (parent_class)->copy (dest, src, link_func, user_data);

  /* don't need to lock for this stuff */
  dest_sample->group_number = src_sample->group_number;
}

/**
 * ipatch_gig_sample_new:
 *
 * Create a new GigaSampler sample object.
 *
 * Returns: New GigaSampler sample with a reference count of 1. Caller
 * owns the reference and removing it will destroy the item, unless another
 * reference is added (if its parented for example).
 */
IpatchGigSample *
ipatch_gig_sample_new (void)
{
  return (IPATCH_GIG_SAMPLE (g_object_new (IPATCH_TYPE_GIG_SAMPLE, NULL)));
}

/**
 * ipatch_gig_sample_first: (skip)
 * @iter: Patch item iterator containing #IpatchGigSample items
 *
 * Gets the first item in a GigaSampler sample iterator. A convenience
 * wrapper for ipatch_iter_first().
 *
 * Returns: The first GigaSampler sample in @iter or %NULL if empty.
 */
IpatchGigSample *
ipatch_gig_sample_first (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_first (iter);
  if (obj) return (IPATCH_GIG_SAMPLE (obj));
  else return (NULL);
}

/**
 * ipatch_gig_sample_next: (skip)
 * @iter: Patch item iterator containing #IpatchGigSample items
 *
 * Gets the next item in a GigaSampler sample iterator. A convenience
 * wrapper for ipatch_iter_next().
 *
 * Returns: The next GigaSampler sample in @iter or %NULL if at
 * the end of the list.
 */
IpatchGigSample *
ipatch_gig_sample_next (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_next (iter);
  if (obj) return (IPATCH_GIG_SAMPLE (obj));
  else return (NULL);
}
