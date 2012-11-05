/*
 * SwamiguiTreeStoreConfig.h - Config tree store (Drivers/Layouts/Plugins/etc)
 *
 * Swami
 * Copyright (C) 1999-2012 Joshua "Element" Green <element@elementsofsound.org>
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
#ifndef __SWAMIGUI_TREE_STORE_CONFIG_H__
#define __SWAMIGUI_TREE_STORE_CONFIG_H__

typedef struct _SwamiguiTreeStoreConfig SwamiguiTreeStoreConfig;
typedef struct _SwamiguiTreeStoreConfigClass SwamiguiTreeStoreConfigClass;

#include <libinstpatch/IpatchItem.h>
#include <swamigui/SwamiguiTreeStore.h>

#define SWAMIGUI_TYPE_TREE_STORE_CONFIG   (swamigui_tree_store_config_get_type ())
#define SWAMIGUI_TREE_STORE_CONFIG(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_TREE_STORE_CONFIG, \
   SwamiguiTreeStoreConfig))
#define SWAMIGUI_TREE_STORE_CONFIG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_TREE_STORE_CONFIG, \
   SwamiguiTreeStoreConfigClass))
#define SWAMIGUI_IS_TREE_STORE_CONFIG(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_TREE_STORE_CONFIG))
#define SWAMIGUI_IS_TREE_STORE_CONFIG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_TREE_STORE_CONFIG))

/* Config tree store object */
struct _SwamiguiTreeStoreConfig
{
  SwamiguiTreeStore parent_instance;	/* derived from SwamiguiTreeStore */
};

/* Config tree store class */
struct _SwamiguiTreeStoreConfigClass
{
  SwamiguiTreeStoreClass parent_class;
};

GType swamigui_tree_store_config_get_type (void);

SwamiguiTreeStore *swamigui_tree_store_config_new (void);

#endif
