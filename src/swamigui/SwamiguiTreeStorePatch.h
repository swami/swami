/*
 * SwamiguiTreeStorePatch.h - Patch tree store (for instruments).
 *
 * Swami
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License only.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA or point your web browser to http://www.gnu.org.
 */
#ifndef __SWAMIGUI_TREE_STORE_PATCH_H__
#define __SWAMIGUI_TREE_STORE_PATCH_H__

typedef struct _SwamiguiTreeStorePatch SwamiguiTreeStorePatch;
typedef struct _SwamiguiTreeStorePatchClass SwamiguiTreeStorePatchClass;

#include <swamigui/SwamiguiTreeStore.h>

#define SWAMIGUI_TYPE_TREE_STORE_PATCH   (swamigui_tree_store_patch_get_type ())
#define SWAMIGUI_TREE_STORE_PATCH(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_TREE_STORE_PATCH, \
   SwamiguiTreeStorePatch))
#define SWAMIGUI_TREE_STORE_PATCH_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_TREE_STORE_PATCH, \
   SwamiguiTreeStorePatchClass))
#define SWAMIGUI_IS_TREE_STORE_PATCH(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_TREE_STORE_PATCH))
#define SWAMIGUI_IS_TREE_STORE_PATCH_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_TREE_STORE_PATCH))

/* Patch tree store object */
struct _SwamiguiTreeStorePatch
{
    SwamiguiTreeStore parent_instance;	/* derived from SwamiguiTreeStore */
};

/* Patch tree store class */
struct _SwamiguiTreeStorePatchClass
{
    SwamiguiTreeStoreClass parent_class;
};

GType swamigui_tree_store_patch_get_type(void);
SwamiguiTreeStorePatch *swamigui_tree_store_patch_new(void);

void swamigui_tree_store_patch_item_add(SwamiguiTreeStore *store,
                                        GObject *item);
void swamigui_tree_store_patch_item_changed(SwamiguiTreeStore *store,
        GObject *item);
#endif
