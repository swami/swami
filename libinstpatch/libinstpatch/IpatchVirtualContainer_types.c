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
 * SECTION: IpatchVirtualContainer_types
 * @short_description: Built in virtual container types
 * @see_also: 
 * @stability: Stable
 */
#include <glib.h>
#include <glib-object.h>
#include "IpatchVirtualContainer_types.h"
#include "IpatchDLS2Inst.h"
#include "IpatchDLS2Sample.h"
#include "IpatchGigInst.h"
#include "IpatchGigSample.h"
#include "IpatchSF2Preset.h"
#include "IpatchSF2Inst.h"
#include "IpatchSF2Sample.h"
#include "IpatchSLIInst.h"
#include "IpatchSLISample.h"
#include "IpatchTypeProp.h"
#include "i18n.h"

IPATCH_VIRTUAL_CONTAINER_CREATE
  (ipatch_virtual_dls2_melodic, IpatchVirtualDLS2Melodic,
   _("Melodic Instruments"), _("Non percussion instruments"),
   IPATCH_TYPE_DLS2_INST);
IPATCH_VIRTUAL_CONTAINER_CREATE
  (ipatch_virtual_dls2_percussion, IpatchVirtualDLS2Percussion,
   _("Percussion Instruments"), _("Percussion Instruments"),
   IPATCH_TYPE_DLS2_INST);
IPATCH_VIRTUAL_CONTAINER_CREATE
  (ipatch_virtual_dls2_samples, IpatchVirtualDLS2Samples,
   _("Samples"), _("Samples"),
   IPATCH_TYPE_DLS2_SAMPLE);

IPATCH_VIRTUAL_CONTAINER_CREATE
  (ipatch_virtual_gig_melodic, IpatchVirtualGigMelodic,
   _("Melodic Instruments"), _("Non percussion instruments"),
   IPATCH_TYPE_GIG_INST);
IPATCH_VIRTUAL_CONTAINER_CREATE
  (ipatch_virtual_gig_percussion, IpatchVirtualGigPercussion,
   _("Percussion Instruments"), _("Percussion Instruments"),
   IPATCH_TYPE_GIG_INST);
IPATCH_VIRTUAL_CONTAINER_CREATE
  (ipatch_virtual_gig_samples, IpatchVirtualGigSamples,
   _("Samples"), _("Samples"), IPATCH_TYPE_GIG_SAMPLE);

IPATCH_VIRTUAL_CONTAINER_CREATE
  (ipatch_virtual_sf2_inst, IpatchVirtualSF2Inst,
   _("Instruments"), _("Instruments"), IPATCH_TYPE_SF2_INST);
IPATCH_VIRTUAL_CONTAINER_CREATE
  (ipatch_virtual_sf2_melodic, IpatchVirtualSF2Melodic,
   _("Melodic Presets"), _("Non percussion presets"), IPATCH_TYPE_SF2_PRESET);
IPATCH_VIRTUAL_CONTAINER_CREATE
  (ipatch_virtual_sf2_percussion, IpatchVirtualSF2Percussion,
   _("Percussion Presets"), _("Percussion Presets"), IPATCH_TYPE_SF2_PRESET);
IPATCH_VIRTUAL_CONTAINER_CREATE
  (ipatch_virtual_sf2_samples, IpatchVirtualSF2Samples,
   _("Samples"), _("Samples"), IPATCH_TYPE_SF2_SAMPLE);
IPATCH_VIRTUAL_CONTAINER_CREATE
  (ipatch_virtual_sf2_rom, IpatchVirtualSF2Rom,
   _("ROM Samples"), _("ROM Samples"), IPATCH_TYPE_SF2_SAMPLE);

IPATCH_VIRTUAL_CONTAINER_CREATE
  (ipatch_virtual_sli_inst, IpatchVirtualSLIInst,
   _("Instruments"), _("Instruments"), IPATCH_TYPE_SLI_INST);
IPATCH_VIRTUAL_CONTAINER_CREATE
  (ipatch_virtual_sli_samples, IpatchVirtualSLISamples,
   _("Samples"), _("Samples"), IPATCH_TYPE_SLI_SAMPLE);
