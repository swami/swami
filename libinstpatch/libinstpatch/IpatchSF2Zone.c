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
 * SECTION: IpatchSF2Zone
 * @short_description: Abstract base class for SoundFont zones
 * @see_also: 
 * @stability: Stable
 *
 * Zones are children of #IpatchSF2Preset and #IpatchSF2Inst and define
 * synthesis parameters and a linked item (#IpatchSF2Inst in the case of
 * #IpatchSF2PZone and #IpatchSF2Sample in the case of #IpatchSF2IZone).
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchSF2Zone.h"
#include "IpatchSF2PZone.h"
#include "IpatchSF2.h"
#include "IpatchSF2Gen.h"
#include "IpatchSF2Mod.h"
#include "IpatchSF2ModItem.h"
#include "IpatchParamProp.h"
#include "IpatchRange.h"
#include "IpatchUnit.h"
#include "builtin_enums.h"
#include "ipatch_priv.h"

static void ipatch_sf2_zone_mod_item_iface_init
  (IpatchSF2ModItemIface *moditem_iface);
static void ipatch_sf2_zone_class_init (IpatchSF2ZoneClass *klass);
static void ipatch_sf2_zone_finalize (GObject *gobject);
static void ipatch_sf2_zone_get_title (IpatchSF2Zone *zone, GValue *value);
static void ipatch_sf2_zone_set_property (GObject *object, guint property_id,
					  const GValue *value, GParamSpec *pspec);
static void ipatch_sf2_zone_get_property (GObject *object,
					  guint property_id, GValue *value,
					  GParamSpec *pspec);
static void ipatch_sf2_zone_item_copy (IpatchItem *dest, IpatchItem *src,
				       IpatchItemCopyLinkFunc link_func,
				       gpointer user_data);
static void ipatch_sf2_zone_item_remove_full (IpatchItem *item, gboolean full);
static void ipatch_sf2_zone_link_item_title_notify (IpatchItemPropNotify *info);

/* generator property IDs go before these */
enum
{
  PROP_0,
  PROP_TITLE,
  PROP_MODULATORS
};


/* For passing between class init and mod item interface init */
static GParamSpec *modulators_spec = NULL;


G_DEFINE_ABSTRACT_TYPE_WITH_CODE (IpatchSF2Zone, ipatch_sf2_zone, IPATCH_TYPE_ITEM,
    G_IMPLEMENT_INTERFACE (IPATCH_TYPE_SF2_MOD_ITEM, ipatch_sf2_zone_mod_item_iface_init))


static void
ipatch_sf2_zone_class_init (IpatchSF2ZoneClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);

  obj_class->finalize = ipatch_sf2_zone_finalize;
  obj_class->get_property = ipatch_sf2_zone_get_property;

  item_class->copy = ipatch_sf2_zone_item_copy;
  item_class->item_set_property = ipatch_sf2_zone_set_property;
  item_class->remove_full = ipatch_sf2_zone_item_remove_full;

  g_object_class_override_property (obj_class, PROP_TITLE, "title");
  g_object_class_override_property (obj_class, PROP_MODULATORS, "modulators");
  modulators_spec = g_object_class_find_property (obj_class, "modulators");
}

/* mod item interface initialization */
static void
ipatch_sf2_zone_mod_item_iface_init (IpatchSF2ModItemIface *moditem_iface)
{
  moditem_iface->modlist_ofs = G_STRUCT_OFFSET (IpatchSF2Zone, mods);

  /* cache the modulators property for fast notifications */
  moditem_iface->mod_pspec = modulators_spec;
}

static void
ipatch_sf2_zone_init (IpatchSF2Zone *zone)
{
}

static void
ipatch_sf2_zone_finalize (GObject *gobject)
{
  IpatchSF2Zone *zone = IPATCH_SF2_ZONE (gobject);
  IpatchItem *link = NULL;
  GSList *p;

  IPATCH_ITEM_WLOCK (zone);

  if (zone->item)
    {
      link = zone->item;
      g_object_unref (zone->item);
      zone->item = NULL;
    }

  p = zone->mods;
  while (p)
    {
      ipatch_sf2_mod_free ((IpatchSF2Mod *)(p->data));
      p = g_slist_next (p);
    }
  g_slist_free (zone->mods);
  zone->mods = NULL;

  IPATCH_ITEM_WUNLOCK (zone);

  if (link)
    ipatch_item_prop_disconnect_matched (link, ipatch_item_pspec_title,
      ipatch_sf2_zone_link_item_title_notify, zone);

  if (G_OBJECT_CLASS (ipatch_sf2_zone_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_sf2_zone_parent_class)->finalize (gobject);
}

static void
ipatch_sf2_zone_get_title (IpatchSF2Zone *zone, GValue *value)
{
  IpatchItem *ref;
  char *s = NULL;

  g_object_get (zone, "link-item", &ref, NULL); /* ++ ref zone linked item */

  if (ref)
    {
      g_object_get (ref, "name", &s, NULL);
      g_object_unref (ref);	/* -- unref zone linked item */
    }

  g_value_take_string (value, s);
}

static void
ipatch_sf2_zone_set_property (GObject *object, guint property_id,
			      const GValue *value, GParamSpec *pspec)
{
  IpatchSF2Zone *zone = IPATCH_SF2_ZONE (object);
  IpatchSF2ModList *list;

  if (property_id == PROP_MODULATORS)
    {
      list = (IpatchSF2ModList *)g_value_get_boxed (value);
      ipatch_sf2_mod_item_set_mods (IPATCH_SF2_MOD_ITEM (zone), list,
				    IPATCH_SF2_MOD_NO_NOTIFY);
    }
  else
    {
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      return;
    }
}

static void
ipatch_sf2_zone_get_property (GObject *object, guint property_id,
			      GValue *value, GParamSpec *pspec)
{
  IpatchSF2Zone *zone = IPATCH_SF2_ZONE (object);
  IpatchSF2ModList *list;

  if (property_id == PROP_TITLE)
    ipatch_sf2_zone_get_title (zone, value);
  else if (property_id == PROP_MODULATORS)
    {
      list = ipatch_sf2_mod_item_get_mods (IPATCH_SF2_MOD_ITEM (zone));
      g_value_take_boxed (value, list);
    }
  else
    {
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      return;
    }
}

static void
ipatch_sf2_zone_item_copy (IpatchItem *dest, IpatchItem *src,
			   IpatchItemCopyLinkFunc link_func,
			   gpointer user_data)
{
  IpatchSF2Zone *src_zone, *dest_zone;
  IpatchItem *refitem;
  IpatchSF2Mod *mod;
  GSList *p;

  src_zone = IPATCH_SF2_ZONE (src);
  dest_zone = IPATCH_SF2_ZONE (dest);

  IPATCH_ITEM_RLOCK (src_zone);

  refitem = IPATCH_ITEM_COPY_LINK_FUNC (dest, src_zone->item,
					link_func, user_data);
  if (refitem)
    ipatch_sf2_zone_set_link_item (dest_zone, refitem);

  p = src_zone->mods;
  while (p)			/* duplicate modulators */
    {
      mod = ipatch_sf2_mod_duplicate ((IpatchSF2Mod *)(p->data));
      dest_zone->mods = g_slist_prepend (dest_zone->mods, mod);
      p = g_slist_next (p);
    }

  /* duplicate generator array */
  memcpy (&dest_zone->genarray, &src_zone->genarray,
	  sizeof (IpatchSF2GenArray));

  IPATCH_ITEM_RUNLOCK (src_zone);

  dest_zone->mods = g_slist_reverse (dest_zone->mods);
}

static void
ipatch_sf2_zone_item_remove_full (IpatchItem *item, gboolean full)
{
  if (full)
    ipatch_sf2_zone_set_link_item (IPATCH_SF2_ZONE (item), NULL);

  if (IPATCH_ITEM_CLASS (ipatch_sf2_zone_parent_class)->remove_full)
    IPATCH_ITEM_CLASS (ipatch_sf2_zone_parent_class)->remove_full (item, full);
}

/**
 * ipatch_sf2_zone_first: (skip)
 * @iter: Patch item iterator containing #IpatchSF2Zone items
 *
 * Gets the first item in a zone iterator. A convenience
 * wrapper for ipatch_iter_first().
 *
 * Returns: The first zone in @iter or %NULL if empty.
 */
IpatchSF2Zone *
ipatch_sf2_zone_first (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_first (iter);
  if (obj) return (IPATCH_SF2_ZONE (obj));
  else return (NULL);
}

/**
 * ipatch_sf2_zone_next: (skip)
 * @iter: Patch item iterator containing #IpatchSF2Zone items
 *
 * Gets the next item in a zone iterator. A convenience wrapper
 * for ipatch_iter_next().
 *
 * Returns: The next zone in @iter or %NULL if at the end of
 *   the list.
 */
IpatchSF2Zone *
ipatch_sf2_zone_next (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_next (iter);
  if (obj) return (IPATCH_SF2_ZONE (obj));
  else return (NULL);
}

/**
 * ipatch_sf2_zone_set_link_item:
 * @zone: Zone to set zone item of
 * @item: (nullable): New item for zone to use
 *
 * Sets the referenced item of a zone (a #IpatchSF2Inst for preset zones,
 * #IpatchSF2Sample for instrument zones). The type specific item set routines
 * for each zone type may be preferred, as this one doesn't do strict type
 * checking.
 */
void
ipatch_sf2_zone_set_link_item (IpatchSF2Zone *zone, IpatchItem *item)
{
  GValue oldval = { 0 }, newval = { 0 };
  IpatchItem *olditem;

  if (!ipatch_sf2_zone_set_link_item_no_notify (zone, item, &olditem)) return;

  g_value_init (&oldval, G_TYPE_OBJECT);
  g_value_take_object (&oldval, olditem);	/* !! take over reference */

  g_value_init (&newval, G_TYPE_OBJECT);
  g_value_set_object (&newval, (GObject *)item);

  ipatch_item_prop_notify_by_name ((IpatchItem *)zone, "link-item",
				   &newval, &oldval);
  g_value_unset (&oldval);
  g_value_unset (&newval);
}

/**
 * ipatch_sf2_zone_set_link_item_no_notify:
 * @zone: Zone to set zone item of
 * @item: (nullable): New item for zone to use
 * @olditem: (out) (optional) (transfer full): Pointer to store old item pointer or %NULL to ignore.
 *   Caller owns reference if specified.
 *
 * Like ipatch_sf2_zone_set_link_item() but performs no property or item
 * change notifications for "link-item" property (shouldn't normally be used outside of derived types),
 * and the old value can be retrieved with the @olditem parameter.
 *
 * Returns: TRUE if property was changed, FALSE otherwise (invalid inputs)
 */
gboolean
ipatch_sf2_zone_set_link_item_no_notify (IpatchSF2Zone *zone, IpatchItem *item,
					 IpatchItem **olditem)
{
  IpatchItem *oldie;
  GValue old_title = { 0 }, new_title = { 0 };

  if (olditem) *olditem = NULL;		/* in case of failure */

  g_return_val_if_fail (IPATCH_IS_SF2_ZONE (zone), FALSE);
  g_return_val_if_fail (!item || IPATCH_IS_ITEM (item), FALSE);

  ipatch_item_get_property_fast (IPATCH_ITEM (zone), ipatch_item_pspec_title, &old_title);      // ++ alloc value

  if (item) g_object_ref (item);	/* ref for zone */

  IPATCH_ITEM_WLOCK (zone);
  oldie = zone->item;
  zone->item = item;
  IPATCH_ITEM_WUNLOCK (zone);

  /* remove "title" notify on old item */
  if (oldie)
    ipatch_item_prop_disconnect_matched (oldie, ipatch_item_pspec_title,
      ipatch_sf2_zone_link_item_title_notify, zone);

  /* add a prop notify on link-item "title" so zone can notify it's title also */
  if (item)
    ipatch_item_prop_connect (item, ipatch_item_pspec_title,
                              ipatch_sf2_zone_link_item_title_notify, NULL, zone);

  /* either caller takes reference to old item or we unref it, unref outside of lock */
  if (olditem) *olditem = oldie;
  else if (oldie) g_object_unref (oldie);

  ipatch_item_get_property_fast (IPATCH_ITEM (zone), ipatch_item_pspec_title, &new_title);    // ++ alloc value
  ipatch_item_prop_notify ((IpatchItem *)zone, ipatch_item_pspec_title, &old_title, &new_title);

  g_value_unset (&old_title);   // -- free value
  g_value_unset (&new_title);   // -- free value

  return (TRUE);
}

/* property notify for when link-item "title" property changes */
static void
ipatch_sf2_zone_link_item_title_notify (IpatchItemPropNotify *info)
{ /* notify that zone's title changed */
  IpatchItem *zone = (IpatchItem *)(info->user_data);
  ipatch_item_prop_notify_by_name (zone, "title", info->new_value, info->old_value);
}

/**
 * ipatch_sf2_zone_get_link_item:
 * @zone: Zone to get referenced item of
 *
 * Gets the referenced item from a zone (a #IpatchSF2Inst for preset zones,
 * #IpatchSF2Sample for instrument zones). The type specific item set routines
 * for each zone type may be preferred, as this one doesn't do strict type
 * checking. The returned item's reference count is incremented and the caller
 * is responsible for unrefing it with g_object_unref().
 *
 * Returns: (transfer full): Zone's referenced item or %NULL if global zone. Remember to
 * unreference the item with g_object_unref() when done with it.
 */
IpatchItem *
ipatch_sf2_zone_get_link_item (IpatchSF2Zone *zone)
{
  IpatchItem *item;

  g_return_val_if_fail (IPATCH_IS_SF2_ZONE (zone), NULL);

  IPATCH_ITEM_RLOCK (zone);
  item = zone->item;
  if (item) g_object_ref (item);
  IPATCH_ITEM_RUNLOCK (zone);

  return (item);
}

/**
 * ipatch_sf2_zone_peek_link_item: (skip)
 * @zone: Zone to get referenced item of
 *
 * Like ipatch_sf2_zone_get_link_item() but does not add a reference to
 * the returned item. This function should only be used if a reference
 * of the returned item is ensured or only the pointer value is of
 * interest.
 *
 * Returns: (transfer none): Zone's linked item. Remember that the item has NOT been referenced.
 */
IpatchItem *
ipatch_sf2_zone_peek_link_item (IpatchSF2Zone *zone)
{
  g_return_val_if_fail (IPATCH_IS_SF2_ZONE (zone), NULL);
  return (zone->item);	/* atomic read */
}
