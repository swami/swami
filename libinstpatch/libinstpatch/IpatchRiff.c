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
/**
 * SECTION: IpatchRiff
 * @short_description: RIFF file parser/composer object
 * @see_also: 
 * @stability: Stable
 *
 * A RIFF file parser/composer.  Used for DLS, SoundFont and GigaSampler files.
 */
#include <string.h>
#include <glib.h>

#include "IpatchRiff.h"
#include "ipatch_priv.h"
#include "i18n.h"

static void ipatch_riff_finalize (GObject *obj);
static void ipatch_riff_update_positions (IpatchRiff *riff);

static gboolean verify_chunk_idstr (char idstr[4]);


G_DEFINE_TYPE (IpatchRiff, ipatch_riff, G_TYPE_OBJECT);


static void
ipatch_riff_class_init (IpatchRiffClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->finalize = ipatch_riff_finalize;
}

static void
ipatch_riff_init (IpatchRiff *riff)
{
  riff->status = IPATCH_RIFF_STATUS_BEGIN;
  riff->mode = IPATCH_RIFF_READ;
  riff->flags = 0;
  riff->handle = NULL;
  riff->chunks = g_array_new (FALSE, FALSE, sizeof (IpatchRiffChunk));
  riff->state_stack = NULL;
}

static void
ipatch_riff_finalize (GObject *obj)
{
  IpatchRiff *riff = IPATCH_RIFF (obj);
  GList *p;

  if (riff->handle) ipatch_file_close (riff->handle); /* -- unref file object */

  g_array_free (riff->chunks, TRUE);

  for (p = riff->state_stack; p; p = g_list_next (p))
    g_array_free ((GArray *)(p->data), TRUE);

  if (G_OBJECT_CLASS (ipatch_riff_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_riff_parent_class)->finalize (obj);
}

/**
 * ipatch_riff_error_quark: (skip)
 */
GQuark
ipatch_riff_error_quark (void)
{
  static GQuark q = 0;

  if (q == 0)
    q = g_quark_from_static_string ("riff-error-quark");

  return (q);
}

/**
 * ipatch_riff_new:
 * @handle: File object handle to parse or %NULL to set later, the handle will be
 *   taken over by the riff object and closed when finalized.
 *
 * Create a new RIFF file riff/composer object
 *
 * Returns: The RIFF object
 */
IpatchRiff *
ipatch_riff_new (IpatchFileHandle *handle)
{
  IpatchRiff *riff;

  g_return_val_if_fail (!handle || IPATCH_IS_FILE_HANDLE (handle), NULL);

  riff = g_object_new (IPATCH_TYPE_RIFF, NULL);
  if (handle) ipatch_riff_set_file_handle (riff, handle);

  return (riff);
}

/**
 * ipatch_riff_set_file_handle:
 * @riff: RIFF object
 * @handle: File object handle to assign
 *
 * Set the file object handle of a RIFF object.  The handle is taken over
 * by the riff object and will be closed when finalized.
 */
void
ipatch_riff_set_file_handle (IpatchRiff *riff, IpatchFileHandle *handle)
{
  g_return_if_fail (IPATCH_IS_RIFF (riff));
  g_return_if_fail (IPATCH_IS_FILE_HANDLE (handle));

  g_array_set_size (riff->chunks, 0);	/* reset chunk state */

  /* Close old handle, if any */
  if (riff->handle) ipatch_file_close (riff->handle);

  riff->handle = handle;
}

/**
 * ipatch_riff_get_file:
 * @riff: RIFF object
 *
 * Get the file handle from a RIFF object.
 *
 * Returns: The file handle or %NULL if not assigned.
 */
IpatchFileHandle *
ipatch_riff_get_file_handle (IpatchRiff *riff)
{
  g_return_val_if_fail (IPATCH_IS_RIFF (riff), NULL);
  return (riff->handle);
}

/**
 * ipatch_riff_get_chunk_level:
 * @riff: RIFF object
 *
 * Gets the current chunk level count (number of embedded chunks) currently
 * being processed in a RIFF file.
 *
 * Returns: Chunk level count (0 = no open chunks)
 */
int
ipatch_riff_get_chunk_level (IpatchRiff *riff)
{
  g_return_val_if_fail (IPATCH_IS_RIFF (riff), 0);
  return (riff->chunks->len);
}

/**
 * ipatch_riff_get_chunk_array:
 * @riff: RIFF object
 * @count: (out): Location to store the number of elements in the returned array
 *
 * Gets the array of open chunk info structures.
 *
 * Returns: (array length=count) (transfer none): Array of #IpatchRiffChunk
 * structures or %NULL if no chunks being processed (processing hasn't started).
 * The returned array is internal and should not be modified or freed. Also note that the
 * array is valid only while the chunk state is unchanged (riff object or file
 * operations). The number of elements in the array is stored in @count.
 */
IpatchRiffChunk *
ipatch_riff_get_chunk_array (IpatchRiff *riff, int *count)
{
  if (count) *count = 0;	/* in case of error */

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), NULL);

  /* Update the chunk positions */
  ipatch_riff_update_positions (riff);

  if (count) *count = riff->chunks->len;

  if (riff->chunks->len > 0)
    return (&g_array_index (riff->chunks, IpatchRiffChunk, 0));
  else return (NULL);
}

/**
 * ipatch_riff_get_chunk:
 * @riff: RIFF object
 * @level: Level of chunk to get (-1 for current chunk)
 *
 * Get the chunk at the specified @level from a RIFF @riff chunk state
 * array.
 *
 * Returns: (transfer none): Pointer to the chunk or %NULL if invalid level. The returned
 * chunk is internal and should not be modified or freed.  Also note that the
 * chunk is valid only while the chunk state is unchanged (riff object or file
 * operations).
 */
IpatchRiffChunk *
ipatch_riff_get_chunk (IpatchRiff *riff, int level)
{
  g_return_val_if_fail (IPATCH_IS_RIFF (riff), NULL);
  g_return_val_if_fail (riff->chunks->len > 0, NULL);

  /* Update the chunk positions */
  ipatch_riff_update_positions (riff);

  if (level == -1) level = riff->chunks->len - 1;
  g_return_val_if_fail (level >= -1 && level < riff->chunks->len, NULL);

  return (&g_array_index (riff->chunks, IpatchRiffChunk, level));
}

/**
 * ipatch_riff_get_total_size:
 * @riff: RIFF object
 *
 * Get total size of toplevel chunk. This is a convenience function
 * that just adds the size of the toplevel chunk and its header, the
 * actual file object size is not checked.
 *
 * Returns: Size of toplevel chunk + header size, in bytes. Actual file size
 *   is not checked.
 */
guint32
ipatch_riff_get_total_size (IpatchRiff *riff)
{
  IpatchRiffChunk *chunk;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), 0);

  /* Update the chunk positions */
  ipatch_riff_update_positions (riff);

  if (riff->chunks->len == 0) return (0);
  chunk = &g_array_index (riff->chunks, IpatchRiffChunk, 0);
  return (chunk->size + IPATCH_RIFF_HEADER_SIZE);
}

/**
 * ipatch_riff_get_position:
 * @riff: RIFF object
 *
 * Get current position in the toplevel RIFF chunk (including header,
 * i.e., the file position).
 *
 * Returns: The current offset, in bytes, into the toplevel RIFF chunk
 * (including header).
 */
guint32
ipatch_riff_get_position (IpatchRiff *riff)
{
  IpatchRiffChunk *chunk;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), 0);

  /* Update the chunk positions */
  ipatch_riff_update_positions (riff);

  if (riff->chunks->len == 0) return (0);
  chunk = &g_array_index (riff->chunks, IpatchRiffChunk, 0);
  return (chunk->position + IPATCH_RIFF_HEADER_SIZE);
}

/**
 * ipatch_riff_push_state:
 * @riff: RIFF object
 *
 * Pushes the current file position and chunk state onto the state stack.
 * This state can be later restored to return to the same position in a RIFF
 * file.
 */
void
ipatch_riff_push_state (IpatchRiff *riff)
{
  GArray *dup_array;

  g_return_if_fail (IPATCH_IS_RIFF (riff));

  /* Update the chunk positions */
  ipatch_riff_update_positions (riff);

  dup_array = g_array_new (FALSE, FALSE, sizeof (IpatchRiffChunk));
  if (riff->chunks->len > 0)
    g_array_append_vals (dup_array, riff->chunks->data, riff->chunks->len);

  riff->state_stack = g_list_prepend (riff->state_stack, dup_array);
}

/**
 * ipatch_riff_pop_state:
 * @riff: RIFF object
 * @err: Location to store error info or %NULL
 *
 * Pops the most recent state pushed onto the state stack. This causes the
 * position in the RIFF file stored by the state to be restored.
 *
 * Returns: %TRUE on success, %FALSE otherwise which is fatal
 */
gboolean
ipatch_riff_pop_state (IpatchRiff *riff, GError **err)
{
  IpatchRiffChunk *chunk;
  gboolean retval;
  guint pos;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), FALSE);
  g_return_val_if_fail (riff->state_stack != NULL, FALSE);

  g_array_free (riff->chunks, TRUE);

  riff->chunks = riff->state_stack->data;
  riff->state_stack = g_list_delete_link (riff->state_stack,
					    riff->state_stack);

  /* We want the current chunk state position, not the current file position
   * which we would get from ipatch_riff_get_position() */
  if (riff->chunks->len > 0)
  {
    chunk = &g_array_index (riff->chunks, IpatchRiffChunk, 0);
    pos = chunk->position + IPATCH_RIFF_HEADER_SIZE;
  }
  else pos = 0;

  retval = ipatch_file_seek (riff->handle, pos, G_SEEK_SET, err);

  return (retval);
}

/* initializes riff object to default state */
static void
ipatch_riff_reset (IpatchRiff *riff)
{
  riff->status = IPATCH_RIFF_STATUS_BEGIN;
  riff->mode = IPATCH_RIFF_READ;
  riff->flags = 0;
  g_array_set_size (riff->chunks, 0);
}

/**
 * ipatch_riff_start_read:
 * @riff: RIFF object
 * @err: Location to store error info or %NULL
 *
 * Start parsing the @riff file object as if it were at the
 * beginning of a RIFF file. Clears any current chunk state,
 * loads a chunk and ensures that it has the "RIFF" or "RIFX" ID. If this call
 * is sucessful there will be one chunk on the chunk stack with the
 * secondary ID of the RIFF chunk. If it is desirable to process a
 * chunk that is not the beginning of a RIFF file,
 * ipatch_riff_start_read_chunk() can be used. This function will also
 * automatically enable byte order swapping if needed.
 *
 * Returns: (transfer none): Pointer to the opened RIFF chunk or %NULL on error. The returned
 * chunk is internal and should not be modified or freed.  Also note that the
 * chunk is valid only while the chunk state is unchanged (riff object or file
 * operations).
 */
IpatchRiffChunk *
ipatch_riff_start_read (IpatchRiff *riff, GError **err)
{
  IpatchRiffChunk *chunk;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), NULL);
  g_return_val_if_fail (riff->status != IPATCH_RIFF_STATUS_FAIL, NULL);
  g_return_val_if_fail (!err || !*err, NULL);

  ipatch_riff_reset (riff);
  riff->mode = IPATCH_RIFF_READ;

  if (!(chunk = ipatch_riff_read_chunk (riff, err))) return (NULL);

  if (chunk->type != IPATCH_RIFF_CHUNK_RIFF)
    {
      g_array_set_size (riff->chunks, 0); /* clear non "RIFF" chunk */
      riff->status = IPATCH_RIFF_STATUS_FAIL;
      g_set_error (&riff->err, IPATCH_RIFF_ERROR,
		   IPATCH_RIFF_ERROR_NOT_RIFF,
		   _("Not a RIFF file"));
      if (err) *err = g_error_copy (riff->err);
      return (NULL);
    }

  return (chunk);
}

/**
 * ipatch_riff_start_read_chunk:
 * @riff: RIFF object
 * @err: Location to store error info or %NULL
 *
 * Start parsing the @riff file object at an arbitrary chunk.
 * Clears any current chunk state and loads a chunk. If this
 * call is sucessful there will be one chunk on the chunk stack. If it
 * is desirable to start processing from the beginning of a RIFF file
 * ipatch_riff_start_read() should be used instead. An end of file
 * condition is considered an error. Note that it is up to the caller to
 * ensure byte order swapping is enabled, if needed.
 *
 * Returns: (transfer none): Pointer to the opened RIFF chunk or %NULL on error. The returned
 * chunk is internal and should not be modified or freed.  Also note that the
 * chunk is valid only while the chunk state is unchanged (riff object or file
 * operations).
 */
IpatchRiffChunk *
ipatch_riff_start_read_chunk (IpatchRiff *riff, GError **err)
{
  IpatchRiffChunk *chunk;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), NULL);
  g_return_val_if_fail (riff->status != IPATCH_RIFF_STATUS_FAIL, NULL);
  g_return_val_if_fail (!err || !*err, NULL);

  ipatch_riff_reset (riff);
  riff->mode = IPATCH_RIFF_READ;

  chunk = ipatch_riff_read_chunk (riff, err);
  return (chunk);
}

/**
 * ipatch_riff_read_chunk:
 * @riff: RIFF object
 * @err: Location to store error info or %NULL
 *
 * Parse next RIFF chunk header. The ipatch_riff_close_chunk()
 * function should be called at the end of parsing a chunk, otherwise this
 * function will return NULL if the current chunk has ended.  When
 * the first RIFF chunk is read the IPATCH_RIFF_FLAG_BIG_ENDIAN flag
 * is cleared or set depending on if its RIFF or RIFX respectively,
 * endian swapping is also enabled if the file uses non-native endian
 * format to the host.
 *
 * Returns: (transfer none): Pointer to new opened chunk or %NULL if current chunk has ended
 * or on error. Returned chunk pointer is internal and should not be modified
 * or freed.  Also note that the chunk is valid only while the chunk state is
 * unchanged (riff object or file operations).
 */
IpatchRiffChunk *
ipatch_riff_read_chunk (IpatchRiff *riff, GError **err)
{
  IpatchRiffChunk *chunk;
  IpatchRiffChunk newchunk;
  guint32 buf[IPATCH_RIFF_HEADER_SIZE / 4];
  guint size;
  guint32 id;
  int i, c;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), NULL);
  g_return_val_if_fail (riff->status != IPATCH_RIFF_STATUS_FAIL, NULL);
  g_return_val_if_fail (riff->mode == IPATCH_RIFF_READ, NULL);
  g_return_val_if_fail (riff->handle != NULL, NULL);
  g_return_val_if_fail (!err || !*err, NULL);

  /* return finished if we already finished */
  if (riff->status == IPATCH_RIFF_STATUS_FINISHED
      || riff->status == IPATCH_RIFF_STATUS_CHUNK_END)
    return (NULL);

  if (riff->chunks->len > 0)
    {
      /* Update the chunk positions */
      ipatch_riff_update_positions (riff);

      chunk = &g_array_index (riff->chunks, IpatchRiffChunk,
			      riff->chunks->len - 1);

      /* current chunk is sub chunk, or pos past end? */
      if (chunk->type == IPATCH_RIFF_CHUNK_SUB
	  || chunk->position >= chunk->size)
	{
	  riff->status = IPATCH_RIFF_STATUS_CHUNK_END;
	  return (NULL);
	}
    }

  /* read FOURCC ID and chunk size */
  if (!ipatch_file_read (riff->handle, buf, IPATCH_RIFF_HEADER_SIZE, &riff->err))
    {
      riff->status = IPATCH_RIFF_STATUS_FAIL;
      if (err) *err = g_error_copy (riff->err);
      return (NULL);
    }

  id = buf[0];

  /* unexpected "RIFF" chunk? */
  if (id == IPATCH_FOURCC_RIFF && riff->chunks->len > 0)
    {
      riff->status = IPATCH_RIFF_STATUS_FAIL;
      g_set_error (&riff->err, IPATCH_RIFF_ERROR,
		   IPATCH_RIFF_ERROR_UNEXPECTED_ID,
		   _("Unexpected 'RIFF' chunk"));
      if (err) *err = g_error_copy (riff->err);
      return (NULL);
    }

  /* Position of chunk data (or LIST secondary chunk ID) */
  newchunk.filepos = ipatch_file_get_position (riff->handle);

  /* is a list chunk (LIST or RIFF)? */
  if (id == IPATCH_FOURCC_LIST || id == IPATCH_FOURCC_RIFF
      || id == IPATCH_FOURCC_RIFX)
    {
      if (id == IPATCH_FOURCC_LIST)
	newchunk.type = IPATCH_RIFF_CHUNK_LIST;
      else if (id == IPATCH_FOURCC_RIFF)
	{
	  newchunk.type = IPATCH_RIFF_CHUNK_RIFF;
	  ipatch_file_set_little_endian (riff->handle->file);
	}
      else			/* RIFX big endian chunk? */
	{
	  newchunk.type = IPATCH_RIFF_CHUNK_RIFF;
	  ipatch_file_set_big_endian (riff->handle->file);
	}

      /* read secondary chunk ID over old ID */
      if (!ipatch_file_read (riff->handle, &buf, IPATCH_RIFF_FOURCC_SIZE, &riff->err))
	{
	  riff->status = IPATCH_RIFF_STATUS_FAIL;
	  if (err) *err = g_error_copy (riff->err);
	  return (NULL);
	}
      newchunk.position = 4;
    }
  else				/* sub chunk */
    {
      newchunk.type = IPATCH_RIFF_CHUNK_SUB;
      newchunk.position = 0;
    }

  newchunk.id = buf[0];
  memcpy (&newchunk.idstr, &newchunk.id, 4);

  if (!verify_chunk_idstr (newchunk.idstr))
    {
      riff->status = IPATCH_RIFF_STATUS_FAIL;
      g_set_error (&riff->err, IPATCH_RIFF_ERROR,
		   IPATCH_RIFF_ERROR_INVALID_ID,
		   _("Invalid RIFF chunk id"));
      if (err) *err = g_error_copy (riff->err);
      return (NULL);
    }

  newchunk.size = IPATCH_FILE_SWAP32 (riff->handle->file, buf + 1);

  /* list chunk size should be even (sub chunks can be odd) */
  if (newchunk.type != IPATCH_RIFF_CHUNK_SUB && newchunk.size % 2)
    {
      riff->status = IPATCH_RIFF_STATUS_FAIL;
      g_set_error (&riff->err, IPATCH_RIFF_ERROR,
		   IPATCH_RIFF_ERROR_ODD_SIZE,
		   _("Invalid RIFF LIST chunk size (odd number)"));
      if (err) *err = g_error_copy (riff->err);
      return (NULL);
    }

  size = (newchunk.size + 1) & ~1; /* round up to even if odd size */

  /* Update the chunk positions */
  ipatch_riff_update_positions (riff);

  /* make sure chunk size does not exceed its parent sizes */
  c = riff->chunks->len;
  for (i=0; i < c; i++)
    {
      chunk = &g_array_index (riff->chunks, IpatchRiffChunk, i);
      if (chunk->position + size - newchunk.position > chunk->size)
	{
	  riff->status = IPATCH_RIFF_STATUS_FAIL;
	  g_set_error (&riff->err, IPATCH_RIFF_ERROR,
		       IPATCH_RIFF_ERROR_SIZE_EXCEEDED,
		       _("Child chunk '%.4s' (size = %d, level = %d) exceeds"
			 " parent chunk '%.4s' (size = %d, level = %d)"),
		       newchunk.idstr, newchunk.size, c,
		       chunk->idstr, chunk->size, i);
	  if (err) *err = g_error_copy (riff->err);
	  return (NULL);
	}
    }

  g_array_append_val (riff->chunks, newchunk);
  riff->status = IPATCH_RIFF_STATUS_NORMAL;

  return ((IpatchRiffChunk *)&g_array_index (riff->chunks, IpatchRiffChunk,
					     riff->chunks->len - 1));
}

/* Update all open chunk positions (called after file position changes) */
static void
ipatch_riff_update_positions (IpatchRiff *riff)
{
  IpatchRiffChunk *chunk;
  gint32 filepos;
  int size, i;

  size = riff->chunks->len;
  if (size == 0) return;

  filepos = ipatch_file_get_position (riff->handle);

  for (i = 0; i < size; i++)
  {
    chunk = &g_array_index (riff->chunks, IpatchRiffChunk, i);
    chunk->position = filepos - chunk->filepos;
  }
}

/**
 * ipatch_riff_read_chunk_verify:
 * @riff: RIFF object
 * @type: Expected chunk type
 * @id: Expected chunk ID
 * @err: Location to store error info or %NULL
 *
 * Like ipatch_riff_read_chunk() but ensures that the new chunk matches a
 * specific type and ID.  If the chunk is not the expected chunk or no more
 * chunks in current list chunk, it is considered an error.
 * ipatch_riff_close_chunk() should be called when finished parsing the
 * opened chunk.
 *
 * Returns: (transfer none): Pointer to new opened chunk or %NULL if current chunk has ended
 * or on error. Returned chunk pointer is internal and should not be modified
 * or freed.  Also note that the chunk is valid only while the chunk state is
 * unchanged (riff object or file operations).
 */
IpatchRiffChunk *
ipatch_riff_read_chunk_verify (IpatchRiff *riff,
			       IpatchRiffChunkType type, guint32 id,
			       GError **err)
{
  IpatchRiffChunk *chunk;
  char *idstr;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), NULL);
  g_return_val_if_fail (riff->status != IPATCH_RIFF_STATUS_FAIL, NULL);
  g_return_val_if_fail (type >= IPATCH_RIFF_CHUNK_RIFF && type <= IPATCH_RIFF_CHUNK_SUB, NULL);
  idstr = (char *)(&id);
  g_return_val_if_fail (verify_chunk_idstr (idstr), NULL);
  g_return_val_if_fail (!err || !*err, NULL);

  if (!(chunk = ipatch_riff_read_chunk (riff, &riff->err)))
    {
      if (!riff->err)
	g_set_error (&riff->err, IPATCH_RIFF_ERROR,
		     IPATCH_RIFF_ERROR_UNEXPECTED_CHUNK_END,
		     _("Unexpected end of LIST while looking for chunk '%.4s'"),
		     idstr);
      if (err) *err = g_error_copy (riff->err);
      return (NULL);
    }

  if (chunk->type != type || chunk->id != id)
    {
      riff->status = IPATCH_RIFF_STATUS_FAIL;
      g_set_error (&riff->err, IPATCH_RIFF_ERROR,
		   IPATCH_RIFF_ERROR_UNEXPECTED_ID,
		   _("Unexpected RIFF chunk with ID '%.4s' (expected '%.4s')"),
		   chunk->idstr, idstr);
      if (err) *err = g_error_copy (riff->err);
      return (NULL);
    }
  return (chunk);
}

/* verify the characters of a chunk ID string */
static gboolean
verify_chunk_idstr (char idstr[4])
{
  int i;
  char c;

  for (i=0; i < 4; i++)	  /* chars of FOURCC should be alphanumeric */
    {
      c = idstr[i];
      if (!(c >= 'A' && c <= 'Z') && !(c >= 'a' && c <= 'z')
	  && !(c >= '0' && c <= '9'))
	break;
    }

  if (i < 4 && c == ' ' && i > 0) /* can pad with spaces (at least 1 char) */
    do { i++; } while (i < 4 && idstr[i] == ' ');

  return (i == 4);
}

/**
 * ipatch_riff_write_chunk:
 * @riff: RIFF object
 * @type: Chunk type (RIFF, LIST, or SUB)
 * @id: Chunk ID (secondary ID for RIFF and LIST chunks)
 * @err: Location to store error info or %NULL
 *
 * Opens a new chunk and writes a chunk header to the file object in @riff.
 * The size field of the chunk is set to 0 and will be filled in when the
 * chunk is closed (see ipatch_riff_close_chunk()).
 *
 * Returns: %TRUE on success, %FALSE otherwise.
 */
gboolean
ipatch_riff_write_chunk (IpatchRiff *riff,
			 IpatchRiffChunkType type, guint32 id, GError **err)
{
  IpatchRiffChunk chunk;
  guint32 buf[3];
  char *idstr;
  int size;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), FALSE);
  g_return_val_if_fail (riff->status != IPATCH_RIFF_STATUS_FAIL, FALSE);
  g_return_val_if_fail (type >= IPATCH_RIFF_CHUNK_RIFF && type <= IPATCH_RIFF_CHUNK_SUB, FALSE);
  idstr = (char *)(&id);
  g_return_val_if_fail (verify_chunk_idstr (idstr), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  riff->mode = IPATCH_RIFF_WRITE;

  buf[1] = 0; /* set chunk size to 0 (will seek back later) */

  if (type == IPATCH_RIFF_CHUNK_LIST || type == IPATCH_RIFF_CHUNK_RIFF)
    {
      if (type == IPATCH_RIFF_CHUNK_LIST)
	buf[0] = IPATCH_FOURCC_LIST;
      else
	{
	  buf[0] = IPATCH_RIFF_BIG_ENDIAN (riff)
	    ? IPATCH_FOURCC_RIFX : IPATCH_FOURCC_RIFF;
	}

      buf[2] = id;	/* set secondary list chunk ID */
      chunk.position = chunk.size = 4;
      size = 12;
    }
  else
    {
      buf[0] = id; /* set sub chunk ID */
      chunk.position = chunk.size = 0;
      size = 8;
    }

  if (!ipatch_file_write (riff->handle, buf, size, &riff->err))
    {
      riff->status = IPATCH_RIFF_STATUS_FAIL;
      if (err) *err = g_error_copy (riff->err);
      return (FALSE);
    }

  /* Update the chunk positions */
  ipatch_riff_update_positions (riff);

  chunk.type = type;
  chunk.id = id;
  memcpy (&chunk.idstr, &id, 4);
  chunk.filepos = ipatch_file_get_position (riff->handle) - chunk.position;

  g_array_append_val (riff->chunks, chunk);

  return (TRUE);
}

/**
 * ipatch_riff_close_chunk:
 * @riff: RIFF object
 * @level: Level of chunk to close (-1 for current chunk)
 * @err: Location to store error info or %NULL
 *
 * Closes the chunk specified by @level and all its children (if any).
 *
 * In write mode the chunk size is filled in for chunks that get closed and
 * therefore the file object of @riff must be seekable (anyone need
 * non-seekable RIFF writing?). The chunk size is padded to an even
 * number if necessary (by writing a NULL byte).
 *
 * Upon successful completion the file position will be where it was prior to
 * the call (write mode) or at the beginning of the next chunk (read mode).
 * There will be @level open chunks (or previous chunk count - 1 if
 * @level == -1). In read mode the status will be
 * #IPATCH_RIFF_STATUS_NORMAL if open chunks remain or
 * #IPATCH_RIFF_STATUS_FINISHED if toplevel chunk was closed. The status is
 * not modified in write mode.
 *
 * Returns: %TRUE on success, %FALSE otherwise.
 */
gboolean
ipatch_riff_close_chunk (IpatchRiff *riff, int level, GError **err)
{
  IpatchRiffChunk *chunk;
  char nul = '\0';		/* null byte to pad odd sized chunks */
  gint32 offset = 0;	   /* current offset from original position */
  gint32 seek;
  guint32 size;
  int retval = TRUE;
  int i;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), FALSE);
  g_return_val_if_fail (riff->status != IPATCH_RIFF_STATUS_FAIL, FALSE);
  g_return_val_if_fail (riff->chunks->len > 0, FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  if (level == -1) level = riff->chunks->len - 1;
  g_return_val_if_fail (level >= -1 && level < riff->chunks->len, FALSE);

  /* Update the chunk positions */
  ipatch_riff_update_positions (riff);

  if (riff->mode == IPATCH_RIFF_READ)	/* read mode? */
    {
      chunk = &g_array_index (riff->chunks, IpatchRiffChunk, level);

      /* round odd chunk sizes to even */
      seek = ((chunk->size + 1) & ~1) - chunk->position;

      /* close all chunks below and including level */
      g_array_set_size (riff->chunks, level);

      if (seek != 0)
        {
          /* seek to the end of the specified chunk */
          if (!ipatch_file_seek (riff->handle, seek, G_SEEK_CUR, &riff->err))
            {
              riff->status = IPATCH_RIFF_STATUS_FAIL;
              if (err) *err = g_error_copy (riff->err);
              return (FALSE);
            }

          /* Update the chunk positions */
          ipatch_riff_update_positions (riff);
        }

      if (level > 0) riff->status = IPATCH_RIFF_STATUS_NORMAL;
      else riff->status = IPATCH_RIFF_STATUS_FINISHED;

      return (TRUE);
    }
  else				/* write mode */
    {
      for (i = riff->chunks->len - 1; i >= level; i--)
	{
	  chunk = &g_array_index (riff->chunks, IpatchRiffChunk, i);

	  /* make sure we don't have a negative chunk size! */
	  if (log_if_fail (chunk->position >= 0))
	    goto fail;

	  /* we don't include padding (if any) in the size */
	  size = chunk->position;

	  if (chunk->position % 2)	/* need pad to even chunk size? */
	    {
	      int i2;

	      if (!ipatch_file_write (riff->handle, &nul, 1, &riff->err))
		goto fail;

	      /* add pad byte to chunk positions */
	      for (i2 = i; i2 >= 0; i2--)
		g_array_index (riff->chunks, IpatchRiffChunk, i2).position++;
	    }

	  /* seek to the chunk size field */
	  seek = -chunk->position - 4 - offset;
	  if (seek != 0)
	    if (!ipatch_file_seek (riff->handle, seek, G_SEEK_CUR,
				   &riff->err))
	      goto fail;

	  offset += seek;

	  /* write the chunk size */
	  if (!ipatch_file_write_u32 (riff->handle, size, &riff->err))
	    goto fail;

	  offset += 4;
	}

      g_array_set_size (riff->chunks, level); /* close chunk(s) */

    ret:
      /* return to the original position */
      if (offset && !ipatch_file_seek (riff->handle, -offset, G_SEEK_CUR,
				       retval ? err : NULL))
	{
	  riff->status = IPATCH_RIFF_STATUS_FAIL;
	  retval = FALSE;
	}

      if (!retval && riff->err && err) *err = g_error_copy (riff->err);

      return (retval);

    fail:
      riff->status = IPATCH_RIFF_STATUS_FAIL;
      retval = FALSE;
      goto ret;
    } /* else - (write mode) */
}

/**
 * ipatch_riff_skip_chunks:
 * @riff: RIFF object
 * @count: Number of chunks to skip
 * @err: Location to store error info or %NULL
 *
 * Skips RIFF chunks at the current chunk level (children of the current
 * chunk).
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
ipatch_riff_skip_chunks (IpatchRiff *riff, guint count, GError **err)
{
  guint i;

  for (i = 0; i < count; i++)
    {
      if (!ipatch_riff_read_chunk (riff, err)) return (FALSE);
      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
    }

  return (TRUE);
}

/**
 * ipatch_riff_get_error:
 * @riff: RIFF object
 * @err: Location to store error info
 *
 * Gets error information from a RIFF object.
 *
 * Returns: %FALSE if status is a #IPATCH_RIFF_STATUS_FAIL condition and info
 * can be found in @err, %TRUE if no error has occured.
 */
gboolean
ipatch_riff_get_error (IpatchRiff *riff, GError **err)
{
  g_return_val_if_fail (IPATCH_IS_RIFF (riff), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  if (riff->status != IPATCH_RIFF_STATUS_FAIL) return (TRUE);
  if (err) *err = g_error_copy (riff->err);
  return (FALSE);
}

/**
 * ipatch_riff_message_detail:
 * @riff: RIFF object
 * @level: Chunk level to generate detail for (-1 for current chunk)
 * @format: Printf style format string of message to display at beginning
 *   of riff detail
 * @...: Arguments for @msg string
 *
 * Generates a detailed message, including current position in RIFF file
 * and a chunk trace back. Useful for debugging purposes.
 *
 * Returns: (transfer none): Detailed message string which is internal to @riff and should
 * not be modified or freed. Also note that this string is only valid until
 * the next call to this function.
 */
char *
ipatch_riff_message_detail (IpatchRiff *riff, int level,
			    const char *format, ...)
{
  va_list args;
  IpatchRiffChunk *chunk;
  char *msg, *debug, *traceback = NULL, *s, *s2;
  int i, riffchunkpos = 0;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), NULL);

  /* Update the chunk positions */
  ipatch_riff_update_positions (riff);

  /* level will be -1 if already -1 and no chunks */
  if (level == -1) level = riff->chunks->len - 1;
  g_return_val_if_fail (level >= -1 && level < riff->chunks->len, NULL);

  va_start (args, format);
  msg = g_strdup_vprintf (format, args);
  va_end (args);

  if (riff->chunks->len > 0)
    {
      chunk = &g_array_index (riff->chunks, IpatchRiffChunk, 0);
      riffchunkpos = chunk->position;
    }

  debug = g_strdup_printf (" (ofs=%x, traceback [", riffchunkpos);

  if (riff->chunks->len > 0)
    {
      i = level;
      while (i >= 0)
	{
	  chunk = &g_array_index (riff->chunks, IpatchRiffChunk, i);
	  s = g_strdup_printf ("'%.4s' ofs=0x%X, size=%d%s", chunk->idstr,
			       riffchunkpos - chunk->position, chunk->size,
			       i != 0 ? " <= " : "");
	  if (traceback)
	    {
	      s2 = g_strconcat (traceback, s, NULL);
	      g_free (s);
	      g_free (traceback);
	      traceback = s2;
	    }
	  else traceback = s;

	  i--;
	}
    }
  else traceback = g_strdup ("<none>");

  s = g_strconcat (msg, debug, traceback, "])", NULL);
  g_free (msg);
  g_free (debug);
  g_free (traceback);

  g_free (riff->msg_detail);
  riff->msg_detail = s;
  return (s);
}
