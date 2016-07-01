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
 * SECTION: IpatchSampleTransform
 * @short_description: Audio format conversion instance
 * @see_also: 
 * @stability: Stable
 *
 * A structure for converting between audio formats (for example the bit width
 * or number of channels).  This structure is initialized with the source and
 * destination audio formats, multi-channel mapping and conversion buffers.
 */
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchSampleTransform.h"
#include "sample.h"
#include "ipatch_priv.h"


G_LOCK_DEFINE_STATIC (transform_pool);
static GList *transform_pool = NULL;


GType
ipatch_sample_transform_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_boxed_type_register_static ("IpatchSampleTransform",
				(GBoxedCopyFunc)ipatch_sample_transform_duplicate,
				(GBoxedFreeFunc)ipatch_sample_transform_free);

  return (type);
}

/**
 * ipatch_sample_transform_new:
 * @src_format: Source sample format or 0 to set later
 * @dest_format: Destination sample format or 0 to set later
 * @channel_map: Channel mapping (use #IPATCH_SAMPLE_UNITY_CHANNEL_MAP
 *   to map all input channels to the same output channels, see
 *   #IPATCH_SAMPLE_MAP_CHANNEL macro for constructing channel map values)
 * 
 * Create a new sample transform object. If @src_format and @dest_format are
 * not 0 then the transform is initialized for the given source and destination
 * formats, otherwise they are expected to be set later with
 * ipatch_sample_transform_set_formats().
 * 
 * Returns: New allocated sample transform structure which should be freed with
 *   ipatch_sample_transform_free() when done with it.
 */
IpatchSampleTransform *
ipatch_sample_transform_new (int src_format, int dest_format, guint32 channel_map)
{
  IpatchSampleTransform *trans;
  int i;

  trans = g_slice_new0 (IpatchSampleTransform);

  /* Setup straight through 1:1 channel mapping */
  for (i = 0; i < 8; i++)
    trans->channel_map[i] = i;

  if (src_format != 0 && dest_format != 0)
    ipatch_sample_transform_set_formats (trans, src_format, dest_format, channel_map);

  return (trans);
}

/**
 * ipatch_sample_transform_free:
 * @transform: Transform structure
 *
 * Free a transform structure as allocated by ipatch_sample_transform_new() and
 * its allocated resources.
 */
void
ipatch_sample_transform_free (IpatchSampleTransform *transform)
{
  g_return_if_fail (transform != NULL);

  if (transform->free_buffers) g_free (transform->buf1);
  g_slice_free (IpatchSampleTransform, transform);
}

/**
 * ipatch_sample_transform_duplicate:
 * @transform: Transform structure
 *
 * Duplicate a sample transform.
 *
 * Returns: New duplicated sample transform structure.
 *
 * Since: 1.1.0
 */
IpatchSampleTransform *
ipatch_sample_transform_duplicate (const IpatchSampleTransform *transform)
{
  IpatchSampleTransform *new;
  guint32 channel_map = 0;
  int i;

  // Convert channel map byte array to guint32
  for (i = 0; i < 8; i++)
    channel_map |= (transform->channel_map[i] & 0x07) << (i * 3);

  new = ipatch_sample_transform_new (transform->src_format, transform->dest_format,
                                     channel_map);
  if (transform->max_frames > 0)
    ipatch_sample_transform_alloc (new, transform->max_frames);

  return (new);
}

/**
 * ipatch_sample_transform_init: (skip)
 * @transform: Sample transform to initialize
 *
 * Initialize a sample transform structure. Usually only used to initialize transform
 * structures allocated on the stack, which is done to avoid mallocs.
 */
void
ipatch_sample_transform_init (IpatchSampleTransform *transform)
{
  int i;

  g_return_if_fail (transform != NULL);

  memset (transform, 0, sizeof (IpatchSampleTransform));

  /* Setup straight through 1:1 channel mapping */
  for (i = 0; i < 8; i++)
    transform->channel_map[i] = i;
}

/**
 * ipatch_sample_transform_pool_acquire: (skip)
 * @src_format: Source sample format
 * @dest_format: Destination sample format
 * @channel_map: Channel mapping (use #IPATCH_SAMPLE_UNITY_CHANNEL_MAP
 *   to map all input channels to the same output channels, see
 *   #IPATCH_SAMPLE_MAP_CHANNEL macro for constructing channel map values)
 *
 * Get an unused sample transform object from the sample transform pool.
 * Used for quickly getting a transform object for temporary use without the
 * overhead of allocating one.  Note though, that if no more transforms exist
 * in the pool an allocation will occur.
 *
 * Returns: Sample transform object.  Should be released after use with
 *   ipatch_sample_transform_pool_release().
 */
IpatchSampleTransform *
ipatch_sample_transform_pool_acquire (int src_format, int dest_format,
                                      guint32 channel_map)
{
  IpatchSampleTransform *trans = NULL;

  g_return_val_if_fail (ipatch_sample_format_transform_verify (src_format, dest_format, channel_map), NULL);

  G_LOCK (transform_pool);

  if (transform_pool)
  {
    trans = (IpatchSampleTransform *)(transform_pool->data);
    transform_pool = g_list_delete_link (transform_pool, transform_pool);
  }

  G_UNLOCK (transform_pool);

  if (!trans)
  {
    trans = ipatch_sample_transform_new (src_format, dest_format, channel_map);
    ipatch_sample_transform_alloc_size (trans, IPATCH_SAMPLE_TRANS_BUFFER_SIZE);
  }
  else ipatch_sample_transform_set_formats (trans, src_format, dest_format, channel_map);

  return (trans);
}

/**
 * ipatch_sample_transform_pool_release: (skip)
 * @transform: Sample transform
 *
 * Release a sample transform object, returned by
 * ipatch_sample_transform_pool_grab(), back to the transform pool.
 */
void
ipatch_sample_transform_pool_release (IpatchSampleTransform *transform)
{
  g_return_if_fail (transform != NULL);

  G_LOCK (transform_pool);
  transform_pool = g_list_prepend (transform_pool, transform);
  G_UNLOCK (transform_pool);
}

/**
 * ipatch_sample_transform_set_formats:
 * @transform: Transform object
 * @src_format: Source audio format to convert from
 * @dest_format: Destination audio format to convert to
 * @channel_map: Channel mapping (use #IPATCH_SAMPLE_UNITY_CHANNEL_MAP
 *   to map all input channels to the same output channels, see
 *   #IPATCH_SAMPLE_MAP_CHANNEL macro for constructing channel map values)
 * 
 * Initialize a sample transform object for converting from
 * @src_format to @dest_format.
 */
void
ipatch_sample_transform_set_formats (IpatchSampleTransform *transform,
				     int src_format, int dest_format,
                                     guint32 channel_map)
{
  guint buf1_max_frame, buf2_max_frame, func_count;
  int i, chans;

  g_return_if_fail (transform != NULL);
  g_return_if_fail (ipatch_sample_format_transform_verify (src_format, dest_format, channel_map));

  transform->src_format = src_format;
  transform->dest_format = dest_format;

  /* Convert channel map integer to byte array */
  for (i = 0; i < 8; i++)
    transform->channel_map[i] = (channel_map >> (i * 3)) & 0x07;

  transform->func_count = 0;

  if (src_format == dest_format)        /* Shortcut identical formats */
    {
      chans = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (src_format);

      /* Straight through channel mapping? */
      for (i = 0; i < chans; i++)
        if (transform->channel_map[i] != i) break;

      if (i == chans)
      {
        transform->buf1_max_frame = ipatch_sample_format_size (src_format);
        transform->buf2_max_frame = 0;
        transform->max_frames = transform->combined_size
          ? transform->combined_size / transform->buf1_max_frame : 0;
        return;
      }
    }

  func_count = ipatch_sample_get_transform_funcs
    (src_format, dest_format, channel_map, &buf1_max_frame, &buf2_max_frame,
     transform->funcs);

  transform->buf1_max_frame = buf1_max_frame;
  transform->buf2_max_frame = buf2_max_frame;
  transform->func_count = func_count;

  /* update max frames if buffer is already assigned */
  if (transform->combined_size)
  {
    transform->max_frames =
      transform->combined_size / (buf1_max_frame + buf2_max_frame);

    transform->buf2 = transform->buf1 + (transform->buf1_max_frame
					 * transform->max_frames);
  }
  else transform->max_frames = 0;
}

/**
 * ipatch_sample_transform_alloc:
 * @transform: Sample transform object
 * @frames: Number of frames to allocate for (max frames processed in batch)
 * 
 * Allocate buffers for transforming between two audio formats, for
 * which @transform has previously been initialized for with
 * ipatch_sample_transform_new() or ipatch_sample_transform_set_formats().
 *
 * Note: Assigning buffers with this function allows sample formats to be
 * changed without re-assigning the buffers.
 */
void
ipatch_sample_transform_alloc (IpatchSampleTransform *transform, guint frames)
{
  g_return_if_fail (transform != NULL);
  g_return_if_fail (frames > 0);
  g_return_if_fail (transform->src_format != 0);

  if (transform->free_buffers) g_free (transform->buf1);

  transform->combined_size =
    (transform->buf1_max_frame + transform->buf2_max_frame) * frames;

  transform->buf1 = g_malloc (transform->combined_size);
  transform->buf2 = transform->buf1 + (transform->buf1_max_frame * frames);
  transform->free_buffers = TRUE;
  transform->max_frames = frames;
}

/**
 * ipatch_sample_transform_alloc_size:
 * @transform: Sample transform object
 * @size: Maximum total size in bytes of allocation for both transform buffers.
 * 
 * Like ipatch_sample_transform_alloc() but allocates buffers based on a
 * maximum size and returns the maximum number of sample frames which can be
 * converted at a time using this size.  Another difference is that conversion
 * formats do not need to be set before calling this function.
 *
 * Returns: The maximum number of frames that can be converted at a time for
 * the given @size of transform buffers.  If conversion formats have not yet
 * been set, then this function returns 0.
 */
int
ipatch_sample_transform_alloc_size (IpatchSampleTransform *transform,
				    guint size)
{
  g_return_val_if_fail (transform != NULL, 0);
  g_return_val_if_fail (size > 32, 0);	/* just a somewhat sane value */

  if (transform->free_buffers) g_free (transform->buf1);

  transform->combined_size = size;
  transform->buf1 = g_malloc (size);
  transform->free_buffers = TRUE;
  transform->buf2 = NULL;
  transform->max_frames = 0;

  /* update buffer split if formats already assigned */
  if (transform->src_format && transform->dest_format)
  {
    transform->max_frames
      = size / (transform->buf1_max_frame + transform->buf2_max_frame);
    transform->buf2 = transform->buf1 + (transform->buf1_max_frame
					 * transform->max_frames);
  }

  return (transform->max_frames);
}

/**
 * ipatch_sample_transform_free_buffers:
 * @transform: Sample transform object
 * 
 * Free sample transform buffers.
 */
void
ipatch_sample_transform_free_buffers (IpatchSampleTransform *transform)
{
  g_return_if_fail (transform != NULL);

  if (transform->free_buffers)
    g_free (transform->buf1);

  transform->buf1 = NULL;
  transform->buf2 = NULL;
  transform->combined_size = 0;
  transform->max_frames = 0;
}

/**
 * ipatch_sample_transform_set_buffers_size:
 * @transform: Sample transform object
 * @buf: (array length=size) (element-type guint8) (transfer none): Buffer to
 *    assign for transforming sample formats
 * @size: Size of @buf in bytes
 * 
 * Assign transform buffers using a single buffer of a
 * specific size in bytes and determine optimal division for source and
 * destination buffers.  Conversion formats must not be set before calling this
 * function (can be set later).
 *
 * Returns: The maximum number of frames that can be converted at a time for
 * the given @size of transform buffers.  If conversion formats have not yet
 * been set, then this function returns 0.
 */
guint
ipatch_sample_transform_set_buffers_size (IpatchSampleTransform *transform,
					  gpointer buf, guint size)
{
  g_return_val_if_fail (transform != NULL, 0);
  g_return_val_if_fail (buf != NULL, 0);
  g_return_val_if_fail (size > 32, 0);		/* some slightly sane value */

  if (transform->free_buffers) g_free (transform->buf1);

  transform->buf1 = buf;
  transform->free_buffers = FALSE;
  transform->combined_size = size;
  transform->buf2 = NULL;
  transform->max_frames = 0;

  /* update buffer split if formats already assigned */
  if (transform->src_format && transform->dest_format)
  {
    transform->max_frames
      = size / (transform->buf1_max_frame + transform->buf2_max_frame);
    transform->buf2 = transform->buf1 + (transform->buf1_max_frame
					 * transform->max_frames);
  }

  return (transform->max_frames);
}

/**
 * ipatch_sample_transform_get_buffers: (skip)
 * @transform: Sample transform object
 * @buf1: (out) (optional): First buffer or %NULL to ignore
 * @buf2: (out) (optional): Second buffer or %NULL to ignore
 * 
 * Get the sample data buffers in a sample transform object.
 */
void
ipatch_sample_transform_get_buffers (IpatchSampleTransform *transform,
				     gpointer *buf1, gpointer *buf2)
{
  g_return_if_fail (transform != NULL);

  if (buf1) *buf1 = transform->buf1;
  if (buf2) *buf2 = transform->buf2;
}

/**
 * ipatch_sample_transform_get_frame_sizes:
 * @transform: Initialized transform object
 * @buf1_size: (out) (optional): Maximum frame size for buffer 1 or %NULL to ignore
 * @buf2_size: (out) (optional): Maximum frame size for buffer 2 or %NULL to ignore
 * 
 * Get max frame sizes for transform buffers. When transforming audio the
 * first buffer must be at least frames * buf1_size bytes in size and the
 * second buffer must be at least frames * buf2_size, where frames is the
 * max number of frames to convert in batch.
 */
void
ipatch_sample_transform_get_frame_sizes (IpatchSampleTransform *transform,
					 guint *buf1_size, guint *buf2_size)
{
  g_return_if_fail (transform != NULL);

  if (buf1_size) *buf1_size = transform->buf1_max_frame;
  if (buf2_size) *buf2_size = transform->buf2_max_frame;
}

/**
 * ipatch_sample_transform_get_max_frames:
 * @transform: Sample transform object
 * 
 * Get the maximum frames that the @transform object can convert at a time.
 *
 * Returns: Max number of frames that can be converted at a time, 0 if buffers
 *   have not been allocated yet.
 */
guint
ipatch_sample_transform_get_max_frames (IpatchSampleTransform *transform)
{
  g_return_val_if_fail (transform != NULL, 0);
  return (transform->max_frames);
}

/**
 * ipatch_sample_transform_convert: (skip)
 * @transform: An initialized sample transform object
 * @src: Audio source buffer (@frames X the source frame size in bytes), can
 *   be %NULL to use internal buffer (provided @frames is less than or equal to
 *   the max frames that can be converted at a time), in which case buf1 of
 *   @transform should already be loaded with source data.
 * @dest: Converted audio destination buffer (@frames X the destination frame
 *   size in bytes), can be %NULL to use internal buffer (provided @frames is
 *   less than or equal to the max frames that can be converted at a time).
 * @frames: Number of audio frames to convert
 *
 * Convert an arbitrary number of audio frames from user provided buffers,
 * contrary to ipatch_sample_transform_convert_single() which uses internal
 * buffers and converts a max of 1 buffer at a time.  The sample formats
 * should already be assigned and internal buffers assigned or allocated.
 *
 * Returns: (transfer none): Pointer to converted data.  Will be @dest if it was not %NULL or
 *   internal buffer containing the converted data otherwise.
 */
gpointer
ipatch_sample_transform_convert (IpatchSampleTransform *transform,
				 gconstpointer src, gpointer dest, guint frames)
{
  int i, func_count, block_size;
  gpointer buf1, buf2;
  int src_frame_size, dest_frame_size, srcchan;

  g_return_val_if_fail (transform != NULL, NULL);
  g_return_val_if_fail (frames > 0, NULL);
  g_return_val_if_fail (transform->buf1 != NULL, NULL);
  g_return_val_if_fail (transform->buf2 != NULL, NULL);
  g_return_val_if_fail (transform->max_frames > 0, NULL);
  g_return_val_if_fail ((src && dest) || frames <= transform->max_frames, NULL);

  block_size = transform->max_frames;
  func_count = transform->func_count;
  buf1 = transform->buf1;
  buf2 = transform->buf2;

  if (!src) src = buf1;

  src_frame_size = ipatch_sample_format_size (transform->src_format);
  srcchan = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (transform->src_format);

  dest_frame_size = ipatch_sample_format_size (transform->dest_format);

  if (func_count == 0)	/* same format?  Just copy it */
  {
    if (dest)
    {
      memcpy (dest, src, frames * src_frame_size);
      return (dest);
    }
    else return ((gpointer)src);
  }

  while (frames > 0)
  {
    if (block_size > frames) block_size = frames;

    transform->frames = block_size;
    transform->samples = block_size * srcchan;

    /* execute first transform function with src buffer directly */
    transform->buf1 = (gpointer)src;
    transform->buf2 = (func_count == 1 && dest) ? dest : buf2;
    (*transform->funcs[0])(transform);

    /* execute remaining transform functions */
    for (i = 1; i < func_count; i++)
    {
      if (i & 1)
      {
	transform->buf1 = buf2;
	transform->buf2 = (i == (func_count - 1) && dest) ? dest : buf1;
      }
      else
      {
	transform->buf1 = buf1;
	transform->buf2 = (i == (func_count - 1) && dest) ? dest : buf2;
      }

      (*transform->funcs[i])(transform);
    }

    frames -= block_size;
    src += block_size * src_frame_size;
    if (dest) dest += block_size * dest_frame_size;
  }

  transform->buf1 = buf1;
  transform->buf2 = buf2;

  if (dest) return (dest);
  else return ((func_count & 1) ? buf2 : buf1);
}

/**
 * ipatch_sample_transform_convert_sizes: (rename-to ipatch_sample_transform_convert)
 * @transform: An initialized sample transform object
 * @src: (array length=src_size) (element-type guint8) (transfer none): Audio source buffer
 * @src_size: Size of src buffer data to convert (in bytes, must be a multiple of the source format)
 * @dest_size: (out): Location to store size of returned converted audio buffer
 *
 * Convert an arbitrary number of audio frames from user provided buffer.
 * This is like ipatch_sample_transform_convert() but friendly to GObject
 * introspection and the destination conversion buffer is allocated.  The sample formats
 * should already be assigned and internal buffers assigned or allocated.
 *
 * Returns: (array length=dest_size) (element-type guint8) (transfer full): Newly allocated
 *   converted audio data buffer.  Free with g_free() when done with it.
 *
 * Since: 1.1.0
 */
gpointer
ipatch_sample_transform_convert_sizes (IpatchSampleTransform *transform,
                                       gconstpointer src, guint src_size,
                                       guint *dest_size)
{
  int src_frame_size, dest_frame_size, frames;
  gpointer destbuf;

  g_return_val_if_fail (transform != NULL, NULL);
  g_return_val_if_fail (src_size > 0, NULL);

  src_frame_size = ipatch_sample_format_size (transform->src_format);
  g_return_val_if_fail (src_size % src_frame_size == 0, NULL);

  dest_frame_size = ipatch_sample_format_size (transform->dest_format);
  g_return_val_if_fail (dest_frame_size > 0, NULL);

  frames = src_size / src_frame_size;

  destbuf = g_malloc (dest_frame_size * frames);        // ++ alloc conversion buffer

  if (dest_size) *dest_size = dest_frame_size * frames;

  if (!ipatch_sample_transform_convert (transform, src, destbuf, frames))
  {
    g_free (destbuf);   // -- free buffer on error
    return (NULL);
  }

  return (destbuf);     // !! caller takes over converted buffer
}

/**
 * ipatch_sample_transform_convert_single: (skip)
 * @transform: An initialized sample transform object
 * @frames: Number of frames to convert (should be less than or equal to the
 *   maximum frames which can be converted at a time
 *   (see ipatch_sample_transform_get_max_frames()).
 * 
 * Convert the format of a single buffer of audio. The @transform object must
 * have had its sample formats set and buffers assigned or allocated.  Use
 * ipatch_sample_transform_convert() to convert from one buffer to another
 * regardless of the number of frames.
 * 
 * Returns: (transfer none): Pointer to converted audio data (buffer is internal to @transform
 * object).
 */
gpointer
ipatch_sample_transform_convert_single (IpatchSampleTransform *transform,
					guint frames)
{
  gpointer temp;
  int i, count;

  g_return_val_if_fail (transform != NULL, NULL);
  g_return_val_if_fail (frames > 0 && frames <= transform->max_frames, NULL);
  g_return_val_if_fail (transform->buf1 != NULL, NULL);
  g_return_val_if_fail (transform->buf2 != NULL, NULL);

  transform->frames = frames;
  transform->samples = frames;

  /* multiply frames by number of channels to get # of samples */
  transform->samples *= IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (transform->src_format);

  count = transform->func_count;

  /* execute each transform function */
  for (i = 0; i < count; i++)
    {
      (*transform->funcs[i])(transform);

      /* swap buffer pointers */
      temp = transform->buf1;
      transform->buf1 = transform->buf2;
      transform->buf2 = temp;
    }

  /* swap 1 more time if odd number of functions to set buffer order right */
  if (count & 1)
    {
      temp = transform->buf1;
      transform->buf1 = transform->buf2;
      transform->buf2 = temp;

      return (transform->buf2);
    }
  else return (transform->buf1);
}
