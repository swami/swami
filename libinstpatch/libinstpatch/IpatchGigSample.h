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
#ifndef __IPATCH_GIG_SAMPLE_H__
#define __IPATCH_GIG_SAMPLE_H__

#include <glib.h>
#include <glib-object.h>

/* forward type declarations */
typedef struct _IpatchGigSample IpatchGigSample;
typedef struct _IpatchGigSampleClass IpatchGigSampleClass;

#include <libinstpatch/IpatchDLS2Sample.h>

#define IPATCH_TYPE_GIG_SAMPLE   (ipatch_gig_sample_get_type ())
#define IPATCH_GIG_SAMPLE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_GIG_SAMPLE, \
  IpatchGigSample))
#define IPATCH_GIG_SAMPLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_GIG_SAMPLE, \
  IpatchGigSampleClass))
#define IPATCH_IS_GIG_SAMPLE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_GIG_SAMPLE))
#define IPATCH_IS_GIG_SAMPLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_GIG_SAMPLE))
#define IPATCH_GIG_SAMPLE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_GIG_SAMPLE, \
  IpatchGigSampleClass))

/* GigaSampler sample object */
struct _IpatchGigSample
{
  IpatchDLS2Sample parent_instance;

  guint32 group_number;	/* sample group number - FIXME - what exactly is it? */
};

struct _IpatchGigSampleClass
{
  IpatchDLS2SampleClass parent_class;
};

GType ipatch_gig_sample_get_type (void);
IpatchGigSample *ipatch_gig_sample_new (void);
IpatchGigSample *ipatch_gig_sample_first (IpatchIter *iter);
IpatchGigSample *ipatch_gig_sample_next (IpatchIter *iter);

#endif
