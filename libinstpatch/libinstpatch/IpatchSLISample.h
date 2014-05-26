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
#ifndef __IPATCH_SLI_SAMPLE_H__
#define __IPATCH_SLI_SAMPLE_H__

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchItem.h>
#include <libinstpatch/IpatchSampleData.h>

/* forward type declarations */

typedef struct _IpatchSLISample IpatchSLISample;
typedef struct _IpatchSLISampleClass IpatchSLISampleClass;

#define IPATCH_TYPE_SLI_SAMPLE   (ipatch_sli_sample_get_type ())
#define IPATCH_SLI_SAMPLE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SLI_SAMPLE, \
  IpatchSLISample))
#define IPATCH_SLI_SAMPLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SLI_SAMPLE, \
  IpatchSLISampleClass))
#define IPATCH_IS_SLI_SAMPLE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SLI_SAMPLE))
#define IPATCH_IS_SLI_SAMPLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SLI_SAMPLE))
#define IPATCH_SLI_SAMPLE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_SLI_SAMPLE, \
  IpatchSLISampleClass))

/* Spectralis sample item */
struct _IpatchSLISample
{
  IpatchItem parent_instance;

  IpatchSampleData *sample_data; /* sample data object */
  char *name;			/* name of sample */
  guint32 rate;                 /* sample rate */
  guint32 loop_start;		/* loop start offset (in samples) */
  guint32 loop_end;		/* loop end offset (in samples) */
  guint8 root_note;		/* root midi note number */
  gint8 fine_tune;		/* fine tuning in cents */
};

struct _IpatchSLISampleClass
{
  IpatchItemClass parent_class;
};

GType ipatch_sli_sample_get_type (void);
IpatchSLISample *ipatch_sli_sample_new (void);

IpatchSLISample *ipatch_sli_sample_first (IpatchIter *iter);
IpatchSLISample *ipatch_sli_sample_next (IpatchIter *iter);

void ipatch_sli_sample_set_name (IpatchSLISample *sample,
                                 const char *name);
char *ipatch_sli_sample_get_name (IpatchSLISample *sample);

void ipatch_sli_sample_set_data (IpatchSLISample *sample,
				 IpatchSampleData *sampledata);
IpatchSampleData *ipatch_sli_sample_get_data (IpatchSLISample *sample);
IpatchSampleData *ipatch_sli_sample_peek_data (IpatchSLISample *sample);

void ipatch_sli_sample_set_blank (IpatchSLISample *sample);

#endif
