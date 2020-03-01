/*
 * SwamiguiStatusbar.c - A statusbar (multiple labels/progresses)
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

#include "SwamiguiStatusbar.h"
#include "i18n.h"
#include "util.h"
#include "libswami/swami_priv.h"

enum
{
    PROP_0,
    PROP_DEFAULT_TIMEOUT
};

typedef struct
{
    SwamiguiStatusbar *statusbar;	/* parent statusbar instance */
    guint id;		/* unique message ID */
    char *group;		/* group ID for this label or NULL */
    int timeout;		/* timeout for this label in milliseconds (0 for none) */
    guint timeout_handle;	/* main loop timeout handle or 0 if none */
    guint8 pos;		/* SwamiguiStatusbarPos */
    GtkWidget *widg;	/* status widget */
    GtkWidget *frame;	/* frame around status widget */
} StatusItem;

/* allocation and release of status item structs */
#define status_item_new()	g_slice_new0 (StatusItem)
#define status_item_free(item)	g_slice_free (StatusItem, item)


/* default message timeout value in milliseconds */
#define DEFAULT_TIMEOUT_VALUE	4000


/* Local Prototypes */

static void swamigui_statusbar_set_property(GObject *object, guint property_id,
        const GValue *value,
        GParamSpec *pspec);
static void swamigui_statusbar_get_property(GObject *object, guint property_id,
        GValue *value, GParamSpec *pspec);

static void swamigui_statusbar_init(SwamiguiStatusbar *statusbar);
static gboolean swamigui_statusbar_item_timeout(gpointer data);
static GList *swamigui_statusbar_find(SwamiguiStatusbar *statusbar, guint id,
                                      const char *group);
static void swamigui_statusbar_cb_item_close_clicked(GtkButton *button,
        gpointer data);

/* define the SwamiguiStatusbar type */
G_DEFINE_TYPE(SwamiguiStatusbar, swamigui_statusbar, GTK_TYPE_FRAME);


static void
swamigui_statusbar_class_init(SwamiguiStatusbarClass *klass)
{
    GObjectClass *obj_class = G_OBJECT_CLASS(klass);

    obj_class->set_property = swamigui_statusbar_set_property;
    obj_class->get_property = swamigui_statusbar_get_property;

    g_object_class_install_property(obj_class, PROP_DEFAULT_TIMEOUT,
                                    g_param_spec_int("default-timeout", _("Default Timeout"),
                                            _("Default timeout in milliseconds"),
                                            0, G_MAXINT, DEFAULT_TIMEOUT_VALUE,
                                            G_PARAM_READWRITE));
}

static void
swamigui_statusbar_set_property(GObject *object, guint property_id,
                                const GValue *value, GParamSpec *pspec)
{
    SwamiguiStatusbar *statusbar = SWAMIGUI_STATUSBAR(object);

    switch(property_id)
    {
    case PROP_DEFAULT_TIMEOUT:
        statusbar->default_timeout = g_value_get_uint(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void
swamigui_statusbar_get_property(GObject *object, guint property_id,
                                GValue *value, GParamSpec *pspec)
{
    SwamiguiStatusbar *statusbar = SWAMIGUI_STATUSBAR(object);

    switch(property_id)
    {
    case PROP_DEFAULT_TIMEOUT:
        g_value_set_uint(value, statusbar->default_timeout);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void
swamigui_statusbar_init(SwamiguiStatusbar *statusbar)
{
    GtkWidget *widg;

    statusbar->id_counter = 1;
    statusbar->default_timeout = DEFAULT_TIMEOUT_VALUE;

    gtk_frame_set_shadow_type(GTK_FRAME(statusbar), GTK_SHADOW_IN);

    statusbar->box = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(statusbar->box);
    gtk_container_add(GTK_CONTAINER(statusbar), statusbar->box);

    /* add the Global group status label item */
    widg = swamigui_statusbar_msg_label_new("", SWAMIGUI_STATUSBAR_GLOBAL_MAXLEN);
    swamigui_statusbar_add(statusbar, "Global", 0,
                           SWAMIGUI_STATUSBAR_POS_RIGHT, widg);
}

/**
 * swamigui_statusbar_new:
 *
 * Create a new status bar widget.
 *
 * Returns: New widget.
 */
GtkWidget *
swamigui_statusbar_new(void)
{
    return (GTK_WIDGET(g_object_new(SWAMIGUI_TYPE_STATUSBAR, NULL)));
}

/**
 * swamigui_statusbar_add:
 * @statusbar: Statusbar widget
 * @group: Group identifier (existing message with same group is replaced,
 *   NULL for no group)
 * @timeout: Timeout of statusbar message in milliseconds
 *   (see #SwamiguiStatusbarTimeout for special values including
 *    #SWAMIGUI_STATUSBAR_TIMEOUT_FOREVER (0) for no timeout and
 *    #SWAMIGUI_STATUSBAR_TIMEOUT_DEFAULT to use "default-timeout" property
 *    value)
 * @pos: Position of message (#SwamiguiStatusbarPos, 0 for default - left)
 * @widg: Status widget to add to status bar
 *
 * Add a widget to a status bar.  The @widg is usually created with one of the
 * helper functions, such as swamigui_statusbar_msg_label_new() or
 * swamigui_statusbar_msg_progress_new(), although an arbitrary widget can
 * be added.
 *
 * Returns: New message unique ID (which can be used to change/remove message)
 */
guint
swamigui_statusbar_add(SwamiguiStatusbar *statusbar, const char *group,
                       int timeout, guint pos, GtkWidget *widg)
{
    StatusItem *item;
    GList *p;

    g_return_val_if_fail(SWAMIGUI_IS_STATUSBAR(statusbar), 0);
    g_return_val_if_fail(GTK_IS_WIDGET(widg), 0);

    if(timeout == SWAMIGUI_STATUSBAR_TIMEOUT_DEFAULT)
    {
        timeout = statusbar->default_timeout;
    }

    if(group)	/* if group specified, search for existing item group match */
    {
        for(p = statusbar->items; p; p = p->next)
        {
            item = (StatusItem *)(p->data);

            if(item->group && strcmp(item->group, group) == 0)	/* group match? */
            {
                /* replace the widget */
                gtk_container_remove(GTK_CONTAINER(item->frame), item->widg);
                gtk_container_add(GTK_CONTAINER(item->frame), widg);
                item->widg = widg;
                gtk_widget_show(widg);

                g_object_set_data(G_OBJECT(widg), "_item", item);

                if(item->timeout_handle)	/* remove old timeout if any */
                {
                    g_source_remove(item->timeout_handle);
                }

                item->timeout = timeout;

                if(timeout)	/* add new timeout callback if timeout given */
                {
                    g_timeout_add(timeout, swamigui_statusbar_item_timeout, item);
                }

                return (item->id);
            }
        }
    }

    item = status_item_new();
    item->statusbar = statusbar;
    item->id = statusbar->id_counter++;
    item->group = g_strdup(group);
    item->timeout = timeout;
    item->pos = pos;
    item->widg = widg;
    item->frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(item->frame), GTK_SHADOW_OUT);

    statusbar->items = g_list_prepend(statusbar->items, item);

    gtk_container_add(GTK_CONTAINER(item->frame), widg);
    gtk_widget_show_all(item->frame);

    g_object_set_data(G_OBJECT(widg), "_item", item);

    /* pack the new item into the statusbar */
    if(pos == SWAMIGUI_STATUSBAR_POS_LEFT)
    {
        gtk_box_pack_start(GTK_BOX(statusbar->box), item->frame, FALSE, FALSE, 2);
    }
    else
    {
        gtk_box_pack_end(GTK_BOX(statusbar->box), item->frame, FALSE, FALSE, 2);
    }

    if(timeout)	/* add new timeout callback if timeout given */
    {
        g_timeout_add(timeout, swamigui_statusbar_item_timeout, item);
    }

    return (item->id);
}

/* timeout callback used to remove statusbar items after a timeout period */
static gboolean
swamigui_statusbar_item_timeout(gpointer data)
{
    StatusItem *item = (StatusItem *)data;
    swamigui_statusbar_remove(item->statusbar, item->id, NULL);
    return (FALSE);
}

/**
 * swamigui_statusbar_remove:
 * @statusbar: Statusbar widget
 * @id: Unique ID of message (0 if @group is specified)
 * @group: Group of message to remove (%NULL if @id is specified)
 *
 * Remove a message by @id or @group.
 */
void
swamigui_statusbar_remove(SwamiguiStatusbar *statusbar, guint id,
                          const char *group)
{
    StatusItem *item;
    GList *p;

    g_return_if_fail(SWAMIGUI_IS_STATUSBAR(statusbar));
    g_return_if_fail(id != 0 || group != NULL);

    p = swamigui_statusbar_find(statusbar, id, group);

    if(!p)
    {
        return;
    }

    item = (StatusItem *)(p->data);

    g_free(item->group);

    if(item->timeout_handle)	/* remove old timeout if any */
    {
        g_source_remove(item->timeout_handle);
    }

    gtk_container_remove(GTK_CONTAINER(statusbar->box), item->frame);

    statusbar->items = g_list_delete_link(statusbar->items, p);
    status_item_free(item);
}

/**
 * swamigui_statusbar_printf:
 * @statusbar: Statusbar widget
 * @format: printf() style format string.
 * @...: Additional arguments for @format string
 *
 * A convenience function to display a message label to a statusbar with the
 * "default-timeout" property value for the timeout, no group and positioned
 * left.  This is commonly used to display an operation that was performed.
 */
void
swamigui_statusbar_printf(SwamiguiStatusbar *statusbar, const char *format,
                          ...)
{
    GtkWidget *label;
    va_list args;
    char *s;

    va_start(args, format);
    s = g_strdup_vprintf(format, args);
    va_end(args);

    label = swamigui_statusbar_msg_label_new(s, 0);
    g_free(s);

    swamigui_statusbar_add(statusbar, NULL,
                           SWAMIGUI_STATUSBAR_TIMEOUT_DEFAULT,
                           SWAMIGUI_STATUSBAR_POS_LEFT, label);
}

/* internal function used to find a statusbar item */
static GList *
swamigui_statusbar_find(SwamiguiStatusbar *statusbar, guint id,
                        const char *group)
{
    StatusItem *item;
    GList *p;

    for(p = statusbar->items; p; p = p->next)
    {
        item = (StatusItem *)(p->data);

        /* criteria matches? */
        if((id && item->id == id)
                || (group && item->group && strcmp(item->group, group) == 0))
        {
            return (p);
        }
    }

    return (NULL);
}

/**
 * swamigui_statusbar_msg_label_new:
 * @label: Label text to assign to new widget
 * @maxlen: Maximum length of label widget (sets size, 0 to set to width of @label)
 *
 * A helper function to create a label widget for use in a statusbar.  Doesn't
 * do a whole lot beyond just creating a regular #GtkLabel and setting its
 * max length.
 */
GtkWidget *
swamigui_statusbar_msg_label_new(const char *label, guint maxlen)
{
    GtkWidget *widg;

    widg = gtk_label_new(label);

    if(maxlen > 0)
    {
        gtk_label_set_width_chars(GTK_LABEL(widg), maxlen);
    }

    gtk_misc_set_alignment(GTK_MISC(widg), 0.0, 0.5);
    gtk_widget_show_all(widg);
    return (widg);
}

/**
 * swamigui_statusbar_msg_progress_new:
 * @label: Label text to assign to new widget
 * @close: Close callback function (%NULL to not have a close button)
 *
 * A helper function to create a progress status bar item.
 */
GtkWidget *
swamigui_statusbar_msg_progress_new(const char *label,
                                    SwamiguiStatusbarCloseFunc close)
{
    GtkWidget *hbox;
    GtkWidget *progress;
    GtkWidget *btn;
    GtkWidget *image;

    hbox = gtk_hbox_new(FALSE, 0);

    progress = gtk_progress_bar_new();

    if(label)
    {
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), label);
    }

    gtk_box_pack_start(GTK_BOX(hbox), progress, FALSE, FALSE, 0);

    if(close)
    {
        btn = gtk_button_new();
        image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_BUTTON);
        gtk_container_add(GTK_CONTAINER(btn), image);
        gtk_box_pack_start(GTK_BOX(hbox), btn, FALSE, FALSE, 0);

        g_object_set_data(G_OBJECT(hbox), "_close", close);
        g_signal_connect(G_OBJECT(btn), "clicked",
                         G_CALLBACK(swamigui_statusbar_cb_item_close_clicked),
                         hbox);
    }

    /* used by swamigui_statusbar_msg_set_progress() */
    g_object_set_data(G_OBJECT(hbox), "_progress", progress);

    gtk_widget_show_all(hbox);
    return (hbox);
}

/* callback which gets called when an item close button is clicked */
static void
swamigui_statusbar_cb_item_close_clicked(GtkButton *button, gpointer data)
{
    GtkWidget *hbox = (GtkWidget *)data;
    SwamiguiStatusbarCloseFunc closefunc;
    StatusItem *statusitem;

    closefunc = g_object_get_data(G_OBJECT(hbox), "_close");
    g_return_if_fail(closefunc != NULL);

    /* "_item" gets set when it is added to the statusbar */
    statusitem = g_object_get_data(G_OBJECT(hbox), "_item");
    g_return_if_fail(statusitem != NULL);

    if(closefunc(statusitem->statusbar, hbox))
    {
        swamigui_statusbar_remove(statusitem->statusbar, statusitem->id, NULL);
    }
}

/**
 * swamigui_statusbar_msg_set_timeout:
 * @statusbar: Statusbar widget
 * @id: Unique ID of message (0 if @group is specified)
 * @group: Group of message (%NULL if @id is specified)
 * @timeout: New timeout of message in milliseconds
 *   (see #SwamiguiStatusbarTimeout for special values including
 *    #SWAMIGUI_STATUSBAR_TIMEOUT_FOREVER (0) for no timeout and
 *    #SWAMIGUI_STATUSBAR_TIMEOUT_DEFAULT to use "default-timeout" property
 *    value)
 *
 * Modify the timeout of an existing message in the statusbar.  Message is
 * selected by @id or @group.
 */
void
swamigui_statusbar_msg_set_timeout(SwamiguiStatusbar *statusbar, guint id,
                                   const char *group, int timeout)
{
    StatusItem *item;
    GList *p;

    g_return_if_fail(SWAMIGUI_IS_STATUSBAR(statusbar));
    g_return_if_fail(id != 0 || group != NULL);

    p = swamigui_statusbar_find(statusbar, id, group);

    if(!p)
    {
        return;
    }

    if(timeout == SWAMIGUI_STATUSBAR_TIMEOUT_DEFAULT)
    {
        timeout = statusbar->default_timeout;
    }

    item = (StatusItem *)(p->data);

    if(item->timeout_handle)	/* remove old timeout if any */
    {
        g_source_remove(item->timeout_handle);
    }

    item->timeout = timeout;

    if(timeout)	/* add new timeout callback if timeout given */
    {
        g_timeout_add(timeout, swamigui_statusbar_item_timeout, item);
    }
}

/**
 * swamigui_statusbar_msg_set_label:
 * @statusbar: Statusbar widget
 * @id: Unique ID of message (0 if @group is specified)
 * @group: Group of message (%NULL if @id is specified)
 * @label: New label text to assign to statusbar item
 *
 * Modify the label of an existing message in the statusbar.  Message is
 * selected by @id or @group.  This function should only be used for #GtkLabel
 * widget status items or those created with
 * swamigui_statusbar_msg_label_new() and swamigui_statusbar_msg_progress_new().
 */
void
swamigui_statusbar_msg_set_label(SwamiguiStatusbar *statusbar,
                                 guint id, const char *group,
                                 const char *label)
{
    StatusItem *item;
    GtkWidget *progress;
    GList *p;

    g_return_if_fail(SWAMIGUI_IS_STATUSBAR(statusbar));
    g_return_if_fail(id != 0 || group != NULL);

    p = swamigui_statusbar_find(statusbar, id, group);

    if(!p)
    {
        return;
    }

    item = (StatusItem *)(p->data);
    progress = g_object_get_data(G_OBJECT(item->widg), "_progress");

    g_return_if_fail(GTK_IS_LABEL(item->widg) || progress);

    if(progress)
    {
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), label);
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(item->widg), label);
    }
}

/**
 * swamigui_statusbar_msg_set_progress:
 * @statusbar: Statusbar widget
 * @id: Unique ID of message (0 if @group is specified)
 * @group: Group of message (%NULL if @id is specified)
 * @val: New progress value (0.0 to 1.0)
 *
 * Modify the progress indicator of an existing message in the statusbar.
 * Message is selected by @id or @group.  This function should only be used for
 * widget status items created with swamigui_statusbar_msg_progress_new().
 */
void
swamigui_statusbar_msg_set_progress(SwamiguiStatusbar *statusbar,
                                    guint id, const char *group, double val)
{
    GtkWidget *progress;
    StatusItem *item;
    GList *p;

    g_return_if_fail(SWAMIGUI_IS_STATUSBAR(statusbar));
    g_return_if_fail(id != 0 || group != NULL);

    p = swamigui_statusbar_find(statusbar, id, group);

    if(!p)
    {
        return;
    }

    item = (StatusItem *)(p->data);
    progress = g_object_get_data(G_OBJECT(item->widg), "_progress");
    g_return_if_fail(progress != NULL);

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), val);
}
