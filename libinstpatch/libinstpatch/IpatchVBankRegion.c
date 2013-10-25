/*
 * libInstPatch
 * Copyright (C) 1999-2010 Joshua "Element" Green <jgreen@users.sourceforge.net>
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
#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchVBankRegion.h"
#include "ipatch_priv.h"
#include "builtin_enums.h"

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_LINK_ITEM,		/* Region linked synthesis item */
  PROP_ID_PROPS,		/* Identification properties */
  PROP_FILE_INDEX,		/* Referenced file index in IpatchVBank parent */
  PROP_NOTE_RANGE,		/* Note range */
  PROP_NOTE_RANGE_MODE,		/* Mode of note range */
  PROP_ROOT_NOTE,		/* Root note */
  PROP_ROOT_NOTE_MODE		/* Mode of root note */
};


static void ipatch_vbank_region_get_title (IpatchVBankRegion *region, GValue *value);
static void ipatch_vbank_region_set_property (GObject *object,
					   guint property_id,
					   const GValue *value,
					   GParamSpec *pspec);
static void ipatch_vbank_region_get_property (GObject *object,
					   guint property_id, GValue *value,
					   GParamSpec *pspec);
static void ipatch_vbank_region_finalize (GObject *object);
static void ipatch_vbank_region_real_set_id_props (IpatchVBankRegion *region,
						   char **id_props,
						   gboolean notify);


G_DEFINE_TYPE (IpatchVBankRegion, ipatch_vbank_region, IPATCH_TYPE_ITEM);


static void
ipatch_vbank_region_class_init (IpatchVBankRegionClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);

  obj_class->get_property = ipatch_vbank_region_get_property;
  obj_class->finalize = ipatch_vbank_region_finalize;

  item_class->item_set_property = ipatch_vbank_region_set_property;

  g_object_class_override_property (obj_class, PROP_TITLE, "title");

  g_object_class_install_property (obj_class, PROP_LINK_ITEM,
		    g_param_spec_object ("link-item", _("Link item"),
					 _("Link item"),
					 IPATCH_TYPE_ITEM,
					 G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_ID_PROPS,
      g_param_spec_boxed ("id-props", _("ID props"), _("Identification properties"),
	                  G_TYPE_STRV, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_FILE_INDEX,
	      g_param_spec_uint ("file-index", _("File index"), _("File index"),
				 0, G_MAXUINT, 0, G_PARAM_READABLE));
  g_object_class_install_property (obj_class, PROP_NOTE_RANGE,
		    ipatch_param_spec_range ("note-range", _("Note range"),
					     _("Note range"), 0, 127, 0, 127,
					     G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_NOTE_RANGE_MODE,
	      g_param_spec_enum ("note-range-mode", _("Note range mode"),
				 _("Note range mode"),
				 IPATCH_TYPE_VBANK_REGION_NOTE_RANGE_MODE,
				 IPATCH_VBANK_REGION_NOTE_RANGE_MODE_INTERSECT,
				 G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_ROOT_NOTE,
	      g_param_spec_int ("root-note", _("Root note"), _("Root note"),
				-127, 127, 0, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_ROOT_NOTE_MODE,
	      g_param_spec_enum ("root-note-mode", _("Root note mode"),
				 _("Root note mode"),
				 IPATCH_TYPE_VBANK_REGION_ROOT_NOTE_MODE,
				 IPATCH_VBANK_REGION_ROOT_NOTE_MODE_OFFSET,
				 G_PARAM_READWRITE));
}

static void
ipatch_vbank_region_get_title (IpatchVBankRegion *region, GValue *value)
{
  IpatchItem *ref;
  char *s = NULL;

  g_object_get (region, "link-item", &ref, NULL); /* ++ ref region linked item */

  if (ref)
  {
    g_object_get (ref, "title", &s, NULL);
    g_object_unref (ref);	/* -- unref region linked item */
    g_value_take_string (value, s);
  }
  else g_value_set_static_string (value, _("<Unresolved>"));
}

static void
ipatch_vbank_region_set_property (GObject *object, guint property_id,
				  const GValue *value, GParamSpec *pspec)
{
  IpatchVBankRegion *region = IPATCH_VBANK_REGION (object);
  IpatchRange *range;
  GObject *obj;
  char **strv;

  switch (property_id)
  {
    case PROP_LINK_ITEM:
      obj = g_value_get_object (value);
      g_return_if_fail (!obj || IPATCH_IS_ITEM (obj));

      if (obj) g_object_ref (obj);

      IPATCH_ITEM_WLOCK (region);
      if (region->item) g_object_unref (region->item);
      region->item = (IpatchItem *)obj;
      IPATCH_ITEM_WUNLOCK (region);
      break;
    case PROP_ID_PROPS:
      strv = g_value_get_boxed (value);
      ipatch_vbank_region_real_set_id_props (region, strv, FALSE);
      break;
    case PROP_NOTE_RANGE:
      range = ipatch_value_get_range (value);

      IPATCH_ITEM_WLOCK (region);
      region->note_range = *range;
      IPATCH_ITEM_WUNLOCK (region);
      break;
    case PROP_NOTE_RANGE_MODE:
      region->note_range_mode = g_value_get_enum (value);
      break;
    case PROP_ROOT_NOTE:
      region->root_note = g_value_get_int (value);
      break;
    case PROP_ROOT_NOTE_MODE:
      region->root_note_mode = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
ipatch_vbank_region_get_property (GObject *object, guint property_id,
				  GValue *value, GParamSpec *pspec)
{
  IpatchVBankRegion *region = IPATCH_VBANK_REGION (object);
  IpatchRange range;
  GObject *obj;
  char **strv;

  switch (property_id)
  {
    case PROP_TITLE:
      ipatch_vbank_region_get_title (region, value);
      break;
    case PROP_LINK_ITEM:
      IPATCH_ITEM_RLOCK (region);
      obj = (GObject *)(region->item);
      if (obj) g_object_ref (obj);
      IPATCH_ITEM_RUNLOCK (region);

      g_value_take_object (value, obj);
      break;
    case PROP_ID_PROPS:
      strv = ipatch_vbank_region_get_id_props (region, NULL);   // ++ alloc strv
      g_value_take_boxed (value, strv);                         // !! value takes over strv
      break;
    case PROP_FILE_INDEX:
      g_value_set_uint (value, region->file_index);
      break;
    case PROP_NOTE_RANGE:
      IPATCH_ITEM_WLOCK (region);
      range = region->note_range;
      IPATCH_ITEM_WUNLOCK (region);

      ipatch_value_set_range (value, &range);
      break;
    case PROP_NOTE_RANGE_MODE:
      g_value_set_enum (value, region->note_range_mode);
      break;
    case PROP_ROOT_NOTE:
      g_value_set_int (value, region->root_note);
      break;
    case PROP_ROOT_NOTE_MODE:
      g_value_set_enum (value, region->root_note_mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
ipatch_vbank_region_init (IpatchVBankRegion *region)
{
  region->note_range.low = 0;
  region->note_range.high = 127;
}

static void
ipatch_vbank_region_finalize (GObject *object)
{
  IpatchVBankRegion *region = IPATCH_VBANK_REGION (object);

  if (region->item) g_object_unref (region->item);
  g_strfreev (region->id_props);

  if (G_OBJECT_CLASS (ipatch_vbank_region_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_vbank_region_parent_class)->finalize (object);
}

/**
 * ipatch_vbank_region_new:
 *
 * Create a new virtual bank region object.
 *
 * Returns: New virtual bank region with a reference count of 1. Caller
 * owns the reference and removing it will destroy the item, unless another
 * reference is added (if its parented for example).
 */
IpatchVBankRegion *
ipatch_vbank_region_new (void)
{
  return (IPATCH_VBANK_REGION (g_object_new (IPATCH_TYPE_VBANK_REGION, NULL)));
}

/**
 * ipatch_vbank_region_first:
 * @iter: Patch item iterator containing #IpatchVBankRegion items
 *
 * Gets the first item in a virtual bank region iterator. A convenience
 * wrapper for ipatch_iter_first().
 *
 * Returns: The first virtual bank region in @iter or %NULL if empty.
 */
IpatchVBankRegion *
ipatch_vbank_region_first (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_first (iter);
  if (obj) return (IPATCH_VBANK_REGION (obj));
  else return (NULL);
}

/**
 * ipatch_vbank_region_next:
 * @iter: Patch item iterator containing #IpatchVBankRegion items
 *
 * Gets the next item in a virtual bank region iterator. A convenience wrapper
 * for ipatch_iter_next().
 *
 * Returns: The next virtual bank region in @iter or %NULL if at the end of
 *   the list.
 */
IpatchVBankRegion *
ipatch_vbank_region_next (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_next (iter);
  if (obj) return (IPATCH_VBANK_REGION (obj));
  else return (NULL);
}

/**
 * ipatch_vbank_region_set_id_props:
 * @region: VBank region
 * @id_props: NULL terminated array of name/value string pairs
 *
 * Set the ID properties of a virtual bank region.  These are used to uniquely
 * identify an item in an external instrument file.  This function is likely
 * only used by the VBank loader before an item is resolved.  Normal users
 * will likely just assign to the "item-link" parameter.
 */
void
ipatch_vbank_region_set_id_props (IpatchVBankRegion *region,
				  char **id_props)
{
  ipatch_vbank_region_real_set_id_props (region, id_props, TRUE);
}

/* the real set ID properties routine, takes a notify parameter to indicate if
 * a property notify should be done */
static void
ipatch_vbank_region_real_set_id_props (IpatchVBankRegion *region,
				       char **id_props, gboolean notify)
{
  char **dup_id_props;

  g_return_if_fail (IPATCH_IS_VBANK_REGION (region));

  dup_id_props = g_strdupv (id_props);	/* ++ alloc */

  IPATCH_ITEM_WLOCK (region);
  g_strfreev (region->id_props);
  region->id_props = dup_id_props;	/* !! owned */
  IPATCH_ITEM_WUNLOCK (region);

  if (notify) g_object_notify (G_OBJECT (region), "id-props");
}

/**
 * ipatch_vbank_region_get_id_props:
 * @region: VBank region
 * @n_elements: Pointer to store count of elements in returned string array
 *   or %NULL to ignore.
 *
 * Get ID properties which uniquely identify the referenced item.  These are
 * usually only available until the item gets resolved, at which point
 * "item-link" is set.
 *
 * Returns: %NULL terminated array of name/value pair property strings or
 * %NULL if none.  Free with g_strfreev() when finished using it.
 */
char **
ipatch_vbank_region_get_id_props (IpatchVBankRegion *region, guint *n_elements)
{
  char **id_props;

  g_return_val_if_fail (IPATCH_IS_VBANK_REGION (region), NULL);

  IPATCH_ITEM_RLOCK (region);
  id_props = g_strdupv (region->id_props);	/* ++ alloc */
  IPATCH_ITEM_RUNLOCK (region);

  if (n_elements) *n_elements = id_props ? g_strv_length (id_props) : 0;

  return (id_props);	/* !! owned */
}
