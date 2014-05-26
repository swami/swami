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
#ifndef __IPATCH_GIG_INST_H__
#define __IPATCH_GIG_INST_H__

#include <glib.h>
#include <glib-object.h>

/* forward type declarations */
typedef struct _IpatchGigInst IpatchGigInst;
typedef struct _IpatchGigInstClass IpatchGigInstClass;
typedef struct _IpatchGigInstParams IpatchGigInstParams;

#include <libinstpatch/IpatchDLS2Inst.h>

#define IPATCH_TYPE_GIG_INST   (ipatch_gig_inst_get_type ())
#define IPATCH_GIG_INST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_GIG_INST, \
  IpatchGigInst))
#define IPATCH_GIG_INST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_GIG_INST, \
  IpatchGigInstClass))
#define IPATCH_IS_GIG_INST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_GIG_INST))
#define IPATCH_IS_GIG_INST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_GIG_INST))
#define IPATCH_GIG_INST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_GIG_INST, \
  IpatchGigInstClass))

/* GigaSampler instrument object */
struct _IpatchGigInst
{
  IpatchDLS2Inst parent_instance;

  guint32 attenuate;
  guint16 effect_send;
  guint16 fine_tune;
  guint16 pitch_bend_range;
  guint8 dim_key_start;		/* bit 1: piano release */
  guint8 dim_key_end;

  guint8 chunk_3ewg[12];	/* 3ewg chunk - FIXME what is it? */
};

struct _IpatchGigInstClass
{
  IpatchDLS2InstClass parent_class;
};

GType ipatch_gig_inst_get_type (void);
IpatchGigInst *ipatch_gig_inst_new (void);
IpatchGigInst *ipatch_gig_inst_first (IpatchIter *iter);
IpatchGigInst *ipatch_gig_inst_next (IpatchIter *iter);

#endif
