/*
 * SwamiguiControlAdj.h - GtkAdjustment SwamiControl object
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
#ifndef __SWAMIGUI_CONTROL_ADJ_H__
#define __SWAMIGUI_CONTROL_ADJ_H__

typedef struct _SwamiguiControlAdj SwamiguiControlAdj;
typedef struct _SwamiguiControlAdjClass SwamiguiControlAdjClass;

#include <gtk/gtk.h>
#include <libswami/SwamiControl.h>

#define SWAMIGUI_TYPE_CONTROL_ADJ  (swamigui_control_adj_get_type ())
#define SWAMIGUI_CONTROL_ADJ(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_CONTROL_ADJ, \
   SwamiguiControlAdj))
#define SWAMIGUI_CONTROL_ADJ_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_CONTROL_ADJ, \
   SwamiguiControlAdjClass))
#define SWAMIGUI_IS_CONTROL_ADJ(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_CONTROL_ADJ))
#define SWAMIGUI_IS_CONTROL_ADJ_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE ((obj), SWAMIGUI_TYPE_CONTROL_ADJ))

struct _SwamiguiControlAdj
{
    SwamiControl parent_instance;
    GtkAdjustment *adj;		/* GTK adjustment of control */
    GParamSpec *pspec;		/* parameter spec */
    gulong value_change_id; /* GtkAdjustment value-changed handler ID */
};

struct _SwamiguiControlAdjClass
{
    SwamiControlClass parent_class;
};

GType swamigui_control_adj_get_type(void);
SwamiguiControlAdj *swamigui_control_adj_new(GtkAdjustment *adj);
void swamigui_control_adj_set(SwamiguiControlAdj *ctrladj, GtkAdjustment *adj);
void swamigui_control_adj_block_changes(SwamiguiControlAdj *ctrladj);
void swamigui_control_adj_unblock_changes(SwamiguiControlAdj *ctrladj);

#endif
