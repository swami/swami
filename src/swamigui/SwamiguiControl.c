/*
 * SwamiguiControl.c - GUI control system
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "SwamiguiControl.h"
#include "SwamiguiRoot.h"
#include "libswami/swami_priv.h"

/*
 * Notes about SwamiguiControl
 *
 * When a control is attached to a widget the following occurs:
 * - widget holds a reference on control (widget->control)
 * - widget uses g_object_set_data_full to associate control to widget,
 *     GDestroyNotify handler calls swami_control_disconnect_unref() on control
 * - control has a reference on widget
 * - widget "destroy" signal is caught by control to release reference
 *
 * When gtk_object_destroy() is called on the widget the following happens:
 * - "destroy" signal is caught by widget control handler which then NULLifies
 *    any pointers to widget and removes its reference to the widget
 * - If all references have been removed from widget, it is finalized
 * - GDestroyNotify is called from widget's finalize function which calls
 *    swami_control_disconnect_unref() which disconnects control and unrefs it
 * - Control is finalized if there are no more external references
 *
 * When writing new SwamiguiControl handlers its important to note that the
 * control network may be operating in a multi-thread environment, while the
 * GUI is single threaded.  For this reason controls are added to the GUI
 * queue, which causes all events to be processed from within the GUI thread.
 * This means that even after a widget has been destroyed there may still be
 * queued control events.  For this reason its important to lock the control
 * in value set/get callbacks and check if the widget is still alive and handle
 * the case where it has been destroyed (ignore events usually).
 *
 * One caveat of this is that currently a control cannot be removed from a
 * widget.  The widget must be destroyed.  Since there is a 1 to 1 mapping of
 * a widget and its control, this shouldn't really be a problem.
 */

#define CONTROL_TABLE_ROW_SPACING  2
#define CONTROL_TABLE_COLUMN_SPACING 4

typedef struct
{
    GType widg_type;
    GType value_type;
    int flags;			/* rank and control/view flags */
    SwamiguiControlHandler handler;
} HandlerInfo;

static int swamigui_control_GCompare_type(gconstpointer a, gconstpointer b);
static int swamigui_control_GCompare_rank(gconstpointer a, gconstpointer b);
static void
_swami_control_free_handler_infos(HandlerInfo *data, gpointer user_data);


G_LOCK_DEFINE_STATIC(control_handlers);
static GList *control_handlers = NULL;

/* quark used to associate a control to a widget using g_object_set_qdata */
GQuark swamigui_control_quark = 0;

/* ------ Initialization/deinitialization of GUI control system --------------*/
/* Ininitialize GUI control system */
void
_swamigui_control_init(void)
{
    control_handlers = NULL;

    swamigui_control_quark
        = g_quark_from_static_string("_SwamiguiControl");
}

/* Free GUI control system */
void
_swamigui_control_deinit(void)
{
    g_list_foreach(control_handlers,
                   (GFunc)_swami_control_free_handler_infos, NULL);
    g_list_free(control_handlers);
}

static void
_swami_control_free_handler_infos(HandlerInfo *data, gpointer user_data)
{
    g_slice_free(HandlerInfo, data);
}

/**
 * swamigui_control_new:
 * @type: A #SwamiControl derived GType of control to create
 *
 * Create a control of the given @type which should be a
 * #SwamiControl derived type. The created control is automatically added
 * to the #SwamiguiRoot GUI control event queue. Just a convenience function
 * really.
 *
 * Returns: New control with a refcount of 1 which the caller owns.
 */
SwamiControl *
swamigui_control_new(GType type)
{
    SwamiControl *control;

    g_return_val_if_fail(g_type_is_a(type, SWAMI_TYPE_CONTROL), NULL);

    control = g_object_new(type, NULL);

    if(control)
    {
        swamigui_control_set_queue(control);
    }

    return (control);
}

/**
 * swamigui_control_new_for_widget:
 * @widget: GUI widget to create a control for
 *
 * Creates a new control for a GUI widget.  Use
 * swami_control_new_for_widget_full() for additional parameters.
 *
 * Returns: The new control or %NULL if @widget not handled.
 * The returned control does NOT have a reference since the @widget is
 * the owner of the control. Destroying the @widget will cause the control
 * to be disconnected and unref'd, if there are no more references the
 * control will be freed.
 */
SwamiControl *
swamigui_control_new_for_widget(GObject *widget)
{
    g_return_val_if_fail(G_IS_OBJECT(widget), NULL);
    return (swamigui_control_new_for_widget_full(widget, 0, NULL, 0));
}

/**
 * swamigui_control_new_for_widget_full:
 * @widget: GUI widget to control
 * @value_type: Control value type or 0 for default
 * @pspec: Parameter spec to define valid ranges and other parameters,
 *   should be a GParamSpec of @value_type or %NULL for defaults
 * @flags: Flags for creating the new control. To create a display only
 *   control just the #SWAMIGUI_CONTROL_VIEW flag can be specified, 0 means
 *   both control and view (#SWAMIGUI_CONTROL_CTRLVIEW).
 *
 * Creates a new control for a GUI widget, provided there is a registered
 * handler for the @widget type/value type combination. The new control is
 * automatically assigned to the GUI queue in #swamigui_root. A widget's
 * control can be retrieved with swamigui_control_lookup().
 * If the given @widget already has a control it is returned. The @pspec
 * parameter allows for additional settings to be applied to the @widget
 * and/or control (such as a valid range or max string length, etc).
 *
 * Returns: The new control or %NULL if @widget/@value_type not handled.
 * The returned control does NOT have a reference since the @widget is
 * the owner of the control. Destroying the @widget will cause the control
 * to be disconnected and unref'd, if there are no more references the
 * control will be freed.
 */
SwamiControl *
swamigui_control_new_for_widget_full(GObject *widget, GType value_type,
                                     GParamSpec *pspec,
                                     SwamiguiControlFlags flags)
{
    SwamiguiControlHandler hfunc = NULL;
    SwamiControl *control = NULL;
    GType cmp_widg_type, cmp_value_type;
    HandlerInfo *hinfo, *bestmatch = NULL;
    GList *p;

    g_return_val_if_fail(G_IS_OBJECT(widget), NULL);
    g_return_val_if_fail(!pspec || G_IS_PARAM_SPEC(pspec), NULL);

    /* return existing control (if any) */
    control = g_object_get_qdata(widget, swamigui_control_quark);

    if(control)
    {
        return (control);
    }

    value_type = swamigui_control_get_alias_value_type(value_type);

    /* doesn't make sense to request control only */
    flags &= SWAMIGUI_CONTROL_CTRLVIEW;

    if(!(flags & SWAMIGUI_CONTROL_VIEW))
    {
        flags = SWAMIGUI_CONTROL_CTRLVIEW;
    }

    cmp_widg_type = G_OBJECT_TYPE(widget);

    switch(G_TYPE_FUNDAMENTAL(value_type))
    {
    case G_TYPE_ENUM:
    case G_TYPE_FLAGS:
        cmp_value_type = G_TYPE_FUNDAMENTAL(value_type);
        break;

    default:
        cmp_value_type = value_type;
        break;
    }

    G_LOCK(control_handlers);

    /* loop over widget control handlers */
    for(p = control_handlers; p; p = p->next)
    {
        hinfo = (HandlerInfo *)(p->data);

        /* widget type does not match? - Skip */
        if(cmp_widg_type != hinfo->widg_type)
        {
            continue;
        }

        /* if wildcard value type or handler type matches compare type, we found it */
        if(!cmp_value_type || cmp_value_type == hinfo->value_type)
        {
            break;
        }

        /* check if compare value and param can be converted to that of the handlers */
        if(G_TYPE_IS_VALUE_TYPE(cmp_value_type)
                && G_TYPE_IS_VALUE_TYPE(hinfo->value_type)
                && g_value_type_transformable(cmp_value_type, hinfo->value_type))
//	&& swami_param_type_transformable_value (cmp_value_type, hinfo->value_type))
        {
            /* check if this match is the best so far */
            if(!bestmatch || ((bestmatch->flags & SWAMIGUI_CONTROL_RANK_MASK)
                              < (hinfo->flags & SWAMIGUI_CONTROL_RANK_MASK)))
            {
                bestmatch = hinfo;
            }
        }
    }

    if(!p && bestmatch)
    {
        hfunc = bestmatch->handler;
    }
    else if(p)
    {
        hfunc = ((HandlerInfo *)(p->data))->handler;
    }

    G_UNLOCK(control_handlers);

    if(hfunc)
    {
        control = hfunc(widget, value_type, pspec, flags);    /* ++ ref new ctrl */
    }

    if(control)
    {
        swamigui_control_set_queue(control);    /* add to GUI queue */
    }

    /* associate control to the widget (!! widget takes over reference) */
    g_object_set_qdata_full(widget, swamigui_control_quark, control,
                            (GDestroyNotify)swami_control_disconnect_unref);

    return (control);
}

/**
 * swamigui_control_lookup:
 * @widget: User interface widget to lookup the #SwamiControl of
 *
 * Returns: The associated #SwamiControl or NULL if none. The return control
 * is NOT referenced for the caller (don't unref it).
 */
SwamiControl *
swamigui_control_lookup(GObject *widget)
{
    SwamiControl *control;

    g_return_val_if_fail(G_IS_OBJECT(widget), NULL);

    control = g_object_get_qdata(widget, swamigui_control_quark);

    if(!control)
    {
        return (NULL);
    }

    return (SWAMI_CONTROL(control));  /* do a type cast for debugging purposes */
}

/**
 * swamigui_control_prop_connect_widget:
 * @object: Object with property to connect
 * @propname: Property of @object to connect
 * @widg: Widget to control object property
 *
 * A convenience function which connects a widget as a control for a given
 * @object property.  Use swamigui_control_prop_connect_widget_full() for
 * additional options.
 */
void
swamigui_control_prop_connect_widget(GObject *object, const char *propname,
                                     GObject *widget)
{
    SwamiControl *propctrl;
    SwamiControl *widgctrl;
    GParamSpec *pspec;

    g_return_if_fail(G_IS_OBJECT(object));
    g_return_if_fail(propname != NULL);
    g_return_if_fail(G_IS_OBJECT(widget));

    pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(object), propname);
    g_return_if_fail(pspec != NULL);

    propctrl = swami_get_control_prop(object, pspec);	/* ++ ref */
    g_return_if_fail(propctrl != NULL);

    /* create widget control using value type from pspec and view only if
     * read only property */
    widgctrl = swamigui_control_new_for_widget_full
               (widget, G_PARAM_SPEC_VALUE_TYPE(pspec), NULL,
                (pspec->flags & G_PARAM_READWRITE) == G_PARAM_READABLE
                ? SWAMIGUI_CONTROL_VIEW : 0);

    if(swami_log_if_fail(widgctrl != NULL))
    {
        g_object_unref(propctrl);	/* -- unref */
        return;
    }

    swami_control_connect(propctrl, widgctrl, SWAMI_CONTROL_CONN_BIDIR_SPEC_INIT);
    g_object_unref(propctrl);	/* -- unref */
}

/**
 * swamigui_control_create_widget:
 * @widg_type: A base type of new widget (GtkWidget, GnomeCanvasItem, etc)
 *   or 0 for default GtkWidget derived types.
 * @value_type: Control value type
 * @pspec: Parameter spec to define valid ranges and other parameters,
 *   should be a GParamSpec of @value_type or %NULL for defaults
 * @flags: Can be used to specify view only controls by passing
 *   #SWAMIGUI_CONTROL_VIEW in which case view only control handlers will be
 *   preferred.  A value of 0 assumes control and view mode
 *   (#SWAMIGUI_CONTROL_CTRLVIEW).
 *
 * Creates a GUI widget suitable for controlling values of type @value_type.
 * The @widg_type parameter is used to specify what base type of widget
 * to create, GTK_TYPE_WIDGET is assumed if 0.
 *
 * Returns: The new GUI widget derived from @widg_type and suitable for
 * controlling values of type @value_type or %NULL if @value_type/@widg_type
 * not handled. The new object uses the Gtk reference counting behavior.
 */
GObject *
swamigui_control_create_widget(GType widg_type, GType value_type,
                               GParamSpec *pspec,
                               SwamiguiControlFlags flags)
{
    SwamiguiControlHandler hfunc = NULL;
    GObject *widget = NULL;
    HandlerInfo *hinfo;
    GList *p, *found = NULL;
    gboolean view_only = FALSE;

    g_return_val_if_fail(value_type != 0, NULL);

    if(widg_type == 0)
    {
        widg_type = GTK_TYPE_WIDGET;
    }

    value_type = swamigui_control_get_alias_value_type(value_type);

    /* doesn't make sense to request control only */
    flags &= SWAMIGUI_CONTROL_CTRLVIEW;

    if(!(flags & SWAMIGUI_CONTROL_VIEW))
    {
        flags = SWAMIGUI_CONTROL_CTRLVIEW;
    }

    /* view only control requested? */
    if((flags & SWAMIGUI_CONTROL_CTRLVIEW) == SWAMIGUI_CONTROL_VIEW)
    {
        view_only = TRUE;
    }

    G_LOCK(control_handlers);

    /* search for matching control handler */
    p = control_handlers;

    while(p)
    {
        hinfo = (HandlerInfo *)(p->data);

        /* handler widget is derived from @widg_type and of @value_type? */
        if(g_type_is_a(hinfo->widg_type, widg_type)
                && hinfo->value_type == value_type)
        {
            if(!found || ((hinfo->flags & SWAMIGUI_CONTROL_CTRLVIEW)
                          == SWAMIGUI_CONTROL_VIEW))
            {
                found = p;
            }

            /* if view only, keep searching for a view only handler */
            if(!view_only || ((hinfo->flags & SWAMIGUI_CONTROL_CTRLVIEW)
                              == SWAMIGUI_CONTROL_VIEW))
            {
                break;
            }
        }

        p = g_list_next(p);
    }

    if(found)
    {
        hinfo = (HandlerInfo *)(found->data);
        hfunc = hinfo->handler;
        widg_type = hinfo->widg_type;
    }

    G_UNLOCK(control_handlers);

    if(found)
    {
        widget = g_object_new(widg_type, NULL);

        /* call handler function to configure UI widget but don't
        create control */
        hfunc(widget, value_type, pspec, flags | SWAMIGUI_CONTROL_NO_CREATE);
    }

    return (widget);
}

/* a GCompareFunc to find a specific widg_type/value_type handler,
   a 0 value_type is a wildcard */
static int
swamigui_control_GCompare_type(gconstpointer a, gconstpointer b)
{
    HandlerInfo *ainfo = (HandlerInfo *)a, *binfo = (HandlerInfo *)b;
    return (!(ainfo->widg_type == binfo->widg_type
              && (!binfo->value_type || ainfo->value_type == binfo->value_type)));
}

/* compares handler info by rank */
static int
swamigui_control_GCompare_rank(gconstpointer a, gconstpointer b)
{
    HandlerInfo *ainfo = (HandlerInfo *)a, *binfo = (HandlerInfo *)b;

    /* sort highest rank first */
    return ((binfo->flags & SWAMIGUI_CONTROL_RANK_MASK)
            - (ainfo->flags & SWAMIGUI_CONTROL_RANK_MASK));
}

/**
 * swamigui_control_set_queue:
 * @control: Control to assign to the GUI queue
 *
 * Set a control to use a GUI queue which is required for all controls
 * that may be controlled from a non-GUI thread.
 */
void
swamigui_control_set_queue(SwamiControl *control)
{
    g_return_if_fail(SWAMI_IS_CONTROL(control));
    swami_control_set_queue(control, swamigui_root->ctrl_queue);
}

/**
 * swamigui_control_register:
 * @widg_type: Type of object control to register
 * @value_type: The control value type
 * @handler: Handler function for creating the control
 * @flags: A rank value between 1:lowest to 63:highest, 0:default (see
 *   SwamiguiControlRank) or'ed with SwamiguiControlFlags defining the
 *   view/control capabilities of this handler. The rank allows
 *   preferred object types to be chosen when there are multiple object
 *   control handlers for the same value and base object types. If
 *   neither #SWAMIGUI_CONTROL_VIEW or #SWAMIGUI_CONTROL_CTRL are specified
 *   then control/view is assumed (#SWAMIGUI_CONTROL_CTRLVIEW).
 *
 * This function registers new GUI control types. It is multi-thread safe
 * and can be called outside of the GUI thread (from a plugin for instance).
 * If the given @widg_type/@value_type already exists then the new @handler
 * is used. The @flags parameter specifies the rank to give preference to
 * handlers with the same @value_type and also contains control/view
 * capability flags.
 */
void
swamigui_control_register(GType widg_type, GType value_type,
                          SwamiguiControlHandler handler, guint flags)
{
    HandlerInfo *hinfo, cmp;
    GList *p;

    g_return_if_fail(g_type_is_a(widg_type, G_TYPE_OBJECT));
    g_return_if_fail(G_TYPE_IS_VALUE_TYPE(value_type) || value_type == G_TYPE_ENUM || value_type == G_TYPE_FLAGS);

    if((flags & SWAMIGUI_CONTROL_RANK_MASK) == 0)
    {
        flags |= SWAMIGUI_CONTROL_RANK_DEFAULT;
    }

    if(!(flags & SWAMIGUI_CONTROL_CTRLVIEW))
    {
        flags |= SWAMIGUI_CONTROL_CTRLVIEW;
    }

    cmp.widg_type = widg_type;
    cmp.value_type = value_type;

    G_LOCK(control_handlers);

    p = g_list_find_custom(control_handlers, &cmp,
                           swamigui_control_GCompare_type);

    if(!p)
    {
        hinfo = g_slice_new0(HandlerInfo);
        control_handlers = g_list_insert_sorted(control_handlers, hinfo,
                                                swamigui_control_GCompare_rank);
    }
    else
    {
        hinfo = (HandlerInfo *)(p->data);
    }

    hinfo->widg_type = widg_type;
    hinfo->value_type = value_type;
    hinfo->flags = flags;
    hinfo->handler = handler;

    G_UNLOCK(control_handlers);
}

/**
 * swamigui_control_unregister:
 * @widg_type: Object type of control handler to unregister
 * @value_type: The value type of the control handler
 *
 * Unregisters a previous @widg_type/@value_type GUI control handler.
 * It is multi-thread safe and can be called outside of GUI thread (from a
 * plugin for instance).
 */
void
swamigui_control_unregister(GType widg_type, GType value_type)
{
    HandlerInfo cmp;
    GList *p;

    g_return_if_fail(g_type_is_a(widg_type, GTK_TYPE_OBJECT));
    g_return_if_fail(G_TYPE_IS_VALUE(value_type));

    cmp.widg_type = widg_type;
    cmp.value_type = value_type;

    G_LOCK(control_handlers);

    p = g_list_find_custom(control_handlers, &cmp,
                           swamigui_control_GCompare_type);

    if(p)
    {
        g_slice_free(HandlerInfo, p->data);
        control_handlers = g_list_delete_link(control_handlers, p);
    }

    G_UNLOCK(control_handlers);

    if(!p)
        g_warning("Failed to find widget handler type '%s' value type '%s'",
                  g_type_name(widg_type), g_type_name(value_type));
}

/* Recursive function to walk a GtkContainer and add all widgets with GtkBuilder
 * names beginning with PROP:: to a list */
static void
swamigui_control_glade_container_foreach(GtkWidget *widget, gpointer data)
{
    GSList **list = data;
    const char *name;

    name = gtk_buildable_get_name(GTK_BUILDABLE(widget));

    if(name && strncmp(name, "PROP::", 6) == 0)
    {
        *list = g_slist_prepend(*list, widget);
    }

    if(GTK_IS_CONTAINER(widget))
        gtk_container_foreach(GTK_CONTAINER(widget),
                              swamigui_control_glade_container_foreach, list);
}

/**
 * swamigui_control_glade_prop_connect:
 * @widget: A GTK widget created by GtkBuilder
 * @obj: Object to control properties of or %NULL to unset active object
 *
 * This function connects a GtkBuilder created @widget, with child widgets whose
 * names are of the form "PROP::[prop-name]", to the corresponding GObject
 * properties of @obj ([prop-name] values).  An example child widget name would be
 * "PROP::volume" which would control the "volume" property of an object.
 * This allows for object GUI interfaces to be created with a minimum of code.
 * In order to work around issues with duplicate GtkBuilder names, a colon ':'
 * and any arbitrary text (a number for example) can be used to make the name
 * unique and is ignored.  "PROP::volume:1" for example.
 */
void
swamigui_control_glade_prop_connect(GtkWidget *widget, GObject *obj)
{
    GtkWidget *widg;
    GParamSpec *pspec = NULL;
    const char *name;
    char *propname;
    SwamiControl *propctrl, *widgctrl;
    GObjectClass *objclass = NULL;
    gboolean viewonly = FALSE;
    GSList *list = NULL, *p;

    g_return_if_fail(GTK_IS_WIDGET(widget));
    g_return_if_fail(!obj || G_IS_OBJECT(obj));

    if(obj)
    {
        objclass = g_type_class_peek(G_OBJECT_TYPE(obj));
        g_return_if_fail(objclass != NULL);
    }

    if(GTK_IS_CONTAINER(widget))          /* Recurse widget tree and add all PROP:: widgets */
        gtk_container_foreach(GTK_CONTAINER(widget),
                              swamigui_control_glade_container_foreach, &list);
    else
    {
        list = g_slist_prepend(list, widget);
    }


    /* loop over widgets */
    for(p = list; p; p = p->next)
    {
        widg = (GtkWidget *)(p->data);
        name = gtk_buildable_get_name(GTK_BUILDABLE(widg));
        name += 6;	/* skip "PROP::" prefix */

        /* to work around duplicate names, everything following a ':' char is ignored */
        if((propname = strchr(name, ':')))
        {
            propname = g_strndup(name, propname - name);    /* ++ alloc */
        }
        else
        {
            propname = g_strdup(name);    /* ++ alloc */
        }

        /* get parameter spec of the property for object (if obj is set) */
        if(obj)
        {
            pspec = g_object_class_find_property(objclass, propname);

            if(!pspec)
            {
                g_warning("Object of type %s has no property '%s'",
                          g_type_name(G_OBJECT_TYPE(obj)), propname);
                g_free(propname);	/* -- free */
                continue;
            }

            viewonly = (pspec->flags & G_PARAM_WRITABLE) == 0;
        }

        /* lookup existing control widget (if any) */
        widgctrl = swamigui_control_lookup(G_OBJECT(widg));

        if(!widgctrl)
        {
            if(!obj)
            {
                g_free(propname);	/* -- free */
                continue;	/* no widget control to disconnect, continue */
            }

            /* create or lookup widget control (view only if property is read only) */
            widgctrl = swamigui_control_new_for_widget_full
                       (G_OBJECT(widg), G_PARAM_SPEC_VALUE_TYPE(pspec), pspec,
                        viewonly ? SWAMIGUI_CONTROL_VIEW : 0);

            if(!widgctrl)
            {
                g_critical("Failed to create widget control for  '%s' of type '%s'",
                           propname, g_type_name(G_OBJECT_TYPE(widg)));
                g_free(propname);	/* -- free */
                continue;
            }
        }	/* disconnect any existing connections */
        else
        {
            swami_control_disconnect_all(widgctrl);
        }

        if(obj)
        {
            /* get a control for the object's property */
            propctrl = swami_get_control_prop(obj, pspec);	/* ++ ref */

            if(propctrl)
            {
                /* connect the property control to the widget control */
                swami_control_connect(propctrl, widgctrl,
                                      (viewonly ? 0 : SWAMI_CONTROL_CONN_BIDIR)
                                      | SWAMI_CONTROL_CONN_INIT
                                      | SWAMI_CONTROL_CONN_SPEC);
                g_object_unref(propctrl);	/* -- unref */
            }
        }

        g_free(propname);	/* -- free */
    }

    g_slist_free(list);	/* -- free list */
}

/**
 * swamigui_control_get_value_alias_type:
 * @type: Type to get alias type of
 *
 * Get the real value type used to control the given @type.
 * For example, all integer and floating point types are handled by
 * G_TYPE_DOUBLE controls.
 *
 * Returns: The alias type for the @type parameter or the same value if
 * type has no alias.
 */
GType
swamigui_control_get_alias_value_type(GType type)
{
    switch(type)
    {
    case G_TYPE_CHAR:
    case G_TYPE_UCHAR:
    case G_TYPE_INT:
    case G_TYPE_UINT:
    case G_TYPE_LONG:
    case G_TYPE_ULONG:
    case G_TYPE_INT64:
    case G_TYPE_UINT64:
    case G_TYPE_FLOAT:
        return (G_TYPE_DOUBLE);

    default:
        return (type);
    }
}
