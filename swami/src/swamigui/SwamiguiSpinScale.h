/*
 * SwamiguiSpinScale.h - A GtkSpinButton/GtkScale combo widget
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
#ifndef __SWAMIGUI_SPIN_SCALE_H__
#define __SWAMIGUI_SPIN_SCALE_H__

#include <gtk/gtk.h>
#include <libswami/SwamiParam.h>
#include <libinstpatch/IpatchUnit.h>

typedef struct _SwamiguiSpinScale SwamiguiSpinScale;
typedef struct _SwamiguiSpinScaleClass SwamiguiSpinScaleClass;

#define SWAMIGUI_TYPE_SPIN_SCALE   (swamigui_spin_scale_get_type ())
#define SWAMIGUI_SPIN_SCALE(obj) \
  (GTK_CHECK_CAST ((obj), SWAMIGUI_TYPE_SPIN_SCALE, SwamiguiSpinScale))
#define SWAMIGUI_SPIN_SCALE_CLASS(klass) \
  (GTK_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_SPIN_SCALE, \
   SwamiguiSpinScaleClass))
#define SWAMIGUI_IS_SPIN_SCALE(obj) \
  (GTK_CHECK_TYPE ((obj), SWAMIGUI_TYPE_SPIN_SCALE))
#define SWAMIGUI_IS_SPIN_SCALE_CLASS(klass) \
  (GTK_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_SPIN_SCALE))

/* Swami SpinScale widget */
struct _SwamiguiSpinScale
{
  GtkHBox parent;
  GtkWidget *spinbtn;		/* spin button widget */
  GtkWidget *hscale;		/* horizontal scale widget */
  gboolean scale_first;		/* indicates order of widgets */
  guint16 adj_units;            // Adjustment units (#IpatchUnitType)
  guint16 disp_units;           // Spin button display units (#IpatchUnitType)
  gboolean ignore_input;        // Hack to stop output/input signal loop
};

/* Swami SpinScale widget class */
struct _SwamiguiSpinScaleClass
{
  GtkHBoxClass parent_class;
};

GType swamigui_spin_scale_get_type (void);
GtkWidget *swamigui_spin_scale_new (void);
void swamigui_spin_scale_set_order (SwamiguiSpinScale *spin_scale,
				    gboolean scale_first);
void swamigui_spin_scale_set_transform (SwamiguiSpinScale *spin_scale,
                                        guint16 adj_units, guint16 disp_units);
#endif

