/*
 * patch_funcs.h - General instrument patch functions header
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
#ifndef __PATCH_FUNCS_H__
#define __PATCH_FUNCS_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/libinstpatch.h>
#include <libswami/libswami.h>

#include "SwamiguiRoot.h"
#include "SwamiguiTree.h"

void swamigui_load_files(GObject *parent_hint, gboolean load_samples);
void swamigui_save_files(IpatchList *item_list, gboolean saveas);
void swamigui_close_files(IpatchList *item_list);
void swamigui_delete_items(IpatchList *item_list);
void swamigui_wtbl_load_patch(IpatchItem *item);
void swamigui_new_item(IpatchItem *parent_hint, GType type);
void swamigui_goto_link_item(IpatchItem *item, SwamiguiTree *tree);
void swamigui_export_samples(IpatchList *samples);
void swamigui_copy_items(IpatchList *items);
void swamigui_paste_items(IpatchItem *dstitem, GList *items);
void swamigui_get_clipboard_items(void);

#endif
