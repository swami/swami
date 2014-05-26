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
#ifndef __IPATCH_VIRTUAL_CONTAINER_TYPES_H__
#define __IPATCH_VIRTUAL_CONTAINER_TYPES_H__

#include <glib.h>
#include <glib-object.h>

/* forward type declarations */

#include <libinstpatch/IpatchVirtualContainer.h>

#define IPATCH_TYPE_VIRTUAL_DLS2_MELODIC \
  (ipatch_virtual_dls2_melodic_get_type ())
#define IPATCH_IS_VIRTUAL_DLS2_MELODIC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_VIRTUAL_DLS2_MELODIC))
#define IPATCH_TYPE_VIRTUAL_DLS2_PERCUSSION \
  (ipatch_virtual_dls2_percussion_get_type ())
#define IPATCH_IS_VIRTUAL_DLS2_PERCUSSION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_VIRTUAL_DLS2_PERCUSSION))
#define IPATCH_TYPE_VIRTUAL_DLS2_SAMPLES \
  (ipatch_virtual_dls2_samples_get_type ())
#define IPATCH_IS_VIRTUAL_DLS2_SAMPLES(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_VIRTUAL_DLS2_SAMPLES))

#define IPATCH_TYPE_VIRTUAL_GIG_MELODIC \
  (ipatch_virtual_gig_melodic_get_type ())
#define IPATCH_IS_VIRTUAL_GIG_MELODIC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_VIRTUAL_GIG_MELODIC))
#define IPATCH_TYPE_VIRTUAL_GIG_PERCUSSION \
  (ipatch_virtual_gig_percussion_get_type ())
#define IPATCH_IS_VIRTUAL_GIG_PERCUSSION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_VIRTUAL_GIG_PERCUSSION))
#define IPATCH_TYPE_VIRTUAL_GIG_SAMPLES \
  (ipatch_virtual_gig_samples_get_type ())
#define IPATCH_IS_VIRTUAL_GIG_SAMPLES(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_VIRTUAL_GIG_SAMPLES))

#define IPATCH_TYPE_VIRTUAL_SF2_INST \
  (ipatch_virtual_sf2_inst_get_type ())
#define IPATCH_IS_VIRTUAL_SF2_INST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_VIRTUAL_SF2_INST))
#define IPATCH_TYPE_VIRTUAL_SF2_MELODIC \
  (ipatch_virtual_sf2_melodic_get_type ())
#define IPATCH_IS_VIRTUAL_SF2_MELODIC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_VIRTUAL_SF2_MELODIC))
#define IPATCH_TYPE_VIRTUAL_SF2_PERCUSSION \
  (ipatch_virtual_sf2_percussion_get_type ())
#define IPATCH_IS_VIRTUAL_SF2_PERCUSSION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_VIRTUAL_SF2_PERCUSSION))
#define IPATCH_TYPE_VIRTUAL_SF2_SAMPLES \
  (ipatch_virtual_sf2_samples_get_type ())
#define IPATCH_IS_VIRTUAL_SF2_SAMPLES(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_VIRTUAL_SF2_SAMPLES))
#define IPATCH_TYPE_VIRTUAL_SF2_ROM \
  (ipatch_virtual_sf2_rom_get_type ())
#define IPATCH_IS_VIRTUAL_SF2_ROM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_VIRTUAL_SF2_ROM))

#define IPATCH_TYPE_VIRTUAL_SLI_INST \
  (ipatch_virtual_sli_inst_get_type ())
#define IPATCH_IS_VIRTUAL_SLI_INST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_VIRTUAL_SLI_INST))
#define IPATCH_TYPE_VIRTUAL_SLI_SAMPLES \
  (ipatch_virtual_sli_samples_get_type ())
#define IPATCH_IS_VIRTUAL_SLI_SAMPLES(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_VIRTUAL_SLI_SAMPLES))


GType ipatch_virtual_dls2_melodic_get_type (void);
GType ipatch_virtual_dls2_percussion_get_type (void);
GType ipatch_virtual_dls2_samples_get_type (void);

GType ipatch_virtual_gig_melodic_get_type (void);
GType ipatch_virtual_gig_percussion_get_type (void);
GType ipatch_virtual_gig_samples_get_type (void);

GType ipatch_virtual_sf2_inst_get_type (void);
GType ipatch_virtual_sf2_melodic_get_type (void);
GType ipatch_virtual_sf2_percussion_get_type (void);
GType ipatch_virtual_sf2_samples_get_type (void);
GType ipatch_virtual_sf2_rom_get_type (void);

GType ipatch_virtual_sli_inst_get_type (void);
GType ipatch_virtual_sli_samples_get_type (void);

#endif
