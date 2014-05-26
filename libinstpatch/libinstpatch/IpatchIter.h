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
#ifndef __IPATCH_ITER_H__
#define __IPATCH_ITER_H__

#include <glib.h>
#include <glib-object.h>

typedef struct _IpatchIter IpatchIter;
typedef struct _IpatchIterMethods IpatchIterMethods;

/* boxed type for IpatchIter */
#define IPATCH_TYPE_ITER   (ipatch_iter_get_type ())

/* list iterator structure */
struct _IpatchIter
{
  /*< private >*/
  IpatchIterMethods *methods;	/* iterator methods */
  gpointer data;		/* method defined data */
  gpointer data2;		/* method defined data */
  gpointer data3;		/* method defined data */
  gpointer data4;		/* method defined data */
};

/* iterator methods */
struct _IpatchIterMethods
{
  gpointer (*get)(IpatchIter *iter); /* get item method */
  gpointer (*next)(IpatchIter *iter); /* next item method */
  gpointer (*first)(IpatchIter *iter);	/* first item method */
  gpointer (*last)(IpatchIter *iter); /* last item method */
  gpointer (*index)(IpatchIter *iter, int index); /* index item method */
  void (*insert)(IpatchIter *iter, gpointer item); /* insert item method */
  void (*remove)(IpatchIter *iter); /* remove current item method */
  int (*count)(IpatchIter *iter); /* count items method */
};

GType ipatch_iter_get_type (void);
IpatchIter *ipatch_iter_alloc (void);
void ipatch_iter_free (IpatchIter *iter);
IpatchIter *ipatch_iter_duplicate (IpatchIter *iter);

#define ipatch_iter_get(iter) (((iter)->methods->get)(iter))
#define ipatch_iter_next(iter) (((iter)->methods->next)(iter))
#define ipatch_iter_first(iter) (((iter)->methods->first)(iter))
#define ipatch_iter_last(iter) (((iter)->methods->last)(iter))
#define ipatch_iter_index(iter, pos) (((iter)->methods->index)(iter, pos))
#define ipatch_iter_insert(iter, item) (((iter)->methods->insert)(iter, item))
#define ipatch_iter_remove(iter) (((iter)->methods->remove)(iter))
#define ipatch_iter_count(iter) (((iter)->methods->count)(iter))

#define IPATCH_ITER_GSLIST_GET_LIST(iter) ((GSList **)(iter->data))
#define IPATCH_ITER_GSLIST_GET_POS(iter) ((GSList *)(iter->data2))

#define IPATCH_ITER_GSLIST_SET_LIST(iter, list) (iter->data = list)
#define IPATCH_ITER_GSLIST_SET_POS(iter, pos) (iter->data2 = pos)

void ipatch_iter_GSList_init (IpatchIter *iter, GSList **list);
gpointer ipatch_iter_GSList_get (IpatchIter *iter);
gpointer ipatch_iter_GSList_next (IpatchIter *iter);
gpointer ipatch_iter_GSList_first (IpatchIter *iter);
gpointer ipatch_iter_GSList_last (IpatchIter *iter);
gpointer ipatch_iter_GSList_index (IpatchIter *iter, int index);
void ipatch_iter_GSList_insert (IpatchIter *iter, gpointer item);
void ipatch_iter_GSList_remove (IpatchIter *iter);
int ipatch_iter_GSList_count (IpatchIter *iter);

#define IPATCH_ITER_GLIST_GET_LIST(iter) ((GList **)(iter->data))
#define IPATCH_ITER_GLIST_GET_POS(iter) ((GList *)(iter->data2))

#define IPATCH_ITER_GLIST_SET_LIST(iter, list) (iter->data = list)
#define IPATCH_ITER_GLIST_SET_POS(iter, pos) (iter->data2 = pos)

void ipatch_iter_GList_init (IpatchIter *iter, GList **list);
gpointer ipatch_iter_GList_get (IpatchIter *iter);
gpointer ipatch_iter_GList_next (IpatchIter *iter);
gpointer ipatch_iter_GList_first (IpatchIter *iter);
gpointer ipatch_iter_GList_last (IpatchIter *iter);
gpointer ipatch_iter_GList_index (IpatchIter *iter, int index);
void ipatch_iter_GList_insert (IpatchIter *iter, gpointer item);
void ipatch_iter_GList_remove (IpatchIter *iter);
int ipatch_iter_GList_count (IpatchIter *iter);

#define IPATCH_ITER_ARRAY_GET_ARRAY(iter) ((gpointer *)(iter->data))
#define IPATCH_ITER_ARRAY_GET_SIZE(iter) (GPOINTER_TO_UINT (iter->data2))
#define IPATCH_ITER_ARRAY_GET_POS(iter) (GPOINTER_TO_INT (iter->data3))

#define IPATCH_ITER_ARRAY_SET_ARRAY(iter, array) (iter->data = array)
#define IPATCH_ITER_ARRAY_SET_SIZE(iter, size) \
  (iter->data2 = GUINT_TO_POINTER (size))
#define IPATCH_ITER_ARRAY_SET_POS(iter, pos) \
  (iter->data3 = GINT_TO_POINTER (pos))

void ipatch_iter_array_init (IpatchIter *iter, gpointer *array, guint size);
gpointer ipatch_iter_array_get (IpatchIter *iter);
gpointer ipatch_iter_array_next (IpatchIter *iter);
gpointer ipatch_iter_array_first (IpatchIter *iter);
gpointer ipatch_iter_array_last (IpatchIter *iter);
gpointer ipatch_iter_array_index (IpatchIter *iter, int index);
void ipatch_iter_array_insert (IpatchIter *iter, gpointer item);
void ipatch_iter_array_remove (IpatchIter *iter);
int ipatch_iter_array_count (IpatchIter *iter);

#endif
