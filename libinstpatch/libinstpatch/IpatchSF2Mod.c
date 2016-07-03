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
 * SECTION: IpatchSF2Mod
 * @short_description: SoundFont modulators
 * @see_also: 
 * @stability: Stable
 *
 * SoundFont modulators are used to define real time MIDI effect controls.
 */
#include <glib.h>
#include <glib-object.h>
#include "IpatchSF2Mod.h"
#include "ipatch_priv.h"

GType
ipatch_sf2_mod_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_boxed_type_register_static ("IpatchSF2Mod",
				(GBoxedCopyFunc)ipatch_sf2_mod_duplicate,
				(GBoxedFreeFunc)ipatch_sf2_mod_free);

  return (type);
}

/**
 * ipatch_sf2_mod_new: (skip)
 *
 * Create a new modulator
 *
 * Returns: New modulator
 */
IpatchSF2Mod *
ipatch_sf2_mod_new (void)
{
  return (g_slice_new0 (IpatchSF2Mod));
}

/**
 * ipatch_sf2_mod_free: (skip)
 * @mod: Modulator to free, should not be referenced by any zones.
 *
 * Free an #IpatchSF2Mod structure
 */
void
ipatch_sf2_mod_free (IpatchSF2Mod *mod)
{
  g_return_if_fail (mod != NULL);
  g_slice_free (IpatchSF2Mod, mod);
}

/**
 * ipatch_sf2_mod_duplicate: (skip)
 * @mod: Modulator to duplicate
 *
 * Duplicate a modulator
 *
 * Returns: New duplicate modulator
 */
IpatchSF2Mod *
ipatch_sf2_mod_duplicate (const IpatchSF2Mod *mod)
{
  IpatchSF2Mod *newmod;

  g_return_val_if_fail (mod != NULL, NULL);

  newmod = ipatch_sf2_mod_new ();

  newmod->src = mod->src;
  newmod->dest = mod->dest;
  newmod->amount = mod->amount;
  newmod->amtsrc = mod->amtsrc;
  newmod->trans = mod->trans;

  return (newmod);
}

