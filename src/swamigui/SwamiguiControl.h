/*
 * SwamiguiControl.h - GUI control system
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
#ifndef __SWAMIGUI_CONTROL_H__
#define __SWAMIGUI_CONTROL_H__

#include <gtk/gtk.h>
#include <libswami/libswami.h>

/* some defined rank values for registered handlers */
typedef enum
{
    SWAMIGUI_CONTROL_RANK_LOWEST  = 1,
    SWAMIGUI_CONTROL_RANK_LOW     = 16,
    SWAMIGUI_CONTROL_RANK_NORMAL  = 32,
    SWAMIGUI_CONTROL_RANK_HIGH    = 48,
    SWAMIGUI_CONTROL_RANK_HIGHEST = 63
} SwamiguiControlRank;

/* value to use for 0 (default) */
#define SWAMIGUI_CONTROL_RANK_DEFAULT SWAMIGUI_CONTROL_RANK_NORMAL

/* rank mask */
#define SWAMIGUI_CONTROL_RANK_MASK 0x3F

typedef enum
{
    SWAMIGUI_CONTROL_CTRL      =  0x40, /* controls values */
    SWAMIGUI_CONTROL_VIEW      =  0x80, /* displays values */
    SWAMIGUI_CONTROL_NO_CREATE = 0x100 /* don't create control, cfg UI obj only */
} SwamiguiControlFlags;

/* convenience for control/view controls */
#define SWAMIGUI_CONTROL_CTRLVIEW  (SWAMIGUI_CONTROL_CTRL | SWAMIGUI_CONTROL_VIEW)

typedef enum  /*< flags >*/
{
    SWAMIGUI_CONTROL_OBJECT_NO_LABELS   = 1 << 0,
    SWAMIGUI_CONTROL_OBJECT_NO_SORT     = 1 << 1,
    SWAMIGUI_CONTROL_OBJECT_PROP_LABELS = 1 << 2
} SwamiguiControlObjectFlags;

/**
 * SwamiguiControlHandler:
 * @widget: GUI widget to create a control for
 * @value_type: Control value type (useful to handler functions that can handle
 *   multiple value types)
 * @pspec: Parameter spec defining the control parameters (value type,
 *   valid range, etc), will be a GParamSpec of the specified @value_type or
 *   %NULL for defaults
 * @flags: Flags indicating if control should display values or control
 *   and display values. If the #SWAMIGUI_CONTROL_NO_CREATE flag is specified
 *   then a control should not be created, but @widget should be configured to
 *   conform to @pspec (or reset to defaults if %NULL).
 *
 * This is a function type to handle the creation of a control that is
 * bound to a GUI interface @widget. The control should be configured
 * according to @flags (if its display only then UI control changes should be
 * ignored or preferably disabled, control only flag will occur only with
 * handlers that don't display value changes). The UI @widget may be modified
 * to conform to @pspec (valid range, max string length, etc) and should be
 * done in a manner that allows @widget to be re-configured (i.e., set default
 * values if @pspec not supplied).
 *
 * Returns: Should return the new control which is controlling the
 *   GUI interface @widget.
 */
typedef SwamiControl *(*SwamiguiControlHandler)(GObject *widget,
        GType value_type,
        GParamSpec *pspec,
        SwamiguiControlFlags flags);

/* quark used in associating a control to a controlled object */
extern GQuark swamigui_control_quark;

SwamiControl *swamigui_control_new(GType type);
SwamiControl *swamigui_control_new_for_widget(GObject *widget);
SwamiControl *swamigui_control_new_for_widget_full(GObject *widget,
        GType value_type,
        GParamSpec *pspec,
        SwamiguiControlFlags flags);
SwamiControl *swamigui_control_lookup(GObject *widget);
void swamigui_control_prop_connect_widget(GObject *object, const char *propname,
        GObject *widget);
GObject *swamigui_control_create_widget(GType widg_type, GType value_type,
                                        GParamSpec *pspec,
                                        SwamiguiControlFlags flags);
void swamigui_control_set_queue(SwamiControl *control);
void swamigui_control_register(GType widg_type, GType value_type,
                               SwamiguiControlHandler handler, guint flags);
void swamigui_control_unregister(GType widg_type, GType value_type);

void swamigui_control_glade_prop_connect(GtkWidget *widget, GObject *obj);
GType swamigui_control_get_alias_value_type(GType type);

#endif
