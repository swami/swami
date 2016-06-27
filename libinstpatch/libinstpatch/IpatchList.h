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
#ifndef __IPATCH_LIST_H__
#define __IPATCH_LIST_H__

#include <glib.h>
#include <glib-object.h>

typedef struct _IpatchList IpatchList;
typedef struct _IpatchListClass IpatchListClass;

#include <libinstpatch/IpatchIter.h>

#define IPATCH_TYPE_LIST   (ipatch_list_get_type ())
#define IPATCH_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_LIST, IpatchList))
#define IPATCH_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_LIST, IpatchListClass))
#define IPATCH_IS_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_LIST))
#define IPATCH_IS_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_LIST))

/* an object containing a duplicated list of referenced objects */
struct _IpatchList
{
  GObject parent_instance;

  /*< public >*/
  GList *items;			/* list of GObject items */
};

/* class for iterator list object */
struct _IpatchListClass
{
  GObjectClass parent_class;
};

GType ipatch_list_get_type (void);
IpatchList *ipatch_list_new (void);
IpatchList *ipatch_list_duplicate (IpatchList *list);
GList *ipatch_list_get_items (IpatchList *list);
void ipatch_list_set_items (IpatchList *list, GList *items);
void ipatch_list_append (IpatchList *list, GObject *object);
void ipatch_list_prepend (IpatchList *list, GObject *object);
void ipatch_list_insert (IpatchList *list, GObject *object, int pos);
gboolean ipatch_list_remove (IpatchList *list, GObject *object);
void ipatch_list_init_iter (IpatchList *list, IpatchIter *iter);

#endif
