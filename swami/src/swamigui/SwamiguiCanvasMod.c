/*
 * SwamiguiCanvasMod.c - Zoom/Scroll canvas modulation object
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
#include <glib.h>
#include <glib-object.h>
#include <string.h>
#include <math.h>
#include "SwamiguiCanvasMod.h"
#include "marshals.h"

enum
{
  PROP_0,
  PROP_TIMEOUT_INTERVAL,	/* timeout interval in msecs */
  PROP_ABS_MIN_ZOOM,		/* absolute minimum zoom value */
  PROP_SNAP
};

/* Equation used for zoom/scroll operations:
 *
 * val = CLAMP (mult * pow (inp, power) + ofs, min, max)
 *
 * val: Value to use as zoom multiplier or scroll offset amount (zoom multiplier
 *   per second or scroll pixels per second)
 * mult: A linear multiplier factor
 * inp: Input source (event time interval for wheel, pixel distance from snap)
 * power: An exponential factor (1.0 = linear)
 * ofs: Offset amount
 *
 * swamigui_canvas_mod_handle_event() is called with canvas events.  This
 * function checks for events related to zoom/scroll operations (middle
 * mouse click, mouse wheel, etc).  A timeout is installed which sends
 * periodic updates for the zoom or scroll values using the "update" signal.
 * The "snap" signal is used for updating the X and/or Y snap lines, which
 * the user of SwamiguiCanvasZoom is responsible for.
 *
 *
 * Equation used for zoom multiplier to convert it to the timeout interval
 * from zoom value per second.  This is done so that any changes to timeout
 * interval only change the number of visual updates per second, but doesn't
 * affect the transformation speed.
 *
 * frac_mult = pow (mult, interval)
 *
 * frac_mult: Fractional zoom multiplier to use for each timeout callback
 * mult: The zoom multiplier for 1 second interval (as calculated from above)
 * interval: The callback interval time in seconds
 *
 *
 * State machine of SwamiguiCanvasMod:
 *
 * snap_active: TRUE indicates that a zoom and/or scroll snap is in progress.
 * last_wheel_dir: When GDK_SCROLL_UP or GDK_SCROLL_DOWN then wheel zoom and/or
 *                 scroll is active
 *
 * When snap_active == TRUE the following variables are valid:
 * xsnap/ysnap: Original coordinates of snap (when mouse button was clicked)
 * cur_xsnap/cur_ysnap: Current coordinates of the mouse
 * cur_xchange/cur_ychange: Indicate if cur_xsnap/cur_ysnap have changed since
 *                          last timeout
 *
 *
 */

#define TIMEOUT_PRIORITY  (G_PRIORITY_HIGH_IDLE + 40)

#define DEFAULT_ZOOM_MODIFIER		GDK_CONTROL_MASK
#define DEFAULT_SCROLL_MODIFIER		GDK_SHIFT_MASK
#define DEFAULT_AXIS_MODIFIER		GDK_MOD1_MASK
#define DEFAULT_SNAP_BUTTON		2	/* snap mouse button */
#define DEFAULT_ACTION_ZOOM		TRUE
#define DEFAULT_ZOOM_DEF_AXIS		SWAMIGUI_CANVAS_MOD_X
#define DEFAULT_SCROLL_DEF_AXIS		SWAMIGUI_CANVAS_MOD_X

#define DEFAULT_ONE_WHEEL_TIME		250	/* initial wheel event 'inp' val */
#define DEFAULT_MIN_ZOOM		1.0000001	/* minimum zoom/sec */
#define DEFAULT_MAX_ZOOM		1000000000.0	/* maximum zoom/sec */
#define DEFAULT_MIN_SCROLL		1.0	/* minimim scroll/sec */
#define DEFAULT_MAX_SCROLL		100000.0	/* maximum scroll/sec */

#define DEFAULT_TIMEOUT_INTERVAL	20	/* timeout interval in msecs */
#define DEFAULT_WHEEL_TIMEOUT		250	/* wheel taper timeout in msecs */


static const SwamiguiCanvasModVars
  default_vars[SWAMIGUI_CANVAS_MOD_AXIS_COUNT][SWAMIGUI_CANVAS_MOD_TYPE_COUNT] =
{ /* { mult, power, ofs } */
  { /* { X } */
    { 0.5, 4.0, 1.0 },		/* MOD_SNAP_ZOOM */
    { 1.0, 2.2, 5.0 },		/* MOD_WHEEL_ZOOM */
    { 10.0, 1.8, 200.0 },	/* MOD_SNAP_SCROLL */
    { 0.6, 1.6, 400.0 }		/* MOD_WHEEL_SCROLL */
  },
  { /* { Y } */
    { 0.5, 4.0, 1.0 },		/* MOD_SNAP_ZOOM */
    { 1.0, 2.2, 5.0 },		/* MOD_WHEEL_ZOOM */
    { 10.0, 1.8, 200.0 },	/* MOD_SNAP_SCROLL */
    { 0.6, 1.6, 400.0 }		/* MOD_WHEEL_SCROLL */
  }
};

enum
{
  UPDATE_SIGNAL,
  SNAP_SIGNAL,
  SIGNAL_COUNT
};


/* an undefined scroll direction for last_wheel_dir field */
#define WHEEL_INACTIVE 0xFF

static guint swamigui_canvas_mod_get_actions (SwamiguiCanvasMod *mod,
					      guint state);
static gboolean swamigui_canvas_mod_timeout (gpointer data);


G_DEFINE_TYPE (SwamiguiCanvasMod, swamigui_canvas_mod, G_TYPE_OBJECT);

static guint signals[SIGNAL_COUNT] = { 0 };


static void
swamigui_canvas_mod_class_init (SwamiguiCanvasModClass *klass)
{
  signals[UPDATE_SIGNAL] =
    g_signal_new ("update", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (SwamiguiCanvasModClass, update), NULL, NULL,
                  swamigui_marshal_VOID__DOUBLE_DOUBLE_DOUBLE_DOUBLE_DOUBLE_DOUBLE,
		  G_TYPE_NONE,
                  6, G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE,
		  G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  signals[SNAP_SIGNAL] =
    g_signal_new ("snap", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (SwamiguiCanvasModClass, snap), NULL, NULL,
                  swamigui_marshal_VOID__UINT_DOUBLE_DOUBLE, G_TYPE_NONE,
                  3, G_TYPE_UINT, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
}

static void
swamigui_canvas_mod_init (SwamiguiCanvasMod *mod)
{
  mod->zoom_modifier = DEFAULT_ZOOM_MODIFIER;
  mod->scroll_modifier = DEFAULT_SCROLL_MODIFIER;
  mod->axis_modifier = DEFAULT_AXIS_MODIFIER;
  mod->snap_button = DEFAULT_SNAP_BUTTON;
  mod->def_action_zoom = DEFAULT_ACTION_ZOOM;
  mod->def_zoom_axis = DEFAULT_ZOOM_DEF_AXIS;
  mod->def_scroll_axis = DEFAULT_SCROLL_DEF_AXIS;

  mod->one_wheel_time = DEFAULT_ONE_WHEEL_TIME;
  mod->min_zoom = DEFAULT_MIN_ZOOM;
  mod->max_zoom = DEFAULT_MAX_ZOOM;
  mod->min_scroll = DEFAULT_MIN_SCROLL;
  mod->max_scroll = DEFAULT_MAX_SCROLL;

  mod->timeout_interval = DEFAULT_TIMEOUT_INTERVAL;
  mod->wheel_timeout = DEFAULT_WHEEL_TIMEOUT;
  mod->last_wheel_dir = WHEEL_INACTIVE;

  memcpy (mod->vars, default_vars, sizeof (SwamiguiCanvasModVars)
	  * SWAMIGUI_CANVAS_MOD_AXIS_COUNT * SWAMIGUI_CANVAS_MOD_TYPE_COUNT);
}

/**
 * swamigui_canvas_mod_new:
 *
 * Create a new canvas zoom/scroll modulator object.  This object is used for
 * handling canvas events and intercepting those which apply to zooming or
 * scrolling operations by the user.
 *
 * Returns: New canvas modulator object with a refcount of 1 which the caller
 *   owns.
 */
SwamiguiCanvasMod *
swamigui_canvas_mod_new (void)
{
  return (SWAMIGUI_CANVAS_MOD (g_object_new (SWAMIGUI_TYPE_CANVAS_MOD, NULL)));
}

/**
 * swamigui_canvas_mod_set_vars:
 * @mod: Canvas zoom/scroll modulator
 * @axis: Axis of variables to assign (X or Y)
 * @type: Modulator type to assign to (snap zoom, snap scroll, wheel zoom or
 *   wheel scroll)
 * @mult: Multiplier value to assign to equation
 * @power: Power value to assign to equation
 * @ofs: Offset value to assign to equation
 *
 * Assigns equation variables for a specific modulator and axis.
 */
void
swamigui_canvas_mod_set_vars (SwamiguiCanvasMod *mod, SwamiguiCanvasModAxis axis,
			      SwamiguiCanvasModType type, double mult,
			      double power, double ofs)
{
  g_return_if_fail (SWAMIGUI_IS_CANVAS_MOD (mod));
  g_return_if_fail (axis >= 0 && axis < SWAMIGUI_CANVAS_MOD_AXIS_COUNT);
  g_return_if_fail (type >= 0 && type < SWAMIGUI_CANVAS_MOD_TYPE_COUNT);

  mod->vars[axis][type].mult = mult;
  mod->vars[axis][type].power = power;
  mod->vars[axis][type].ofs = ofs;
}

/**
 * swamigui_canvas_mod_get_vars:
 * @mod: Canvas zoom/scroll modulator
 * @axis: Axis of variables to get (X or Y)
 * @type: Modulator type to get vars from (snap zoom, snap scroll, wheel zoom or
 *   wheel scroll)
 * @mult: Location to store multiplier value of equation or %NULL
 * @power: Location to store power value of equation or %NULL
 * @ofs: Location to store offset value of equation or %NULL
 *
 * Gets equation variables for a specific modulator and axis.
 */
void
swamigui_canvas_mod_get_vars (SwamiguiCanvasMod *mod, SwamiguiCanvasModAxis axis,
			      SwamiguiCanvasModType type, double *mult,
			      double *power, double *ofs)
{
  g_return_if_fail (SWAMIGUI_IS_CANVAS_MOD (mod));
  g_return_if_fail (axis >= 0 && axis < SWAMIGUI_CANVAS_MOD_AXIS_COUNT);
  g_return_if_fail (type >= 0 && type < SWAMIGUI_CANVAS_MOD_TYPE_COUNT);

  if (mult) *mult = mod->vars[axis][type].mult;
  if (power) *power = mod->vars[axis][type].power;
  if (ofs) *ofs = mod->vars[axis][type].ofs;
}

/**
 * swamigui_canvas_mod_handle_event:
 * @mod: Canvas zoom/scroll modulator instance
 * @event: Canvas event to handle
 *
 * Processes canvas events and handles events related to zoom/scroll
 * actions.
 *
 * Returns: %TRUE if event was a zoom/scroll related event (was handled),
 *   %FALSE otherwise
 */
gboolean
swamigui_canvas_mod_handle_event (SwamiguiCanvasMod *mod, GdkEvent *event)
{
  GdkEventMotion *motion_event;
  GdkEventButton *btn_event;
  GdkEventScroll *scroll_event;
  guint actions;

  switch (event->type)
  {
  case GDK_BUTTON_PRESS:	/* button press only applies to snap */
    btn_event = (GdkEventButton *)event;

    if (btn_event->button != mod->snap_button) return (FALSE);

    mod->snap_active = TRUE;

    mod->xsnap = btn_event->x;
    mod->cur_xsnap = btn_event->x;
    mod->cur_xchange = TRUE;

    mod->ysnap = btn_event->y;
    mod->cur_ysnap = btn_event->y;
    mod->cur_ychange = TRUE;

    /* add a timeout callback for zoom/scroll if not already added */
    if (!mod->timeout_handler)
    {
      mod->timeout_handler
	= g_timeout_add_full (TIMEOUT_PRIORITY,
			      mod->timeout_interval,
			      swamigui_canvas_mod_timeout,
			      mod, NULL);
    }

    actions = swamigui_canvas_mod_get_actions (mod, btn_event->state);
    g_signal_emit (mod, signals[SNAP_SIGNAL], 0, actions, (double)(mod->xsnap),
		   (double)(mod->ysnap));
    break;

  case GDK_MOTION_NOTIFY:	/* motion applies only to snap */
    if (!mod->snap_active) return (FALSE);

    motion_event = (GdkEventMotion *)event;

    if (mod->cur_xsnap != motion_event->x)
    {
      mod->cur_xsnap = motion_event->x;
      mod->cur_xchange = TRUE;
    }

    if (mod->cur_ysnap != motion_event->y)
    {
      mod->cur_ysnap = motion_event->y;
      mod->cur_ychange = TRUE;
    }
    break;

  case GDK_BUTTON_RELEASE:	/* button release only applies to snap */
    if (!mod->snap_active
	|| ((GdkEventButton *)event)->button != mod->snap_button)
      return (FALSE);

    mod->snap_active = FALSE;

    /* remove timeout if wheel not active */
    if (mod->last_wheel_dir == WHEEL_INACTIVE && mod->timeout_handler)
    {
      g_source_remove (mod->timeout_handler);
      mod->timeout_handler = 0;
    }

    g_signal_emit (mod, signals[SNAP_SIGNAL], 0, 0, mod->xsnap, mod->ysnap);
    break;

  case GDK_SCROLL:		/* mouse wheel zooming */
    scroll_event = (GdkEventScroll *)event;

    if (scroll_event->direction != GDK_SCROLL_UP
	&& scroll_event->direction != GDK_SCROLL_DOWN)
      return (FALSE);

    /* wheel was previously scrolled in the other direction? - Stop immediately */
    if (mod->last_wheel_dir != WHEEL_INACTIVE
	&& mod->last_wheel_dir != scroll_event->direction)
    {	/* remove timeout handler if snap is not also active */
      if (!mod->snap_active && mod->timeout_handler)
      {
	g_source_remove (mod->timeout_handler);
	mod->timeout_handler = 0;
      }

      mod->last_wheel_dir = WHEEL_INACTIVE;

      return (TRUE);
    }

    /* wheel not yet scrolled? */
    if (mod->last_wheel_dir == WHEEL_INACTIVE)
    {
      mod->last_wheel_dir = scroll_event->direction;
      mod->wheel_time = mod->one_wheel_time;
      mod->xwheel = scroll_event->x;
      mod->ywheel = scroll_event->y;

      if (!mod->timeout_handler)
      {
	mod->timeout_handler
	  = g_timeout_add_full (TIMEOUT_PRIORITY,
				mod->timeout_interval,
				swamigui_canvas_mod_timeout,
				mod, NULL);
      }
    }	/* wheel previously scrolled in the same direction */
    else mod->wheel_time = scroll_event->time - mod->last_wheel_time;

    /* get current time for wheel timeout tapering (can't use GDK event time
     * since there doesn't seem to be a way to arbitrarily get current value) */
    g_get_current_time (&mod->last_wheel_real_time);

    mod->last_wheel_time = scroll_event->time;
    break;

  default:
    return (FALSE);
  }

  return (TRUE);
}

static guint
swamigui_canvas_mod_get_actions (SwamiguiCanvasMod *mod, guint state)
{
  gboolean zoom_mod, scroll_mod, axis_mod;
  guint actions = 0;

  zoom_mod = (state & mod->zoom_modifier) != 0;
  scroll_mod = (state & mod->scroll_modifier) != 0;
  axis_mod = (state & mod->axis_modifier) != 0;

  /* if ZOOM and SCROLL modifier not active - use default action */
  if (!zoom_mod && !scroll_mod)
  {
    if (mod->def_action_zoom) zoom_mod = TRUE;
    else scroll_mod = TRUE;
  }

  if (zoom_mod) actions |= 1 << (mod->def_zoom_axis ^ axis_mod);
  if (scroll_mod) actions |= 1 << ((mod->def_scroll_axis ^ axis_mod) + 2);

  return (actions);
}

/* equation calculation for the zoom/scroll operation */
static double
calc_val (SwamiguiCanvasMod *mod, double inp, SwamiguiCanvasModAxis axis,
	  SwamiguiCanvasModType type)
{
  SwamiguiCanvasModVars *vars;
  double val;

  vars = &mod->vars[axis][type];
  val = vars->mult * pow (inp, vars->power) + vars->ofs;

  return (val);
}

/* timeout handler which is called at regular intervals to update active
 * zoom and or scroll */
static gboolean
swamigui_canvas_mod_timeout (gpointer data)
{
  SwamiguiCanvasMod *mod = SWAMIGUI_CANVAS_MOD (data);
  GdkModifierType state;
  GTimeVal curtime;
  int lastwheel;	/* last wheel event time in milliseconds */
  double wheeltaper;	/* taper value for wheel timeout */
  double val, inp, inpy;
  double xpos, ypos;
  guint actions;

  /* get current keyboard modifier state and resulting actions */
  gdk_display_get_pointer (gdk_display_get_default (), NULL, NULL, NULL, &state);
  actions = swamigui_canvas_mod_get_actions (mod, state);

  mod->xzoom_amt = 1.0;
  mod->yzoom_amt = 1.0;
  mod->xscroll_amt = 0.0;
  mod->yscroll_amt = 0.0;

  /* mouse wheel operation is active */
  if (mod->last_wheel_dir != WHEEL_INACTIVE)
  {
    inp = mod->wheel_timeout - mod->wheel_time;
    inp = CLAMP (inp, 0.0, mod->wheel_timeout);

    /* calculate the last wheel event time in milliseconds, unfortunately I
     * don't think we can get the current GDK event time, so we have to use
     * g_get_current_time(). */
    g_get_current_time (&curtime);

    lastwheel = (curtime.tv_sec - mod->last_wheel_real_time.tv_sec) * 1000;

    if (curtime.tv_usec > mod->last_wheel_real_time.tv_usec)
      lastwheel += (curtime.tv_usec
		    - mod->last_wheel_real_time.tv_usec + 500) / 1000;
    else lastwheel -= (mod->last_wheel_real_time.tv_usec
		       - curtime.tv_usec + 500) / 1000;

    /* At end of wheel activity timeout? */
    if (lastwheel >= mod->wheel_timeout)
    {
      mod->last_wheel_dir = WHEEL_INACTIVE;

      if (mod->snap_active) return (TRUE);	/* keep timeout if snap active */

      mod->timeout_handler = 0;
      return (FALSE);
    }

    /* wheel timeout taper multiplier (1.0 to 0.0) */
    wheeltaper = 1.0 - (mod->wheel_timeout - lastwheel)
      / (double)(mod->wheel_timeout);

    if (actions & SWAMIGUI_CANVAS_MOD_ZOOM_X)
    {
      val = calc_val (mod, inp, SWAMIGUI_CANVAS_MOD_X,
		      SWAMIGUI_CANVAS_MOD_WHEEL_ZOOM);
      mod->xzoom_amt = 1.0 + (val - 1.0) * wheeltaper;
    }

    if (actions & SWAMIGUI_CANVAS_MOD_ZOOM_Y)
    {
      val = calc_val (mod, inp, SWAMIGUI_CANVAS_MOD_Y,
		      SWAMIGUI_CANVAS_MOD_WHEEL_ZOOM);
      mod->yzoom_amt = 1.0 + (val - 1.0) * wheeltaper;
    }

    if (actions & SWAMIGUI_CANVAS_MOD_SCROLL_X)
    {
      val = calc_val (mod, inp, SWAMIGUI_CANVAS_MOD_X,
		      SWAMIGUI_CANVAS_MOD_WHEEL_SCROLL);
      mod->xscroll_amt = val * wheeltaper;
    }

    if (actions & SWAMIGUI_CANVAS_MOD_SCROLL_Y)
    {
      val = calc_val (mod, inp, SWAMIGUI_CANVAS_MOD_Y,
		      SWAMIGUI_CANVAS_MOD_WHEEL_SCROLL);
      mod->yscroll_amt = val * wheeltaper;
    }

    /* set "direction" of values depending on wheel direction */
    if (mod->last_wheel_dir == GDK_SCROLL_DOWN)
    {
      mod->xzoom_amt = 1.0 / mod->xzoom_amt;
      mod->yzoom_amt = 1.0 / mod->yzoom_amt;
      mod->xscroll_amt = -mod->xscroll_amt;
      /* y scroll is already inverted */
    }
    else mod->yscroll_amt = -mod->yscroll_amt;	/* yscroll is inverted */

    xpos = mod->xwheel;
    ypos = mod->ywheel;
  }
  else		/* snap is active */
  {
    inp = ABS (mod->cur_xsnap - mod->xsnap);
    inpy = ABS (mod->cur_ysnap - mod->ysnap);

    /* short circuit 0 case (but keep timeout) */
    if (inp == 0 && inpy == 0) return (TRUE);

    if (actions & SWAMIGUI_CANVAS_MOD_ZOOM_X)
      mod->xzoom_amt = calc_val (mod, inp, SWAMIGUI_CANVAS_MOD_X,
				 SWAMIGUI_CANVAS_MOD_SNAP_ZOOM);
    if (actions & SWAMIGUI_CANVAS_MOD_ZOOM_Y)
      mod->yzoom_amt = calc_val (mod, inpy, SWAMIGUI_CANVAS_MOD_Y,
				 SWAMIGUI_CANVAS_MOD_SNAP_ZOOM);
    if (actions & SWAMIGUI_CANVAS_MOD_SCROLL_X)
      mod->xscroll_amt = calc_val (mod, inp, SWAMIGUI_CANVAS_MOD_X,
				   SWAMIGUI_CANVAS_MOD_SNAP_SCROLL);
    if (actions & SWAMIGUI_CANVAS_MOD_SCROLL_Y)
      mod->yscroll_amt = calc_val (mod, inpy, SWAMIGUI_CANVAS_MOD_Y,
				   SWAMIGUI_CANVAS_MOD_SNAP_SCROLL);

    /* set "direction" of values depending on snap  */
    if (mod->cur_xsnap < mod->xsnap)
    {
      mod->xzoom_amt = 1.0 / mod->xzoom_amt;
      mod->yzoom_amt = 1.0 / mod->yzoom_amt;
      mod->xscroll_amt = -mod->xscroll_amt;
      /* y scroll is already inverted */
    }
    else mod->yscroll_amt = -mod->yscroll_amt;	/* yscroll is inverted */

    xpos = mod->xsnap;
    ypos = mod->ysnap;
  }

  if (mod->xzoom_amt != 1.0 || mod->yzoom_amt != 1.0
      || mod->xscroll_amt != 0.0 || mod->yscroll_amt != 0.0)
  {
    double interval = mod->timeout_interval / 1000.0;

    /* factor in interval so that zoom/scroll amounts are the same rate
     * regardless of timeout interval */
    mod->xzoom_amt = pow (mod->xzoom_amt, interval);
    mod->yzoom_amt = pow (mod->yzoom_amt, interval);
    mod->xscroll_amt *= interval;
    mod->yscroll_amt *= interval;

/*
    printf ("Update: xzoom:%f yzoom:%f xscroll:%f yscroll:%f xpos:%f ypos:%f\n",
	    mod->xzoom_amt, mod->yzoom_amt, mod->xscroll_amt, mod->yscroll_amt,
	    xpos, ypos);
*/

    g_signal_emit (mod, signals[UPDATE_SIGNAL], 0, mod->xzoom_amt,
		   mod->yzoom_amt, mod->xscroll_amt, mod->yscroll_amt,
		   xpos, ypos);
  }

  /* keep timeout if wheel is active or snap is active */
  if (mod->last_wheel_dir != WHEEL_INACTIVE || mod->snap_active)
    return (TRUE);

  mod->timeout_handler = 0;
  return (FALSE);
}
