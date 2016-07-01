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
/**
 * SECTION: IpatchSampleList
 * @short_description: Sample list data types and functions
 * @see_also: 
 * @stability: Stable
 *
 * Sample lists define audio data from concatenated segments of other
 * audio sources.  The lists are always mono (a single channel can be
 * selected from multi-channel sources).  Multi-channel audio can be
 * created from combining multiple sample lists of the same length.
 */
#include "IpatchSampleList.h"
#include "ipatch_priv.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

static GList *list_item_free_next (IpatchSampleList *list, GList *itemp);


GType
ipatch_sample_list_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_boxed_type_register_static ("IpatchSampleList",
                                (GBoxedCopyFunc)ipatch_sample_list_duplicate,
                                (GBoxedFreeFunc)ipatch_sample_list_free);
  return (type);
}

GType
ipatch_sample_list_item_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_boxed_type_register_static ("IpatchSampleListItem",
                                (GBoxedCopyFunc)ipatch_sample_list_item_duplicate,
                                (GBoxedFreeFunc)ipatch_sample_list_item_free);
  return (type);
}

/**
 * ipatch_sample_list_new:
 *
 * Creates a new empty (zero item terminated) sample list.
 *
 * Returns: Newly allocated empty sample list.
 */
IpatchSampleList *
ipatch_sample_list_new (void)
{
  return (g_slice_new0 (IpatchSampleList));
}

/**
 * ipatch_sample_list_free:
 * @list: Sample list to free
 *
 * Free a sample list.
 */
void
ipatch_sample_list_free (IpatchSampleList *list)
{
  GList *p;

  g_return_if_fail (list != NULL);

  /* free the items in the list */
  for (p = list->items; p; p = g_list_delete_link (p, p))
    ipatch_sample_list_item_free ((IpatchSampleListItem *)(p->data));

  g_slice_free (IpatchSampleList, list);
}

/**
 * ipatch_sample_list_duplicate:
 * @list: Sample list to duplicate
 *
 * Duplicate a sample list.
 *
 * Returns: Newly allocated duplicate of @list.
 */
IpatchSampleList *
ipatch_sample_list_duplicate (IpatchSampleList *list)
{
  IpatchSampleListItem *item;
  IpatchSampleList *newlist;
  GList *p;

  g_return_val_if_fail (list != NULL, NULL);

  newlist = ipatch_sample_list_new ();
  newlist->total_size = list->total_size;

  for (p = list->items; p; p = p->next)
  {
    item = ipatch_sample_list_item_duplicate ((IpatchSampleListItem *)(p->data));
    newlist->items = g_list_prepend (newlist->items, item);
  }

  newlist->items = g_list_reverse (newlist->items);

  return (newlist);
}

/**
 * ipatch_sample_list_item_new:
 *
 * Create a new node for a sample list.
 *
 * Returns: Newly allocated list node which should be freed with
 * ipatch_sample_list_item_free() when finished using it.
 */
IpatchSampleListItem *
ipatch_sample_list_item_new (void)
{
  return (g_slice_new0 (IpatchSampleListItem));
}

/**
 * ipatch_sample_list_item_new_init:
 * @sample: Sample containing audio for the segment
 * @ofs: Offset in @sample of audio segment
 * @size: Size of audio segment in frames
 * @channel: Channel to use from @sample
 *
 * Create a new sample list item and initialize it with the provided parameters.
 *
 * Returns: Newly allocated sample list item which should be freed with
 * ipatch_sample_list_item_free() when done with it.
 */
IpatchSampleListItem *
ipatch_sample_list_item_new_init (IpatchSample *sample, guint ofs, guint size,
				  guint channel)
{
  IpatchSampleListItem *item;
  guint sample_size;

  g_return_val_if_fail (IPATCH_IS_SAMPLE (sample), NULL);
  g_return_val_if_fail (size > 0, NULL);

  sample_size = ipatch_sample_get_size (sample, NULL);
  g_return_val_if_fail (ofs + size <= sample_size, NULL);

  item = ipatch_sample_list_item_new ();
  item->sample = g_object_ref (sample);
  item->size = size;
  item->ofs = ofs;
  item->channel = channel;

  return (item);
}

/**
 * ipatch_sample_list_item_free:
 * @item: Sample list item to free
 *
 * Free a sample list item previously allocated by ipatch_sample_list_item_new().
 */
void
ipatch_sample_list_item_free (IpatchSampleListItem *item)
{
  if (item->sample) g_object_unref (item->sample);
  g_slice_free (IpatchSampleListItem, item);
}

/**
 * ipatch_sample_list_item_duplicate:
 * @item: Sample list item to duplicate
 *
 * Duplicate a sample list item node.
 *
 * Returns: New duplicate sample list item.
 */
IpatchSampleListItem *
ipatch_sample_list_item_duplicate (IpatchSampleListItem *item)
{
  IpatchSampleListItem *newitem;

  newitem = ipatch_sample_list_item_new ();
  newitem->sample = item->sample ? g_object_ref (item->sample) : NULL;
  newitem->ofs = item->ofs;
  newitem->size = item->size;
  newitem->channel = item->channel;

  return (newitem);
}

/**
 * ipatch_sample_list_append:
 * @list: Sample list
 * @sample: Sample containing audio segment to append
 * @ofs: Offset in @sample of beginning of audio segment (in frames)
 * @size: Number of frames of audio segment to append
 * @channel: Channel to use from @sample
 *
 * Append an audio segment to a sample list.
 */
void
ipatch_sample_list_append (IpatchSampleList *list, IpatchSample *sample,
			   guint ofs, guint size, guint channel)
{
  IpatchSampleListItem *item;

  g_return_if_fail (list != NULL);

  item = ipatch_sample_list_item_new_init (sample, ofs, size, channel);
  g_return_if_fail (item != NULL);

  list->items = g_list_append (list->items, item);
  list->total_size += size;
}

/**
 * ipatch_sample_list_prepend:
 * @list: Sample list
 * @sample: Sample containing audio segment to prepend
 * @ofs: Offset in @sample of beginning of audio segment (in frames)
 * @size: Number of frames of audio segment to prepend
 * @channel: Channel to use from @sample
 *
 * Prepend an audio segment to a sample list.
 */
void
ipatch_sample_list_prepend (IpatchSampleList *list, IpatchSample *sample,
                            guint ofs, guint size, guint channel)
{
  IpatchSampleListItem *item;

  g_return_if_fail (list != NULL);

  item = ipatch_sample_list_item_new_init (sample, ofs, size, channel);
  g_return_if_fail (item != NULL);

  list->items = g_list_prepend (list->items, item);
  list->total_size += size;
}

/**
 * ipatch_sample_list_insert_index:
 * @list: Sample list
 * @index: List index to insert segment before (0 = prepend, -1 = append)
 * @sample: Sample containing audio segment to insert
 * @ofs: Offset in @sample of beginning of audio segment (in frames)
 * @size: Number of frames of audio segment to insert
 * @channel: Channel to use from @sample
 *
 * Insert an audio segment into a sample list before a given list segment
 * @index.
 */
void
ipatch_sample_list_insert_index (IpatchSampleList *list, guint index,
				 IpatchSample *sample, guint ofs,
				 guint size, guint channel)
{
  IpatchSampleListItem *item;

  g_return_if_fail (list != NULL);

  item = ipatch_sample_list_item_new_init (sample, ofs, size, channel);
  g_return_if_fail (item != NULL);

  list->items = g_list_insert (list->items, item, index);
  list->total_size += size;
}

/**
 * ipatch_sample_list_insert:
 * @list: Sample list
 * @pos: Position in audio data in frames to insert segment at
 * @sample: Sample containing audio segment to insert
 * @ofs: Offset in @sample of beginning of audio segment (in frames)
 * @size: Number of frames of audio segment to insert
 * @channel: Channel to use from @sample
 *
 * Insert an audio segment into a sample list at a given sample position
 * in frames (@pos).  Existing segments will be split as needed to accomodate
 * the inserted segment.
 */
void
ipatch_sample_list_insert (IpatchSampleList *list, guint pos,
			   IpatchSample *sample, guint ofs,
			   guint size, guint channel)
{
  IpatchSampleListItem *item, *newitem, *splititem;
  guint startofs, sz;
  GList *p;

  g_return_if_fail (list != NULL);
  g_return_if_fail (pos <= list->total_size);

  newitem = ipatch_sample_list_item_new_init (sample, ofs, size, channel);
  g_return_if_fail (newitem != NULL);

  /* search for segment containing pos */
  for (p = list->items, startofs = 0; p; p = p->next)
  {
    item = (IpatchSampleListItem *)(p->data);
    if (pos >= startofs && pos < (startofs + item->size)) break;
    startofs += item->size;
  }

  /* if list was exhausted, it means pos is past the end - append */
  if (!p) list->items = g_list_append (list->items, newitem);
  else if (pos != startofs)	/* need to split segment? */
  {
    sz = pos - startofs;	/* new size of first split segment */

    /* create 2nd split segment */
    splititem = ipatch_sample_list_item_new_init (item->sample, item->ofs + sz,
						  item->size - sz, item->channel);
    item->size = sz;

    /* insert new item after 1st split segment (assign to suppress GCC warning) */
    p = g_list_insert (p, newitem, 1);

    /* insert 2nd split segment */
    p = g_list_insert (p, splititem, 2);
  }	/* position is at the beginning of another segment, insert before */
  else list->items = g_list_insert_before (list->items, p, newitem);

  list->total_size += size;
}

/**
 * ipatch_sample_list_cut:
 * @list: Sample list
 * @pos: Start position of sample data to cut, in frames
 * @size: Size of area to cut, in frames
 *
 * Cut a segment of audio from a sample list.
 */
void
ipatch_sample_list_cut (IpatchSampleList *list, guint pos, guint size)
{
  IpatchSampleListItem *item = NULL, *newitem;
  guint startofs, fsegsize;
  GList *p;

  g_return_if_fail (list != NULL);
  g_return_if_fail (pos + size <= list->total_size);

  list->total_size -= size;

  /* search for segment containing pos */
  for (p = list->items, startofs = 0; p; p = p->next)
  {
    item = (IpatchSampleListItem *)(p->data);
    if (pos >= startofs && pos < (startofs + item->size)) break;
    startofs += item->size;
  }

  g_return_if_fail (p != NULL);	/* means total_size is out of sync! */

  if (pos == startofs)	/* position is at start of segment? */
  {
    if (size < item->size)	/* size is less than segment size? */
    {	/* increase segment offset and decrease size, we are done */
      item->ofs += size;
      item->size -= size;
      return;
    }

    /* adjust size for remaining after segment removed */
    size -= item->size;		/* !! must be before item delete! */

    /* size is greater or equal to segment - remove segment and advance p */
    p = list_item_free_next (list, p);

    if (size == 0) return;	/* only this segment cut? - done */
  }
  else if (size < item->size - (pos - startofs))  /* cut within segment? */
  {
    fsegsize = pos - startofs;	/* size of first segment before cut */

    /* create new segment for audio after cut */
    newitem = ipatch_sample_list_item_new_init
      (item->sample, item->ofs + fsegsize + size, item->size - fsegsize - size,
       item->channel);

    item->size = fsegsize;

    /* insert 2nd seg after the 1st segment (assign to suppress GCC warning) */
    p = g_list_insert (p, newitem, 1);

    return;	/* done */
  }
  else	/* beginning of cut is within segment and continues into more segments */
  {
    fsegsize = pos - startofs;		/* new size of truncated segment */
    size -= (item->size - fsegsize);	/* update size to remaining after seg */
    item->size = fsegsize;		/* assign new size to segment */

    p = p->next;			/* next segment */
    startofs += fsegsize;		/* startofs of next segment */
  }

  /* search for last segment which is cut, and remove all segments in between */
  while (p)
  {
    item = (IpatchSampleListItem *)(p->data);
    if (size < item->size) break;	/* remaining size is within segment? */

    /* remaining cut size includes entire segment - remove it */
    size -= item->size;		/* !! must be before item delete! */
    p = list_item_free_next (list, p);
  }

  if (p && size > 0)	/* cut goes into this segment? */
  {
    item->ofs += size;		/* advance offset to after cut */
    item->size -= size;		/* decrease segment size by remaining cut size */
  }
}

/* remove the sample item at the given list pointer and return the next item
   in the list */
static GList *
list_item_free_next (IpatchSampleList *list, GList *itemp)
{
  GList *retp;

  retp = itemp->next;

  ipatch_sample_list_item_free ((IpatchSampleListItem *)(itemp->data));
  list->items = g_list_delete_link (list->items, itemp);

  return (retp);
}

/**
 * ipatch_sample_list_render: (skip)
 * @list: Sample list
 * @buf: Buffer to store the rendered audio to (should be @frames * bytes_per_frame)
 * @pos: Position in sample list audio to start from, in frames
 * @frames: Size of sample data to render in frames
 * @format: Sample format to render to (must be mono)
 * @err: Location to store error to or %NULL to ignore
 *
 * Copies sample data from a sample list, converting as necessary and storing
 * to @buf.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_sample_list_render (IpatchSampleList *list, gpointer buf,
                           guint pos, guint frames, int format, GError **err)
{
  IpatchSampleListItem *item = NULL;
  guint startofs, block, format_size;
  GList *p;

  g_return_val_if_fail (list != NULL, FALSE);
  g_return_val_if_fail (ipatch_sample_format_verify (format), FALSE);
  g_return_val_if_fail (pos + frames <= list->total_size, FALSE);
  g_return_val_if_fail (buf != NULL, FALSE);
  g_return_val_if_fail (IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (format) == 1, FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  /* search for segment containing pos */
  for (p = list->items, startofs = 0; p; p = p->next)
  {
    item = (IpatchSampleListItem *)(p->data);
    if (pos >= startofs && pos < (startofs + item->size)) break;
    startofs += item->size;
  }

  g_return_val_if_fail (p != NULL, FALSE);	/* means total_size is out of sync! */

  block = item->size - (pos - startofs);	/* remaining size of segment */
  format_size = ipatch_sample_format_size (format);

  while (frames > 0 && p)
  {
    if (block > frames) block = frames;

    /* Read sample data and transform (if necessary) */
    if (!ipatch_sample_read_transform
        (item->sample, item->ofs + (pos - startofs), block, buf, format,
         IPATCH_SAMPLE_MAP_CHANNEL (0, item->channel), err))
      return (FALSE);

    buf += block * format_size;
    frames -= block;

    p = p->next;

    if (p)
    {
      item = (IpatchSampleListItem *)(p->data);
      block = item->size;
      startofs += item->size;
      pos = startofs;
    }
  }

  g_return_val_if_fail (frames == 0, FALSE);	/* means total_size is out of sync! */

  return (TRUE);
}

/**
 * ipatch_sample_list_render_alloc: (rename-to ipatch_sample_list_render)
 * @list: Sample list
 * @pos: Position in sample list audio to start from, in frames
 * @size: Size of sample data to render in bytes (must be a multiple of frame size)
 * @format: Sample format to render to (must be mono)
 * @err: Location to store error to or %NULL to ignore
 *
 * Copies sample data from a sample list, converting as necessary and returning
 * an allocated buffer.
 *
 * Returns: (array length=size) (element-type guint8) (transfer full): Buffer containing
 *   rendered audio of the requested @format or %NULL on error (in which case @err may be set)
 *
 * Since: 1.1.0
 */
gpointer
ipatch_sample_list_render_alloc (IpatchSampleList *list, guint pos, guint size,
                                 int format, GError **err)
{
  gpointer buf;
  int frame_size;

  g_return_val_if_fail (size > 0, NULL);

  frame_size = ipatch_sample_format_size (format);
  g_return_val_if_fail (size % frame_size == 0, NULL);

  buf = g_malloc (size);        // ++ alloc

  if (!ipatch_sample_list_render (list, buf, pos, size / frame_size, format, err))
  {
    g_free (buf);               // -- free
    return (NULL);
  }

  return (buf);                 // !! caller takes over buffer
}

#ifdef IPATCH_DEBUG
/**
 * ipatch_sample_list_dump: (skip)
 *
 * For debugging purposes, dumps sample list info to stdout.
 */
void
ipatch_sample_list_dump (IpatchSampleList *list)
{
  IpatchSampleListItem *item;
  guint startofs = 0;
  int sample_format;
  guint sample_size;
  GList *p;
  int i = 0;

  printf ("Dump of sample list with %d segments totaling %d frames\n",
	  g_list_length (list->items), list->total_size);

  for (p = list->items; p; p = p->next, i++)
  {
    item = (IpatchSampleListItem *)(p->data);

    if (item->sample)
    {
      sample_format = ipatch_sample_get_format (item->sample);
      sample_size = ipatch_sample_get_size (item->sample, NULL);
    }
    else
    {
      sample_format = 0;
      sample_size = 0;
    }

    printf ("%02d-%06x size=%d ofs=%d chan=%d sample=(%p format %03x size=%d)\n",
	    i, startofs, item->size, item->ofs, item->channel, item->sample,
	    sample_format, sample_size);
    startofs += item->size;
  }
}

#endif
