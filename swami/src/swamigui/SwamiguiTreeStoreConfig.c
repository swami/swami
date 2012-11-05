/*
 * SwamiguiTreeStoreConfig.c - Config tree store (for instruments).
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
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#include <libswami/libswami.h>
#include "SwamiguiTreeStoreConfig.h"
#include "i18n.h"

static void swamigui_tree_store_config_item_add (SwamiguiTreeStore *store, GObject *item);
static void swamigui_tree_store_config_item_changed (SwamiguiTreeStore *store, GObject *item);


G_DEFINE_TYPE (SwamiguiTreeStoreConfig, swamigui_tree_store_config, SWAMIGUI_TYPE_TREE_STORE);


static void
swamigui_tree_store_config_class_init (SwamiguiTreeStoreConfigClass *klass)
{
  SwamiguiTreeStoreClass *store_class = SWAMIGUI_TREE_STORE_CLASS (klass);

  store_class->item_add = swamigui_tree_store_config_item_add;
  store_class->item_changed = swamigui_tree_store_config_item_changed;
}

static void
swamigui_tree_store_config_init (SwamiguiTreeStoreConfig *store)
{
}

/**
 * swamigui_tree_store_config_new:
 *
 * Create a new config tree store for preferences and configuration.
 *
 * Returns: New config tree store object with a ref count of 1.
 */
SwamiguiTreeStore *
swamigui_tree_store_config_new (void)
{
  return (SWAMIGUI_TREE_STORE
	  (g_object_new (SWAMIGUI_TYPE_TREE_STORE_CONFIG, NULL)));
}

static void
swamigui_tree_store_config_item_add (SwamiguiTreeStore *store, GObject *item)
{
  char *name;

  g_return_if_fail (SWAMIGUI_IS_TREE_STORE_CONFIG (store));
  g_return_if_fail (G_IS_OBJECT (item));

  swami_object_get (item, "name", &name, NULL);
  if (!name) name = g_strdup (_("Untitled"));

  swamigui_tree_store_insert_before (store, item, name, NULL, NULL, NULL, NULL);

  g_free (name);
}

static void
swamigui_tree_store_config_item_changed (SwamiguiTreeStore *store, GObject *item)
{
  char *title;

  /* get title of item */
  g_object_get (item, "title", &title, NULL);
  g_return_if_fail (title != NULL);

  swamigui_tree_store_change (store, item, title, NULL);
  g_free (title);
}
