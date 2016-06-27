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
#ifndef __IPATCH_SF2_MOD_LIST_H__
#define __IPATCH_SF2_MOD_LIST_H__

#include <glib.h>
#include <glib-object.h>

#include <libinstpatch/IpatchItem.h>
#include <libinstpatch/IpatchSF2Mod.h>

/* forward type declarations */

typedef GSList IpatchSF2ModList;

/* IpatchSF2ModList has a glib boxed type */
#define IPATCH_TYPE_SF2_MOD_LIST   (ipatch_sf2_mod_list_get_type ())

GType ipatch_sf2_mod_list_get_type (void);

IpatchSF2ModList *ipatch_sf2_mod_list_duplicate (const IpatchSF2ModList *list);
IpatchSF2ModList *ipatch_sf2_mod_list_override (const IpatchSF2ModList *alist,
                                                const IpatchSF2ModList *blist, gboolean copy);
IpatchSF2ModList *ipatch_sf2_mod_list_override_copy (const IpatchSF2ModList *alist,
                                                     const IpatchSF2ModList *blist);
void ipatch_sf2_mod_list_free (IpatchSF2ModList *list, gboolean free_mods);
void ipatch_sf2_mod_list_boxed_free (IpatchSF2ModList *list);
IpatchSF2ModList *ipatch_sf2_mod_list_insert (IpatchSF2ModList *mods,
                                              const IpatchSF2Mod *modvals, int pos);
IpatchSF2ModList *ipatch_sf2_mod_list_remove (IpatchSF2ModList *mods,
                                              const IpatchSF2Mod *modvals, gboolean *changed);
gboolean ipatch_sf2_mod_list_change (IpatchSF2ModList *mods, const IpatchSF2Mod *oldvals,
				     const IpatchSF2Mod *newvals);
IpatchSF2ModList *ipatch_sf2_mod_list_offset (const IpatchSF2ModList *alist,
                                              const IpatchSF2ModList *blist);
G_CONST_RETURN IpatchSF2ModList *ipatch_sf2_mod_list_get_default (void);

#endif
