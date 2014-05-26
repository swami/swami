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
#ifndef __IPATCH_GIG_H__
#define __IPATCH_GIG_H__

#include <glib.h>
#include <glib-object.h>

/* NOTE: See IpatchGigFile.h for some info on GigaSampler file format */

typedef struct _IpatchGig IpatchGig;
typedef struct _IpatchGigClass IpatchGigClass;

#include <libinstpatch/IpatchDLS2.h>
#include <libinstpatch/IpatchSF2Gen.h>

#define IPATCH_TYPE_GIG   (ipatch_gig_get_type ())
#define IPATCH_GIG(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_GIG, IpatchGig))
#define IPATCH_GIG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_GIG, IpatchGigClass))
#define IPATCH_IS_GIG(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_GIG))
#define IPATCH_IS_GIG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_GIG))
#define IPATCH_GIG_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_GIG, IpatchGigClass))

/* GigaSampler object */
struct _IpatchGig
{
  IpatchDLS2 parent_instance;	/* derived from DLS2 object */
  GSList *group_names;		/* sample group names */
};

struct _IpatchGigClass
{
  IpatchDLS2Class parent_class;
};

/* Default GigaSampler sample group name */
#define IPATCH_GIG_DEFAULT_SAMPLE_GROUP_NAME "Default Sample Group"

GType ipatch_gig_get_type (void);
IpatchGig *ipatch_gig_new (void);

#endif
