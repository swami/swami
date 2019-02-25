/*
 * SwamiguiPiano.c - Piano canvas item
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
#include <math.h>
#include <gtk/gtk.h>

#include <libswami/libswami.h>

#include "SwamiguiPiano.h"
#include "SwamiguiRoot.h"
#include "SwamiguiStatusbar.h"
#include "i18n.h"
#include "util.h"

/* Piano keys: C C# D D# E F F# G G# A A# B */

enum
{
  PROP_0,
  PROP_WIDTH_PIXELS,            /* width in pixels */
  PROP_HEIGHT_PIXELS,           /* height in pixels */
  PROP_KEY_COUNT,               /* number of keys in piano */
  PROP_START_OCTAVE,            /* piano start octave (first key) */
  PROP_LOWER_OCTAVE,            /* lower keyboard octave */
  PROP_UPPER_OCTAVE,            /* upper keyboard octave */
  PROP_LOWER_VELOCITY,          /* lower keyboard velocity */
  PROP_UPPER_VELOCITY,          /* upper keyboard velocity */
  PROP_MIDI_CONTROL,            /* Piano MIDI control */
  PROP_EXPRESS_CONTROL,         /* Piano expression control */
  PROP_MIDI_CHANNEL,            /* MIDI channel to send events on */
  PROP_BG_COLOR,                /* color of border and in between white keys */
  PROP_WHITE_KEY_COLOR,         /* color of white keys */
  PROP_BLACK_KEY_COLOR,         /* color of black keys */
  PROP_SHADOW_EDGE_COLOR,       /* color of white key shadow edge */
  PROP_WHITE_KEY_PLAY_COLOR,    /* color of white key play highlight color */
  PROP_BLACK_KEY_PLAY_COLOR     /* color of black key play highlight color */
};

enum
{
  NOTE_ON,
  NOTE_OFF,
  LAST_SIGNAL
};

#define DEFAULT_BG_COLOR                SWAMIGUI_RGB (0, 0, 0)
#define DEFAULT_WHITE_KEY_COLOR         SWAMIGUI_RGB (255, 255, 255)
#define DEFAULT_BLACK_KEY_COLOR         SWAMIGUI_RGB (0, 0, 0)
#define DEFAULT_SHADOW_EDGE_COLOR       SWAMIGUI_RGB (128, 128, 128)
#define DEFAULT_WHITE_KEY_PLAY_COLOR    SWAMIGUI_RGB (169, 127, 255)
#define DEFAULT_BLACK_KEY_PLAY_COLOR    SWAMIGUI_RGB (169, 127, 255)

/* piano vertical line width to white key width scale */
#define PIANO_VLINE_TO_KEY_SCALE  (1.0/5.0)

/* piano horizontal line width to piano height scale */
#define PIANO_HLINE_TO_HEIGHT_SCALE (1.0/48.0)

/* piano black key height to piano height */
#define PIANO_BLACK_TO_HEIGHT_SCALE (26.0/48.0)

/* piano white key grey edge to piano height scale */
#define PIANO_GREY_TO_HEIGHT_SCALE (2.0/48.0)

/* piano white key indicator width to key scale */
#define PIANO_WHITE_INDICATOR_WIDTH_SCALE (4.0/8.0)

/* piano white key indicator height to piano height scale */
#define PIANO_WHITE_INDICATOR_HEIGHT_SCALE (2.0/48.0)

/* piano white key velocity indicator range to height scale */
#define PIANO_WHITE_INDICATOR_RANGE_SCALE (18.0/48.0)

/* piano white key velocity offset to height scale */
#define PIANO_WHITE_INDICATOR_OFS_SCALE (28.0/48.0)


/* piano black key indicator width to key scale */
#define PIANO_BLACK_INDICATOR_WIDTH_SCALE (3.0/5.0)

/* piano black key indicator height to piano height scale */
#define PIANO_BLACK_INDICATOR_HEIGHT_SCALE (3.0/48.0)

/* piano black key velocity indicator range scale */
#define PIANO_BLACK_INDICATOR_RANGE_SCALE (22.0/28.0)

/* piano black key velocity offset scale */
#define PIANO_BLACK_INDICATOR_OFS_SCALE (2.0/28.0)

/* piano active black key shorten scale (to look pressed down) */
#define PIANO_BLACK_SHORTEN_SCALE (1.0/26.0)


static void swamigui_piano_set_property (GObject *object, guint property_id,
                                        const GValue *value,
                                        GParamSpec *pspec);
static void swamigui_piano_get_property (GObject *object, guint property_id,
                                        GValue *value, GParamSpec *pspec);
static void swamigui_piano_finalize (GObject *object);
static void swamigui_piano_midi_ctrl_callback (SwamiControl *control,
                                              SwamiControlEvent *event,
                                              const GValue *value);

static gboolean swamigui_piano_draw (GtkWidget *widget, cairo_t *cr);
static gboolean swamigui_piano_event (GtkWidget *widget, GdkEvent *event);
static void swamigui_piano_update_mouse_note (SwamiguiPiano *piano,
                                             double x, double y);
static gboolean swamigui_piano_update_note (SwamiguiPiano *piano, int note,
                                            int velocity);

static GObjectClass *parent_class = NULL;
static guint piano_signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE (SwamiguiPiano, swamigui_piano, GTK_TYPE_DRAWING_AREA);

static void
swamigui_piano_class_init (SwamiguiPianoClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widg_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  obj_class->set_property = swamigui_piano_set_property;
  obj_class->get_property = swamigui_piano_get_property;
  obj_class->finalize = swamigui_piano_finalize;

  widg_class->draw = swamigui_piano_draw;
  widg_class->event = swamigui_piano_event;

  piano_signals[NOTE_ON] =
    g_signal_new ("note-on", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (SwamiguiPianoClass, note_on), NULL, NULL,
                  g_cclosure_marshal_VOID__UINT,
                  G_TYPE_NONE, 1, G_TYPE_UINT);

  piano_signals[NOTE_OFF] =
    g_signal_new ("note-off", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (SwamiguiPianoClass, note_off), NULL, NULL,
                  g_cclosure_marshal_VOID__UINT,
                  G_TYPE_NONE, 1, G_TYPE_UINT);

  g_object_class_install_property (obj_class, PROP_WIDTH_PIXELS,
                        g_param_spec_int ("width-pixels", _("Pixel Width"),
                                          _("Width in pixels"),
                                          1, G_MAXINT,
                                          SWAMIGUI_PIANO_DEFAULT_WIDTH,
                                          G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_HEIGHT_PIXELS,
                        g_param_spec_int ("height-pixels", _("Pixel Height"),
                                          _("Height in pixels"),
                                          1, G_MAXINT,
                                          SWAMIGUI_PIANO_DEFAULT_HEIGHT,
                                          G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_KEY_COUNT,
                        g_param_spec_int ("key-count", _("Key Count"),
                                          _("Size of piano in keys"),
                                          1, 128, 128,
                                          G_PARAM_READWRITE
                                          | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (obj_class, PROP_START_OCTAVE,
                g_param_spec_int ("start-octave", _("Start Octave"),
                                  _("Piano start octave (0 = MIDI note 0"),
                                  0, 10, 0, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_LOWER_OCTAVE,
                g_param_spec_int ("lower-octave", _("Lower Octave"),
                                  _("Lower keyboard start octave"),
                                  0, 10, SWAMIGUI_PIANO_DEFAULT_LOWER_OCTAVE,
                                  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_UPPER_OCTAVE,
                g_param_spec_int ("upper-octave", _("Upper Octave"),
                                  _("Upper keyboard start octave"),
                                  0, 10, SWAMIGUI_PIANO_DEFAULT_UPPER_OCTAVE,
                                  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_LOWER_VELOCITY,
                g_param_spec_int ("lower-velocity", _("Lower Velocity"),
                                  _("Lower keyboard velocity"),
                                  0, 127, 127,
                                  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_UPPER_VELOCITY,
                g_param_spec_int ("upper-velocity", _("Upper Velocity"),
                                  _("Upper keyboard velocity"),
                                  0, 127, 127,
                                  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_MIDI_CONTROL,
                g_param_spec_object ("midi-control", _("MIDI Control"),
                                     _("Piano MIDI control"),
                                     SWAMI_TYPE_CONTROL,
                                     G_PARAM_READABLE));
  g_object_class_install_property (obj_class, PROP_EXPRESS_CONTROL,
        g_param_spec_object ("expression-control",
                             _("Expression Control"),
                        _("Piano expression control (vertical mouse axis)"),
                             SWAMI_TYPE_CONTROL,
                             G_PARAM_READABLE));
  g_object_class_install_property (obj_class, PROP_MIDI_CHANNEL,
                g_param_spec_int ("midi-channel", _("MIDI Channel"),
                                  _("MIDI channel to send events on"),
                                  0, 15, 0, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_BG_COLOR,
    ipatch_param_set (g_param_spec_uint ("bg-color", _("Background color"),
                                         _("Color of border and between white keys"),
                                         0, G_MAXUINT, DEFAULT_BG_COLOR,
                                         G_PARAM_READWRITE),
                      "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
  g_object_class_install_property (obj_class, PROP_WHITE_KEY_COLOR,
    ipatch_param_set (g_param_spec_uint ("white-key-color", _("White key color"),
                                         _("White key color"),
                                         0, G_MAXUINT, DEFAULT_WHITE_KEY_COLOR,
                                         G_PARAM_READWRITE),
                      "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
  g_object_class_install_property (obj_class, PROP_BLACK_KEY_COLOR,
    ipatch_param_set (g_param_spec_uint ("black-key-color", _("Black key color"),
                                         _("Black key color"),
                                         0, G_MAXUINT, DEFAULT_BLACK_KEY_COLOR,
                                         G_PARAM_READWRITE),
                      "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
  g_object_class_install_property (obj_class, PROP_SHADOW_EDGE_COLOR,
    ipatch_param_set (g_param_spec_uint ("shadow-edge-color", _("Shadow edge color"),
                                         _("Bottom shadow edge color"),
                                         0, G_MAXUINT, DEFAULT_SHADOW_EDGE_COLOR,
                                         G_PARAM_READWRITE),
                      "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
  g_object_class_install_property (obj_class, PROP_WHITE_KEY_PLAY_COLOR,
    ipatch_param_set (g_param_spec_uint ("white-key-play-color",
                                         _("White key player color"),
                                         _("Color of white key play highlight"),
                                         0, G_MAXUINT, DEFAULT_WHITE_KEY_PLAY_COLOR,
                                         G_PARAM_READWRITE),
                      "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
  g_object_class_install_property (obj_class, PROP_BLACK_KEY_PLAY_COLOR,
    ipatch_param_set (g_param_spec_uint ("black-key-play-color",
                                         _("Black key play color"),
                                         _("Color of black key play highlight"),
                                         0, G_MAXUINT, DEFAULT_BLACK_KEY_PLAY_COLOR,
                                         G_PARAM_READWRITE),
                      "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
}

static void
swamigui_piano_set_property (GObject *object, guint property_id,
                            const GValue *value, GParamSpec *pspec)
{
  SwamiguiPiano *piano = SWAMIGUI_PIANO (object);

  switch (property_id)
  {
    case PROP_WIDTH_PIXELS:
      piano->width = g_value_get_int (value);
      piano->up2date = FALSE;
      gtk_widget_queue_draw (GTK_WIDGET (piano));
      break;
    case PROP_HEIGHT_PIXELS:
      piano->height = g_value_get_int (value);
      piano->up2date = FALSE;
      gtk_widget_queue_draw (GTK_WIDGET (piano));
      break;
    case PROP_KEY_COUNT:
      piano->key_count = g_value_get_int (value);
      g_free (piano->key_velocity);
      piano->key_velocity = g_malloc0 (piano->key_count);
      gtk_widget_queue_draw (GTK_WIDGET (piano));
      break;
    case PROP_START_OCTAVE:
      piano->start_note = g_value_get_int (value) * 12;
      break;
    case PROP_LOWER_OCTAVE:
      piano->lower_octave = g_value_get_int (value);
      break;
    case PROP_UPPER_OCTAVE:
      piano->upper_octave = g_value_get_int (value);
      break;
    case PROP_LOWER_VELOCITY:
      piano->lower_velocity = g_value_get_int (value);
      break;
    case PROP_UPPER_VELOCITY:
      piano->upper_velocity = g_value_get_int (value);
      break;
    case PROP_MIDI_CHANNEL:
      piano->midi_chan = g_value_get_int (value);
      break;
    case PROP_BG_COLOR:
      piano->bg_color = g_value_get_uint (value);
      gtk_widget_queue_draw (GTK_WIDGET (piano));
      break;
    case PROP_WHITE_KEY_COLOR:
      piano->white_key_color = g_value_get_uint (value);
      gtk_widget_queue_draw (GTK_WIDGET (piano));
      break;
    case PROP_BLACK_KEY_COLOR:
      piano->black_key_color = g_value_get_uint (value);
      gtk_widget_queue_draw (GTK_WIDGET (piano));
      break;
    case PROP_SHADOW_EDGE_COLOR:
      piano->shadow_edge_color = g_value_get_uint (value);
      gtk_widget_queue_draw (GTK_WIDGET (piano));
      break;
    case PROP_WHITE_KEY_PLAY_COLOR:
      piano->white_key_play_color = g_value_get_uint (value);
      gtk_widget_queue_draw (GTK_WIDGET (piano));
      break;
    case PROP_BLACK_KEY_PLAY_COLOR:
      piano->black_key_play_color = g_value_get_uint (value);
      gtk_widget_queue_draw (GTK_WIDGET (piano));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
swamigui_piano_get_property (GObject *object, guint property_id,
                            GValue *value, GParamSpec *pspec)
{
  SwamiguiPiano *piano = SWAMIGUI_PIANO (object);

  switch (property_id)
  {
    case PROP_WIDTH_PIXELS:
      g_value_set_int (value, piano->width);
      break;
    case PROP_HEIGHT_PIXELS:
      g_value_set_int (value, piano->height);
      break;
    case PROP_KEY_COUNT:
      g_value_set_int (value, piano->key_count);
      break;
    case PROP_START_OCTAVE:
      g_value_set_int (value, piano->start_note / 12);
      break;
    case PROP_LOWER_OCTAVE:
      g_value_set_int (value, piano->lower_octave);
      break;
    case PROP_UPPER_OCTAVE:
      g_value_set_int (value, piano->upper_octave);
      break;
    case PROP_LOWER_VELOCITY:
      g_value_set_int (value, piano->lower_velocity);
      break;
    case PROP_UPPER_VELOCITY:
      g_value_set_int (value, piano->upper_velocity);
      break;
    case PROP_MIDI_CONTROL:
      g_value_set_object (value, piano->midi_ctrl);
      break;
    case PROP_EXPRESS_CONTROL:
      g_value_set_object (value, piano->express_ctrl);
      break;
    case PROP_MIDI_CHANNEL:
      g_value_set_int (value, piano->midi_chan);
      break;
    case PROP_BG_COLOR:
      g_value_set_uint (value, piano->bg_color);
      break;
    case PROP_WHITE_KEY_COLOR:
      g_value_set_uint (value, piano->white_key_color);
      break;
    case PROP_BLACK_KEY_COLOR:
      g_value_set_uint (value, piano->black_key_color);
      break;
    case PROP_SHADOW_EDGE_COLOR:
      g_value_set_uint (value, piano->shadow_edge_color);
      break;
    case PROP_WHITE_KEY_PLAY_COLOR:
      g_value_set_uint (value, piano->white_key_play_color);
      break;
    case PROP_BLACK_KEY_PLAY_COLOR:
      g_value_set_uint (value, piano->black_key_play_color);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
swamigui_piano_init (SwamiguiPiano *piano)
{
  int i;

  /* create MIDI control */
  piano->midi_ctrl = SWAMI_CONTROL (swami_control_midi_new ());
  swami_control_set_queue (piano->midi_ctrl, swamigui_root->ctrl_queue);
  swami_control_midi_set_callback (SWAMI_CONTROL_MIDI (piano->midi_ctrl),
                                   swamigui_piano_midi_ctrl_callback, piano);

  /* create expression control */
  piano->express_ctrl = SWAMI_CONTROL (swami_control_value_new ());
  swami_control_set_queue (piano->express_ctrl, swamigui_root->ctrl_queue);
  swami_control_set_spec (piano->express_ctrl,
                          g_param_spec_int ("expression", "Expression",
                                            "Expression",
                                            0, 127, 0,
                                            G_PARAM_READWRITE));
  swami_control_value_alloc_value (SWAMI_CONTROL_VALUE (piano->express_ctrl));

  piano->velocity = 127;        /* default velocity */
  piano->key_count = 128;
  piano->key_velocity = g_malloc0 (piano->key_count);

  piano->width = SWAMIGUI_PIANO_DEFAULT_WIDTH;
  piano->height = SWAMIGUI_PIANO_DEFAULT_HEIGHT;
  piano->start_note = 0;
  piano->lower_octave = SWAMIGUI_PIANO_DEFAULT_LOWER_OCTAVE;
  piano->upper_octave = SWAMIGUI_PIANO_DEFAULT_UPPER_OCTAVE;
  piano->lower_velocity = 127;
  piano->upper_velocity = 127;
  piano->mouse_note = 128;        /* disabled mouse key (> 127) */

  piano->bg_color = DEFAULT_BG_COLOR;
  piano->white_key_color = DEFAULT_WHITE_KEY_COLOR;
  piano->black_key_color = DEFAULT_BLACK_KEY_COLOR;
  piano->shadow_edge_color = DEFAULT_SHADOW_EDGE_COLOR;
  piano->white_key_play_color = DEFAULT_WHITE_KEY_PLAY_COLOR;
  piano->black_key_play_color = DEFAULT_BLACK_KEY_PLAY_COLOR;

  /* get the white key count for the number of keys in the piano */
  piano->white_count = piano->key_count / 12 * 7; /* 7 whites per oct */
  i = piano->key_count % 12; /* calculate for octave remainder (if any) */
  if (i < 6) piano->white_count += (i + 1) / 2;
  else piano->white_count += (i | 0x1) / 2 + 1;

  gtk_widget_add_events (GTK_WIDGET (piano), GDK_BUTTON_PRESS_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK);
}

static void
swamigui_piano_finalize (GObject *object)
{
  SwamiguiPiano *piano = SWAMIGUI_PIANO (object);

  g_free (piano->key_velocity);

  g_object_unref (piano->midi_ctrl); /* -- unref MIDI control */
  g_object_unref (piano->express_ctrl);        /* -- unref expression control */

  if (G_OBJECT_CLASS (parent_class)->finalize)
    (* G_OBJECT_CLASS (parent_class)->finalize)(object);
}

/* SwamiControlFunc set value func for MIDI control */
static void
swamigui_piano_midi_ctrl_callback (SwamiControl *control,
                                  SwamiControlEvent *event,
                                  const GValue *value)
{
  SwamiguiPiano *piano = SWAMIGUI_PIANO (SWAMI_CONTROL_FUNC(control)->user_data);
  GArray *valarray = NULL;
  SwamiMidiEvent *midi;
  int i, count = 1;                /* default for single values */

  /* if its multiple values, fetch the value array */
  if (G_VALUE_TYPE (value) == G_TYPE_ARRAY)
  {
    valarray = g_value_get_boxed (value);
    count = valarray->len;
  }

  for (i = 0; i < count; i++)
  {
    if (valarray) value = &g_array_index (valarray, GValue, i);

    if (G_VALUE_TYPE (value) == SWAMI_TYPE_MIDI_EVENT
        && (midi = g_value_get_boxed (value)) && midi->channel == piano->midi_chan)
    {
      if (midi->type == SWAMI_MIDI_NOTE_ON)
        swamigui_piano_update_note (piano, midi->data.note.note, midi->data.note.velocity);
      else if (midi->type == SWAMI_MIDI_NOTE_OFF)
        swamigui_piano_update_note (piano, midi->data.note.note, 0);
    }
  }
}

static gboolean
swamigui_piano_draw (GtkWidget *widget, cairo_t *cr)
{
  SwamiguiPiano *piano = SWAMIGUI_PIANO (widget);
  gboolean isblack;
  double x, x1, w, h, v;
  int vlineh1, vlineh2;
  int i;

  if (!piano->up2date)
  {
    piano->key_width = (double)piano->width / piano->key_count;
    piano->key_width_half = piano->key_width / 2;

    piano->vline_width = (int)(piano->key_width * PIANO_VLINE_TO_KEY_SCALE + 0.5);
    piano->hline_width = (int)(piano->height * PIANO_HLINE_TO_HEIGHT_SCALE + 0.5);

    /* black key dimensions */
    piano->black_height = (double)piano->height * PIANO_BLACK_TO_HEIGHT_SCALE;

    /* height of grey edge */
    piano->shadow_height = piano->height * PIANO_GREY_TO_HEIGHT_SCALE;

    /* black key velocity range and offset cache values */
    piano->black_vel_ofs = piano->black_height * PIANO_BLACK_INDICATOR_OFS_SCALE;
    piano->black_vel_range = piano->black_height * PIANO_BLACK_INDICATOR_RANGE_SCALE;

    /* white key velocity range and offset cache values */
    piano->white_vel_ofs = (double)piano->height * PIANO_WHITE_INDICATOR_OFS_SCALE;
    piano->white_vel_range = (double)piano->height * PIANO_WHITE_INDICATOR_RANGE_SCALE;

    piano->up2date = TRUE;
  }

  cairo_rectangle (cr, 0, 0, piano->width, piano->height);
  swamigui_util_set_cairo_rgba (cr, piano->bg_color);
  cairo_fill (cr);

  /* calculate halves of vline_width */
  vlineh1 = (int)(piano->vline_width + 0.5);
  vlineh2 = vlineh1 / 2;
  vlineh1 -= vlineh2;

  for (i = 0, isblack = FALSE; i < piano->key_count; )
  {
    x = i * (double)piano->width / piano->key_count;

    /* Draw white keys before black keys that preceed them so that black keys overlap */
    if (!isblack || i < piano->key_count - 1)
    {
      if (isblack) i++;

      if (i % 12 == 0 || i % 12 == 5) x1 = x + vlineh1;
      else x1 = x - piano->key_width_half + vlineh1;

      if (i % 12 == 4 || i % 12 == 11 || i == piano->key_count - 1)
        w = piano->key_width - (vlineh1 + vlineh2);
      else w = piano->key_width + piano->key_width_half - (vlineh1 + vlineh2);

      h = piano->height - piano->hline_width * 2.0;
      v = h * piano->key_velocity[i] / 127.0;

      if (piano->key_velocity[i] == 0)
        h -= piano->shadow_height;
      else h -= v;

      /* Draw key velocity if note is on */
      if (piano->key_velocity[i] > 0)
      {
        swamigui_util_set_cairo_rgba (cr, piano->white_key_play_color);
        cairo_rectangle (cr, x1, piano->hline_width, w, v);
        cairo_fill (cr);
      }

      /* Draw white key */
      if (piano->key_velocity[i] < 127)
      {
        swamigui_util_set_cairo_rgba (cr, piano->white_key_color);
        cairo_rectangle (cr, x1, piano->hline_width + v, w, h);
        cairo_fill (cr);
      }

      /* Draw white key shadow if note is off */
      if (piano->key_velocity[i] == 0)
      {
        swamigui_util_set_cairo_rgba (cr, piano->shadow_edge_color);
        cairo_rectangle (cr, x1, piano->height - piano->hline_width - piano->shadow_height, w, piano->shadow_height);
        cairo_fill (cr);
      }

      if (isblack) i--;
      else i++;
    }

    if (isblack)
    {
      if (piano->key_velocity[i] > 0)          /* Note is on? - Draw black key velocity */
      {
        h = piano->black_height - piano->black_height * PIANO_BLACK_SHORTEN_SCALE;
        v = h * piano->key_velocity[i] / 127.0;

        swamigui_util_set_cairo_rgba (cr, piano->black_key_play_color);
        cairo_rectangle (cr, x, piano->hline_width, piano->key_width, v);
        cairo_fill (cr);
      }
      else
      {
        h = piano->black_height;
        v = 0.0;
      }

      if (piano->key_velocity[i] < 127)
      {
        swamigui_util_set_cairo_rgba (cr, piano->black_key_color);
        cairo_rectangle (cr, x, piano->hline_width + v, piano->key_width, h - v);
        cairo_fill (cr);
      }

      i += 2;
    }

    isblack = (i & 1) ^ (i % 12 >= 5);
  }

  return TRUE;  // Stop additional hanlders
}

static gboolean
swamigui_piano_event (GtkWidget *widget, GdkEvent *event)
{
  SwamiguiPiano *piano = SWAMIGUI_PIANO (widget);
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  int note, velocity;

  switch (event->type)
  {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *)event;
      if (bevent->button != 1) break;
      note = swamigui_piano_pos_to_note (piano, bevent->x, bevent->y, &velocity, NULL);
      if (note < 0) break;        /* click not on a note? */
      piano->mouse_note = note;
      swamigui_piano_note_on (piano, note, velocity);
      return (TRUE);
    case GDK_BUTTON_RELEASE:
      if (piano->mouse_note > 127) break; /* no mouse selected note? */
      note = piano->mouse_note;
      piano->mouse_note = 128;        /* no more selected mouse note */
      swamigui_piano_note_off (piano, note, 127);
      break;
    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *)event;
      swamigui_piano_update_mouse_note (piano, mevent->x, mevent->y);
      break;
    case GDK_LEAVE_NOTIFY:
      swamigui_statusbar_msg_set_label (swamigui_root->statusbar, 0, "Global", NULL);
      break;
    default:
      break;
  }

  return (FALSE);
}

static void
swamigui_piano_update_mouse_note (SwamiguiPiano *piano, double x, double y)
{
  int note, velocity = 127;
  char midiNote[5];
  char *statusmsg;
  int *velp = NULL;

  if (x < 0.0) x = 0.0;
  else if (x > piano->width) x = piano->width;
  else velp = &velocity;

  if (y < 0.0 || y > piano->height)
  {
    y = 0.0;
    velp = NULL;
  }

  note = swamigui_piano_pos_to_note (piano, x, y, velp, NULL);
  note = CLAMP (note, 0, 127);        /* force clamp note */

  swami_util_midi_note_to_str (note, midiNote);
  statusmsg = g_strdup_printf (_("Note: %-3s (%d) Velocity: %d"), midiNote, note, velocity);
  swamigui_statusbar_msg_set_label (swamigui_root->statusbar, 0, "Global", statusmsg);
  g_free (statusmsg);

  if (piano->mouse_note > 127) return;        /* if mouse note play not active, we done */

  if (note != piano->mouse_note) /* note changed? */
  {
    swamigui_piano_note_off (piano, piano->mouse_note, 127);    /* note off */
    piano->mouse_note = note;
    swamigui_piano_note_on (piano, note, velocity);     /* new note on */
  }
  else swamigui_piano_update_note (piano, note, velocity);      /* same note, update indicator for velocity */
}

/* Internal function for updating a note on or off (velocity=0) without sending a MIDI event */
static gboolean
swamigui_piano_update_note (SwamiguiPiano *piano, int note, int velocity)
{
  double x, width;

  g_return_val_if_fail (SWAMIGUI_IS_PIANO (piano), FALSE);
  g_return_val_if_fail (note >= 0 && note <= 127, FALSE);
  g_return_val_if_fail (velocity >= 0 && velocity <= 127, FALSE);

  if (note < piano->start_note || note >= piano->start_note + piano->key_count)
    return (FALSE);

  piano->key_velocity[note - piano->start_note] = velocity;

  x = swamigui_piano_note_to_pos (piano, note, -1, TRUE, NULL, &width);
  gtk_widget_queue_draw_area (GTK_WIDGET (piano), floor (x), 0, ceil (width) + (x - floor (x)), piano->height);

  return (TRUE);
}

/**
 * swamigui_piano_note_on:
 * @piano: Piano object
 * @note: MIDI note number to turn on
 * @velocity: MIDI velocity number (-1 to use current keyboard velocity)
 *
 * Piano note on. Turns on note visually on piano as well as sends an event to
 * #SwamiControl objects connected to piano MIDI event control.
 */
void
swamigui_piano_note_on (SwamiguiPiano *piano, int note, int velocity)
{
  if (velocity == -1) velocity = piano->velocity;
  if (velocity == 0) swamigui_piano_note_off (piano, note, velocity);

  if (swamigui_piano_update_note (piano, note, velocity))
    swami_control_midi_transmit (SWAMI_CONTROL_MIDI (piano->midi_ctrl), SWAMI_MIDI_NOTE_ON,
                                 piano->midi_chan, note, velocity);
}

/**
 * swamigui_piano_note_off:
 * @piano: Piano object
 * @note: MIDI note number to turn off
 * @velocity: MIDI note off velocity
 *
 * Piano note off. Turns off note visually on piano as well as sends an event
 * to it's connected MIDI driver (if any).
 */
void
swamigui_piano_note_off (SwamiguiPiano *piano, int note, int velocity)
{
  if (swamigui_piano_update_note (piano, note, 0))
    swami_control_midi_transmit (SWAMI_CONTROL_MIDI (piano->midi_ctrl), SWAMI_MIDI_NOTE_OFF,
                                 piano->midi_chan, note, velocity);
}

/**
 * swamigui_piano_pos_to_note:
 * @piano: Piano object
 * @x: X coordinate in canvas world units
 * @y: Y coordinate in canvas world units
 * @velocity: Location to store velocity or %NULL
 * @isblack: Location to store a boolean if key is black/white, or %NULL
 *
 * Get the MIDI note corresponding to a point on the piano. The velocity
 * relates to the vertical axis of the note. Positions towards the tip
 * of the key generate higher velocities.
 *
 * Returns: The MIDI note number or -1 if not a valid key.
 */
int
swamigui_piano_pos_to_note (SwamiguiPiano *piano, double x, double y,
                            int *velocity, gboolean *isblack)
{
  gboolean black;
  gdouble keyofs;
  int note, mod;

  g_return_val_if_fail (SWAMIGUI_IS_PIANO (piano), -1);

  if (x < 0.0 || x > piano->width || y < 0.0 || y > piano->height)
    return (-1);

  /* calculate note */
  note = x / piano->key_width;

  if (note >= piano->key_count) note = piano->key_count - 1;
  mod = note % 12;
  black = (note & 1) ^ (mod >= 5);

  if (black && y > piano->black_height)        /* select white key, if below black keys */
  {
    keyofs = x - (note * piano->key_width);

    if (keyofs >= piano->key_width_half)
    {
      note++;
      if (note >= piano->key_count) note = piano->key_count - 1;
    }
    else note--;

    black = FALSE;
  }

  if (velocity)
  {
    if (black)
    {
      if (y < piano->black_vel_ofs) *velocity = 1;
      else if (y > piano->black_vel_ofs + piano->black_vel_range)
        *velocity = 127;
      else *velocity = 1.0 + (y - piano->black_vel_ofs) / (piano->black_vel_range) * 126.0 + 0.5;
    }
    else                        /* white key */
    {
      if (y < piano->white_vel_ofs) *velocity = 1;
      else if (y > piano->white_vel_ofs + piano->white_vel_range)
        *velocity = 127;
      else *velocity = 1.0 + (y - piano->white_vel_ofs) / (piano->white_vel_range) * 126.0 + 0.5;
    }
  }

  if (isblack) *isblack = black;

  return (piano->start_note + note);
}

/**
 * swamigui_piano_note_to_pos (SwamiguiPiano *piano, int note, gboolean isblack)
 * @piano: Piano object
 * @note: MIDI note number to convert to canvas position
 * @edge: Which edge to get position of (-1 = left, 0 = center, 1 = right)
 * @realnote: If %TRUE then the coordinate is adjusted for the actual drawn key
 *   rather than the active area (equal for all keys), when %FALSE is specified.
 * @isblack: (out) (optional): Pointer to store a gboolean which is set if key is black, or %NULL
 * @width: (out) (optional): Pointer to store width of key
 *
 * Find the canvas X coordinate for a given MIDI note.
 *
 * Returns: X canvas coordinate of @note for the specified @edge.
 */
double
swamigui_piano_note_to_pos (SwamiguiPiano *piano, int note, int edge,
                            gboolean realnote, gboolean *isblack, double *width)
{
  double pos;
  gboolean black;
  int noteofs;
  int mod, vlineh1, vlineh2;

  g_return_val_if_fail (SWAMIGUI_IS_PIANO (piano), 0.0);
  g_return_val_if_fail (note >= piano->start_note, 0.0);

  noteofs = note - piano->start_note;
  g_return_val_if_fail (noteofs < piano->key_count, 0.0);

  pos = noteofs * piano->width / piano->key_count;

  mod = note % 12;
  black = (note & 1) ^ (mod >= 5);

  if (!realnote || black)        /* active area request or black key? */
  { /* active area is equal for all keys and black keys are equal to active area */
    if (edge == 0) pos += piano->key_width_half;
    else if (edge == 1) pos += piano->key_width;

    if (width) *width = piano->key_width;
  }
  else                /* white key */
  { /* calculate halves of vline_width */
    vlineh1 = (int)(piano->vline_width + 0.5);
    vlineh2 = vlineh1 / 2;
    vlineh1 -= vlineh2;

    if (edge == -1)
    {
      pos += vlineh1;

      if (mod != 0 && mod != 5)
        pos -= piano->key_width_half;
    }
    else if (edge == 0)
    {
      if (mod == 0 || mod == 5)
        pos += (piano->key_width + piano->key_width_half) / 2.0;
      else if (mod == 4 || mod == 11)
        pos += piano->key_width_half / 2.0;
      else pos += piano->key_width / 2.0;
    }
    else
    {
      pos -= vlineh2;

      if (mod == 0 || mod == 5)
        pos += piano->key_width + piano->key_width_half;
      else if (mod == 4 || mod == 11)
        pos += piano->key_width;
      else pos += piano->key_width + piano->key_width_half;
    }

    if (width)
    {
      if (mod == 0 || mod == 5 || mod == 4 || mod == 11)
        *width = piano->key_width + piano->key_width_half - (vlineh1 + vlineh2);
      else *width = piano->key_width * 2.0 - (vlineh1 + vlineh2);
    }
  }

  if (isblack) *isblack = black;

  return (pos);
}

