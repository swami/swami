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

/* GSList has a glib boxed type */
#define IPATCH_TYPE_SF2_MOD_LIST   (ipatch_sf2_mod_list_get_type ())

GType ipatch_sf2_mod_list_get_type (void);

GSList *ipatch_sf2_mod_list_duplicate (const GSList *list);
GSList *ipatch_sf2_mod_list_override (const GSList *alist,
                                                const GSList *blist, gboolean copy);
GSList *ipatch_sf2_mod_list_override_copy (const GSList *alist, const GSList *blist);
void ipatch_sf2_mod_list_free (GSList *list, gboolean free_mods);
void ipatch_sf2_mod_list_boxed_free (GSList *list);
GSList *ipatch_sf2_mod_list_insert (GSList *mods, const IpatchSF2Mod *modvals, int pos);
GSList *ipatch_sf2_mod_list_remove (GSList *mods, const IpatchSF2Mod *modvals,
                                              gboolean *changed);
gboolean ipatch_sf2_mod_list_change (GSList *mods, const IpatchSF2Mod *oldvals,
				     const IpatchSF2Mod *newvals);
GSList *ipatch_sf2_mod_list_offset (const GSList *alist, const GSList *blist);
G_CONST_RETURN GSList *ipatch_sf2_mod_list_get_default (void);

#endif
