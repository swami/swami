/*
 * SwamiguiSpinScale.c - A GtkSpinButton/GtkScale combo widget
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
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include "SwamiguiSpinScale.h"
#include "i18n.h"
#include "util.h"

enum
{
    PROP_0,
    PROP_ADJUSTMENT,
    PROP_DIGITS,
    PROP_VALUE,
    PROP_SCALE_FIRST	/* the order of the widgets */
};


/* Local Prototypes */

static void swamigui_spin_scale_set_property(GObject *object, guint property_id,
        const GValue *value,
        GParamSpec *pspec);
static void swamigui_spin_scale_get_property(GObject *object, guint property_id,
        GValue *value, GParamSpec *pspec);
static void swamigui_spin_scale_init(SwamiguiSpinScale *spin_scale);
static gboolean swamigui_spin_scale_cb_output(GtkSpinButton *spin_button, gpointer user_data);
static gint swamigui_spin_scale_cb_input(GtkSpinButton *spinbutton, gdouble *newval, gpointer user_data);
static void swamigui_spin_scale_cb_activate(GtkSpinButton *spin_button, gpointer user_data);
static gboolean swamigui_spin_scale_real_set_order(SwamiguiSpinScale *spin_scale,
        gboolean scale_first);


/* define the SwamiguiSpinScale type */
G_DEFINE_TYPE(SwamiguiSpinScale, swamigui_spin_scale, GTK_TYPE_HBOX);


static void
swamigui_spin_scale_class_init(SwamiguiSpinScaleClass *klass)
{
    GObjectClass *obj_class = G_OBJECT_CLASS(klass);

    obj_class->set_property = swamigui_spin_scale_set_property;
    obj_class->get_property = swamigui_spin_scale_get_property;

    g_object_class_install_property(obj_class, PROP_ADJUSTMENT,
                                    g_param_spec_object("adjustment", "Adjustment", "Adjustment",
                                            GTK_TYPE_ADJUSTMENT, G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_DIGITS,
                                    g_param_spec_uint("digits", "Digits", "Digits",
                                            0, 20, 0, G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_VALUE,
                                    g_param_spec_double("value", "Value", "Value",
                                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                                            G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_SCALE_FIRST,
                                    g_param_spec_boolean("scale-first", "Scale first", "Scale first",
                                            FALSE, G_PARAM_READWRITE));
}

static void
swamigui_spin_scale_set_property(GObject *object, guint property_id,
                                 const GValue *value, GParamSpec *pspec)
{
    SwamiguiSpinScale *sc = SWAMIGUI_SPIN_SCALE(object);
    GtkAdjustment *adj;
    guint digits;
    double d;

    switch(property_id)
    {
    case PROP_ADJUSTMENT:
        adj = GTK_ADJUSTMENT(g_value_get_object(value));
        gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(sc->spinbtn), adj);
        gtk_range_set_adjustment(GTK_RANGE(sc->hscale), adj);
        break;

    case PROP_DIGITS:
        digits = g_value_get_uint(value);
        gtk_spin_button_set_digits(GTK_SPIN_BUTTON(sc->spinbtn), digits);
        gtk_scale_set_digits(GTK_SCALE(sc->hscale), digits);
        break;

    case PROP_VALUE:
        d = g_value_get_double(value);
        adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(sc->spinbtn));
        gtk_adjustment_set_value(adj, d);
        break;

    case PROP_SCALE_FIRST:
        swamigui_spin_scale_real_set_order(sc, g_value_get_boolean(value));
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void
swamigui_spin_scale_get_property(GObject *object, guint property_id,
                                 GValue *value, GParamSpec *pspec)
{
    SwamiguiSpinScale *sc = SWAMIGUI_SPIN_SCALE(object);

    switch(property_id)
    {
    case PROP_ADJUSTMENT:
        g_value_set_object(value, (GObject *)gtk_spin_button_get_adjustment
                           (GTK_SPIN_BUTTON(sc->spinbtn)));
        break;

    case PROP_DIGITS:
        g_value_set_uint(value, gtk_spin_button_get_digits
                         (GTK_SPIN_BUTTON(sc->spinbtn)));
        break;

    case PROP_VALUE:
        g_value_set_double(value, gtk_spin_button_get_value
                           (GTK_SPIN_BUTTON(sc->spinbtn)));
        break;

    case PROP_SCALE_FIRST:
        g_value_set_boolean(value, sc->scale_first);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void
swamigui_spin_scale_init(SwamiguiSpinScale *spin_scale)
{
    GtkAdjustment *adj;

    spin_scale->scale_first = FALSE;

    adj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0));

    spin_scale->spinbtn = gtk_spin_button_new(adj, 1.0, 0);
    gtk_widget_show(spin_scale->spinbtn);
    gtk_box_pack_start(GTK_BOX(spin_scale), spin_scale->spinbtn, FALSE, FALSE, 0);

    /*
      Because text displayed in "text entry" is expected to have a different
      representation and different resolution that adjustment value, it is
      important to keep:
      1)"text entry" input disconnected from internal automatic update of adjustment value.
      2)"spin button" input disconnected from internal automatic update of "text entry" value.
      3)cb output being solely responsable of update "text entry" value converted
        from adjustment value.

      To insure (1) and (2) we need to connect callbacks on both signals "activate" and "input".
      To insure (3) we need to connect a callback on "output" signal.
    */
    g_signal_connect(spin_scale->spinbtn, "output", G_CALLBACK(swamigui_spin_scale_cb_output), spin_scale);
    g_signal_connect(spin_scale->spinbtn, "input", G_CALLBACK(swamigui_spin_scale_cb_input), spin_scale);
    g_signal_connect(spin_scale->spinbtn, "activate", G_CALLBACK(swamigui_spin_scale_cb_activate), spin_scale);

    spin_scale->hscale = gtk_hscale_new(adj);
    gtk_scale_set_draw_value(GTK_SCALE(spin_scale->hscale), FALSE);
    gtk_widget_show(spin_scale->hscale);
    gtk_box_pack_start(GTK_BOX(spin_scale), spin_scale->hscale, TRUE, TRUE, 0);
}

/* Callback for spin button output update (transform value if transform function set) */
static gboolean
swamigui_spin_scale_cb_output(GtkSpinButton *spin_button, gpointer user_data)
{
    SwamiguiSpinScale *spin_scale = SWAMIGUI_SPIN_SCALE(user_data);
    GValue adjval = { 0 }, dispval = { 0 };
    GtkAdjustment *adj;
    gchar *text;
    int digits;

    if(spin_scale->adj_units == IPATCH_UNIT_TYPE_NONE)
    {
        return (FALSE);
    }

    adj = gtk_spin_button_get_adjustment(spin_button);

    g_value_init(&adjval, G_TYPE_DOUBLE);
    g_value_set_double(&adjval, gtk_adjustment_get_value(adj));
    g_value_init(&dispval, G_TYPE_DOUBLE);

    ipatch_unit_convert(spin_scale->adj_units, spin_scale->disp_units, &adjval, &dispval);

    digits = gtk_spin_button_get_digits(spin_button);

    text = g_strdup_printf("%.*f", digits, g_value_get_double(&dispval));         // ++ alloc
    gtk_entry_set_text(GTK_ENTRY(spin_button), text);
    g_free(text);                                                                 // -- free

    // Probably not needed - but just for good measure
    g_value_unset(&adjval);
    g_value_unset(&dispval);

    return (TRUE);
}

/* Callback for spin button input */
static gint
swamigui_spin_scale_cb_input(GtkSpinButton *spinbutton, gdouble *newval, gpointer user_data)
{
    GtkAdjustment *adj = gtk_spin_button_get_adjustment(spinbutton);
    *newval = adj->value;

    return TRUE;
}

/*
 Callback for "text entry" input.
 Take text entry value and convert it to adjustment value.
*/
static void
swamigui_spin_scale_cb_activate(GtkSpinButton *spin_button, gpointer user_data)
{

    SwamiguiSpinScale *spin_scale = SWAMIGUI_SPIN_SCALE(user_data);
    GtkAdjustment *adj;
    GValue adjval = { 0 }, dispval = { 0 };

    /* take text coming from "text entry". */
    /* The buffer is internal and shouldn't be freed. */
    const char *text = gtk_entry_get_text(GTK_ENTRY(spin_button));

    if(!text)
    {
        return;
    }

    /* Convert dispval -> adjval */
    g_value_init(&dispval, G_TYPE_DOUBLE);
    g_value_set_double(&dispval, atof(text));
    g_value_init(&adjval, G_TYPE_DOUBLE);

    ipatch_unit_convert(spin_scale->disp_units, spin_scale->adj_units, &dispval, &adjval);

    /* set adjustement value */
    adj = gtk_spin_button_get_adjustment(spin_button);
    gtk_adjustment_set_value(adj, g_value_get_double(&adjval));

    return;
}

/**
 * swamigui_spin_scale_new:
 *
 * Create a new spin button/scale combo widget.
 *
 * Returns: New widget.
 */
GtkWidget *
swamigui_spin_scale_new(void)
{
    return (GTK_WIDGET(g_object_new(SWAMIGUI_TYPE_SPIN_SCALE, NULL)));
}

/**
 * swamigui_spin_scale_set_order:
 * @spin_scale: Spin scale widget
 * @scale_first: %TRUE if the GtkHScale should be before the GtkSpinButton,
 *   %FALSE otherwise.
 *
 * Sets the order that the horizontal scale and spin button widgets appear.
 */
void
swamigui_spin_scale_set_order(SwamiguiSpinScale *spin_scale,
                              gboolean scale_first)
{
    if(swamigui_spin_scale_real_set_order(spin_scale, scale_first))
    {
        g_object_notify(G_OBJECT(spin_scale), "scale-first");
    }
}

static gboolean
swamigui_spin_scale_real_set_order(SwamiguiSpinScale *spin_scale,
                                   gboolean scale_first)
{
    g_return_val_if_fail(SWAMIGUI_IS_SPIN_SCALE(spin_scale), FALSE);

    scale_first = (scale_first != 0);	/* force booleanize it */

    if(spin_scale->scale_first == scale_first)
    {
        return (FALSE);
    }

    spin_scale->scale_first = scale_first;
    gtk_box_reorder_child(GTK_BOX(spin_scale), spin_scale->hscale, !scale_first);

    return (TRUE);
}

/**
 * swamigui_spin_scale_set_transform:
 * @spin_scale: Spin scale widget
 * @adj_units: (type IpatchUnitType): Adjustment control units
 * @disp_units: (type IpatchUnitType): Spin button display units
 *
 * Since: 2.1.0
 */
void
swamigui_spin_scale_set_transform(SwamiguiSpinScale *spin_scale,
                                  guint16 adj_units, guint16 disp_units)
{
    IpatchUnitInfo *unitinfo;

    g_return_if_fail(SWAMIGUI_IS_SPIN_SCALE(spin_scale));

    spin_scale->adj_units = adj_units;
    spin_scale->disp_units = disp_units;

    unitinfo = ipatch_unit_lookup(disp_units);

    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_scale->spinbtn), unitinfo ? unitinfo->digits : 0);
}

