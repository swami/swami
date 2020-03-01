/*
 * SwamiguiProp.c - GObject property GUI control object
 * For creating user interfaces for controlling GObject properties.
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
#include <gtk/gtk.h>

#include <libinstpatch/libinstpatch.h>

#include "SwamiguiControl.h"
#include "SwamiguiRoot.h"
#include "SwamiguiProp.h"
#include "SwamiguiPanel.h"
#include "i18n.h"
#include "util.h"

enum
{
    PROP_0,
    PROP_ITEM_SELECTION
};

/* Hash value used for prop_registry */
typedef struct
{
    char *widg_name;		/* name of Glade widget or NULL if handler set */
    SwamiguiPropHandler handler;	/* handler function or NULL if widg_name set */
} PropInfo;

/* Local Prototypes */

static PropInfo *prop_info_new(void);
static void prop_info_free(gpointer data);

static void swamigui_prop_class_init(SwamiguiPropClass *klass);
static void swamigui_prop_panel_iface_init(SwamiguiPanelIface *panel_iface);
static gboolean swamigui_prop_panel_iface_check_selection(IpatchList *selection,
        GType *selection_types);
static void swamigui_prop_set_property(GObject *object, guint property_id,
                                       const GValue *value,
                                       GParamSpec *pspec);
static void swamigui_prop_get_property(GObject *object, guint property_id,
                                       GValue *value, GParamSpec *pspec);
static void swamigui_prop_finalize(GObject *object);
static void swamigui_prop_init(SwamiguiProp *prop);
static gboolean swamigui_prop_real_set_selection(SwamiguiProp *prop,
        IpatchList *selection);

static GObjectClass *parent_class = NULL;

/* Property type registry hash (GType -> PropInfo) */
static GHashTable *prop_registry = NULL;

/* function for creating a PropInfo structure */
static PropInfo *
prop_info_new(void)
{
    return (g_slice_new0(PropInfo));
}

/* function for freeing a PropInfo structure */
static void
prop_info_free(gpointer data)
{
    PropInfo *info = (PropInfo *)data;

    g_return_if_fail(data != NULL);

    g_free(info->widg_name);
    g_slice_free(PropInfo, data);
}

/**
 * swamigui_register_prop_glade_widg:
 * @objtype: Type of object which the interface will control
 * @name: Name of a Glade widget in the Swami Glade file
 *
 * Registers a Swami Glade widget as an interface control for a given @objtype.
 * The Glade widget should contain child widgets of the form "PROP::<prop-name>"
 * which will cause the given widget to control the property "<prop-name>" of
 * objects of the given type.  Use swamigui_prop_register_handler() instead if
 * additional customization is needed to create an interface.
 */
void
swamigui_register_prop_glade_widg(GType objtype, const char *name)
{
    PropInfo *info;

    g_return_if_fail(objtype != 0);
    g_return_if_fail(name != NULL);

    info = prop_info_new();
    info->widg_name = g_strdup(name);

    g_hash_table_insert(prop_registry, (gpointer)objtype, info);
}

/**
 * swamigui_register_prop_handler:
 * @objtype: Type of object which the interface will control
 * @handler: Handler function to create the interface
 *
 * Registers a handler function to create an interface control for a given
 * @objtype.  The handler should create a widget (if its widg parameter is
 * %NULL), or use previously created widget, and connect its controls to the
 * properties of the passed in object.  Use swamigui_prop_register_glade_widg()
 * instead if no special customization is needed beyond simply connecting
 * widgets to an object's properties.
 */
void
swamigui_register_prop_handler(GType objtype, SwamiguiPropHandler handler)
{
    PropInfo *info;

    g_return_if_fail(objtype != 0);
    g_return_if_fail(handler != NULL);

    info = prop_info_new();
    info->handler = handler;

    g_hash_table_insert(prop_registry, (gpointer)objtype, info);
}

GType
swamigui_prop_get_type(void)
{
    static GType obj_type = 0;

    if(!obj_type)
    {
        static const GTypeInfo obj_info =
        {
            sizeof(SwamiguiPropClass), NULL, NULL,
            (GClassInitFunc) swamigui_prop_class_init, NULL, NULL,
            sizeof(SwamiguiProp), 0,
            (GInstanceInitFunc) swamigui_prop_init,
        };
        static const GInterfaceInfo panel_info =
        { (GInterfaceInitFunc)swamigui_prop_panel_iface_init, NULL, NULL };

        obj_type = g_type_register_static(GTK_TYPE_SCROLLED_WINDOW,
                                          "SwamiguiProp", &obj_info, 0);
        g_type_add_interface_static(obj_type, SWAMIGUI_TYPE_PANEL, &panel_info);

        prop_registry = g_hash_table_new_full(NULL, NULL, NULL, prop_info_free);
    }

    return (obj_type);
}

/* Override mouse button event:
   To avoid lose of focus in pannels selector (GtkNoteBook tabs selector).
   Otherwise,the user would be forced to clic two times when he want to select
   another pannel.
*/
static gboolean swamigui_prop_press_button(GtkWidget *widget,
        GdkEventButton *event)
{
    /*  mouse click button propagation is ignored */
    return TRUE;
}

static void
swamigui_prop_class_init(SwamiguiPropClass *klass)
{
    GObjectClass *obj_class = G_OBJECT_CLASS(klass);

    /* Override mouse button event */
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->button_press_event = swamigui_prop_press_button ;

    parent_class = g_type_class_peek_parent(klass);

    obj_class->set_property = swamigui_prop_set_property;
    obj_class->get_property = swamigui_prop_get_property;
    obj_class->finalize = swamigui_prop_finalize;

    g_object_class_override_property(obj_class, PROP_ITEM_SELECTION, "item-selection");
}

static void
swamigui_prop_panel_iface_init(SwamiguiPanelIface *panel_iface)
{
    panel_iface->label = _("Properties");
    panel_iface->blurb = _("Edit general properties of items");
    panel_iface->stockid = GTK_STOCK_PROPERTIES;
    panel_iface->check_selection = swamigui_prop_panel_iface_check_selection;
}

static gboolean
swamigui_prop_panel_iface_check_selection(IpatchList *selection,
        GType *selection_types)
{
    /* one item only and is a registered prop interface type */
    return (!selection->items->next
            && g_hash_table_lookup(prop_registry, (gpointer)(*selection_types)));
}

static void
swamigui_prop_set_property(GObject *object, guint property_id,
                           const GValue *value, GParamSpec *pspec)
{
    SwamiguiProp *prop = SWAMIGUI_PROP(object);

    switch(property_id)
    {
    case PROP_ITEM_SELECTION:
        swamigui_prop_real_set_selection(prop,
                                         (IpatchList *)g_value_get_object(value));
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void
swamigui_prop_get_property(GObject *object, guint property_id,
                           GValue *value, GParamSpec *pspec)
{
    SwamiguiProp *prop = SWAMIGUI_PROP(object);

    switch(property_id)
    {
    case PROP_ITEM_SELECTION:
        g_value_set_object(value, prop->selection);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void
swamigui_prop_finalize(GObject *object)
{
    SwamiguiProp *prop = SWAMIGUI_PROP(object);

    if(prop->selection)
    {
        g_object_unref(prop->selection);
    }

    (*parent_class->finalize)(object);
}

static void
swamigui_prop_init(SwamiguiProp *prop)
{
    GtkObject *hadj, *vadj;

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(prop),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    hadj = gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    vadj = gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

    g_object_set(prop,
                 "hadjustment", hadj,
                 "vadjustment", vadj,
                 NULL);

    prop->viewport = gtk_viewport_new(GTK_ADJUSTMENT(hadj), GTK_ADJUSTMENT(vadj));
    gtk_widget_show(prop->viewport);
    gtk_container_add(GTK_CONTAINER(prop), prop->viewport);

    prop->selection = NULL;
}

/**
 * swamigui_prop_new:
 *
 * Create a new Swami properties object
 *
 * Returns: Swami properties object
 */
GtkWidget *
swamigui_prop_new(void)
{
    return (GTK_WIDGET(g_object_new(SWAMIGUI_TYPE_PROP, NULL)));
}

/**
 * swamigui_prop_set_selection:
 * @prop: Properties object
 * @select: Item selection to control or %NULL to unset.  If list contains
 *   multiple items, then selection will be unset.
 *
 * Set the object to control properties of.
 */
void
swamigui_prop_set_selection(SwamiguiProp *prop, IpatchList *selection)
{
    if(swamigui_prop_real_set_selection(prop, selection))
    {
        g_object_notify(G_OBJECT(prop), "item-selection");
    }
}

static gboolean
swamigui_prop_real_set_selection(SwamiguiProp *prop, IpatchList *selection)
{
    GtkWidget *widg = NULL;
    GObject *item = NULL, *olditem = NULL;
    gboolean same_type;
    PropInfo *info = NULL;

    g_return_val_if_fail(SWAMIGUI_IS_PROP(prop), FALSE);
    g_return_val_if_fail(!selection || IPATCH_IS_LIST(selection), FALSE);

    /* selection should be a single item, otherwise set it to NULL */
    if(selection && (!selection->items || selection->items->next))
    {
        selection = NULL;
    }

    if(selection)
    {
        item = (GObject *)(selection->items->data);
        g_return_val_if_fail(G_IS_OBJECT(item), FALSE);
    }

    if(prop->selection)
    {
        olditem = G_OBJECT(prop->selection->items->data);
    }

    /* same item is already set? */
    if(item == olditem)
    {
        return (FALSE);
    }

    if(item)
    {
        info = g_hash_table_lookup(prop_registry, (gpointer)G_OBJECT_TYPE(item));

        if(!info)	/* No interface found for this object type? */
        {
            if(!olditem)
            {
                return (FALSE);    /* already unset, return */
            }

            item = NULL;
            selection = NULL;
        }
    }

    /* selected item is of the same type? */
    same_type = (item && olditem && G_OBJECT_TYPE(item) == G_OBJECT_TYPE(olditem));

    /* destroy the old interface (if any) if not of the same type as the new item */
    if(!same_type)
        gtk_container_foreach(GTK_CONTAINER(prop->viewport),
                              (GtkCallback)gtk_object_destroy, NULL);

    if(item)	/* setting item? */
    {
        if(!same_type)	/* create new interface for different type? */
        {
            if(info->widg_name)	/* Create by widget name? */
            {
                widg = swamigui_util_glade_create(info->widg_name);
                swamigui_control_glade_prop_connect(widg, item);
            }
            else
            {
                widg = info->handler(NULL, item);    /* Create using handler */
            }

            gtk_widget_show(widg);
            gtk_container_add(GTK_CONTAINER(prop->viewport), widg);
        }
        else	/* same type (re-using interface) */
        {
            widg = gtk_bin_get_child(GTK_BIN(prop->viewport));

            if(widg)
            {
                /* update the interface, by widget name method or handler method */
                if(info->widg_name)
                {
                    swamigui_control_glade_prop_connect(widg, item);
                }
                else
                {
                    info->handler(widg, item);
                }
            }
        }

        if(!widg)
        {
            selection = NULL;
        }
    }

    if(prop->selection)
    {
        g_object_unref(prop->selection);    // -- unref old selection
    }

    prop->selection = selection;

    if(prop->selection)
    {
        g_object_ref(prop->selection);    /* ++ reference selection */
    }

    return (TRUE);
}

