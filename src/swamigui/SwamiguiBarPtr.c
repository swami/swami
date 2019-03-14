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
 * Copyright (C) 1999-2018 Element Green <element@elementsofsound.org>
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


enum
{
  PROP_0,
  PROP_POINTER_WIDTH,
  PROP_POINTER_HEIGHT,
  PROP_TYPE,
  PROP_INTERACTIVE,
  PROP_COLOR,
  PROP_LABEL,
  PROP_TOOLTIP
};

static void swamigui_bar_ptr_set_property (GObject *object, guint property_id,
					   const GValue *value,
					   GParamSpec *pspec);
static void swamigui_bar_ptr_get_property (GObject *object, guint property_id,
					   GValue *value, GParamSpec *pspec);
static void swamigui_bar_ptr_finalize (GObject *object);


G_DEFINE_TYPE (SwamiguiBarPtr, swamigui_bar_ptr, G_TYPE_OBJECT);


static void
swamigui_bar_ptr_class_init (SwamiguiBarPtrClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->set_property = swamigui_bar_ptr_set_property;
  obj_class->get_property = swamigui_bar_ptr_get_property;
  obj_class->finalize = swamigui_bar_ptr_finalize;

  g_object_class_install_property (obj_class, PROP_POINTER_WIDTH,
			g_param_spec_int ("pointer-width", "Pointer width",
					  "Width of pointer in pixels",
					  0, G_MAXINT, 0,
					  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_POINTER_HEIGHT,
			g_param_spec_int ("pointer-height", "Pointer height",
					  "Height of pointer in pixels",
					  0, G_MAXINT, 0,
					  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_TYPE,
			g_param_spec_enum ("type", "Type",
					   "Pointer type",
					   SWAMIGUI_TYPE_BAR_PTR_TYPE,
					   SWAMIGUI_BAR_PTR_POSITION,
					   G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_INTERACTIVE,
			g_param_spec_boolean ("interactive", "Interactive",
					      "Interactive",
					      TRUE, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_COLOR,
			g_param_spec_boxed ("color", NULL, NULL,
                                            GDK_TYPE_RGBA,
					    G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_LABEL,
			g_param_spec_string ("label", "Label", "Label",
					     NULL, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_TOOLTIP,
			g_param_spec_string ("tooltip", "Tooltip", "Tooltip",
					     NULL, G_PARAM_READWRITE));
}

static void
swamigui_bar_ptr_set_property (GObject *object, guint property_id,
			       const GValue *value, GParamSpec *pspec)
{
  SwamiguiBarPtr *barptr = SWAMIGUI_BAR_PTR (object);
  GdkRGBA *rgba;

  switch (property_id)
  {
    case PROP_POINTER_WIDTH:
      barptr->pointer_width = g_value_get_int (value);
      break;
    case PROP_POINTER_HEIGHT:
      barptr->pointer_height = g_value_get_int (value);
      break;
    case PROP_TYPE:
      barptr->type = g_value_get_enum (value);
      break;
    case PROP_INTERACTIVE:
      barptr->interactive = g_value_get_boolean (value);
      break;
    case PROP_COLOR:
      rgba = g_value_get_boxed (value);
      if (rgba) barptr->color = *rgba;
      break;
    case PROP_LABEL:
      barptr->label = g_value_dup_string (value);
      break;
    case PROP_TOOLTIP:
      barptr->tooltip = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
swamigui_bar_ptr_get_property (GObject *object, guint property_id,
			       GValue *value, GParamSpec *pspec)
{
  SwamiguiBarPtr *barptr = SWAMIGUI_BAR_PTR (object);

  switch (property_id)
  {
    case PROP_POINTER_WIDTH:
      g_value_set_int (value, barptr->pointer_width);
      break;
    case PROP_POINTER_HEIGHT:
      g_value_set_int (value, barptr->pointer_height);
      break;
    case PROP_TYPE:
      g_value_set_enum (value, barptr->type);
      break;
    case PROP_INTERACTIVE:
      g_value_set_boolean (value, barptr->interactive);
      break;
    case PROP_COLOR:
      g_value_set_boxed (value, &barptr->color);
      break;
    case PROP_LABEL:
      g_value_set_string (value, barptr->label);
      break;
    case PROP_TOOLTIP:
      g_value_set_string (value, barptr->tooltip);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
swamigui_bar_ptr_init (SwamiguiBarPtr *barptr)
{
  barptr->type = SWAMIGUI_BAR_PTR_POSITION;
  barptr->interactive = TRUE;
  barptr->label = NULL;
  barptr->tooltip = NULL;

  gdk_rgba_parse (&barptr->color, "#0F0");
}

static void
swamigui_bar_ptr_finalize (GObject *object)
{
  SwamiguiBarPtr *barptr = SWAMIGUI_BAR_PTR (object);

  g_free (barptr->label);
  g_free (barptr->tooltip);

  if (G_OBJECT_CLASS (swamigui_bar_ptr_parent_class)->finalize)
    (* G_OBJECT_CLASS (swamigui_bar_ptr_parent_class)->finalize)(object);
}

/**
 * swamigui_bar_ptr_new:
 *
 * Create a new bar pointer object.
 *
 * Returns: New bar pointer object with a reference count of 1 which the caller owns
 */
SwamiguiBarPtr *
swamigui_bar_ptr_new (void)
{
  return SWAMIGUI_BAR_PTR (g_object_new (SWAMIGUI_TYPE_BAR_PTR, NULL));
}

