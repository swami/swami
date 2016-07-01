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
 * SECTION: IpatchVBankRegion
 * @short_description: Virtual bank instrument region
 * @see_also: #IpatchVBankInst, #IpatchVBank
 * @stability: Stable
 *
 * Virtual bank regions are children to #IpatchVBankInst objects and reference
 * synthesizable #IpatchItem objects from other files.  This object forms the
 * bases for constructing new instruments from one or more items in other
 * instrument bank files.
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
static void ipatch_vbank_region_remove_full (IpatchItem *item, gboolean full);
static void ipatch_vbank_region_real_set_item (IpatchVBankRegion *region,
                                               IpatchItem *item,
                                               gboolean sample_notify);

G_DEFINE_TYPE (IpatchVBankRegion, ipatch_vbank_region, IPATCH_TYPE_ITEM);

static GParamSpec *link_item_pspec;

static void
ipatch_vbank_region_class_init (IpatchVBankRegionClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);

  obj_class->get_property = ipatch_vbank_region_get_property;
  obj_class->finalize = ipatch_vbank_region_finalize;

  item_class->item_set_property = ipatch_vbank_region_set_property;
  item_class->remove_full = ipatch_vbank_region_remove_full;

  g_object_class_override_property (obj_class, PROP_TITLE, "title");

  link_item_pspec = g_param_spec_object ("link-item", _("Link item"),
					 _("Link item"),
					 IPATCH_TYPE_ITEM,
					 G_PARAM_READWRITE);

  g_object_class_install_property (obj_class, PROP_LINK_ITEM, link_item_pspec);
  g_object_class_install_property (obj_class, PROP_ID_PROPS,
      g_param_spec_value_array ("id-props", _("ID props"),
				_("Identification properties"),
	  g_param_spec_string ("name-value", "Name value", "Name value pairs",
			       NULL, G_PARAM_READWRITE), G_PARAM_READWRITE));
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
  GValueArray *valarray;
  IpatchRange *range;
  char **strv;
  int i;

  switch (property_id)
  {
    case PROP_LINK_ITEM:
      ipatch_vbank_region_real_set_item (region,
                                         (IpatchItem *)g_value_get_object (value),
                                         FALSE);
      break;
    case PROP_ID_PROPS:
      valarray = g_value_get_boxed (value);

      if (valarray)
      {
	strv = g_new (char *, valarray->n_values + 1);	/* ++ alloc */

	for (i = 0; i < valarray->n_values; i++)
	  strv[i] = (char *)g_value_get_string (g_value_array_get_nth (valarray, i));

	strv[valarray->n_values] = NULL;

	ipatch_vbank_region_real_set_id_props (region, strv, FALSE);

	g_free (strv);	/* -- free - strings themselves were not allocated */
      }
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
  GValueArray *valarray;
  IpatchRange range;
  guint n_elements;
  char **strv;
  GValue val = { 0 };
  int i;

  switch (property_id)
  {
    case PROP_TITLE:
      ipatch_vbank_region_get_title (region, value);
      break;
    case PROP_LINK_ITEM:
      g_value_take_object (value, ipatch_vbank_region_get_item (region));
      break;
    case PROP_ID_PROPS:
      strv = ipatch_vbank_region_get_id_props (region, &n_elements);	/* ++ alloc */

      if (strv)
      {
	valarray = g_value_array_new (n_elements);	/* ++ alloc */
	g_value_init (&val, G_TYPE_STRING);

	for (i = 0; i < n_elements; i++)
	{
	  g_value_set_string (&val, strv[i]);	/* ++ alloc */
	  g_value_array_append (valarray, &val);
	  g_value_reset (&val);			/* -- free */
	}

	g_value_unset (&val);

	g_value_take_boxed (value, valarray);	/* !! owned */
	g_strfreev (strv);	/* -- free */
      }
      else g_value_set_boxed (value, NULL);
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
ipatch_vbank_region_remove_full (IpatchItem *item, gboolean full)
{
  if (full)
    ipatch_vbank_region_real_set_item (IPATCH_VBANK_REGION (item), NULL, TRUE);

  if (IPATCH_ITEM_CLASS (ipatch_vbank_region_parent_class)->remove_full)
    IPATCH_ITEM_CLASS (ipatch_vbank_region_parent_class)->remove_full (item, full);
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
 * ipatch_vbank_region_first: (skip)
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
 * ipatch_vbank_region_next: (skip)
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
 * @id_props: (array zero-terminated=1): NULL terminated array of name/value string pairs
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
 * @n_elements: (out) (optional): Pointer to store count of elements in
 *   returned string array or %NULL to ignore.
 *
 * Get ID properties which uniquely identify the referenced item.  These are
 * usually only available until the item gets resolved, at which point
 * "item-link" is set.
 *
 * Returns: (array zero-terminated=1) (transfer full): %NULL terminated array of name/value
 * pair property strings or %NULL if none.  Free with g_strfreev() when finished using it.
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

/**
 * ipatch_vbank_region_set_item:
 * @region: Region to set sample of
 * @item: Instrument item to assign
 *
 * Sets the referenced instrument item of a virtual bank region.
 *
 * Since: 1.1.0
 */
void
ipatch_vbank_region_set_item (IpatchVBankRegion *region, IpatchItem *item)
{
  g_return_if_fail (IPATCH_IS_VBANK_REGION (region));
  g_return_if_fail (IPATCH_IS_ITEM (item));

  ipatch_vbank_region_real_set_item (region, item, TRUE);
}

static void
ipatch_vbank_region_real_set_item (IpatchVBankRegion *region,
				   IpatchItem *item,
				   gboolean sample_notify)
{
  GValue newval = { 0 }, oldval = { 0 };
  IpatchItem *olditem;

  if (item) g_object_ref (item);        // !! ref item for region

  IPATCH_ITEM_WLOCK (region);
  olditem = region->item;
  region->item = item;
  IPATCH_ITEM_WUNLOCK (region);

  if (sample_notify)
  {
    g_value_init (&oldval, IPATCH_TYPE_ITEM);
    g_value_set_object (&oldval, olditem);
    g_value_init (&newval, IPATCH_TYPE_ITEM);
    g_value_set_object (&newval, item);
    ipatch_item_prop_notify ((IpatchItem *)region, link_item_pspec,
                             &newval, &oldval);
    g_value_unset (&newval);
    g_value_unset (&oldval);
  }

  if (olditem) g_object_unref (olditem);        // !! unref old item (no longer referenced by region)

  /* notify title property change */
  g_value_init (&newval, G_TYPE_STRING);
  ipatch_vbank_region_get_title (region, &newval);
  ipatch_item_prop_notify ((IpatchItem *)region, ipatch_item_pspec_title,
			   &newval, NULL);
  g_value_unset (&newval);
}

/**
 * ipatch_vbank_region_get_item:
 * @region: Region to get referenced instrument item from
 *
 * Gets the referenced instrument item from a region.
 *
 * Returns: (transfer full): Region's referenced item or %NULL if not set yet. Remember to
 * unreference the item with g_object_unref() when done with it.
 *
 * Since: 1.1.0
 */
IpatchItem *
ipatch_vbank_region_get_item (IpatchVBankRegion *region)
{
  IpatchItem *item;

  g_return_val_if_fail (IPATCH_IS_VBANK_REGION (region), NULL);

  IPATCH_ITEM_RLOCK (region);
  item = region->item;
  if (item) g_object_ref (item);        // ++ reference for caller
  IPATCH_ITEM_RUNLOCK (region);

  return (item);                        // !! caller takes on reference
}

