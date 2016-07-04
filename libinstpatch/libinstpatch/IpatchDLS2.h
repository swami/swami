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
#ifndef __IPATCH_DLS2_H__
#define __IPATCH_DLS2_H__

#include <glib.h>
#include <glib-object.h>

/* forward type declarations */
typedef struct _IpatchDLS2 IpatchDLS2;
typedef struct _IpatchDLS2Class IpatchDLS2Class;

#include <libinstpatch/IpatchBase.h>
#include <libinstpatch/IpatchList.h>
#include <libinstpatch/IpatchDLSFile.h>
#include <libinstpatch/IpatchDLS2Info.h>
#include <libinstpatch/IpatchDLS2Inst.h>
#include <libinstpatch/IpatchDLS2Sample.h>

#define IPATCH_TYPE_DLS2   (ipatch_dls2_get_type ())
#define IPATCH_DLS2(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_DLS2, IpatchDLS2))
#define IPATCH_DLS2_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_DLS2, IpatchDLS2Class))
#define IPATCH_IS_DLS2(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_DLS2))
#define IPATCH_IS_DLS2_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_DLS2))
#define IPATCH_DLS2_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_DLS2, IpatchDLS2Class))

/* DLS level 2 object */
struct _IpatchDLS2
{
  /*< public >*/
  IpatchBase parent_instance;

  /* NOTE: This is not the DLS version! Optional descriptive stamp version */
  guint32 ms_version; /* most significant 32 bits of 64 bit version */
  guint32 ls_version; /* least significant 32 bits of 64 bit version */

  IpatchDLS2Info *info;		/* info strings */
  GSList *insts;		/* list of #IpatchDLS2Inst objects */
  GSList *samples;		/* list of #IpatchDLS2Sample objects */

  guint8 *dlid;	/* 16 bytes or NULL - globally unique ID (indicates changes) */
};

/* DLS level 2 class */
struct _IpatchDLS2Class
{
  IpatchBaseClass parent_class;
};

/**
 * IpatchDLS2Flags:
 * @IPATCH_DLS2_VERSION_SET: Set if the ms_version/ls_version values are valid.
 */
typedef enum
{
  IPATCH_DLS2_VERSION_SET = 1 << IPATCH_BASE_UNUSED_FLAG_SHIFT
} IpatchDLS2Flags;

/* reserve a couple flags for expansion */
/**
 * IPATCH_DLS2_UNUSED_FLAG_SHIFT: (skip)
 */
#define IPATCH_DLS2_UNUSED_FLAG_SHIFT (IPATCH_BASE_UNUSED_FLAG_SHIFT + 4)

GType ipatch_dls2_get_type (void);
IpatchDLS2 *ipatch_dls2_new (void);

#define ipatch_dls2_get_insts(dls) \
    ipatch_container_get_children (IPATCH_CONTAINER (dls), \
				   IPATCH_TYPE_DLS2_INST)
#define ipatch_dls2_get_samples(dls) \
    ipatch_container_get_children (IPATCH_CONTAINER (dls), \
				   IPATCH_TYPE_DLS2_SAMPLE)

void ipatch_dls2_set_file (IpatchDLS2 *dls, IpatchDLSFile *file);
IpatchDLSFile *ipatch_dls2_get_file (IpatchDLS2 *dls);
char *ipatch_dls2_get_info (IpatchDLS2 *dls, guint32 fourcc);
void ipatch_dls2_set_info (IpatchDLS2 *dls, guint32 fourcc, const char *val);
char *ipatch_dls2_make_unique_name (IpatchDLS2 *dls, GType child_type,
				    const char *name,
				    const IpatchItem *exclude);
IpatchDLS2Inst *ipatch_dls2_find_inst (IpatchDLS2 *dls, const char *name,
				       int bank, int program,
				       const IpatchDLS2Inst *exclude);
IpatchDLS2Sample *ipatch_dls2_find_sample (IpatchDLS2 *dls, const char *name,
					   const IpatchDLS2Sample *exclude);
IpatchList *ipatch_dls2_get_region_references (IpatchDLS2Sample *sample);

#endif
