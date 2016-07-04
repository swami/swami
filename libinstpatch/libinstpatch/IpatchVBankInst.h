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
#ifndef __IPATCH_VBANK_INST_H__
#define __IPATCH_VBANK_INST_H__

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchContainer.h>
#include <libinstpatch/IpatchVBankRegion.h>

/* forward type declarations */

typedef struct _IpatchVBankInst IpatchVBankInst;
typedef struct _IpatchVBankInstClass IpatchVBankInstClass;

#define IPATCH_TYPE_VBANK_INST   (ipatch_vbank_inst_get_type ())
#define IPATCH_VBANK_INST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_VBANK_INST, \
  IpatchVBankInst))
#define IPATCH_VBANK_INST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_VBANK_INST, \
  IpatchVBankInstClass))
#define IPATCH_IS_VBANK_INST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_VBANK_INST))
#define IPATCH_IS_VBANK_INST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_VBANK_INST))
#define IPATCH_VBANK_INST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_VBANK_INST, \
  IpatchVBankInstClass))

/* Virtual bank instrument item */
struct _IpatchVBankInst
{
  IpatchContainer parent_instance;

  char *name;			/* name of instrument */
  guint16 bank;			/* MIDI bank map number */
  guint16 program;		/* MIDI program number */
  GSList *regions;		/* list of instrument regions */
};

struct _IpatchVBankInstClass
{
  IpatchContainerClass parent_class;
};

/**
 * IPATCH_VBANK_INST_NAME_SIZE: (skip)
 *
 * Maximum length of a virtual bank instrument name.
 */
#define IPATCH_VBANK_INST_NAME_SIZE	64

GType ipatch_vbank_inst_get_type (void);
IpatchVBankInst *ipatch_vbank_inst_new (void);

#define ipatch_vbank_inst_get_regions(inst) \
    ipatch_container_get_children (IPATCH_CONTAINER (inst), \
				   IPATCH_TYPE_VBANK_REGION)

IpatchVBankInst *ipatch_vbank_inst_first (IpatchIter *iter);
IpatchVBankInst *ipatch_vbank_inst_next (IpatchIter *iter);

void ipatch_vbank_inst_new_region (IpatchVBankInst *inst, IpatchItem *item);

void ipatch_vbank_inst_set_midi_locale (IpatchVBankInst *inst,
					int bank, int program);
void ipatch_vbank_inst_get_midi_locale (IpatchVBankInst *inst,
					int *bank, int *program);

int ipatch_vbank_inst_compare (const IpatchVBankInst *p1,
			       const IpatchVBankInst *p2);

#endif
