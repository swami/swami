/**
 * SECTION:SwamiguiBar
 * @short_description: Canvas item for displaying multiple pointers or ranges.
 * @see_also: #SwamiguiBarPtr
 *
 * Horizontal bar canvas item for displaying multiple pointers and/or ranges.
 * A #SwamiguiBar is composed of one or more #SwamiguiBarPtr items.
 */

/*
 * Swami
 * Copyright (C) 1999-2010 Joshua "Element" Green <jgreen@users.sourceforge.net>
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
#include <string.h>
#include <gtk/gtk.h>

#include "libswami/swami_priv.h"

#include "SwamiguiBar.h"
#include "i18n.h"

enum
{
  PROP_0,
  PROP_MIN_HEIGHT,	/* min height of pointers (top of stack) */
  PROP_MAX_HEIGHT	/* max height of pointers (bottom of stack) */
};

/* structure which defines an interface pointer */
typedef struct
{
  char *id;			/* id of this item */
  GnomeCanvasItem *barptr;	/* pointer canvas item */
  SwamiguiBar *bar;		/* parent bar object */
  gboolean mouse_sel;		/* TRUE if mouse selection is in progress */
} PtrInfo;

static void swamigui_bar_class_init (SwamiguiBarClass *klass);
static void swamigui_bar_set_property (GObject *object, guint property_id,
					const GValue *value,
					GParamSpec *pspec);
static void swamigui_bar_get_property (GObject *object, guint property_id,
					GValue *value, GParamSpec *pspec);
static void swamigui_bar_init (SwamiguiBar *bar);
static void swamigui_bar_finalize (GObject *object);

static gboolean swamigui_bar_cb_ptr_event (GnomeCanvasItem *item,
					   GdkEvent *event, gpointer data);
static void swamigui_bar_update_ptr_heights (SwamiguiBar *bar);

static GObjectClass *parent_class = NULL;

GType
swamigui_bar_get_type (void)
{
  static GType obj_type = 0;

  if (!obj_type)
    {
      static const GTypeInfo obj_info =
	{
	  sizeof (SwamiguiBarClass), NULL, NULL,
	  (GClassInitFunc) swamigui_bar_class_init, NULL, NULL,
	  sizeof (SwamiguiBar), 0,
	  (GInstanceInitFunc) swamigui_bar_init,
	};

      obj_type = g_type_register_static (GNOME_TYPE_CANVAS_GROUP,
					 "SwamiguiBar", &obj_info, 0);
    }

  return (obj_type);
}

static void
swamigui_bar_class_init (SwamiguiBarClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  obj_class->set_property = swamigui_bar_set_property;
  obj_class->get_property = swamigui_bar_get_property;
  obj_class->finalize = swamigui_bar_finalize;

  g_object_class_install_property (obj_class, PROP_MIN_HEIGHT,
			g_param_spec_int ("min-height", _("Min height"),
					  _("Minimum height of pointers"),
					  1, G_MAXINT, 16,
					  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_MAX_HEIGHT,
			g_param_spec_int ("max-height", _("Max height"),
					  _("Maximum height of pointers"),
					  1, G_MAXINT, 48,
					  G_PARAM_READWRITE));
}

static void
swamigui_bar_set_property (GObject *object, guint property_id,
			    const GValue *value, GParamSpec *pspec)
{
  SwamiguiBar *bar = SWAMIGUI_BAR (object);

  switch (property_id)
    {
    case PROP_MIN_HEIGHT:
      bar->min_height = g_value_get_int (value);
      swamigui_bar_update_ptr_heights (bar);
      break;
    case PROP_MAX_HEIGHT:
      bar->max_height = g_value_get_int (value);
      swamigui_bar_update_ptr_heights (bar);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swamigui_bar_get_property (GObject *object, guint property_id,
			    GValue *value, GParamSpec *pspec)
{
  SwamiguiBar *bar = SWAMIGUI_BAR (object);

  switch (property_id)
    {
    case PROP_MIN_HEIGHT:
      g_value_set_int (value, bar->min_height);
      break;
    case PROP_MAX_HEIGHT:
      g_value_set_int (value, bar->max_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swamigui_bar_init (SwamiguiBar *bar)
{
  bar->min_height = 16;
  bar->max_height = 48;
  bar->ptrlist = NULL;
}

static void
swamigui_bar_finalize (GObject *object)
{
  SwamiguiBar *bar = SWAMIGUI_BAR (object);
  PtrInfo *info;
  GList *p;

  for (p = bar->ptrlist; p; p = g_list_delete_link (p, p))
    {
      info = (PtrInfo *)(p->data);
      g_free (info->id);
      g_free (p->data);
    }

  if (G_OBJECT_CLASS (parent_class)->finalize)
    (* G_OBJECT_CLASS (parent_class)->finalize)(object);
}

/* callback for SwamiguiBarPtr events */
static gboolean
swamigui_bar_cb_ptr_event (GnomeCanvasItem *item, GdkEvent *event,
			   gpointer data)
{
  PtrInfo *ptrinfo = (PtrInfo *)data;
  SwamiguiBar *bar = SWAMIGUI_BAR (ptrinfo->bar);
  GdkEventButton *bevent;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *)event;
      if (bevent->button != 1) break;

      ptrinfo->mouse_sel = TRUE;

      /* make sure the item is raised to the top */
      swamigui_bar_raise_pointer_to_top (bar, ptrinfo->id);

      gnome_canvas_item_grab (item, GDK_POINTER_MOTION_MASK
			      | GDK_BUTTON_RELEASE_MASK, NULL,
			      bevent->time);
      return (TRUE);
    case GDK_BUTTON_RELEASE:
      if (!ptrinfo->mouse_sel) break; /* not selected? */

      ptrinfo->mouse_sel = FALSE;
      gnome_canvas_item_ungrab (item, event->button.time);
      break;
    case GDK_MOTION_NOTIFY:
      if (!ptrinfo->mouse_sel) break; /* not selected? */

      break;
    default:
      break;
    }

  return (FALSE);
}

/* update heights of all pointers based on current stacking order */
static void
swamigui_bar_update_ptr_heights (SwamiguiBar *bar)
{
  PtrInfo *info;
  int count, height_diff, min_height, max_height;
  double xform[6];
  GList *p;
  int height;
  int i;

  count = g_list_length (bar->ptrlist);
  if (count == 0) return;

  /* just in case they are swapped */
  min_height = MIN (bar->min_height, bar->max_height);
  max_height = MAX (bar->min_height, bar->max_height);
  height_diff = max_height - min_height;

  /* set height and y-position of pointers */
  for (i = 0, p = bar->ptrlist; i < count; i++, p = p->next)
    {
      info = (PtrInfo *)(p->data);

      /* set the height of the pointer between min-height and max-height */
      if (count != 1) height = min_height + (height_diff * i) / (count - 1);
      else height = max_height;

      g_object_set (info->barptr, "height", height, NULL);

      /* set the y pos of pointer so they line up on the bottom */
      gnome_canvas_item_i2w_affine (info->barptr, xform);
      xform[5] = max_height - height;
      gnome_canvas_item_affine_absolute (info->barptr, xform);
    }
}

/**
 * swamigui_bar_create_pointer:
 * @bar: Bar canvas item
 * @id: String identifier to use for this pointer
 * @first_property_name: Name of first #SwamiguiBarPtr property to assign to or
 *   %NULL to not set any pointer properties.
 * @...: First value to assign to @first_property_name followed by additional
 *   name/value pairs, terminated with a %NULL name argument.
 *
 * Creates a new #SwamiguiBarPtr, sets properties and adds it to a bar canvas
 * item.
 */
void
swamigui_bar_create_pointer (SwamiguiBar *bar, const char *id,
			     const char *first_property_name, ...)
{
  SwamiguiBarPtr *barptr;
  va_list args;

  g_return_if_fail (SWAMIGUI_IS_BAR (bar));
  g_return_if_fail (id != NULL);

  barptr = g_object_new (SWAMIGUI_TYPE_BAR_PTR, NULL);

  va_start (args, first_property_name);
  g_object_set_valist (G_OBJECT (barptr), first_property_name, args);
  va_end (args);

  swamigui_bar_add_pointer (bar, barptr, id);
}

/**
 * swamigui_bar_add_pointer:
 * @bar: Bar canvas item
 * @barptr: Existing bar pointer to add to bar canvas item.
 * @id: String identifier to use for this pointer
 *
 * Add an existing bar pointer to a bar canvas item.
 */
void
swamigui_bar_add_pointer (SwamiguiBar *bar, SwamiguiBarPtr *barptr,
			  const char *id)
{
  PtrInfo *info;

  g_return_if_fail (SWAMIGUI_IS_BAR (bar));
  g_return_if_fail (SWAMIGUI_IS_BAR_PTR (barptr));
  g_return_if_fail (id != NULL);

  info = g_slice_new (PtrInfo);
  info->id = g_strdup (id);
  info->barptr = GNOME_CANVAS_ITEM (barptr);
  info->bar = bar;
  info->mouse_sel = FALSE;

  bar->ptrlist = g_list_append (bar->ptrlist, info);

  gnome_canvas_item_reparent (GNOME_CANVAS_ITEM (barptr),
			      GNOME_CANVAS_GROUP (bar));
  gnome_canvas_item_lower_to_bottom (GNOME_CANVAS_ITEM (barptr));

  g_signal_connect (G_OBJECT (barptr),
		    "event", G_CALLBACK (swamigui_bar_cb_ptr_event), info);

  swamigui_bar_update_ptr_heights (bar);
}

/**
 * @bar: Bar canvas item
 * @id: String identifier of pointer to find
 *
 * Get a bar pointer object in a bar canvas item identified by its string ID.
 *
 * Returns: The bar pointer object with the given string @id or %NULL if not
 *   found.
 */
GnomeCanvasItem *
swamigui_bar_get_pointer (SwamiguiBar *bar, const char *id)
{
  PtrInfo *info;
  GList *p;

  g_return_val_if_fail (SWAMIGUI_IS_BAR (bar), NULL);
  g_return_val_if_fail (id != NULL, NULL);

  for (p = bar->ptrlist; p; p = p->next)
    {
      info = (PtrInfo *)(p->data);
      if (strcmp (info->id, id) == 0) break;
    }

  return (p ? info->barptr : NULL);
}

/**
 * swamigui_bar_set_pointer_position:
 * @bar: Bar canvas item
 * @id: String identifier of pointer to set position of
 * @position: Position in pixels to set pointer to
 *
 * Set the position of a #SwamiguiBarPtr item in a bar canvas item.  Pointer
 * is centered on the given @position regardless of mode (position or range).
 */
void
swamigui_bar_set_pointer_position (SwamiguiBar *bar, const char *id,
				   int position)
{
  GnomeCanvasItem *barptr;
  double xform[6];
  int width;

  barptr = swamigui_bar_get_pointer (bar, id);
  g_return_if_fail (barptr != NULL);

  g_object_get (barptr, "width", &width, NULL);

  position -= width / 2;

  gnome_canvas_item_i2w_affine (barptr, xform);
  xform[4] = position;
  gnome_canvas_item_affine_absolute (barptr, xform);
}

/**
 * swamigui_bar_set_pointer_range:
 * @bar: Bar canvas item
 * @id: String identifier of pointer to set position of
 * @start: Position in pixels of start of range
 * @end: Position in pixels of end of range
 *
 * Set the range of a #SwamiguiBarPtr item in a bar canvas item.  Pointer is
 * set to the range defined by @start and @end.
 */
void
swamigui_bar_set_pointer_range (SwamiguiBar *bar, const char *id,
				int start, int end)
{
  GnomeCanvasItem *barptr;
  double xform[6];
  int temp;

  barptr = swamigui_bar_get_pointer (bar, id);
  g_return_if_fail (barptr != NULL);

  if (start > end)	/* swap if backwards */
    {
      temp = start;
      start = end;
      end = temp;
    }

  g_object_set (barptr, "width", end - start + 1, NULL);

  gnome_canvas_item_i2w_affine (barptr, xform);
  xform[4] = start;
  gnome_canvas_item_affine_absolute (barptr, xform);
}

/**
 * swamigui_bar_get_pointer_order:
 * @bar: Bar canvas item
 * @id: String identifier of pointer to get stacking order position of
 *
 * Get the stacking order of a pointer.
 *
 * Returns: Stacking order of pointer (0 = top, -1 if pointer not found).
 */
int
swamigui_bar_get_pointer_order (SwamiguiBar *bar, const char *id)
{
  PtrInfo *info;
  GList *p;
  int i;

  g_return_val_if_fail (SWAMIGUI_IS_BAR (bar), -1);
  g_return_val_if_fail (id != NULL, -1);

  for (p = bar->ptrlist, i = 0; p; p = p->next, i++)
    {
      info = (PtrInfo *)(p->data);
      if (strcmp (info->id, id) == 0) break;
    }

  return (p ? i : -1);  
}

/**
 * swamigui_bar_set_pointer_order:
 * @bar: Bar canvas item
 * @id: String identifier of pointer to set stacking order position of
 * @pos: Absolute position in stacking order (0 = top, -1 = bottom)
 *
 * Set the stacking order of a pointer to @pos.
 */
void
swamigui_bar_set_pointer_order (SwamiguiBar *bar, const char *id, int pos)
{
  PtrInfo *info;
  GList *p;
  int i;

  g_return_if_fail (SWAMIGUI_IS_BAR (bar));
  g_return_if_fail (id != NULL);

  for (p = bar->ptrlist, i = 0; p; p = p->next, i++)
    {
      info = (PtrInfo *)(p->data);
      if (strcmp (info->id, id) == 0) break;
    }

  g_return_if_fail (p != NULL);

  /* already at the requested position? */
  if (pos == i || (pos == -1 && !p->next))
    return;

  /* re-order the element */
  bar->ptrlist = g_list_delete_link (bar->ptrlist, p);
  bar->ptrlist = g_list_insert (bar->ptrlist, info, pos);

  /* set the stacking order of the canvas item */
  if (pos != -1)
    {	/* raise to top and then lower to the given position */
      gnome_canvas_item_raise_to_top (info->barptr);
      gnome_canvas_item_lower (info->barptr, pos);
    }
  else gnome_canvas_item_lower_to_bottom (info->barptr);

  swamigui_bar_update_ptr_heights (bar);  /* update the pointer heights */
}

/**
 * swamigui_bar_raise_pointer_to_top:
 * @bar: Bar canvas item
 * @id: String identifier of pointer to raise to top of stacking order
 *
 * Raise a pointer to the top of the stacking order.
 */
void
swamigui_bar_raise_pointer_to_top (SwamiguiBar *bar, const char *id)
{
  swamigui_bar_set_pointer_order (bar, id, 0);
}

/**
 * swamigui_bar_lower_pointer_to_bottom:
 * @bar: Bar canvas item
 * @id: String identifier of pointer to lower to bottom of stacking order
 *
 * Lower a pointer to the bottom of the stacking order.
 */
void
swamigui_bar_lower_pointer_to_bottom (SwamiguiBar *bar, const char *id)
{
  swamigui_bar_set_pointer_order (bar, id, -1);
}
