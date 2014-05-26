/*
 * libInstPatch
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1
 * of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA or on the web at http://www.gnu.org.
 */
#ifndef __IPATCH_DLS2_CONN_H__
#define __IPATCH_DLS2_CONN_H__

#include <glib.h>
#include <glib-object.h>


/* forward type declarations */

typedef struct _IpatchDLS2Conn IpatchDLS2Conn;
typedef struct _IpatchDLS2ConnInfo IpatchDLS2ConnInfo;

/* IpatchDLS2Conn has a GObject boxed type */
#define IPATCH_TYPE_DLS2_CONN   (ipatch_dls2_conn_get_type ())

/* DLS2 connection (to set parameter values and define modulators) */
struct _IpatchDLS2Conn
{
  /*< public >*/
  guint16 src;			/* source enum */
  guint16 ctrlsrc;		/* second source enum */
  guint16 dest;			/* destination enum */
  guint16 trans;		/* transform enum */
  gint32 scale;			/* scale value */
};

/* Compare two DLS connections to see if they are identical
   (source, control and dest are identical) */
#define IPATCH_DLS2_CONN_ARE_IDENTICAL(a, b) \
  ((a)->src == (b)->src && (a)->ctrlsrc == (b)->ctrlsrc \
   && (a)->dest == (b)->dest)

/* connection info and constraints structure */
struct _IpatchDLS2ConnInfo
{
  /*< public >*/
  guint16 type;			/* IpatchDLS2ConnDestType */
  gint32 min;			/* minimum value allowed */
  gint32 max;			/* maximum value allowed */
  gint32 def;			/* default value */
  int unit;			/* #IpatchUnitType type */
  char *label;			/* short descriptive label */
  char *descr;			/* more complete description */
};

/* source connection types */
typedef enum
{
  IPATCH_DLS2_CONN_SRC_NONE = 0, /* No source */
  IPATCH_DLS2_CONN_SRC_LFO = 1,	/* Modulation LFO */
  IPATCH_DLS2_CONN_SRC_VELOCITY = 2, /* MIDI Note-On velocity */
  IPATCH_DLS2_CONN_SRC_NOTE = 3, /* MIDI Note number */
  IPATCH_DLS2_CONN_SRC_EG1 = 4,	/* Envelope Generator 1 */
  IPATCH_DLS2_CONN_SRC_EG2 = 5,	/* Envelope Generator 2 */
  IPATCH_DLS2_CONN_SRC_PITCH_WHEEL = 6, /* Pitch Wheel */
  IPATCH_DLS2_CONN_SRC_POLY_PRESSURE = 7, /* Polyphonic pressure */
  IPATCH_DLS2_CONN_SRC_CHANNEL_PRESSURE = 8, /* Channel Pressure */
  IPATCH_DLS2_CONN_SRC_VIBRATO = 9, /* Vibrato LFO */

  /* defined MIDI controller sources */
  IPATCH_DLS2_CONN_SRC_CC1 = 0x0081, /* Modulation */
  IPATCH_DLS2_CONN_SRC_CC7 = 0x0087, /* Volume */
  IPATCH_DLS2_CONN_SRC_CC10 = 0x008A, /* Pan */
  IPATCH_DLS2_CONN_SRC_CC11 = 0x008B, /* Expression */
  IPATCH_DLS2_CONN_SRC_CC91 = 0x00DB, /* Chorus Send */
  IPATCH_DLS2_CONN_SRC_CC93 = 0x00DD, /* Reverb Send */

  /* MIDI registered parameter numbers */
  IPATCH_DLS2_CONN_SRC_RPN0 = 0x0100, /* Pitch bend range */
  IPATCH_DLS2_CONN_SRC_RPN1 = 0x0101, /* Fine tune */
  IPATCH_DLS2_CONN_SRC_RPN2 = 0x0102 /* Coarse tune */
} IpatchDLS2ConnSrcType;

/* destination connection types */
typedef enum
{
  IPATCH_DLS2_CONN_DEST_NONE = 0,
  IPATCH_DLS2_CONN_DEST_GAIN = 1,
  IPATCH_DLS2_CONN_DEST_RESERVED = 2,
  IPATCH_DLS2_CONN_DEST_PITCH = 3,
  IPATCH_DLS2_CONN_DEST_PAN = 4,
  IPATCH_DLS2_CONN_DEST_NOTE = 5,

  IPATCH_DLS2_CONN_DEST_LEFT = 0x0010,
  IPATCH_DLS2_CONN_DEST_RIGHT = 0x0011,
  IPATCH_DLS2_CONN_DEST_CENTER = 0x0012,
  IPATCH_DLS2_CONN_DEST_LFE_CHANNEL = 0x0013,
  IPATCH_DLS2_CONN_DEST_LEFT_REAR = 0x0014,
  IPATCH_DLS2_CONN_DEST_RIGHT_REAR = 0x0015,

  IPATCH_DLS2_CONN_DEST_CHORUS = 0x0080,
  IPATCH_DLS2_CONN_DEST_REVERB = 0x0081,

  IPATCH_DLS2_CONN_DEST_LFO_FREQ = 0x0104,
  IPATCH_DLS2_CONN_DEST_LFO_DELAY = 0x0105,

  IPATCH_DLS2_CONN_DEST_VIB_FREQ = 0x0114,
  IPATCH_DLS2_CONN_DEST_VIB_DELAY = 0x0115,

  IPATCH_DLS2_CONN_DEST_EG1_ATTACK = 0x0206,
  IPATCH_DLS2_CONN_DEST_EG1_DECAY = 0x0207,
  IPATCH_DLS2_CONN_DEST_EG1_RESERVED = 0x0208,
  IPATCH_DLS2_CONN_DEST_EG1_RELEASE = 0x0209,
  IPATCH_DLS2_CONN_DEST_EG1_SUSTAIN = 0x020A,
  IPATCH_DLS2_CONN_DEST_EG1_DELAY = 0x020B,
  IPATCH_DLS2_CONN_DEST_EG1_HOLD = 0x020C,
  IPATCH_DLS2_CONN_DEST_EG1_SHUTDOWN = 0x020D,

  IPATCH_DLS2_CONN_DEST_EG2_ATTACK = 0x030A,
  IPATCH_DLS2_CONN_DEST_EG2_DECAY = 0x030B,
  IPATCH_DLS2_CONN_DEST_EG2_RESERVED = 0x030C,
  IPATCH_DLS2_CONN_DEST_EG2_RELEASE = 0x030D,
  IPATCH_DLS2_CONN_DEST_EG2_SUSTAIN = 0x030E,
  IPATCH_DLS2_CONN_DEST_EG2_DELAY = 0x030F,
  IPATCH_DLS2_CONN_DEST_EG2_HOLD = 0x0310,

  IPATCH_DLS2_CONN_DEST_FILTER_CUTOFF = 0x0500,
  IPATCH_DLS2_CONN_DEST_FILTER_Q = 0x0501
} IpatchDLS2ConnDestType;

#define IPATCH_DLS2_CONN_OUTPUT_TRANS_NONE 0

/* connection transform types */
typedef enum
{
  IPATCH_DLS2_CONN_TRANS_LINEAR = 0,
  IPATCH_DLS2_CONN_TRANS_CONCAVE = 1,
  IPATCH_DLS2_CONN_TRANS_CONVEX = 2,
  IPATCH_DLS2_CONN_TRANS_SWITCH = 3
} IpatchDLS2ConnTransformType;

/* connection polarity types */
typedef enum
{
  IPATCH_DLS2_CONN_POLARITY_UNI = 0,
  IPATCH_DLS2_CONN_POLARITY_BI  = 1
} IpatchDLS2ConnPolarityType;

/* masks for IpatchDLS2Conn->trans field */
typedef enum
{
  IPATCH_DLS2_CONN_MASK_OUTPUT_TRANS = 0x000F, /* Output transform mask */
  IPATCH_DLS2_CONN_MASK_CTRLSRC_TRANS = 0x00F0,	/* Control transform mask */
  IPATCH_DLS2_CONN_MASK_CTRLSRC_POLARITY = 0x0100, /* Control polarity mask */
  IPATCH_DLS2_CONN_MASK_CTRLSRC_INVERT = 0x0200, /* Control invert mask */
  IPATCH_DLS2_CONN_MASK_SRC_TRANS = 0x3C00, /* Source transform mask */
  IPATCH_DLS2_CONN_MASK_SRC_POLARITY = 0x4000, /* Source polarity mask */
  IPATCH_DLS2_CONN_MASK_SRC_INVERT = 0x8000 /* Source invert mask */
} IpatchDLS2ConnTransformMasks;

/* bit shifts for IpatchDLS2Conn->trans field */
typedef enum
{
  IPATCH_DLS2_CONN_SHIFT_OUTPUT_TRANS = 0, /* Output transform shift */
  IPATCH_DLS2_CONN_SHIFT_CTRLSRC_TRANS = 4, /* Control transform shift */
  IPATCH_DLS2_CONN_SHIFT_CTRLSRC_POLARITY = 8, /* Control polarity shift */
  IPATCH_DLS2_CONN_SHIFT_CTRLSRC_INVERT = 9, /* Control invert shift */
  IPATCH_DLS2_CONN_SHIFT_SRC_TRANS = 10, /* Source transform shift */
  IPATCH_DLS2_CONN_SHIFT_SRC_POLARITY = 14, /* Source polarity shift */
  IPATCH_DLS2_CONN_SHIFT_SRC_INVERT = 15 /* Source invert shift */
} IpatchDLS2ConnTransformShifts;

extern IpatchDLS2ConnInfo ipatch_dls2_conn_info[];

GType ipatch_dls2_conn_get_type (void);
IpatchDLS2Conn *ipatch_dls2_conn_new (void);
void ipatch_dls2_conn_free (IpatchDLS2Conn *conn);
IpatchDLS2Conn *ipatch_dls2_conn_duplicate (const IpatchDLS2Conn *conn);

void ipatch_dls2_conn_list_set (GSList **list, const IpatchDLS2Conn *conn);
void ipatch_dls2_conn_list_unset (GSList **list, const IpatchDLS2Conn *conn);
GSList *ipatch_dls2_conn_list_duplicate (const GSList *list);
GSList *ipatch_dls2_conn_list_duplicate_fast (const GSList *list);
void ipatch_dls2_conn_list_free (GSList *list, gboolean free_conns);

#endif
