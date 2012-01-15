/*
 * libInstPatch
 * Copyright (C) 1999-2010 Joshua "Element" Green <jgreen@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Moderal Public License
 * as published by the Free Software Foundation; version 2.1
 * of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Moderal Public License for more details.
 *
 * You should have received a copy of the GNU Moderal Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA or on the web at http://www.gnu.org.
 */
#include <glib.h>
#include <glib-object.h>
#include "IpatchSF2ModItem.h"
#include "IpatchSF2Gen.h"
#include "ipatch_priv.h"

static void ipatch_sf2_mod_item_iface_init (IpatchSF2ModItemIface *iface);


GType
ipatch_sf2_mod_item_get_type (void)
{
  static GType itype = 0;

  if (!itype)
    {
      static const GTypeInfo info =
	{
	  sizeof (IpatchSF2ModItemIface),
	  NULL,			/* base_init */
	  NULL,			/* base_finalize */
	  (GClassInitFunc) ipatch_sf2_mod_item_iface_init,
	  (GClassFinalizeFunc) NULL
	};

      itype = g_type_register_static (G_TYPE_INTERFACE, "IpatchSF2ModItemIface",
				      &info, 0);

      /* IpatchSF2ModItemIface types must be IpatchItem objects (for locking) */
      g_type_interface_add_prerequisite (itype, IPATCH_TYPE_ITEM);
    }

  return (itype);
}

static void
ipatch_sf2_mod_item_iface_init (IpatchSF2ModItemIface *iface)
{
  g_object_interface_install_property (iface,
    g_param_spec_boxed ("modulators", _("Modulators"),
			_("Modulators"), IPATCH_TYPE_SF2_MOD_LIST,
			G_PARAM_READWRITE));
}

/**
 * ipatch_sf2_mod_item_get_mods:
 * @item: Item with modulators
 *
 * Gets a list of modulators from an item with modulators. List should be freed
 * with ipatch_sf2_mod_list_free() (free_mods set to %TRUE) when finished
 * with it.
 *
 * Returns: (element-type IpatchSF2Mod): New list of modulators (#IpatchSF2Mod)
 *   in @item or %NULL if no modulators. Remember to free it with
 *   ipatch_sf2_mod_list_free() when finished.
 */
GSList *
ipatch_sf2_mod_item_get_mods (IpatchSF2ModItem *item)
{
  IpatchSF2ModItemIface *iface;
  GSList **pmods, *newlist = NULL;
  IpatchSF2Mod *mod;
  GSList *p;

  g_return_val_if_fail (IPATCH_IS_SF2_MOD_ITEM (item), NULL);

  /* get pointer to GSList from IpatchSF2ModItemIface->modlist_ofs */
  iface = IPATCH_SF2_MOD_ITEM_GET_IFACE (item);
  g_return_val_if_fail (iface->modlist_ofs != 0, NULL);
  pmods = (GSList **)G_STRUCT_MEMBER_P (item, iface->modlist_ofs);

  IPATCH_ITEM_RLOCK (item);

  p = *pmods;
  while (p)
    {
      mod = ipatch_sf2_mod_duplicate ((IpatchSF2Mod *)(p->data));
      newlist = g_slist_prepend (newlist, mod);
      p = p->next;
    }

  IPATCH_ITEM_RUNLOCK (item);

  newlist = g_slist_reverse (newlist);

  return (newlist);
}

/**
 * ipatch_sf2_mod_item_set_mods:
 * @item: Item with modulators
 * @mod_list: (element-type IpatchSF2Mod): Modulator list to assign to zone.
 * @flags: Flags for controlling list duplication and item property
 *   notification (#IpatchSF2ModFlags).  If #IPATCH_SF2_MOD_NO_DUPLICATE
 *   is set then ownership of @mod_list is taken over (not duplicated).
 *   If #IPATCH_SF2_MOD_NO_NOTIFY is set, then item property notify will not
 *   be done.
 *
 * Sets the complete modulator list of an item with modulators.
 * If #IPATCH_SF2_MOD_NO_NOTIFY is not in @flags then #IpatchItem property
 * notify is done.
 */
void
ipatch_sf2_mod_item_set_mods (IpatchSF2ModItem *item, GSList *mod_list, int flags)
{
  GValue old_value = { 0 }, new_value = { 0 };
  GSList **pmods, *oldlist, *newlist;
  IpatchSF2ModItemIface *iface;

  g_return_if_fail (IPATCH_IS_SF2_MOD_ITEM (item));

  /* get pointer to GSList from IpatchSF2ModItemIface->modlist_ofs */
  iface = IPATCH_SF2_MOD_ITEM_GET_IFACE (item);
  g_return_if_fail (iface->modlist_ofs != 0);
  pmods = (GSList **)G_STRUCT_MEMBER_P (item, iface->modlist_ofs);

  /* duplicate list if NO_DUPLICATE flag not set */
  if (!(flags & IPATCH_SF2_MOD_NO_DUPLICATE))
    newlist = ipatch_sf2_mod_list_duplicate (mod_list);
  else newlist = mod_list;

  IPATCH_ITEM_RLOCK (item);
  oldlist = *pmods;
  *pmods = newlist;
  IPATCH_ITEM_RUNLOCK (item);

  /* do property notify if NO_NOTIFY flag not set */
  if (!(flags & IPATCH_SF2_MOD_NO_NOTIFY))
    {	/* old notify value takes over old list */
      g_value_init (&old_value, IPATCH_TYPE_SF2_MOD_LIST);
      g_value_take_boxed (&old_value, oldlist);

      g_value_init (&new_value, IPATCH_TYPE_SF2_MOD_LIST);
      g_value_set_static_boxed (&new_value, mod_list);

      ipatch_item_prop_notify (item, iface->mod_pspec, &new_value, &old_value);
      g_value_unset (&new_value);
      g_value_unset (&old_value);
    }
  else ipatch_sf2_mod_list_free (oldlist, TRUE);  /* free old list if no notify */
}

/**
 * ipatch_sf2_mod_item_insert:
 * @item: Item with modulators
 * @mod: Modulator to insert (a new modulator is created and the
 *   values are copied to it)
 * @pos: Index position in zone's modulator list to insert
 *   (0 = first, < 0 = last)
 *
 * Inserts a modulator into an item's modulator list. Does not check for
 * duplicates! The modulator is not used directly, a new one is created and
 * the values in @mod are copied to it.
 * An #IpatchItem property notify is done. 
 */
void
ipatch_sf2_mod_item_insert (IpatchSF2ModItem *item,
			    const IpatchSF2Mod *mod, int pos)
{
  IpatchSF2Mod *newmod;
  GSList *newlist;

  g_return_if_fail (IPATCH_IS_SF2_MOD_ITEM (item));
  g_return_if_fail (mod != NULL);

  /* get the current MOD list */
  newlist = ipatch_sf2_mod_item_get_mods (item);

  newmod = ipatch_sf2_mod_duplicate (mod);	/* duplicate the modulator vals */
  newlist = g_slist_insert (newlist, newmod, pos);	/* insert mod */

  /* assign the new modulator list (item takes it over, no duplicating) */
  ipatch_sf2_mod_item_set_mods (item, newlist, IPATCH_SF2_MOD_NO_DUPLICATE);
}

/**
 * ipatch_sf2_mod_item_remove:
 * @item: Item with modulators
 * @mod: Matching values of modulator to remove
 *
 * Remove a modulator from an item with modulators. The modulator values in @mod
 * are used to search the modulator list. The first modulator
 * that matches all fields in @mod is removed.
 * An #IpatchItem property notify is done. 
 */
void
ipatch_sf2_mod_item_remove (IpatchSF2ModItem *item, const IpatchSF2Mod *mod)
{
  GSList *newlist;
  gboolean changed;

  g_return_if_fail (IPATCH_IS_SF2_MOD_ITEM (item));
  g_return_if_fail (mod != NULL);

  /* get the current MOD list */
  newlist = ipatch_sf2_mod_item_get_mods (item);

  /* remove the modulator */
  newlist = ipatch_sf2_mod_list_remove (newlist, mod, &changed);

  /* assign the new modulator list (item takes it over, no duplicating) */
  if (changed)
    ipatch_sf2_mod_item_set_mods (item, newlist, IPATCH_SF2_MOD_NO_DUPLICATE);
  else ipatch_sf2_mod_list_free (newlist, TRUE);	/* not changed, free list */
}

/**
 * ipatch_sf2_mod_item_change:
 * @item: Item with modulators
 * @oldmod: Current values of modulator to set
 * @newmod: New modulator values
 *
 * Sets the values of an existing modulator in an item with modulators. The
 * modulator list in item is searched for a modulator that matches the values in
 * @oldmod. If a modulator is found its values are set to those in @newmod.
 * If it is not found, nothing is done.
 * If change occurs #IpatchItem property notify is done.
 */
void
ipatch_sf2_mod_item_change (IpatchItem *item, const IpatchSF2Mod *oldmod,
			    const IpatchSF2Mod *newmod)
{
  GSList *newlist;
  gboolean changed;

  g_return_if_fail (IPATCH_IS_SF2_MOD_ITEM (item));
  g_return_if_fail (oldmod != NULL);
  g_return_if_fail (newmod != NULL);

  /* get the current MOD list */
  newlist = ipatch_sf2_mod_item_get_mods (item);

  /* change the modulator */
  changed = ipatch_sf2_mod_list_change (newlist, oldmod, newmod);

  /* if changed - assign the new modulator list (item takes it over) */
  if (changed)
    ipatch_sf2_mod_item_set_mods (item, newlist, IPATCH_SF2_MOD_NO_DUPLICATE);
  else ipatch_sf2_mod_list_free (newlist, TRUE);  /* not changed, free list */
}

/**
 * ipatch_sf2_mod_item_count:
 * @item: Item with modulators
 *
 * Count number of modulators in an item with modulators.
 *
 * Returns: Count of modulators
 */
guint
ipatch_sf2_mod_item_count (IpatchSF2ModItem *item)
{
  IpatchSF2ModItemIface *iface;
  GSList **pmods;
  guint i;

  g_return_val_if_fail (IPATCH_IS_SF2_MOD_ITEM (item), 0);

  /* get pointer to GSList from IpatchSF2ModItemIface->modlist_ofs */
  iface = IPATCH_SF2_MOD_ITEM_GET_IFACE (item);
  g_return_val_if_fail (iface->modlist_ofs != 0, 0);
  pmods = (GSList **)G_STRUCT_MEMBER_P (item, iface->modlist_ofs);

  IPATCH_ITEM_RLOCK (item);
  i = g_slist_length (*pmods);
  IPATCH_ITEM_RUNLOCK (item);

  return (i);
}
