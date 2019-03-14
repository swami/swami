/*
 * SwamiguiCanvasMod.h - Zoom/Scroll canvas modulation object
 * Allows scroll/zoom of canvas as a user interface.
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
#ifndef __SWAMIGUI_CANVAS_MOD_H__
#define __SWAMIGUI_CANVAS_MOD_H__

#include <glib-object.h>
#include <gdk/gdk.h>

typedef struct _SwamiguiCanvasMod SwamiguiCanvasMod;
typedef struct _SwamiguiCanvasModClass SwamiguiCanvasModClass;

#define SWAMIGUI_TYPE_CANVAS_MOD   (swamigui_canvas_mod_get_type ())
#define SWAMIGUI_CANVAS_MOD(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_CANVAS_MOD, \
   SwamiguiCanvasMod))
#define SWAMIGUI_CANVAS_MOD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_CANVAS_MOD, \
   SwamiguiCanvasModClass))
#define SWAMIGUI_IS_CANVAS_MOD(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_CANVAS_MOD))
#define SWAMIGUI_IS_CANVAS_MOD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_CANVAS_MOD))

/* Canvas modulation type */
typedef enum
{
  SWAMIGUI_CANVAS_MOD_SNAP_ZOOM,	/* snap line zoom */
  SWAMIGUI_CANVAS_MOD_WHEEL_ZOOM,	/* wheel zoom */
  SWAMIGUI_CANVAS_MOD_SNAP_SCROLL,	/* snap line scroll */
  SWAMIGUI_CANVAS_MOD_WHEEL_SCROLL	/* wheel scroll */
} SwamiguiCanvasModType;

#define SWAMIGUI_CANVAS_MOD_TYPE_COUNT	4

/* axis enum */
typedef enum
{
  SWAMIGUI_CANVAS_MOD_X,
  SWAMIGUI_CANVAS_MOD_Y
} SwamiguiCanvasModAxis;

#define SWAMIGUI_CANVAS_MOD_AXIS_COUNT	2

typedef enum
{
  SWAMIGUI_CANVAS_MOD_ENABLED  = 1 << 0	/* TRUE if modulator is enabled */
} SwamiguiCanvasModFlags;

/* Modifier action flags */
typedef enum
{
  SWAMIGUI_CANVAS_MOD_ZOOM_X   = 1 << 0,
  SWAMIGUI_CANVAS_MOD_ZOOM_Y   = 1 << 1,
  SWAMIGUI_CANVAS_MOD_SCROLL_X = 1 << 2,
  SWAMIGUI_CANVAS_MOD_SCROLL_Y = 1 << 3
} SwamiguiCanvasModActions;


/* Variables used for zoom/scroll equation */
typedef struct
{
  double mult;
  double power;
  double ofs;
} SwamiguiCanvasModVars;

/* canvas zoom object */
struct _SwamiguiCanvasMod
{
  GObject parent_instance;

  guint zoom_modifier;		/* GdkModifierType for zooming */
  guint scroll_modifier;	/* GdkModifierType for scrolling */
  guint axis_modifier;		/* GdkModifierType to toggle axis */

  guint8 snap_button;		/* snap mouse button # (1-5) */
  guint8 def_action_zoom;	/* TRUE: Zoom = def, FALSE: Scroll = def */
  guint8 def_zoom_axis;		/* default SwamiguiCanvasModAxis for zoom */
  guint8 def_scroll_axis;	/* default SwamiguiCanvasModAxis for scroll */

  guint32 one_wheel_time;	/* time in msecs for first mouse wheel event */
  double min_zoom;		/* minimum zoom value */
  double max_zoom;		/* maximum zoom value */
  double min_scroll;		/* minimum scroll value */
  double max_scroll;		/* maximum scroll value */

  /* variables for [modulator][axis] scroll/zoom equations */
  SwamiguiCanvasModVars vars[SWAMIGUI_CANVAS_MOD_AXIS_COUNT][SWAMIGUI_CANVAS_MOD_TYPE_COUNT];

  guint timeout_handler;	/* timeout callback handler ID */
  guint timeout_interval;	/* timeout handler interval in msecs */
  guint wheel_timeout;		/* inactive time in msecs of wheel till stop */

  /* for mouse wheel zooming */
  GdkScrollDirection last_wheel_dir; /* last wheel direction (UP/DOWN/INACTIVE) */
  guint32 last_wheel_time;	/* last event time stamp of mouse wheel */
  guint32 wheel_time;		/* current wheel time value (between events) */
  int xwheel;			/* X coordinate of last wheel event */
  int ywheel;			/* Y coordinate of last wheel event */

  /* also stores the last wheel time, but in clock time since it doesn't appear
   * we can arbitrarily load the #&^@ing GDK event time in the timeout callback */
  GTimeVal last_wheel_real_time;

  gboolean snap_active;		/* TRUE if snap is active */
  int xsnap;			/* X coordinate of snap */
  int ysnap;			/* Y coordinate of snap */
  int cur_xsnap;		/* current X coordinate */
  int cur_ysnap;		/* current Y coordinate */
  gboolean cur_xchange;		/* X coordinate has changed? (needs refresh) */
  gboolean cur_ychange;		/* Y coordinate has changed? (needs refresh) */

  double xzoom_amt;		/* current X zoom amount */
  double yzoom_amt;		/* current Y zoom amount */
  double xscroll_amt;		/* current X scroll amount */
  double yscroll_amt;		/* current Y scroll amount */
};

struct _SwamiguiCanvasModClass
{
  GObjectClass parent_class;

  /* zoom/scroll update signal (zoom = 1.0 or scroll = 0.0 for no change) */
  void (* update)(SwamiguiCanvasMod *mod, double xzoom, double yzoom,
		  double xscroll, double yscroll, double xpos, double ypos);
  void (* snap)(SwamiguiCanvasMod *mod, guint actions, double xsnap, double ysnap);
};


GType swamigui_canvas_mod_get_type (void);
SwamiguiCanvasMod *swamigui_canvas_mod_new (void);
void swamigui_canvas_mod_set_vars (SwamiguiCanvasMod *mod,
				   SwamiguiCanvasModAxis axis,
				   SwamiguiCanvasModType type, double mult,
				   double power, double ofs);
void swamigui_canvas_mod_get_vars (SwamiguiCanvasMod *mod,
				   SwamiguiCanvasModAxis axis,
				   SwamiguiCanvasModType type, double *mult,
				   double *power, double *ofs);
gboolean swamigui_canvas_mod_handle_event (SwamiguiCanvasMod *mod,
					   GdkEvent *event);

#endif
