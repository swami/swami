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
#ifndef __IPATCH_SAMPLE_LIST_H__
#define __IPATCH_SAMPLE_LIST_H__

#include <glib.h>
#include <glib-object.h>

typedef struct _IpatchSampleList IpatchSampleList;
typedef struct _IpatchSampleListItem IpatchSampleListItem;

#include <libinstpatch/IpatchSample.h>

/* Boxed type for sample list and list item */
#define IPATCH_TYPE_SAMPLE_LIST   (ipatch_sample_list_get_type ())
#define IPATCH_TYPE_SAMPLE_LIST_ITEM   (ipatch_sample_list_item_get_type ())

/**
 * IpatchSampleList:
 *
 * A sample edit list.  Allows for non-destructive sample editing by defining
 * new audio samples from one or more audio sample segments.  
 */
struct _IpatchSampleList
{
  GList *items;			/* list of IpatchSampleListItem structs */
  guint total_size;		/* total size of audio data in frames */
};

/**
 * IpatchSampleListItem:
 *
 * Defines an audio segment in a #IpatchSampleList.
 */
struct _IpatchSampleListItem
{
  IpatchSample *sample;	        /* Sample for this segment */
  guint ofs;                    /* Offset in sample of segment start */
  guint size;			/* Size in frames of audio segment */
  guint32 channel : 3;          /* Channel to use in sample */
  guint32 reserved : 29;
};


GType ipatch_sample_list_get_type (void);
GType ipatch_sample_list_item_get_type (void);
IpatchSampleList *ipatch_sample_list_new (void);
void ipatch_sample_list_free (IpatchSampleList *list);
IpatchSampleList *ipatch_sample_list_duplicate (IpatchSampleList *list);
IpatchSampleListItem *ipatch_sample_list_item_new (void);
IpatchSampleListItem *ipatch_sample_list_item_new_init (IpatchSample *sample,
                                                        guint ofs, guint size,
				                        guint channel);
void ipatch_sample_list_item_free (IpatchSampleListItem *item);
IpatchSampleListItem *ipatch_sample_list_item_duplicate (IpatchSampleListItem *item);
void ipatch_sample_list_append (IpatchSampleList *list, IpatchSample *sample,
                                guint ofs, guint size, guint channel);
void ipatch_sample_list_prepend (IpatchSampleList *list, IpatchSample *sample,
                                 guint ofs, guint size, guint channel);
void ipatch_sample_list_insert_index (IpatchSampleList *list, guint index,
				      IpatchSample *sample, guint ofs,
				      guint size, guint channel);
void ipatch_sample_list_insert (IpatchSampleList *list, guint pos,
			        IpatchSample *sample, guint ofs,
			        guint size, guint channel);
void ipatch_sample_list_cut (IpatchSampleList *list, guint pos, guint size);
gboolean ipatch_sample_list_render (IpatchSampleList *list, gpointer buf,
                                    guint pos, guint frames, int format, GError **err);
gpointer ipatch_sample_list_render_alloc (IpatchSampleList *list, guint pos, guint size,
                                          int format, GError **err);
#endif
