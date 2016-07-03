/*
 * libInstPatch
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
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
/**
 * SECTION: IpatchSF2ModItem
 * @short_description: SoundFont modulator item interface
 * @see_also: 
 * @stability: Stable
 *
 * An interface type which is used by #IpatchSF2Preset, #IpatchSF2Inst,
 * #IpatchSF2PZone and #IpatchSF2IZone objects to add modulator realtime effect
 * functionality.
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

      itype = g_type_register_static (G_TYPE_INTERFACE, "IpatchSF2ModItem",
				      &info, 0);

      /* IpatchSF2ModItemIface types must be IpatchItem objects (for locking) */
      g_type_interface_add_prerequisite (itype, IPATCH_TYPE_ITEM);
    }

  return (itype);
}

static void
ipatch_sf2_mod_item_iface_init (IpatchSF2ModItemIface *iface)
{
  /**
   * IpatchSF2ModItem:modulators: (type GSList(Ipatch.SF2Mod))
   *
   * GSList of IpatchSF2Mod modulators.
   */
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
 * Returns: (element-type Ipatch.SF2Mod) (transfer full) (nullable):
 *   New list of modulators (#IpatchSF2Mod) in @item or %NULL if no modulators.
 *   Remember to free it with ipatch_sf2_mod_list_free() when finished.
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
 * ipatch_sf2_mod_item_set_mods: (skip)
 * @item: Item with modulators
 * @mod_list: (element-type Ipatch.SF2Mod): Modulator list to assign to zone.
 * @flags: (type IpatchSF2ModFlags): Flags for controlling list duplication and item property
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

  if (!(flags & IPATCH_SF2_MOD_NO_DUPLICATE))
    newlist = ipatch_sf2_mod_list_duplicate (mod_list);         // ++ Duplicate mod_list
  else newlist = mod_list;                                      // !! Just use mod_list directly

  if (!(flags & IPATCH_SF2_MOD_NO_NOTIFY))
    mod_list = ipatch_sf2_mod_list_duplicate (mod_list);        // ++ Duplicate mod_list for notify

  IPATCH_ITEM_WLOCK (item);
  oldlist = *pmods;
  *pmods = newlist;
  IPATCH_ITEM_WUNLOCK (item);

  /* do property notify if NO_NOTIFY flag not set */
  if (!(flags & IPATCH_SF2_MOD_NO_NOTIFY))
  {
    g_value_init (&old_value, IPATCH_TYPE_SF2_MOD_LIST);
    g_value_take_boxed (&old_value, oldlist);                   // -- Take over oldlist

    g_value_init (&new_value, IPATCH_TYPE_SF2_MOD_LIST);
    g_value_take_boxed (&new_value, mod_list);                  // -- Take over mod_list

    ipatch_item_prop_notify (IPATCH_ITEM (item), iface->mod_pspec, &new_value, &old_value);
    g_value_unset (&new_value);
    g_value_unset (&old_value);
  }
  else ipatch_sf2_mod_list_free (oldlist, TRUE);                // -- free old list if no notify
}

/**
 * ipatch_sf2_mod_item_set_mods_copy: (rename-to ipatch_sf2_mod_item_set_mods)
 * @item: Item with modulators
 * @mod_list: (element-type Ipatch.SF2Mod) (transfer none) (nullable): Modulator list to assign to zone.
 *
 * Sets the modulator list of an item with modulators.
 */
void
ipatch_sf2_mod_item_set_mods_copy (IpatchSF2ModItem *item, GSList *mod_list)
{
  ipatch_sf2_mod_item_set_mods (item, mod_list, 0);
}

/**
 * ipatch_sf2_mod_item_add_mod:
 * @item: Item with modulators
 * @mod: (transfer none): Modulator to append to end of @item object's modulator list
 *
 * Append a modulator to an item's modulator list.
 * NOTE: Does not check for duplicates!
 */
void
ipatch_sf2_mod_item_add (IpatchSF2ModItem *item, const IpatchSF2Mod *mod)
{
  ipatch_sf2_mod_item_insert (item, mod, -1);
}

/**
 * ipatch_sf2_mod_item_insert:
 * @item: Item with modulators
 * @mod: (transfer none): Modulator to insert
 * @pos: Index position in zone's modulator list to insert
 *   (0 = first, < 0 = last)
 *
 * Inserts a modulator into an item's modulator list.
 * NOTE: Does not check for duplicates!
 */
void
ipatch_sf2_mod_item_insert (IpatchSF2ModItem *item,
			    const IpatchSF2Mod *mod, int pos)
{
  GValue old_value = { 0 }, new_value = { 0 };
  GSList **pmods, *oldlist, *newlist;
  IpatchSF2ModItemIface *iface;
  IpatchSF2Mod *newmod;

  g_return_if_fail (IPATCH_IS_SF2_MOD_ITEM (item));
  g_return_if_fail (mod != NULL);

  /* get pointer to GSList from IpatchSF2ModItemIface->modlist_ofs */
  iface = IPATCH_SF2_MOD_ITEM_GET_IFACE (item);
  g_return_if_fail (iface->modlist_ofs != 0);
  pmods = (GSList **)G_STRUCT_MEMBER_P (item, iface->modlist_ofs);

  newmod = ipatch_sf2_mod_duplicate (mod);                      // ++ duplicate the new modulator

  IPATCH_ITEM_WLOCK (item);
  newlist = ipatch_sf2_mod_list_duplicate (*pmods);             // ++ Duplicate current list
  newlist = g_slist_insert (newlist, newmod, pos);
  oldlist = *pmods;
  *pmods = newlist;                                             // !! Item takes over new modulator list
  newlist = ipatch_sf2_mod_list_duplicate (newlist);            // ++ Duplicate newlist for notify
  IPATCH_ITEM_WUNLOCK (item);

  g_value_init (&old_value, IPATCH_TYPE_SF2_MOD_LIST);
  g_value_take_boxed (&old_value, oldlist);                   // -- Take over oldlist

  g_value_init (&new_value, IPATCH_TYPE_SF2_MOD_LIST);
  g_value_take_boxed (&new_value, newlist);                   // -- Take over newlist

  ipatch_item_prop_notify (IPATCH_ITEM (item), iface->mod_pspec, &new_value, &old_value);
  g_value_unset (&new_value);
  g_value_unset (&old_value);
}

/**
 * ipatch_sf2_mod_item_remove:
 * @item: Item with modulators
 * @mod: (transfer none): Matching values of modulator to remove
 *
 * Remove a modulator from an item with modulators. The modulator values in @mod
 * are used to search the modulator list. The first modulator
 * that matches all fields in @mod is removed.
 */
void
ipatch_sf2_mod_item_remove (IpatchSF2ModItem *item, const IpatchSF2Mod *mod)
{
  GValue old_value = { 0 }, new_value = { 0 };
  GSList **pmods, *oldlist, *newlist;
  IpatchSF2ModItemIface *iface;
  gboolean changed;

  g_return_if_fail (IPATCH_IS_SF2_MOD_ITEM (item));
  g_return_if_fail (mod != NULL);

  /* get pointer to GSList from IpatchSF2ModItemIface->modlist_ofs */
  iface = IPATCH_SF2_MOD_ITEM_GET_IFACE (item);
  g_return_if_fail (iface->modlist_ofs != 0);
  pmods = (GSList **)G_STRUCT_MEMBER_P (item, iface->modlist_ofs);

  IPATCH_ITEM_WLOCK (item);
  newlist = ipatch_sf2_mod_list_duplicate (*pmods);                     // ++ Duplicate current list
  newlist = ipatch_sf2_mod_list_remove (newlist, mod, &changed);        // Remove the modulator
  oldlist = *pmods;
  *pmods = newlist;                                             // !! Item takes over new modulator list

  if (changed)
    newlist = ipatch_sf2_mod_list_duplicate (newlist);            // ++ Duplicate newlist for notify
  IPATCH_ITEM_WUNLOCK (item);

  if (changed)
  {
    g_value_init (&old_value, IPATCH_TYPE_SF2_MOD_LIST);
    g_value_take_boxed (&old_value, oldlist);                   // -- Take over oldlist

    g_value_init (&new_value, IPATCH_TYPE_SF2_MOD_LIST);
    g_value_take_boxed (&new_value, newlist);                   // -- Take over newlist

    ipatch_item_prop_notify (IPATCH_ITEM (item), iface->mod_pspec, &new_value, &old_value);
    g_value_unset (&new_value);
    g_value_unset (&old_value);
  }
  else
  {
    ipatch_sf2_mod_list_free (oldlist, TRUE);                   // -- free oldlist
    ipatch_sf2_mod_list_free (newlist, TRUE);                   // -- free newlist
  }
}

/**
 * ipatch_sf2_mod_item_change:
 * @item: Item with modulators
 * @oldmod: (transfer none): Current values of modulator to set
 * @newmod: (transfer none): New modulator values
 *
 * Sets the values of an existing modulator in an item with modulators. The
 * modulator list in item is searched for a modulator that matches the values in
 * @oldmod. If a modulator is found its values are set to those in @newmod.
 * If it is not found, nothing is done.
 */
void
ipatch_sf2_mod_item_change (IpatchSF2ModItem *item, const IpatchSF2Mod *oldmod,
			    const IpatchSF2Mod *newmod)
{
  GValue old_value = { 0 }, new_value = { 0 };
  GSList **pmods, *oldlist, *newlist;
  IpatchSF2ModItemIface *iface;
  gboolean changed;

  g_return_if_fail (IPATCH_IS_SF2_MOD_ITEM (item));
  g_return_if_fail (oldmod != NULL);
  g_return_if_fail (newmod != NULL);

  /* get pointer to GSList from IpatchSF2ModItemIface->modlist_ofs */
  iface = IPATCH_SF2_MOD_ITEM_GET_IFACE (item);
  g_return_if_fail (iface->modlist_ofs != 0);
  pmods = (GSList **)G_STRUCT_MEMBER_P (item, iface->modlist_ofs);

  IPATCH_ITEM_WLOCK (item);
  newlist = ipatch_sf2_mod_list_duplicate (*pmods);                     // ++ Duplicate current list
  changed = ipatch_sf2_mod_list_change (newlist, oldmod, newmod);       // Change the modulator
  oldlist = *pmods;
  *pmods = newlist;                                             // !! Item takes over new modulator list

  if (changed)
    newlist = ipatch_sf2_mod_list_duplicate (newlist);            // ++ Duplicate newlist for notify
  IPATCH_ITEM_WUNLOCK (item);

  if (changed)
  {
    g_value_init (&old_value, IPATCH_TYPE_SF2_MOD_LIST);
    g_value_take_boxed (&old_value, oldlist);                   // -- Take over oldlist

    g_value_init (&new_value, IPATCH_TYPE_SF2_MOD_LIST);
    g_value_take_boxed (&new_value, newlist);                   // -- Take over newlist

    ipatch_item_prop_notify (IPATCH_ITEM (item), iface->mod_pspec, &new_value, &old_value);
    g_value_unset (&new_value);
    g_value_unset (&old_value);
  }
  else
  {
    ipatch_sf2_mod_list_free (oldlist, TRUE);                   // -- free oldlist
    ipatch_sf2_mod_list_free (newlist, TRUE);                   // -- free newlist
  }
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
