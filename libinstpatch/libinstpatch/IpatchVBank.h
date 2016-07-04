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
#ifndef __IPATCH_VBANK_H__
#define __IPATCH_VBANK_H__

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchBase.h>

/* forward type declarations */

typedef struct _IpatchVBank IpatchVBank;
typedef struct _IpatchVBankClass IpatchVBankClass;

#include <libinstpatch/IpatchVBankInst.h>

#define IPATCH_TYPE_VBANK   (ipatch_vbank_get_type ())
#define IPATCH_VBANK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_VBANK, \
  IpatchVBank))
#define IPATCH_VBANK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_VBANK, \
  IpatchVBankClass))
#define IPATCH_IS_VBANK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_VBANK))
#define IPATCH_IS_VBANK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_VBANK))
#define IPATCH_VBANK_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_VBANK, \
  IpatchVBankClass))

/**
 * IPATCH_VBANK_INFO_COUNT: (skip)
 *
 * Count of info strings
 */
/* !! Keep synchronized with count of properties in IpatchVBank.c */
#define IPATCH_VBANK_INFO_COUNT		7

/* Virtual bank */
struct _IpatchVBank
{
  IpatchBase parent_instance;

  /*< private >*/
  char *info[IPATCH_VBANK_INFO_COUNT];
  GSList *insts;
};

struct _IpatchVBankClass
{
  IpatchBaseClass parent_class;
};

GType ipatch_vbank_get_type (void);
IpatchVBank *ipatch_vbank_new (void);

#define ipatch_vbank_get_insts(vbank) \
    ipatch_container_get_children (IPATCH_CONTAINER (vbank), \
				   IPATCH_TYPE_VBANK_INST)
IpatchVBankInst *
ipatch_vbank_find_inst (IpatchVBank *vbank, const char *name, int bank,
			int program, const IpatchVBankInst *exclude);
char *
ipatch_vbank_make_unique_name (IpatchVBank *vbank, const char *name,
			       const IpatchVBankInst *exclude);

#endif
