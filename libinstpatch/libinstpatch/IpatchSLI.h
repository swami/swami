/*
 * libInstPatch
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * Author of this file: (C) 2012 BALATON Zoltan <balaton@eik.bme.hu>
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
#ifndef __IPATCH_SLI_H__
#define __IPATCH_SLI_H__

#include <glib.h>
#include <glib-object.h>

/* forward type declarations */
typedef struct _IpatchSLI IpatchSLI;
typedef struct _IpatchSLIClass IpatchSLIClass;

#include <libinstpatch/IpatchBase.h>
#include <libinstpatch/IpatchList.h>
#include <libinstpatch/IpatchSLIFile.h>
#include <libinstpatch/IpatchSLIInst.h>
#include <libinstpatch/IpatchSLISample.h>

#define IPATCH_TYPE_SLI   (ipatch_sli_get_type ())
#define IPATCH_SLI(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SLI, IpatchSLI))
#define IPATCH_SLI_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SLI, IpatchSLIClass))
#define IPATCH_IS_SLI(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SLI))
#define IPATCH_IS_SLI_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SLI))
#define IPATCH_SLI_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_SLI, IpatchSLIClass))

/* Spectralis object */
struct _IpatchSLI
{
  /*< public >*/
  IpatchBase parent_instance;
  GSList *insts;		/* list of #IpatchSLIInst objects */
  GSList *samples;		/* list of #IpatchSLISample objects */
};

/* Spectralis class */
struct _IpatchSLIClass
{
  IpatchBaseClass parent_class;
};

GType ipatch_sli_get_type (void);
IpatchSLI *ipatch_sli_new (void);

#define ipatch_sli_get_insts(sli) \
    ipatch_container_get_children (IPATCH_CONTAINER (sli), \
				   IPATCH_TYPE_SLI_INST)
#define ipatch_sli_get_samples(sli) \
    ipatch_container_get_children (IPATCH_CONTAINER (sli), \
				   IPATCH_TYPE_SLI_SAMPLE)

void ipatch_sli_set_file (IpatchSLI *sli, IpatchSLIFile *file);
IpatchSLIFile *ipatch_sli_get_file (IpatchSLI *sli);

char *ipatch_sli_make_unique_name (IpatchSLI *sli, GType child_type,
                                   const char *name,
                                   const IpatchItem *exclude);
IpatchSLIInst *ipatch_sli_find_inst (IpatchSLI *sli, const char *name,
                                     const IpatchSLIInst *exclude);
IpatchSLISample *ipatch_sli_find_sample (IpatchSLI *sli, const char *name,
                                         const IpatchSLISample *exclude);
IpatchList *ipatch_sli_get_zone_references (IpatchSLISample *sample);

#endif
