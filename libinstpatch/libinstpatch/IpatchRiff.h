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
#ifndef __IPATCH_RIFF_H__
#define __IPATCH_RIFF_H__

#include <glib.h>
#include <libinstpatch/IpatchFile.h>

typedef struct _IpatchRiff IpatchRiff;
typedef struct _IpatchRiffClass IpatchRiffClass;
typedef struct _IpatchRiffChunk IpatchRiffChunk;

#define IPATCH_TYPE_RIFF   (ipatch_riff_get_type ())
#define IPATCH_RIFF(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_RIFF, IpatchRiff))
#define IPATCH_RIFF_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_RIFF, IpatchRiffClass))
#define IPATCH_IS_RIFF(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_RIFF))
#define IPATCH_IS_RIFF_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_RIFF))

typedef enum
{
  IPATCH_RIFF_STATUS_FAIL = 0,	/* error occured */
  IPATCH_RIFF_STATUS_BEGIN,	/* parsing has not yet began */
  IPATCH_RIFF_STATUS_FINISHED,	/* no more parsing to be done */
  IPATCH_RIFF_STATUS_NORMAL,	/* normal status */
  IPATCH_RIFF_STATUS_CHUNK_END	/* end of a chunk */
} IpatchRiffStatus;

typedef enum
{
  IPATCH_RIFF_READ,
  IPATCH_RIFF_WRITE
} IpatchRiffMode;

/* RIFF object */
struct _IpatchRiff
{
  GObject parent_instance;	/* derived from GObject */

  IpatchRiffStatus status;	/* current status */
  IpatchRiffMode mode;		/* I/O mode (read/write) */
  guint flags;			/* some flags */
  IpatchFileHandle *handle;	/* file object being parsed */

  GError *err;	 /* error information if status == IPATCH_RIFF_FAIL */
  char *msg_detail;	      /* last generated message detail string */

  GArray *chunks;	/* chunk trail (IpatchRiffChunk structures) */
  GList *state_stack;		/* saved states (positions) */
};

/* RIFF class */
struct _IpatchRiffClass
{
  GObjectClass parent_class;
};

#define IPATCH_RIFF_NEED_SWAP(riff)  IPATCH_FILE_NEED_SWAP (riff->handle->file)
#define IPATCH_RIFF_BIG_ENDIAN(riff) IPATCH_FILE_BIG_ENDIAN (riff->handle->file)

typedef enum
{
  IPATCH_RIFF_CHUNK_RIFF,	/* toplevel "RIFF" (or "RIFX") list chunk */
  IPATCH_RIFF_CHUNK_LIST,	/* a "LIST" chunk */
  IPATCH_RIFF_CHUNK_SUB		/* a sub chunk */
} IpatchRiffChunkType;

/* structure describing a RIFF chunk */
struct _IpatchRiffChunk
{
  IpatchRiffChunkType type;	/* type of chunk */
  guint32 id;	  /* chunk ID in integer format for easy comparison */
  char idstr[4];		/* four character chunk ID string */
  gint32 position; /* current position in chunk (read or write mode) */
  guint32 size;			/* size of chunk (read mode only) */
  guint32 filepos;              /* Position in file object of chunk data */
};

/* error domain for g_set_error */
#define IPATCH_RIFF_ERROR  ipatch_riff_error_quark()

typedef enum
{
  IPATCH_RIFF_ERROR_NOT_RIFF,	/* not a RIFF file */
  IPATCH_RIFF_ERROR_UNEXPECTED_ID, /* unexpected chunk ID */
  IPATCH_RIFF_ERROR_UNEXPECTED_CHUNK_END,  /* unexpected LIST chunk end */
  IPATCH_RIFF_ERROR_INVALID_ID, /* invalid chunk FOURCC ID */
  IPATCH_RIFF_ERROR_ODD_SIZE,	/* chunk size is ODD */
  IPATCH_RIFF_ERROR_SIZE_EXCEEDED, /* chunk size exceeded */

  /* convenience errors - not used by the riff object itself */
  IPATCH_RIFF_ERROR_SIZE_MISMATCH, /* chunk size mismatch */
  IPATCH_RIFF_ERROR_INVALID_DATA /* generic invalid data error */
} IpatchRiffError;

/* macro to convert 4 char RIFF ids to a guint32 integer for comparisons */
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define IPATCH_FOURCC(c1, c2, c3, c4) \
  ((guint32)(c1) | ((guint32)(c2) << 8) | ((guint32)(c3) << 16) \
   | ((guint32)(c4) << 24))
#elif G_BYTE_ORDER == G_BIG_ENDIAN
#define IPATCH_FOURCC(c1, c2, c3, c4) \
  ((guint32)(c4) | ((guint32)(c3) << 8) | ((guint32)(c2) << 16) \
   | ((guint32)(c1) << 24))
#else
#error Processor not big or little endian?
#endif

/* RIFF chunk FOURCC guint32 integers */
#define IPATCH_FOURCC_RIFF IPATCH_FOURCC ('R','I','F','F')
#define IPATCH_FOURCC_RIFX IPATCH_FOURCC ('R','I','F','X') /* big endian */
#define IPATCH_FOURCC_LIST IPATCH_FOURCC ('L','I','S','T')

/**
 * IPATCH_RIFF_HEADER_SIZE: (skip)
 */
#define IPATCH_RIFF_HEADER_SIZE 8 /* size of RIFF chunk headers (ID + size) */

/**
 * IPATCH_RIFF_FOURCC_SIZE: (skip)
 */
#define IPATCH_RIFF_FOURCC_SIZE 4 /* RIFF FOURCC ID size */

/**
 * IPATCH_RIFF_LIST_HEADER_SIZE: (skip)
 */
/* chunk header + 4 character list type */
#define IPATCH_RIFF_LIST_HEADER_SIZE \
  (IPATCH_RIFF_HEADER_SIZE + IPATCH_RIFF_FOURCC_SIZE)

/* some RIFF WAVE file specific defines */
/**
 * IPATCH_RIFF_WAVE_FMT_PCM: (skip)
 */
#define IPATCH_RIFF_WAVE_FMT_PCM   0x1

/**
 * IPATCH_RIFF_WAVE_FMT_FLOAT: (skip)
 */
#define IPATCH_RIFF_WAVE_FMT_FLOAT 0x3

GType ipatch_riff_get_type (void);
GQuark ipatch_riff_error_quark (void);
IpatchRiff *ipatch_riff_new (IpatchFileHandle *handle);
void ipatch_riff_set_file_handle (IpatchRiff *riff, IpatchFileHandle *handle);
IpatchFileHandle *ipatch_riff_get_file_handle (IpatchRiff *riff);
int ipatch_riff_get_chunk_level (IpatchRiff *riff);
IpatchRiffChunk *ipatch_riff_get_chunk_array (IpatchRiff *riff, int *count);
IpatchRiffChunk *ipatch_riff_get_chunk (IpatchRiff *riff, int level);
guint32 ipatch_riff_get_total_size (IpatchRiff *riff);
guint32 ipatch_riff_get_position (IpatchRiff *riff);
void ipatch_riff_push_state (IpatchRiff *riff);
gboolean ipatch_riff_pop_state (IpatchRiff *riff, GError **err);
IpatchRiffChunk *ipatch_riff_start_read (IpatchRiff *riff, GError **err);
IpatchRiffChunk *ipatch_riff_start_read_chunk (IpatchRiff *riff, GError **err);
IpatchRiffChunk *ipatch_riff_read_chunk_verify (IpatchRiff *riff,
						IpatchRiffChunkType type,
						guint32 id, GError **err);
IpatchRiffChunk *ipatch_riff_read_chunk (IpatchRiff *riff, GError **err);
#define ipatch_riff_write_list_chunk(parser, id, err) \
  ipatch_riff_write_chunk (parser, IPATCH_RIFF_CHUNK_LIST, id, err)
#define ipatch_riff_write_sub_chunk(parser, id, err) \
  ipatch_riff_write_chunk (parser, IPATCH_RIFF_CHUNK_SUB, id, err)
gboolean ipatch_riff_write_chunk (IpatchRiff *riff, IpatchRiffChunkType type,
                                  guint32 id, GError **err);
#define ipatch_riff_end_chunk(parser, err) \
  ipatch_riff_close_chunk (parser, -1, err)
gboolean ipatch_riff_close_chunk (IpatchRiff *riff, int level, GError **err);
#define ipatch_riff_skip_chunk(parser, err) \
  ipatch_riff_skip_chunks (parser, 1, err)
gboolean ipatch_riff_skip_chunks (IpatchRiff *riff, guint count, GError **err);
gboolean ipatch_riff_get_error (IpatchRiff *riff, GError **err);
char *ipatch_riff_message_detail (IpatchRiff *riff, int level,
                                  const char *format, ...);
#endif
