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
 * SECTION: IpatchSampleStoreVirtual
 * @short_description: Virtual sample storage object
 * @see_also: #IpatchSampleList
 * @stability: Stable
 *
 * A sample store that does in place sample conversions of other samples
 * using sample lists (#IpatchSampleList).
 */
#include <glib.h>
#include <glib-object.h>
#include "IpatchSampleStoreVirtual.h"
#include "ipatch_priv.h"
#include "i18n.h"

/* properties */
enum
{
  PROP_0,
  PROP_SAMPLE_LISTS
};

static void ipatch_sample_store_virtual_sample_iface_init (IpatchSampleIface *iface);
static void
ipatch_sample_store_virtual_set_property (GObject *object, guint property_id,
                                          const GValue *value, GParamSpec *pspec);
static void
ipatch_sample_store_virtual_get_property (GObject *object, guint property_id,
                                          GValue *value, GParamSpec *pspec);
static void ipatch_sample_store_virtual_finalize (GObject *object);
static gboolean
ipatch_sample_store_virtual_sample_iface_open (IpatchSampleHandle *handle,
                                               GError **err);
static void
ipatch_sample_store_virtual_sample_iface_close (IpatchSampleHandle *handle);
static gboolean
ipatch_sample_store_virtual_sample_iface_read (IpatchSampleHandle *handle,
                                               guint offset, guint frames,
                                               gpointer buf, GError **err);


G_DEFINE_TYPE_WITH_CODE (IpatchSampleStoreVirtual, ipatch_sample_store_virtual,
                         IPATCH_TYPE_SAMPLE_STORE,
                         G_IMPLEMENT_INTERFACE (IPATCH_TYPE_SAMPLE,
                                                ipatch_sample_store_virtual_sample_iface_init))

static void
ipatch_sample_store_virtual_sample_iface_init (IpatchSampleIface *iface)
{
  iface->open = ipatch_sample_store_virtual_sample_iface_open;
  iface->close = ipatch_sample_store_virtual_sample_iface_close;
  iface->read = ipatch_sample_store_virtual_sample_iface_read;
}

static void
ipatch_sample_store_virtual_class_init (IpatchSampleStoreVirtualClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);

  obj_class->finalize = ipatch_sample_store_virtual_finalize;
  obj_class->get_property = ipatch_sample_store_virtual_get_property;
  item_class->item_set_property = ipatch_sample_store_virtual_set_property;


  g_object_class_install_property (obj_class, PROP_SAMPLE_LISTS,
      g_param_spec_value_array ("sample-lists", "Sample lists", "Sample lists",
                                g_param_spec_boxed ("value", "value", "value",
                                                    IPATCH_TYPE_SAMPLE_LIST, G_PARAM_READWRITE),
                                G_PARAM_READWRITE));
}

static void
ipatch_sample_store_virtual_set_property (GObject *object, guint property_id,
                                          const GValue *value, GParamSpec *pspec)
{
  IpatchSampleStoreVirtual *store = IPATCH_SAMPLE_STORE_VIRTUAL (object);
  IpatchSampleList *list;
  GValueArray *array;
  int chan;

  switch (property_id)
  {
    case PROP_SAMPLE_LISTS:
      array = g_value_get_boxed (value);

      for (chan = 0; chan < 2; chan++)
      {
        if (array && chan < array->n_values)
          list = g_value_dup_boxed (g_value_array_get_nth (array, chan));
        else list = NULL;

        ipatch_sample_store_virtual_set_list (store, chan, list);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
ipatch_sample_store_virtual_get_property (GObject *object, guint property_id,
                                          GValue *value, GParamSpec *pspec)
{
  IpatchSampleStoreVirtual *store = IPATCH_SAMPLE_STORE_VIRTUAL (object);
  GValueArray *array;
  GValue local_value = { 0 };
  int chan;

  switch (property_id)
  {
    case PROP_SAMPLE_LISTS:
      array = g_value_array_new (0);    /* ++ alloc new value array */

      for (chan = 0; chan < 2 && store->lists[chan]; chan++)
      {
        g_value_init (&local_value, IPATCH_TYPE_SAMPLE_LIST);
        g_value_set_boxed (&local_value, store->lists[chan]);
        g_value_array_append (array, &local_value);
        g_value_unset (&local_value);
      }

      g_value_take_boxed (value, array);        /* !takes over ownership of array */
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void ipatch_sample_store_virtual_init (IpatchSampleStoreVirtual *store)
{
}

static void
ipatch_sample_store_virtual_finalize (GObject *object)
{
  IpatchSampleStoreVirtual *vstore = IPATCH_SAMPLE_STORE_VIRTUAL (object);

  if (vstore->lists[0]) ipatch_sample_list_free (vstore->lists[0]);
  if (vstore->lists[1]) ipatch_sample_list_free (vstore->lists[1]);

  if (G_OBJECT_CLASS (ipatch_sample_store_virtual_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_sample_store_virtual_parent_class)->finalize (object);
}

static gboolean
ipatch_sample_store_virtual_sample_iface_open (IpatchSampleHandle *handle,
                                               GError **err)
{
  IpatchSampleStoreVirtual *store = IPATCH_SAMPLE_STORE_VIRTUAL (handle->sample);
  int format, channels;

  g_return_val_if_fail (store->lists[0] != NULL, FALSE);

  format = ipatch_sample_store_get_format (store);
  channels = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (format);

  g_return_val_if_fail (channels >= 1 && channels <= 2, FALSE);

  handle->data1 = GINT_TO_POINTER (format);

  if (channels == 2)
  {
    g_return_val_if_fail (store->lists[1] != NULL, FALSE);

    handle->data2 = g_malloc (IPATCH_SAMPLE_TRANS_BUFFER_SIZE); /* Allocate stereo interleave buffer */
    handle->data3 = GUINT_TO_POINTER (ipatch_sample_format_width (format)); /* Format width in bytes */
  }

  return (TRUE);
}

static void
ipatch_sample_store_virtual_sample_iface_close (IpatchSampleHandle *handle)
{
  g_free (handle->data2);       /* Free stereo interleave buffer */
}

static gboolean
ipatch_sample_store_virtual_sample_iface_read (IpatchSampleHandle *handle,
                                               guint offset, guint frames,
                                               gpointer buf, GError **err)
{
  IpatchSampleStoreVirtual *store = IPATCH_SAMPLE_STORE_VIRTUAL (handle->sample);
  int format = GPOINTER_TO_INT (handle->data1);
  gpointer interbuf = handle->data2;    /* Stereo interleave buffer */
  int bytewidth = GPOINTER_TO_INT (handle->data3);
  guint8 *bbuf, *bleft, *bright;
  guint16 *wbuf, *wleft, *wright;
  guint32 *dbuf, *dleft, *dright;
  guint64 *qbuf, *qleft, *qright;
  int mi, si;
  int block;

  if (!interbuf)   /* Store is mono? - Just render it */
  {
    if (!ipatch_sample_list_render (store->lists[0], buf, offset, frames, format, err))
      return (FALSE);

    return (TRUE);
  }

  /* Stereo virtual store */

  /* Max samples that can be interleaved at a time */
  block = IPATCH_SAMPLE_TRANS_BUFFER_SIZE / bytewidth / 2;

  while (frames > 0)
  {
    if (block > frames) block = frames;

    if (!ipatch_sample_list_render (store->lists[0], interbuf, offset, block, format, err))
      return (FALSE);

    if (!ipatch_sample_list_render (store->lists[1],
                                    interbuf + IPATCH_SAMPLE_TRANS_BUFFER_SIZE / 2,
                                    offset, block, format, err))
      return (FALSE);

    /* choose interleaving based on sample width */
    if (bytewidth == 1)
    {
      bbuf = (guint8 *)buf;
      bleft = (guint8 *)interbuf;
      bright = (guint8 *)(interbuf + IPATCH_SAMPLE_TRANS_BUFFER_SIZE / 2);

      for (mi = 0, si = 0; mi < block; mi++, si += 2)
      {
        bbuf[si] = bleft[mi];
        bbuf[si+1] = bright[mi];
      }

      buf = bbuf;
    }
    else if (bytewidth == 2)
    {
      wbuf = (guint16 *)buf;
      wleft = (guint16 *)interbuf;
      wright = (guint16 *)(interbuf + IPATCH_SAMPLE_TRANS_BUFFER_SIZE / 2);

      for (mi = 0, si = 0; mi < block; mi++, si += 2)
      {
        wbuf[si] = wleft[mi];
        wbuf[si+1] = wright[mi];
      }

      buf = wbuf;
    }
    else if (bytewidth == 3)    /* Real 24 bit - Copy as 16 bit word and 1 byte */
    {
      bbuf = (guint8 *)buf;
      bleft = (guint8 *)interbuf;
      bright = (guint8 *)(interbuf + IPATCH_SAMPLE_TRANS_BUFFER_SIZE / 2);

      for (mi = 0, si = 0; mi < block; mi += 3, si += 6)
      {
        *(guint16 *)(bbuf + si) = *(guint16 *)(bleft + mi);
        bbuf[si+2] = bleft[mi+2];
        *(guint16 *)(bbuf + si + 3) = *(guint16 *)(bright + mi);
        bbuf[si+5] = bright[mi+2];
      }

      buf = bbuf;
    }
    else if (bytewidth == 4)
    {
      dbuf = (guint32 *)buf;
      dleft = (guint32 *)interbuf;
      dright = (guint32 *)(interbuf + IPATCH_SAMPLE_TRANS_BUFFER_SIZE / 2);

      for (mi = 0, si = 0; mi < block; mi++, si += 2)
      {
        dbuf[si] = dleft[mi];
        dbuf[si+1] = dright[mi];
      }

      buf = dbuf;
    }
    else if (bytewidth == 8)
    {
      qbuf = (guint64 *)buf;
      qleft = (guint64 *)interbuf;
      qright = (guint64 *)(interbuf + IPATCH_SAMPLE_TRANS_BUFFER_SIZE / 2);

      for (mi = 0, si = 0; mi < block; mi++, si += 2)
      {
        qbuf[si] = qleft[mi];
        qbuf[si+1] = qright[mi];
      }

      buf = qbuf;
    }
    else g_return_val_if_fail (NOT_REACHED, FALSE);

    frames -= block;
    offset += block;
  }

  return (TRUE);
}

/**
 * ipatch_sample_store_virtual_new:
 *
 * Creates a new virtual sample store.
 *
 * Returns: (type IpatchSampleStoreVirtual): New virtual sample store, cast
 *   as a #IpatchSample for convenience.
 */
IpatchSample *
ipatch_sample_store_virtual_new (void)
{
  return (IPATCH_SAMPLE (g_object_new (IPATCH_TYPE_SAMPLE_STORE_VIRTUAL, NULL)));
}

/**
 * ipatch_sample_store_virtual_get_list:
 * @store: Virtual store to get sample list from
 * @chan: Which channel to get sample list from (0 = mono or left stereo channel,
 *   1 = right stereo channel).
 *
 * Gets a sample list from a virtual sample store.
 *
 * Returns: (transfer none): The sample list for the corresponding channel or %NULL if not
 *   assigned.  The list is internal and should not be modified or freed and
 *   should be used only as long as @store.
 */
IpatchSampleList *
ipatch_sample_store_virtual_get_list (IpatchSampleStoreVirtual *store, guint chan)
{
  guint chancount;
  int format;

  g_return_val_if_fail (IPATCH_IS_SAMPLE_STORE_VIRTUAL (store), NULL);

  format = ipatch_sample_store_get_format (store);
  chancount = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (format);

  g_return_val_if_fail (chancount <= 2, NULL);
  g_return_val_if_fail (chan < chancount, NULL);

  /* no locking required - lists shouldn't be changed after store is active */
  return (store->lists[chan]);
}

/**
 * ipatch_sample_store_virtual_set_list:
 * @store: Virtual store to set sample list of
 * @chan: Which channel to set sample list of (0 = mono or left stereo channel,
 *   1 = right stereo channel).
 * @list: (transfer full): List to assign to virtual store.  The allocation is taken over by
 *   the virtual store (if caller would like to continue using it beyond the
 *   life of @store or modify it, it should be duplicated).
 *
 * Sets a sample list of a virtual sample store.  Can only be assigned before
 * the sample store is active.  The size of @store is set to that of @list.
 */
void
ipatch_sample_store_virtual_set_list (IpatchSampleStoreVirtual *store,
				      guint chan, IpatchSampleList *list)
{
  guint chancount;
  int format;

  g_return_if_fail (IPATCH_IS_SAMPLE_STORE_VIRTUAL (store));

  format = ipatch_sample_store_get_format (store);
  chancount = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (format);

  g_return_if_fail (chancount <= 2);
  g_return_if_fail (chan < chancount);

  if (store->lists[chan]) ipatch_sample_list_free (store->lists[chan]);

  /* no locking required - lists should only be assigned by a single thread */
  store->lists[chan] = list;

  ((IpatchSampleStore *)store)->size = list->total_size;
}
