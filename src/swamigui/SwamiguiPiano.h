/*
 * SwamiguiPiano.h - Piano canvas item
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
#ifndef __SWAMIGUI_PIANO_H__
#define __SWAMIGUI_PIANO_H__

#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include <libinstpatch/libinstpatch.h>

typedef struct _SwamiguiPiano SwamiguiPiano;
typedef struct _SwamiguiPianoClass SwamiguiPianoClass;

#define SWAMIGUI_TYPE_PIANO   (swamigui_piano_get_type ())
#define SWAMIGUI_PIANO(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_PIANO, SwamiguiPiano))
#define SWAMIGUI_PIANO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_PIANO, \
   SwamiguiPianoClass))
#define SWAMIGUI_IS_PIANO(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_PIANO))
#define SWAMIGUI_IS_PIANO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_PIANO))

#define SWAMIGUI_PIANO_DEFAULT_WIDTH  640 /* default width in pixels */
#define SWAMIGUI_PIANO_DEFAULT_HEIGHT 48	/* default hight in pixels */
#define SWAMIGUI_PIANO_DEFAULT_LOWER_OCTAVE 3 /* default lower keys octave */
#define SWAMIGUI_PIANO_DEFAULT_UPPER_OCTAVE 4 /* default upper keys octave */

/* Swami Piano Object */
struct _SwamiguiPiano
{
    GnomeCanvasGroup parent_instance; /* derived from GnomeCanvasGroup */

    SwamiControl *midi_ctrl;	/* MIDI control object */
    SwamiControl *express_ctrl;	/* expression control object */
    int midi_chan;		/* MIDI channel to send events on */
    int velocity;			/* default MIDI note on velocity to use */

    int width, height;		/* width and height in pixels */
    gpointer key_info; /* array of KeyInfo structures (see SwamiguiPiano.c) */
    GnomeCanvasItem *bg;		/* black background (outline/separators) */
    GnomeCanvasItem *median_c;  /* median C marker */
    guint8 key_count;		/* number of keys */
    guint8 start_note;		/* note piano starts on (always note C) */
    guint8 lower_octave;		/* lower computer keyboard octave # */
    guint8 upper_octave;		/* upper computer keyboard octave # */
    guint8 lower_velocity;	/* lower keyboard velocity */
    guint8 upper_velocity;	/* upper keyboard velocity */
    guint8 mouse_note;		/* mouse selected note >127 = none */

    /* cached values */
    int white_count;        /* number of white key  */
    gboolean up2date;		/* are variables below up to date? */
    double world_width, world_height;
    double shadow_top;

    double key_white_width, key_white_width_half;
    double black_width_half;
    int black_width_lh, black_width_rh;
    double black_height;
    double vline_width, hline_width;
    double black_vel_ofs, black_vel_range;
    double white_vel_ofs, white_vel_range;

    guint32 bg_color;
    guint32 white_key_color;
    guint32 black_key_color;
    guint32 shadow_edge_color;
    guint32 white_key_play_color;
    guint32 black_key_play_color;
};

struct _SwamiguiPianoClass
{
    GnomeCanvasGroupClass parent_class;

    void (*note_on)(SwamiguiPiano *piano, guint keynum);
    void (*note_off)(SwamiguiPiano *piano, guint keynum);
};

GType swamigui_piano_get_type(void);

void swamigui_piano_note_on(SwamiguiPiano *piano, int note, int velocity);
void swamigui_piano_note_off(SwamiguiPiano *piano, int note, int velocity);
int swamigui_piano_pos_to_note(SwamiguiPiano *piano, double x, double y,
                               int *velocity, gboolean *isblack);
double swamigui_piano_note_to_pos(SwamiguiPiano *piano, int note, int edge,
                                  gboolean realnote, gboolean *isblack);

#endif
