/**
 * SECTION:SwamiguiBarPtr
 * @short_description: Canvas item used as a position or range indicator.
 * @see_also: #SwamiguiBar
 *
 * This canvas item is used by #SwamiguiBar to display position and range
 * indicators.  A #SwamiguiBar is composed of one or more #SwamiguiBarPtr
 * items.
 */

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

#include <stdio.h>
#include <gtk/gtk.h>

#include "SwamiguiBarPtr.h"
#include "builtin_enums.h"
#include "i18n.h"

enum
{
    PROP_0,
    PROP_WIDTH,		/* width in pixels or rectangle and pointer */
    PROP_HEIGHT,		/* total height in pixels (including pointer if any) */
    PROP_POINTER_HEIGHT,	/* height of pointer (must be less than height) */
    PROP_TYPE,
    PROP_INTERACTIVE,
    PROP_COLOR,
    PROP_LABEL,
    PROP_TOOLTIP
};

static void swamigui_bar_ptr_class_init(SwamiguiBarPtrClass *klass);
static void swamigui_bar_ptr_set_property(GObject *object, guint property_id,
        const GValue *value,
        GParamSpec *pspec);
static void swamigui_bar_ptr_get_property(GObject *object, guint property_id,
        GValue *value, GParamSpec *pspec);
static void swamigui_bar_ptr_init(SwamiguiBarPtr *barptr);
static void swamigui_bar_ptr_finalize(GObject *object);
static void
swamigui_bar_ptr_update(GnomeCanvasItem *item, double *affine,
                        ArtSVP *clip_path, int flags);

static GObjectClass *parent_class = NULL;


GType
swamigui_bar_ptr_get_type(void)
{
    static GType obj_type = 0;

    if(!obj_type)
    {
        static const GTypeInfo obj_info =
        {
            sizeof(SwamiguiBarPtrClass), NULL, NULL,
            (GClassInitFunc) swamigui_bar_ptr_class_init, NULL, NULL,
            sizeof(SwamiguiBarPtr), 0,
            (GInstanceInitFunc) swamigui_bar_ptr_init,
        };

        obj_type = g_type_register_static(GNOME_TYPE_CANVAS_GROUP,
                                          "SwamiguiBarPtr", &obj_info, 0);
    }

    return (obj_type);
}

static void
swamigui_bar_ptr_class_init(SwamiguiBarPtrClass *klass)
{
    GObjectClass *obj_class = G_OBJECT_CLASS(klass);
    GnomeCanvasItemClass *item_class = GNOME_CANVAS_ITEM_CLASS(klass);

    parent_class = g_type_class_peek_parent(klass);

    obj_class->set_property = swamigui_bar_ptr_set_property;
    obj_class->get_property = swamigui_bar_ptr_get_property;
    obj_class->finalize = swamigui_bar_ptr_finalize;

    item_class->update = swamigui_bar_ptr_update;

    g_object_class_install_property(obj_class, PROP_WIDTH,
                                    g_param_spec_int("width", _("Width"),
                                            _("Width in pixels"),
                                            0, G_MAXINT, 0,
                                            G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_HEIGHT,
                                    g_param_spec_int("height", _("Height"),
                                            _("Height in pixels"),
                                            0, G_MAXINT, 0,
                                            G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_POINTER_HEIGHT,
                                    g_param_spec_int("pointer-height", _("Pointer height"),
                                            _("Height of pointer in pixels"),
                                            0, G_MAXINT, 0,
                                            G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_TYPE,
                                    g_param_spec_enum("type", _("Type"),
                                            _("Pointer type"),
                                            SWAMIGUI_TYPE_BAR_PTR_TYPE,
                                            SWAMIGUI_BAR_PTR_POSITION,
                                            G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_INTERACTIVE,
                                    g_param_spec_boolean("interactive", _("Interactive"),
                                            _("Interactive"),
                                            TRUE, G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_COLOR,
                                    g_param_spec_uint("color", _("Color"), _("Color"),
                                            0, G_MAXUINT, 0x00FFFFFF,
                                            G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_LABEL,
                                    g_param_spec_string("label", _("Label"), _("Label"),
                                            NULL, G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_TOOLTIP,
                                    g_param_spec_string("tooltip", _("Tooltip"), _("Tooltip"),
                                            NULL, G_PARAM_READWRITE));
}

static void
swamigui_bar_ptr_set_property(GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
    SwamiguiBarPtr *barptr = SWAMIGUI_BAR_PTR(object);
    GnomeCanvasItem *item = GNOME_CANVAS_ITEM(object);

    switch(property_id)
    {
    case PROP_WIDTH:
        barptr->width = g_value_get_int(value);
        gnome_canvas_item_request_update(item);
        break;

    case PROP_HEIGHT:
        barptr->height = g_value_get_int(value);
        gnome_canvas_item_request_update(item);
        break;

    case PROP_POINTER_HEIGHT:
        barptr->pointer_height = g_value_get_int(value);
        gnome_canvas_item_request_update(item);
        break;

    case PROP_TYPE:
        barptr->type = g_value_get_enum(value);
        gnome_canvas_item_request_update(item);
        break;

    case PROP_INTERACTIVE:
        barptr->interactive = g_value_get_boolean(value);
        break;

    case PROP_COLOR:
        barptr->color = g_value_get_uint(value);
        gnome_canvas_item_request_update(item);
        break;

    case PROP_LABEL:
        barptr->label = g_value_dup_string(value);
        gnome_canvas_item_request_update(item);
        break;

    case PROP_TOOLTIP:
        barptr->tooltip = g_value_dup_string(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void
swamigui_bar_ptr_get_property(GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
    SwamiguiBarPtr *barptr = SWAMIGUI_BAR_PTR(object);

    switch(property_id)
    {
    case PROP_WIDTH:
        g_value_set_int(value, barptr->width);
        break;

    case PROP_HEIGHT:
        g_value_set_int(value, barptr->height);
        break;

    case PROP_POINTER_HEIGHT:
        g_value_set_int(value, barptr->pointer_height);
        break;

    case PROP_TYPE:
        g_value_set_enum(value, barptr->type);
        break;

    case PROP_INTERACTIVE:
        g_value_set_boolean(value, barptr->interactive);
        break;

    case PROP_COLOR:
        g_value_set_uint(value, barptr->color);
        break;

    case PROP_LABEL:
        g_value_set_string(value, barptr->label);
        break;

    case PROP_TOOLTIP:
        g_value_set_string(value, barptr->tooltip);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void
swamigui_bar_ptr_init(SwamiguiBarPtr *barptr)
{
    barptr->type = SWAMIGUI_BAR_PTR_POSITION;
    barptr->interactive = TRUE;
    barptr->color = 0x00FFFFFF;
    barptr->label = NULL;
    barptr->tooltip = NULL;
}

static void
swamigui_bar_ptr_finalize(GObject *object)
{
    SwamiguiBarPtr *barptr = SWAMIGUI_BAR_PTR(object);

    g_free(barptr->label);
    g_free(barptr->tooltip);

    if(G_OBJECT_CLASS(parent_class)->finalize)
    {
        (* G_OBJECT_CLASS(parent_class)->finalize)(object);
    }
}

/**
 * swamigui_bar_ptr_new:
 *
 * Create a new bar pointer object for adding to a #SwamiguiBar object.
 *
 * Returns: New bar pointer with a refcount of 1 which the caller owns.
 */
GnomeCanvasItem *
swamigui_bar_ptr_new(void)
{
    return (GNOME_CANVAS_ITEM(g_object_new(SWAMIGUI_TYPE_BAR_PTR, NULL)));
}

/* update bar pointer graphic primitives */
static void
swamigui_bar_ptr_update(GnomeCanvasItem *item, double *affine,
                        ArtSVP *clip_path, int flags)
{
    SwamiguiBarPtr *barptr = SWAMIGUI_BAR_PTR(item);
    GnomeCanvasPoints *points;

    if(barptr->type == SWAMIGUI_BAR_PTR_RANGE)	/* range pointer? */
    {
        if(!barptr->rect)
            barptr->rect = gnome_canvas_item_new
                           (GNOME_CANVAS_GROUP(barptr),
                            GNOME_TYPE_CANVAS_RECT,
                            "x1", (gdouble)0.0,
                            "y1", (gdouble)0.0,
                            NULL);

        if(barptr->ptr)	/* pointer rectangle not needed for range */
        {
            gtk_object_destroy(GTK_OBJECT(barptr->ptr));
            barptr->ptr = NULL;
        }

        g_object_set(barptr->rect,
                     "x2", (double)(barptr->width),
                     "y2", (double)(barptr->height),
                     "fill-color-rgba", barptr->color,
                     NULL);
    }
    else		/* position pointer mode */
    {
        int rheight = barptr->height - barptr->pointer_height; /* rectangle height */

        if(!barptr->rect)
            barptr->rect = gnome_canvas_item_new
                           (GNOME_CANVAS_GROUP(barptr),
                            GNOME_TYPE_CANVAS_RECT,
                            "x1", (gdouble)0.0,
                            "y1", (gdouble)0.0,
                            NULL);

        if(!barptr->ptr)
            barptr->rect = gnome_canvas_item_new
                           (GNOME_CANVAS_GROUP(barptr),
                            GNOME_TYPE_CANVAS_POLYGON,
                            NULL);

        g_object_set(barptr->rect,
                     "x2", (double)(barptr->width),
                     "y2", (double)(rheight),
                     "fill-color-rgba", barptr->color,
                     NULL);

        points = gnome_canvas_points_new(3);

        /* configure as a triangle on the bottom of the rectangle */
        points->coords[0] = 0;
        points->coords[1] = rheight;
        points->coords[2] = barptr->width / 2.0;
        points->coords[3] = barptr->height;
        points->coords[4] = barptr->width;
        points->coords[5] = rheight;

        g_object_set(barptr->ptr,
                     "points", points,
                     "fill-color-rgba", barptr->color,
                     NULL);

        gnome_canvas_points_free(points);
    }

    if(GNOME_CANVAS_ITEM_CLASS(parent_class)->update)
        GNOME_CANVAS_ITEM_CLASS(parent_class)->update(item, affine, clip_path,
                flags);
}
