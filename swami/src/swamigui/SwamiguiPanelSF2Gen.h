/*
 * SwamiguiCtrlSF2Gen.h - User interface generator control object header file
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
#ifndef __SWAMIGUI_GEN_CTRL_H__
#define __SWAMIGUI_GEN_CTRL_H__

#include <gtk/gtk.h>
#include <libinstpatch/libinstpatch.h>

typedef struct _SwamiguiPanelSF2Gen SwamiguiPanelSF2Gen;
typedef struct _SwamiguiPanelSF2GenClass SwamiguiPanelSF2GenClass;
typedef struct _SwamiguiPanelSF2GenCtrlInfo SwamiguiPanelSF2GenCtrlInfo;

#define SWAMIGUI_TYPE_PANEL_SF2_GEN   (swamigui_panel_sf2_gen_get_type ())
#define SWAMIGUI_PANEL_SF2_GEN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_PANEL_SF2_GEN, \
   SwamiguiPanelSF2Gen))
#define SWAMIGUI_PANEL_SF2_GEN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_SF2_GEN, \
   SwamiguiPanelSF2GenClass))
#define SWAMIGUI_IS_PANEL_SF2_GEN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_PANEL_SF2_GEN))
#define SWAMIGUI_IS_PANEL_SF2_GEN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_PANEL_SF2_GEN))

struct _SwamiguiPanelSF2Gen
{
  GtkScrolledWindow parent_instance; /* derived from GtkScrolledWindow */

  IpatchList *selection;	/* item selection */
  int seltype;			/* current selection type (see SelType in $.c) */

  SwamiguiPanelSF2GenCtrlInfo *ctrlinfo;        /* Array of control info */
  gpointer genwidgets;		/* GenWidget array (see GenWidget in $.c) */
  int genwidget_count;          /* Count of widgets in genwidgets array */
};

struct _SwamiguiPanelSF2GenClass
{
  GtkScrolledWindowClass parent_class;
};

/**
 * SwamiguiPanelSF2GenCtrlInfo:
 * @genid: Generator ID or value from #SwamiguiPanelSF2GenOp
 * @icon: Icon name or other string value (label text for example)
 */
struct _SwamiguiPanelSF2GenCtrlInfo
{
  guint8 genid;
  char *icon;
};

/**
 * SwamiguiPanelSF2GenOther:
 * @SWAMIGUI_PANEL_SF2_GEN_LABEL: A label
 * @SWAMIGUI_PANEL_SF2_GEN_COLUMN: Marks beginning of next column
 * @SWAMIGUI_PANEL_SF2_GEN_END: End of #SwamiguiPanelSF2GenCtrlInfo array
 *
 * Operator values stored in #SwamiguiPanelSF2GenCtrlInfo genid field.
 */
typedef enum
{
  SWAMIGUI_PANEL_SF2_GEN_LABEL = 200,
  SWAMIGUI_PANEL_SF2_GEN_COLUMN,
  SWAMIGUI_PANEL_SF2_GEN_END                 /* End of list */
} SwamiguiPanelSF2GenOp;


GType swamigui_panel_sf2_gen_get_type (void);
GtkWidget *swamigui_panel_sf2_gen_new (void);

void
swamigui_panel_sf2_gen_set_controls (SwamiguiPanelSF2Gen *genpanel,
                                     SwamiguiPanelSF2GenCtrlInfo *ctrlinfo);

#endif
