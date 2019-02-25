/*
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
#ifndef __SWAMIGUI_UTIL_H__
#define __SWAMIGUI_UTIL_H__

#include <stdio.h>
#include <gtk/gtk.h>

/* size in bytes of buffers used for converting audio formats */
#define SWAMIGUI_SAMPLE_TRANSFORM_SIZE  (64 * 1024)

typedef void (*UtilQuickFunc) (gpointer userdata, GtkWidget *popup);

/** A guint RGBA color unit type for GParamSpec "unit-type" IpatchParamProp */
#define SWAMIGUI_UNIT_RGBA_COLOR  swamigui_util_unit_rgba_color_get_type()

/** Macro for specifying a 32 bit color using RGB values (0-255) */
#define SWAMIGUI_RGB(r, g, b)           ((r) << 24 | (g) << 16 | (b) << 8 | 0xFF)

/** Macro for specifying a 32 bit color using RGBA values (0-255) */
#define SWAMIGUI_RGBA(r, g, b, a)       ((r) << 24 | (g) << 16 | (b) << 8 | (a))

typedef enum {
  SWAMIGUI_RESOURCE_PATH_ROOT,
  SWAMIGUI_RESOURCE_PATH_UIXML,
  SWAMIGUI_RESOURCE_PATH_IMAGES
} SwamiResourcePath;

void swamigui_util_init (void);
guint swamigui_util_unit_rgba_color_get_type (void);
void swamigui_util_set_cairo_rgba (cairo_t *cr, guint32 rgba);
GtkWidget *swamigui_util_quick_popup (gchar * msg, gchar * btn1, ...);
GtkWidget *swamigui_util_lookup_unique_dialog (gchar *strkey, gint key2);
gboolean swamigui_util_register_unique_dialog (GtkWidget *dialog, gchar *strkey,
					       gint key2);
void swamigui_util_unregister_unique_dialog (GtkWidget *dialog);
gboolean swamigui_util_activate_unique_dialog (gchar *strkey, gint key2);

gpointer swamigui_util_waitfor_widget_action (GtkWidget *widg);
void swamigui_util_widget_action (GtkWidget *cbwidg, gpointer value);
GtkWidget *swamigui_util_glade_create (const char *name);
GtkWidget *swamigui_util_glade_lookup (GtkWidget *widget, const char *name);
GtkWidget *swamigui_util_glade_lookup_nowarn (GtkWidget *widget, const char *name);

int swamigui_util_option_menu_index (GtkWidget *opmenu);

// void log_view (gchar * title);

char *swamigui_util_str_crlf2lf (char *str);
char *swamigui_util_str_lf2crlf (char *str);
int swamigui_util_substrcmp (char *sub, char *str);

gchar *swamigui_util_get_resource_path (SwamiResourcePath kind);

#endif
