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
#include <string.h>
#include <gtk/gtk.h>

#include "SwamiguiBar.h"
#include "builtin_enums.h"
#include "marshals.h"

/* Number of pixels of movement required to activate a pointer move */
#define SWAMIGUI_BAR_MOVE_TOLERANCE   2

enum
{
  PROP_0,
  PROP_HEIGHT,		  /* Height of bar */
  PROP_OVERLAP_POS,       /* Position of overlaps (top or bottom) */
  PROP_OVERLAP_HEIGHT     /* Overlap height in pixels */
};

/* Signals */
enum
{
  POINTER_CHANGED,
  SIGNAL_COUNT
};

/* structure which defines an interface pointer */
typedef struct
{
  char *id;			/* id of this item */
  SwamiguiBarPtr *barptr;	/* pointer item */
  int start;			/* X position of left side of range or position of pointer */
  int end;			/* X position of right side of range (range only) */
  SwamiguiBar *bar;		/* parent bar object */
  gboolean mouse_sel;		/* TRUE if mouse selection is in progress */
} PtrInfo;

static void swamigui_bar_set_property (GObject *object, guint property_id,
					const GValue *value,
					GParamSpec *pspec);
static void swamigui_bar_get_property (GObject *object, guint property_id,
					GValue *value, GParamSpec *pspec);
static void swamigui_bar_finalize (GObject *object);
static gboolean swamigui_bar_draw (GtkWidget *widget, cairo_t *cr);
static PtrInfo *swamigui_bar_get_pointer_at (SwamiguiBar *bar, int xpos, int ypos);
static gboolean swamigui_bar_event (GtkWidget *widget, GdkEvent  *event);

static PtrInfo *swamigui_bar_get_pointer_info (SwamiguiBar *bar, const char *id);


static guint bar_signals[SIGNAL_COUNT];

G_DEFINE_TYPE (SwamiguiBar, swamigui_bar, GTK_TYPE_DRAWING_AREA);


static void
swamigui_bar_class_init (SwamiguiBarClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widg_class = GTK_WIDGET_CLASS (klass);

  obj_class->set_property = swamigui_bar_set_property;
  obj_class->get_property = swamigui_bar_get_property;
  obj_class->finalize = swamigui_bar_finalize;

  widg_class->draw = swamigui_bar_draw;
  widg_class->event = swamigui_bar_event;

  bar_signals[POINTER_CHANGED] =
    g_signal_new ("pointer-changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (SwamiguiBarClass, pointer_changed), NULL, NULL,
		  swamigui_marshal_VOID__STRING_INT_INT,
		  G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);

  g_object_class_install_property (obj_class, PROP_HEIGHT,
	  g_param_spec_int ("height", "Height", "Height",
			    0, G_MAXINT, 24, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_OVERLAP_POS,
	  g_param_spec_enum ("overlap-pos", "Overlap pos", "Overlap position",
			     SWAMIGUI_TYPE_BAR_OVERLAP_POS,
			     SWAMIGUI_BAR_OVERLAP_POS_TOP, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_OVERLAP_HEIGHT,
	  g_param_spec_int ("overlap-height", "Overlap height", "Overlap height",
			    0, G_MAXINT, 4, G_PARAM_READWRITE));
}

static void
swamigui_bar_set_property (GObject *object, guint property_id,
			    const GValue *value, GParamSpec *pspec)
{
  SwamiguiBar *bar = SWAMIGUI_BAR (object);

  switch (property_id)
  {
    case PROP_HEIGHT:
      bar->height = g_value_get_int (value);
      gtk_widget_queue_draw (GTK_WIDGET (bar));
      break;
    case PROP_OVERLAP_POS:
      bar->overlap_pos = g_value_get_enum (value);
      gtk_widget_queue_draw (GTK_WIDGET (bar));
      break;
    case PROP_OVERLAP_HEIGHT:
      bar->overlap_height = g_value_get_int (value);
      gtk_widget_queue_draw (GTK_WIDGET (bar));
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
    case PROP_HEIGHT:
      g_value_set_int (value, bar->height);
      break;
    case PROP_OVERLAP_POS:
      g_value_set_enum (value, bar->overlap_pos);
      break;
    case PROP_OVERLAP_HEIGHT:
      g_value_set_int (value, bar->overlap_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swamigui_bar_init (SwamiguiBar *bar)
{
  bar->height = 24;
  bar->overlap_pos = SWAMIGUI_BAR_OVERLAP_POS_TOP;
  bar->overlap_height = 4;
  bar->ptrlist = NULL;

  gtk_widget_set_events (GTK_WIDGET (bar), GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
                         | GDK_POINTER_MOTION_MASK);
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
    g_object_unref (info->barptr);
    g_free (info->id);
    g_slice_free (PtrInfo, p->data);
  }

  if (G_OBJECT_CLASS (swamigui_bar_parent_class)->finalize)
    (* G_OBJECT_CLASS (swamigui_bar_parent_class)->finalize)(object);
}

static gboolean
swamigui_bar_draw (GtkWidget *widget, cairo_t *cr)
{
  SwamiguiBar *bar = SWAMIGUI_BAR (widget);
  SwamiguiBarPtr *barptr;
  PtrInfo *info;
  int x, y, width, height, count, i;
  GList *p;

  count = g_list_length (bar->ptrlist);
  height = bar->height - (count - 1) * bar->overlap_height;

  // Draw backwards, since it is top first order
  for (p = g_list_last (bar->ptrlist), i = 0; p; p = p->prev, i++)
  {
    info = (PtrInfo *)(p->data);
    barptr = info->barptr;

    if (barptr->type == SWAMIGUI_BAR_PTR_RANGE)
    {
      x = info->start;

      if (bar->overlap_pos == SWAMIGUI_BAR_OVERLAP_POS_TOP)
        y = i * bar->overlap_height;
      else y = (count - i - 1) * bar->overlap_height;

      width = info->end - info->start + 1;

      gdk_cairo_set_source_rgba (cr, &barptr->color);
      cairo_rectangle (cr, x, y, width, height);
      cairo_fill (cr);
    }
    else        // FIXME
    {
    }
  }

  return TRUE;  // Stop additional handlers
}

/* Get pointer at a specific X/Y coordinate or NULL if none */
static PtrInfo *
swamigui_bar_get_pointer_at (SwamiguiBar *bar, int xpos, int ypos)
{
  SwamiguiBarPtr *barptr;
  PtrInfo *info;
  int y, height, count, i;
  GList *p;

  count = g_list_length (bar->ptrlist);
  height = bar->height - (count - 1) * bar->overlap_height;

  for (p = bar->ptrlist, i = 0; p; p = p->next, i++)
  {
    info = (PtrInfo *)(p->data);
    barptr = info->barptr;

    if (barptr->type == SWAMIGUI_BAR_PTR_RANGE)
    {
      if (bar->overlap_pos == SWAMIGUI_BAR_OVERLAP_POS_TOP)
        y = (count - i - 1) * bar->overlap_height;
      else y = i * bar->overlap_height;

      if ((xpos >= info->start && xpos <= info->end)
          && (ypos >= y && ypos < y + height))
        return info;
    }
    else        // FIXME
    {
    }
  }

  return NULL;
}

static gboolean
swamigui_bar_event (GtkWidget *widget, GdkEvent  *event)
{
  SwamiguiBar *bar = SWAMIGUI_BAR (widget);
  PtrInfo *ptrinfo;
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  int ptrtype;
  int xpos;
  int start;

  switch (event->type)
  {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *)event;
      if (bevent->button != 1 && bevent->button != 2) break;

      ptrinfo = swamigui_bar_get_pointer_at (bar, bevent->x, bevent->y);
      if (!ptrinfo) break;

      xpos = bevent->x;

      bar->move_pointer = ptrinfo;
      bar->move_toler_done = FALSE;
      bar->move_click_xpos = xpos;
      bar->move_click_xofs = xpos - ptrinfo->start;

      /* make sure the item is raised to the top */
      swamigui_bar_raise_pointer_to_top (bar, ptrinfo->id);

      if (bevent->button == 1)
      {
	g_object_get (ptrinfo->barptr, "type", &ptrtype, NULL);

	if (ptrtype == SWAMIGUI_BAR_PTR_RANGE)
	{
	  if (ABS (ptrinfo->start - xpos) <= ABS (ptrinfo->end - xpos))
	    bar->move_sel = -1;
	  else bar->move_sel = 1;
	}
      }
      else bar->move_sel = 0;

      return (TRUE);    // Stop event propagation
    case GDK_BUTTON_RELEASE:
      if (!bar->move_pointer) break; /* not selected? */

      bar->move_pointer = NULL;
      break;
    case GDK_MOTION_NOTIFY:
      if (!bar->move_pointer) break; /* not selected? */
      mevent = (GdkEventMotion *)event;

      ptrinfo = (PtrInfo *)(bar->move_pointer);

      xpos = mevent->x;
      if (xpos < 0) xpos = 0;

      /* Move tolerance not yet reached? */
      if (!bar->move_toler_done
	  && ABS (bar->move_click_xpos - xpos) < SWAMIGUI_BAR_MOVE_TOLERANCE)
	break;

      bar->move_toler_done = TRUE;

      g_object_get (ptrinfo->barptr, "type", &ptrtype, NULL);

      if (ptrtype == SWAMIGUI_BAR_PTR_RANGE)
      {
	if (bar->move_sel == -1)    /* Left range? */
	{
	  if (xpos > ptrinfo->end)
	  {
	    bar->move_sel = 1;
	    ptrinfo->start = ptrinfo->end;
	    ptrinfo->end = xpos;
	  }
	  else ptrinfo->start = xpos;
	}
	else if (bar->move_sel == 1)
	{
	  if (xpos < ptrinfo->start)
	  {
	    bar->move_sel = -1;
	    ptrinfo->end = ptrinfo->start;
	    ptrinfo->start = xpos;
	  }
	  else ptrinfo->end = xpos;
	}
	else
	{
	  start = xpos - bar->move_click_xofs;
	  if (start < 0) start = 0;

	  ptrinfo->end = start + (ptrinfo->end - ptrinfo->start);
	  ptrinfo->start = start;
	}

	swamigui_bar_set_pointer_range (bar, ptrinfo->id, ptrinfo->start,
					ptrinfo->end);
      }
      else swamigui_bar_set_pointer_position (bar, ptrinfo->id, xpos);

      break;
    default:
      break;
  }

  return FALSE;         // Let event propagate
}

/**
 * swamigui_bar_new:
 *
 * Create a new range/pointer bar widget.
 *
 * Returns: New bar widget with a refcount of 1 which the caller owns.
 */
GtkWidget *
swamigui_bar_new (void)
{
  return g_object_new (SWAMIGUI_TYPE_BAR, NULL);
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
  PtrInfo *info;
  va_list args;

  g_return_if_fail (SWAMIGUI_IS_BAR (bar));
  g_return_if_fail (id != NULL);

  barptr = swamigui_bar_ptr_new ();

  va_start (args, first_property_name);
  g_object_set_valist (G_OBJECT (barptr), first_property_name, args);
  va_end (args);

  info = g_slice_new0 (PtrInfo);
  info->id = g_strdup (id);
  info->barptr = barptr;
  info->bar = bar;
  info->mouse_sel = FALSE;

  bar->ptrlist = g_list_append (bar->ptrlist, info);

  gtk_widget_queue_draw (GTK_WIDGET (bar));
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
SwamiguiBarPtr *
swamigui_bar_get_pointer (SwamiguiBar *bar, const char *id)
{
  PtrInfo *info;

  g_return_val_if_fail (SWAMIGUI_IS_BAR (bar), NULL);
  g_return_val_if_fail (id != NULL, NULL);

  info = swamigui_bar_get_pointer_info (bar, id);

  return (info ? info->barptr : NULL);
}

/* Get pointer info for a given pointer ID */
static PtrInfo *
swamigui_bar_get_pointer_info (SwamiguiBar *bar, const char *id)
{
  GList *p;

  for (p = bar->ptrlist; p; p = p->next)
  {
    if (strcmp (((PtrInfo *)(p->data))->id, id) == 0)
      return ((PtrInfo *)(p->data));
  }

  return (NULL);
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
swamigui_bar_set_pointer_position (SwamiguiBar *bar, const char *id, int position)
{
  PtrInfo *ptrinfo;

  ptrinfo = swamigui_bar_get_pointer_info (bar, id);
  g_return_if_fail (ptrinfo != NULL);

  ptrinfo->start = position;

  gtk_widget_queue_draw (GTK_WIDGET (bar));
  g_signal_emit (bar, bar_signals[POINTER_CHANGED], 0, ptrinfo->id, position, position);
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
swamigui_bar_set_pointer_range (SwamiguiBar *bar, const char *id, int start, int end)
{
  PtrInfo *ptrinfo;
  int temp;

  ptrinfo = swamigui_bar_get_pointer_info (bar, id);
  g_return_if_fail (ptrinfo != NULL);

  if (start < 0) start = 0;
  if (end < 0) end = 0;

  if (start > end)	/* swap if backwards */
  {
    temp = start;
    start = end;
    end = temp;
  }

  ptrinfo->start = start;
  ptrinfo->end = end;

  gtk_widget_queue_draw (GTK_WIDGET (bar));
  g_signal_emit (bar, bar_signals[POINTER_CHANGED], 0, ptrinfo->id, start, end);
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

  gtk_widget_queue_draw (GTK_WIDGET (bar));
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

