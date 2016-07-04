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
#ifndef __IPATCH_DLS2_INST_H__
#define __IPATCH_DLS2_INST_H__

#include <glib.h>
#include <glib-object.h>

/* forward type declarations */
typedef struct _IpatchDLS2Inst IpatchDLS2Inst;
typedef struct _IpatchDLS2InstClass IpatchDLS2InstClass;

#include <libinstpatch/IpatchContainer.h>
#include <libinstpatch/IpatchDLS2Conn.h>
#include <libinstpatch/IpatchDLS2Info.h>

#define IPATCH_TYPE_DLS2_INST   (ipatch_dls2_inst_get_type ())
#define IPATCH_DLS2_INST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_DLS2_INST, \
  IpatchDLS2Inst))
#define IPATCH_DLS2_INST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_DLS2_INST, \
  IpatchDLS2InstClass))
#define IPATCH_IS_DLS2_INST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_DLS2_INST))
#define IPATCH_IS_DLS2_INST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_DLS2_INST))
#define IPATCH_DLS2_INST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_DLS2_INST, \
  IpatchDLS2InstClass))

/* DLS instrument object */
struct _IpatchDLS2Inst
{
  IpatchContainer parent_instance;

  guint16 bank;			/* MIDI locale Bank */
  guint16 program;		/* MIDI locale Program */

  IpatchDLS2Info *info;	       /* info strings */
  GSList *regions;	     /* list of IpatchDLS2Region structures */
  GSList *conns;	/* list of global IpatchDLS2Conn structures */

  guint8 *dlid;			/* 16 byte unique ID or NULL */
};

struct _IpatchDLS2InstClass
{
  IpatchContainerClass parent_class;
};

/**
 * IPATCH_DLS2_INST_BANK_MAX:
 * Max value for instrument MIDI bank (14 bits = 2 normalized MIDI bytes)
 */
#define IPATCH_DLS2_INST_BANK_MAX  0x3FFF

/**
 * IpatchDLS2InstFlags:
 * @IPATCH_DLS2_INST_PERCUSSION: Set if percussion instrument
 */
typedef enum
{
  IPATCH_DLS2_INST_PERCUSSION = 1 << IPATCH_CONTAINER_UNUSED_FLAG_SHIFT
} IpatchDLS2InstFlags;

/**
 * IPATCH_DLS2_INST_UNUSED_FLAG_SHIFT: (skip)
 */
/* 1 flag */
#define IPATCH_DLS2_INST_UNUSED_FLAG_SHIFT \
  (IPATCH_CONTAINER_UNUSED_FLAG_SHIFT + 1)


GType ipatch_dls2_inst_get_type (void);
IpatchDLS2Inst *ipatch_dls2_inst_new (void);

#define ipatch_dls2_inst_get_regions(inst) \
    ipatch_container_get_children (IPATCH_CONTAINER (inst), \
				   IPATCH_TYPE_DLS2_REGION)

IpatchDLS2Inst *ipatch_dls2_inst_first (IpatchIter *iter);
IpatchDLS2Inst *ipatch_dls2_inst_next (IpatchIter *iter);

char *ipatch_dls2_inst_get_info (IpatchDLS2Inst *inst, guint32 fourcc);
void ipatch_dls2_inst_set_info (IpatchDLS2Inst *inst, guint32 fourcc,
				const char *val);
void ipatch_dls2_inst_set_midi_locale (IpatchDLS2Inst *inst, int bank,
				       int program);
void ipatch_dls2_inst_get_midi_locale (IpatchDLS2Inst *inst, int *bank,
				       int *program);
int ipatch_dls2_inst_compare (const IpatchDLS2Inst *p1,
			      const IpatchDLS2Inst *p2);

GSList *ipatch_dls2_inst_get_conns (IpatchDLS2Inst *inst);
void ipatch_dls2_inst_set_conn (IpatchDLS2Inst *inst,
				const IpatchDLS2Conn *conn);
void ipatch_dls2_inst_unset_conn (IpatchDLS2Inst *inst,
				  const IpatchDLS2Conn *conn);
void ipatch_dls2_inst_unset_all_conns (IpatchDLS2Inst *inst);
guint ipatch_dls2_inst_conn_count (IpatchDLS2Inst *inst);

#endif
