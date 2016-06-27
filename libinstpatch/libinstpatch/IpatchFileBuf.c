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
/*
 * IpatchFile.c - File buffer and integer read/write funcs
 */
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchFile.h"

/* Defined in IpatchFile.c */
extern gboolean
_ipatch_file_read_no_pos_update (IpatchFileHandle *handle, gpointer buf,
                                 guint size, GError **err);
extern gboolean
_ipatch_file_write_no_pos_update (IpatchFileHandle *handle, gconstpointer buf,
                                  guint size, GError **err);

/**
 * ipatch_file_read_u8:
 * @handle: File handle
 * @val: Location to store value
 * @err: Location to store error info or %NULL
 *
 * Read an unsigned 8 bit integer from a file.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_file_read_u8 (IpatchFileHandle *handle, guint8 *val, GError **err)
{
  return (ipatch_file_read (handle, val, sizeof (guint8), err));
}

/**
 * ipatch_file_read_u16:
 * @handle: File handle
 * @val: Location to store value
 * @err: Location to store error info or %NULL
 *
 * Read an unsigned 16 bit integer from a file and performs endian
 * byte swapping if necessary.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_file_read_u16 (IpatchFileHandle *handle, guint16 *val, GError **err)
{
  if (!ipatch_file_read (handle, val, sizeof (guint16), err)) return (FALSE);
  *val = IPATCH_FILE_SWAP16 (handle->file, val);
  return (TRUE);
}

/**
 * ipatch_file_read_u32:
 * @handle: File handle
 * @val: Location to store value
 * @err: Location to store error info or %NULL
 *
 * Read an unsigned 32 bit integer from a file and performs endian
 * byte swapping if necessary.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_file_read_u32 (IpatchFileHandle *handle, guint32 *val, GError **err)
{
  if (!ipatch_file_read (handle, val, sizeof (guint32), err)) return (FALSE);
  *val = IPATCH_FILE_SWAP32 (handle->file, val);
  return (TRUE);
}

/**
 * ipatch_file_read_u64:
 * @handle: File handle
 * @val: Location to store value
 * @err: Location to store error info or %NULL
 *
 * Read an unsigned 64 bit integer from a file and performs endian
 * byte swapping if necessary.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_file_read_u64 (IpatchFileHandle *handle, guint64 *val, GError **err)
{
  if (!ipatch_file_read (handle, val, sizeof (guint64), err)) return (FALSE);
  *val = IPATCH_FILE_SWAP64 (handle->file, val);
  return (TRUE);
}

/**
 * ipatch_file_read_s8:
 * @handle: File handle
 * @val: Location to store value
 * @err: Location to store error info or %NULL
 *
 * Read a signed 8 bit integer from a file.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_file_read_s8 (IpatchFileHandle *handle, gint8 *val, GError **err)
{
  return (ipatch_file_read (handle, val, sizeof (gint8), err));
}

/**
 * ipatch_file_read_s16:
 * @handle: File handle
 * @val: Location to store value
 * @err: Location to store error info or %NULL
 *
 * Read a signed 16 bit integer from a file and performs endian
 * byte swapping if necessary.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_file_read_s16 (IpatchFileHandle *handle, gint16 *val, GError **err)
{
  if (!ipatch_file_read (handle, val, sizeof (gint16), err)) return (FALSE);
  *val = IPATCH_FILE_SWAP16 (handle->file, val);
  return (TRUE);
}

/**
 * ipatch_file_read_s32:
 * @handle: File handle
 * @val: Location to store value
 * @err: Location to store error info or %NULL
 *
 * Read a signed 32 bit integer from a file and performs endian
 * byte swapping if necessary.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_file_read_s32 (IpatchFileHandle *handle, gint32 *val, GError **err)
{
  if (!ipatch_file_read (handle, val, sizeof (gint32), err)) return (FALSE);
  *val = IPATCH_FILE_SWAP32 (handle->file, val);
  return (TRUE);
}

/**
 * ipatch_file_read_s64:
 * @handle: File handle
 * @val: Location to store value
 * @err: Location to store error info or %NULL
 *
 * Read a signed 64 bit integer from a file and performs endian
 * byte swapping if necessary.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_file_read_s64 (IpatchFileHandle *handle, gint64 *val, GError **err)
{
  if (!ipatch_file_read (handle, val, sizeof (gint64), err)) return (FALSE);
  *val = IPATCH_FILE_SWAP64 (handle->file, val);
  return (TRUE);
}

/**
 * ipatch_file_write_u8:
 * @handle: File handle
 * @val: Value to store
 * @err: Location to store error info or %NULL
 *
 * Write an unsigned 8 bit integer to a file.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_file_write_u8 (IpatchFileHandle *handle, guint8 val, GError **err)
{
  return (ipatch_file_write (handle, &val, sizeof (guint8), err));
}

/**
 * ipatch_file_write_u16:
 * @handle: File handle
 * @val: Value to store
 * @err: Location to store error info or %NULL
 *
 * Write an unsigned 16 bit integer to a file and performs endian
 * byte swapping if necessary.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_file_write_u16 (IpatchFileHandle *handle, guint16 val, GError **err)
{
  g_return_val_if_fail (handle != NULL, FALSE);
  g_return_val_if_fail (IPATCH_IS_FILE (handle->file), FALSE);

  val = IPATCH_FILE_SWAP16 (handle->file, &val);
  if (!ipatch_file_write (handle, &val, sizeof (guint16), err)) return (FALSE);
  return (TRUE);
}

/**
 * ipatch_file_write_u32:
 * @handle: File handle
 * @val: Value to store
 * @err: Location to store error info or %NULL
 *
 * Write an unsigned 32 bit integer to a file and performs endian
 * byte swapping if necessary.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_file_write_u32 (IpatchFileHandle *handle, guint32 val, GError **err)
{
  g_return_val_if_fail (handle != NULL, FALSE);
  g_return_val_if_fail (IPATCH_IS_FILE (handle->file), FALSE);

  val = IPATCH_FILE_SWAP32 (handle->file, &val);
  if (!ipatch_file_write (handle, &val, sizeof (guint32), err)) return (FALSE);
  return (TRUE);
}

/**
 * ipatch_file_write_u64:
 * @handle: File handle
 * @val: Value to store
 * @err: Location to store error info or %NULL
 *
 * Write an unsigned 64 bit integer to a file and performs endian
 * byte swapping if necessary.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_file_write_u64 (IpatchFileHandle *handle, guint64 val, GError **err)
{
  g_return_val_if_fail (handle != NULL, FALSE);
  g_return_val_if_fail (IPATCH_IS_FILE (handle->file), FALSE);

  val = IPATCH_FILE_SWAP64 (handle->file, &val);
  if (!ipatch_file_write (handle, &val, sizeof (guint64), err)) return (FALSE);
  return (TRUE);
}

/**
 * ipatch_file_write_s8:
 * @handle: File handle
 * @val: Value to store
 * @err: Location to store error info or %NULL
 *
 * Write a signed 8 bit integer to a file.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_file_write_s8 (IpatchFileHandle *handle, gint8 val, GError **err)
{
  return (ipatch_file_write (handle, &val, sizeof (gint8), err));
}

/**
 * ipatch_file_write_s16:
 * @handle: File handle
 * @val: Value to store
 * @err: Location to store error info or %NULL
 *
 * Write a signed 16 bit integer to a file and performs endian
 * byte swapping if necessary.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_file_write_s16 (IpatchFileHandle *handle, gint16 val, GError **err)
{
  g_return_val_if_fail (handle != NULL, FALSE);
  g_return_val_if_fail (IPATCH_IS_FILE (handle->file), FALSE);

  val = IPATCH_FILE_SWAP16 (handle->file, &val);
  if (!ipatch_file_write (handle, &val, sizeof (gint16), err)) return (FALSE);
  return (TRUE);
}

/**
 * ipatch_file_write_s32:
 * @handle: File handle
 * @val: Value to store
 * @err: Location to store error info or %NULL
 *
 * Write a signed 32 bit integer to a file and performs endian
 * byte swapping if necessary.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_file_write_s32 (IpatchFileHandle *handle, gint32 val, GError **err)
{
  g_return_val_if_fail (handle != NULL, FALSE);
  g_return_val_if_fail (IPATCH_IS_FILE (handle->file), FALSE);

  val = IPATCH_FILE_SWAP32 (handle->file, &val);
  if (!ipatch_file_write (handle, &val, sizeof (gint32), err)) return (FALSE);
  return (TRUE);
}

/**
 * ipatch_file_write_s64:
 * @handle: File handle
 * @val: Value to store
 * @err: Location to store error info or %NULL
 *
 * Write a signed 64 bit integer to a file and performs endian
 * byte swapping if necessary.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_file_write_s64 (IpatchFileHandle *handle, gint64 val, GError **err)
{
  g_return_val_if_fail (handle != NULL, FALSE);
  g_return_val_if_fail (IPATCH_IS_FILE (handle->file), FALSE);

  val = IPATCH_FILE_SWAP64 (handle->file, &val);
  if (!ipatch_file_write (handle, &val, sizeof (gint64), err)) return (FALSE);
  return (TRUE);
}

/**
 * ipatch_file_buf_load:
 * @handle: File handle
 * @size: Size of data to load
 * @err: Location to store error info or %NULL
 *
 * Load data from a file into a buffer for error checking convenience.
 * I/O errors need only be checked on this function and not on the subsequent
 * buffered read function calls.  It is an error if an end of file is
 * encountered before all the requested data is read.
 *
 * Returns: %TRUE on success, %FALSE on error.
 */
gboolean
ipatch_file_buf_load (IpatchFileHandle *handle, guint size, GError **err)
{
  g_return_val_if_fail (handle != NULL, FALSE);
  g_return_val_if_fail (size != 0, FALSE);

  /* If there is still buffered data, flush it (add to file position) */
  if (handle->buf_position < handle->buf->len)
    handle->position += handle->buf->len - handle->buf_position;

  g_byte_array_set_size (handle->buf, size);
  handle->buf_position = 0;

  if (!_ipatch_file_read_no_pos_update (handle, handle->buf->data, size, err))
    return (FALSE);

  return (TRUE);
}

/**
 * ipatch_file_buf_read:
 * @handle: File handle with loaded data to read from
 * @buf: Buffer to copy data to
 * @size: Amount of data to copy in bytes
 *
 * Read data from a file handle's buffer and advance the buffer's current
 * position.  A call to ipatch_file_buf_load() must have been previously
 * executed and there must be enough remaining data in the buffer for the read.
 */
void
ipatch_file_buf_read (IpatchFileHandle *handle, gpointer buf, guint size)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (handle->buf_position + size <= handle->buf->len);

  memcpy (buf, handle->buf->data + handle->buf_position, size);
  handle->buf_position += size;
  handle->position += size;
}

/**
 * ipatch_file_buf_write:
 * @handle: File handle to append buffered data to
 * @buf: Buffer to copy data from
 * @size: Amount of data to copy in bytes
 *
 * Write data to a file handle's buffer and advance the buffer's current
 * position. Buffer is expanded if necessary (since version 1.1.0).
 * Data will not actually be written to file till ipatch_file_buf_commit() is
 * called.
 */
void
ipatch_file_buf_write (IpatchFileHandle *handle, gconstpointer buf, guint size)
{
  g_return_if_fail (handle != NULL);

  if (size == 0) return;

  if (handle->buf_position + size > handle->buf->len)
    g_byte_array_set_size (handle->buf, handle->buf_position + size);

  memcpy (handle->buf->data + handle->buf_position, buf, size);
  handle->buf_position += size;
  handle->position += size;
}

/**
 * ipatch_file_buf_memset:
 * @handle: File handle to set buffered data of
 * @c: Character to write
 * @size: Size of data to set
 *
 * Sets the given @size in bytes to the character @c and advances the
 * current position.  Buffer is expanded if necessary.
 */
void
ipatch_file_buf_memset (IpatchFileHandle *handle, char c, guint size)
{
  g_return_if_fail (handle != NULL);

  if (size == 0) return;

  if (handle->buf_position + size > handle->buf->len)
    g_byte_array_set_size (handle->buf, handle->buf_position + size);

  memset (handle->buf->data + handle->buf_position, c, size);
  handle->buf_position += size;
  handle->position += size;
}

/**
 * ipatch_file_buf_set_size:
 * @handle: File handle to adjust buffer of
 * @size: New size of buffer
 *
 * Sets the size of the buffer of @handle to @size bytes. The buffer is
 * expanded without initializing the newly allocated part or truncated
 * as necessary discarding any content over the new size. The current position
 * is updated to point to the end of the buffer if it would point outside the
 * new size of the buffer after truncating it.
 *
 * Since: 1.1.0
 */
void
ipatch_file_buf_set_size (IpatchFileHandle *handle, guint size)
{
  g_return_if_fail (handle != NULL);

  if (size == handle->buf->len) return;

  g_byte_array_set_size (handle->buf, size);
  if (handle->buf_position > size)
  {
    handle->position += size - handle->buf_position;
    handle->buf_position = size;
  }
}

/**
 * ipatch_file_buf_commit:
 * @handle: File handle with buffered data to write
 * @err: Location to store error info or %NULL
 *
 * Writes all data in a file handle's buffer to the file and resets the
 * buffer to empty.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
ipatch_file_buf_commit (IpatchFileHandle *handle, GError **err)
{
  g_return_val_if_fail (handle != NULL, FALSE);

  if (handle->buf->len == 0) return (TRUE); /* no data? */

  if (!_ipatch_file_write_no_pos_update (handle, handle->buf->data, handle->buf->len, err))
    return (FALSE);

  g_byte_array_set_size (handle->buf, 0);
  handle->buf_position = 0;

  return (TRUE);
}

/**
 * ipatch_file_buf_seek: 
 * @handle: File handle
 * @offset: Offset to seek
 * @type: Seek type
 *
 * Seeks the current position in a file handle's buffer specified by an @offset
 * and seek @type.  It is an error to seek outside of the current size of
 * buffered data (loaded or written).  The @offset is relative to the buffer,
 * not the file position.
 */
void
ipatch_file_buf_seek (IpatchFileHandle *handle, int offset, GSeekType type)
{
  g_return_if_fail (handle != NULL);

  if (type == G_SEEK_CUR)
  {
    g_return_if_fail (handle->buf_position + offset >= 0);
    g_return_if_fail (handle->buf_position + offset < handle->buf->len);
    handle->buf_position += offset;
    handle->position += offset;
  }
  else if (type == G_SEEK_SET)
  {
    g_return_if_fail (offset >= 0 && offset < handle->buf->len);
    handle->position += offset - handle->buf_position;
    handle->buf_position = offset;
  }
  else if (type == G_SEEK_END)
  {
    g_return_if_fail (handle->buf->len + offset >= 0);
    g_return_if_fail (handle->buf->len + offset < handle->buf->len);
    handle->position = (handle->buf->len + offset) - handle->buf_position;
    handle->buf_position = handle->buf->len + offset;
  }
}

/**
 * ipatch_file_buf_read_u8:
 * @handle: File handle with loaded data
 *
 * Reads an unsigned byte from a file buffer and advances the buffer's
 * current position.
 *
 * Returns: The value.
 */
guint8
ipatch_file_buf_read_u8 (IpatchFileHandle *handle)
{
  guint8 *val;

  g_return_val_if_fail (handle != NULL, 0);
  g_return_val_if_fail (handle->buf_position + 1 <= handle->buf->len, 0);

  val = (guint8 *)(handle->buf->data + handle->buf_position);
  handle->buf_position++;
  handle->position++;
  return (*val);
}

/**
 * ipatch_file_buf_read_u16:
 * @handle: File handle with loaded data
 *
 * Reads an unsigned 16 bit word from a file buffer and advances the buffer's
 * current position. Performs endian byte swapping if necessary.
 *
 * Returns: The value.
 */
guint16
ipatch_file_buf_read_u16 (IpatchFileHandle *handle)
{
  guint16 *val;

  g_return_val_if_fail (handle != NULL, 0);
  g_return_val_if_fail (handle->buf_position + 2 <= handle->buf->len, 0);

  val = (guint16 *)(handle->buf->data + handle->buf_position);
  handle->buf_position += 2;
  handle->position += 2;
  return (IPATCH_FILE_SWAP16 (handle->file, val));
}

/**
 * ipatch_file_buf_read_u32:
 * @handle: File handle with loaded data
 *
 * Reads an unsigned 32 bit word from a file buffer and advances the buffer's
 * current position. Performs endian byte swapping if necessary.
 *
 * Returns: The value.
 */
guint32
ipatch_file_buf_read_u32 (IpatchFileHandle *handle)
{
  guint32 *val;

  g_return_val_if_fail (handle != NULL, 0);
  g_return_val_if_fail (handle->buf_position + 4 <= handle->buf->len, 0);

  val = (guint32 *)(handle->buf->data + handle->buf_position);
  handle->buf_position += 4;
  handle->position += 4;
  return (IPATCH_FILE_SWAP32 (handle->file, val));
}

/**
 * ipatch_file_buf_read_u64:
 * @handle: File handle with loaded data
 *
 * Reads an unsigned 64 bit word from a file buffer and advances the buffer's
 * current position. Performs endian byte swapping if necessary.
 *
 * Returns: The value.
 */
guint64
ipatch_file_buf_read_u64 (IpatchFileHandle *handle)
{
  guint64 *val;

  g_return_val_if_fail (handle != NULL, 0);
  g_return_val_if_fail (handle->buf_position + 8 <= handle->buf->len, 0);

  val = (guint64 *)(handle->buf->data + handle->buf_position);
  handle->buf_position += 8;
  handle->position += 8;
  return (IPATCH_FILE_SWAP64 (handle->file, val));
}

/**
 * ipatch_file_buf_read_s8:
 * @handle: File handle with loaded data
 *
 * Reads a signed byte from a file buffer and advances the buffer's
 * current position.
 *
 * Returns: The value.
 */
gint8
ipatch_file_buf_read_s8 (IpatchFileHandle *handle)
{
  gint8 *val;

  g_return_val_if_fail (handle != NULL, 0);
  g_return_val_if_fail (handle->buf_position + 1 <= handle->buf->len, 0);

  val = (gint8 *)(handle->buf->data + handle->buf_position);
  handle->buf_position++;
  handle->position++;
  return (*val);
}

/**
 * ipatch_file_buf_read_s16:
 * @handle: File handle with loaded data
 *
 * Reads a signed 16 bit word from a file buffer and advances the buffer's
 * current position. Performs endian byte swapping if necessary.
 *
 * Returns: The value.
 */
gint16
ipatch_file_buf_read_s16 (IpatchFileHandle *handle)
{
  gint16 *val;

  g_return_val_if_fail (handle != NULL, 0);
  g_return_val_if_fail (handle->buf_position + 2 <= handle->buf->len, 0);

  val = (gint16 *)(handle->buf->data + handle->buf_position);
  handle->buf_position += 2;
  handle->position += 2;
  return (IPATCH_FILE_SWAP16 (handle->file, val));
}

/**
 * ipatch_file_buf_read_s32:
 * @handle: File handle with loaded data
 *
 * Reads a signed 32 bit word from a file buffer and advances the buffer's
 * current position. Performs endian byte swapping if necessary.
 *
 * Returns: The value.
 */
gint32
ipatch_file_buf_read_s32 (IpatchFileHandle *handle)
{
  gint32 *val;

  g_return_val_if_fail (handle != NULL, 0);
  g_return_val_if_fail (handle->buf_position + 4 <= handle->buf->len, 0);

  val = (gint32 *)(handle->buf->data + handle->buf_position);
  handle->buf_position += 4;
  handle->position += 4;
  return (IPATCH_FILE_SWAP32 (handle->file, val));
}

/**
 * ipatch_file_buf_read_s64:
 * @handle: File handle with loaded data
 *
 * Reads a signed 64 bit word from a file buffer and advances the buffer's
 * current position. Performs endian byte swapping if necessary.
 *
 * Returns: The value.
 */
gint64
ipatch_file_buf_read_s64 (IpatchFileHandle *handle)
{
  gint64 *val;

  g_return_val_if_fail (handle != NULL, 0);
  g_return_val_if_fail (handle->buf_position + 8 <= handle->buf->len, 0);

  val = (gint64 *)(handle->buf->data + handle->buf_position);
  handle->buf_position += 8;
  handle->position += 8;
  return (IPATCH_FILE_SWAP64 (handle->file, val));
}

/**
 * ipatch_file_buf_write_u8:
 * @handle: File handle
 * @val: Value to write into file buffer
 *
 * Writes an unsigned byte to a file buffer and advances the buffer's
 * current position. The file buffer is expanded if needed.
 */
void
ipatch_file_buf_write_u8 (IpatchFileHandle *handle, guint8 val)
{
  g_return_if_fail (handle != NULL);

  if (handle->buf_position + 1 > handle->buf->len)
    g_byte_array_set_size (handle->buf, handle->buf_position + 1);

  *(guint8 *)(handle->buf->data + handle->buf_position) = val;
  handle->buf_position++;
  handle->position++;
}

/**
 * ipatch_file_buf_write_u16:
 * @handle: File handle
 * @val: Value to write into file buffer
 *
 * Writes an unsigned 16 bit word to a file buffer and advances the buffer's
 * current position. Performs endian byte swapping if necessary.
 * The file buffer is expanded if needed.
 */
void
ipatch_file_buf_write_u16 (IpatchFileHandle *handle, guint16 val)
{
  g_return_if_fail (handle != NULL);

  if (handle->buf_position + 2 > handle->buf->len)
    g_byte_array_set_size (handle->buf, handle->buf_position + 2);

  *(guint16 *)(handle->buf->data + handle->buf_position)
    = IPATCH_FILE_SWAP16 (handle->file, &val);
  handle->buf_position += 2;
  handle->position += 2;
}

/**
 * ipatch_file_buf_write_u32:
 * @handle: File handle
 * @val: Value to write into file buffer
 *
 * Writes an unsigned 32 bit word to a file buffer and advances the buffer's
 * current position. Performs endian byte swapping if necessary.
 * The file buffer is expanded if needed.
 */
void
ipatch_file_buf_write_u32 (IpatchFileHandle *handle, guint32 val)
{
  g_return_if_fail (handle != NULL);

  if (handle->buf_position + 4 > handle->buf->len)
    g_byte_array_set_size (handle->buf, handle->buf_position + 4);

  *(guint32 *)(handle->buf->data + handle->buf_position)
    = IPATCH_FILE_SWAP32 (handle->file, &val);
  handle->buf_position += 4;
  handle->position += 4;
}

/**
 * ipatch_file_buf_write_u64:
 * @handle: File handle
 * @val: Value to write into file buffer
 *
 * Writes an unsigned 64 bit word to a file buffer and advances the buffer's
 * current position. Performs endian byte swapping if necessary.
 * The file buffer is expanded if needed.
 */
void
ipatch_file_buf_write_u64 (IpatchFileHandle *handle, guint64 val)
{
  g_return_if_fail (handle != NULL);

  if (handle->buf_position + 8 > handle->buf->len)
    g_byte_array_set_size (handle->buf, handle->buf_position + 8);

  *(guint64 *)(handle->buf->data + handle->buf_position)
    = IPATCH_FILE_SWAP64 (handle->file, &val);
  handle->buf_position += 8;
  handle->position += 8;
}

/**
 * ipatch_file_buf_write_s8:
 * @handle: File handle
 * @val: Value to write into file buffer
 *
 * Writes a signed byte to a file buffer and advances the buffer's
 * current position. The file buffer is expanded if needed.
 */
void
ipatch_file_buf_write_s8 (IpatchFileHandle *handle, gint8 val)
{
  g_return_if_fail (handle != NULL);

  if (handle->buf_position + 1 > handle->buf->len)
    g_byte_array_set_size (handle->buf, handle->buf_position + 1);

  *(gint8 *)(handle->buf->data + handle->buf_position) = val;
  handle->buf_position++;
  handle->position++;
}

/**
 * ipatch_file_buf_write_s16:
 * @handle: File handle
 * @val: Value to write into file buffer
 *
 * Writes a signed 16 bit word to a file buffer and advances the buffer's
 * current position. Performs endian byte swapping if necessary.
 * The file buffer is expanded if needed.
 */
void
ipatch_file_buf_write_s16 (IpatchFileHandle *handle, gint16 val)
{
  g_return_if_fail (handle != NULL);

  if (handle->buf_position + 2 > handle->buf->len)
    g_byte_array_set_size (handle->buf, handle->buf_position + 2);

  *(gint16 *)(handle->buf->data + handle->buf_position)
    = IPATCH_FILE_SWAP16 (handle->file, &val);
  handle->buf_position += 2;
  handle->position += 2;
}

/**
 * ipatch_file_buf_write_s32:
 * @handle: File handle
 * @val: Value to write into file buffer
 *
 * Writes a signed 32 bit word to a file buffer and advances the buffer's
 * current position. Performs endian byte swapping if necessary.
 * The file buffer is expanded if needed.
 */
void
ipatch_file_buf_write_s32 (IpatchFileHandle *handle, gint32 val)
{
  g_return_if_fail (handle != NULL);

  if (handle->buf_position + 4 > handle->buf->len)
    g_byte_array_set_size (handle->buf, handle->buf_position + 4);

  *(gint32 *)(handle->buf->data + handle->buf_position)
    = IPATCH_FILE_SWAP32 (handle->file, &val);
  handle->buf_position += 4;
  handle->position += 4;
}

/**
 * ipatch_file_buf_write_s64:
 * @handle: File handle
 * @val: Value to write into file buffer
 *
 * Writes a signed 64 bit word to a file buffer and advances the buffer's
 * current position. Performs endian byte swapping if necessary.
 * The file buffer is expanded if needed.
 */
void
ipatch_file_buf_write_s64 (IpatchFileHandle *handle, gint64 val)
{
  g_return_if_fail (handle != NULL);

  if (handle->buf_position + 8 > handle->buf->len)
    g_byte_array_set_size (handle->buf, handle->buf_position + 8);

  *(gint64 *)(handle->buf->data + handle->buf_position)
    = IPATCH_FILE_SWAP64 (handle->file, &val);
  handle->buf_position += 8;
  handle->position += 8;
}
