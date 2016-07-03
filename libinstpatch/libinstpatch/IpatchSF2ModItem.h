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
#ifndef __IPATCH_SF2_MOD_ITEM_H__
#define __IPATCH_SF2_MOD_ITEM_H__

#include <glib.h>
#include <glib-object.h>

#include <libinstpatch/IpatchSF2Mod.h>

/* forward type declarations */

typedef struct _IpatchSF2ModItem IpatchSF2ModItem;              // dummy typedef
typedef struct _IpatchSF2ModItemIface IpatchSF2ModItemIface;

#define IPATCH_TYPE_SF2_MOD_ITEM   (ipatch_sf2_mod_item_get_type ())
#define IPATCH_SF2_MOD_ITEM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SF2_MOD_ITEM, \
   IpatchSF2ModItem))
#define IPATCH_SF2_MOD_ITEM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SF2_MOD_ITEM, \
   IpatchSF2ModItemIface))
#define IPATCH_IS_SF2_MOD_ITEM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SF2_MOD_ITEM))
#define IPATCH_SF2_MOD_ITEM_GET_IFACE(obj) \
  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), IPATCH_TYPE_SF2_MOD_ITEM, \
   IpatchSF2ModItemIface))

/* modulator item interface */
struct _IpatchSF2ModItemIface
{
  GTypeInterface parent_iface;

  guint modlist_ofs;	/* offset in item instance to modulator list pointer */
  GParamSpec *mod_pspec;  /* cached value of modulator pspec for fast notifies */
};

GType ipatch_sf2_mod_item_get_type (void);
GSList *ipatch_sf2_mod_item_get_mods (IpatchSF2ModItem *item);
void ipatch_sf2_mod_item_set_mods (IpatchSF2ModItem *item, GSList *mod_list,
				   int flags);
void ipatch_sf2_mod_item_set_mods_copy (IpatchSF2ModItem *item, GSList *mod_list);
void ipatch_sf2_mod_item_add (IpatchSF2ModItem *item, const IpatchSF2Mod *mod);
void ipatch_sf2_mod_item_insert (IpatchSF2ModItem *item,
				 const IpatchSF2Mod *mod, int pos);
void ipatch_sf2_mod_item_remove (IpatchSF2ModItem *item,
				 const IpatchSF2Mod *mod);
void ipatch_sf2_mod_item_change (IpatchSF2ModItem *item,
				 const IpatchSF2Mod *oldmod,
				 const IpatchSF2Mod *newmod);
guint ipatch_sf2_mod_item_count (IpatchSF2ModItem *item);

#endif
