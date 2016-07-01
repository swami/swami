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
 * SECTION: IpatchSample
 * @short_description: Sample audio interface
 * @see_also: 
 * @stability: Stable
 *
 * This interface provides a basic API for accessing audio of sample objects.
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>	/* for g_unlink */

#include "IpatchSample.h"
#include "IpatchSndFile.h"
#include "IpatchSampleStoreSndFile.h"
#include "builtin_enums.h"
#include "sample.h"
#include "i18n.h"
#include "ipatch_priv.h"


/* some public loop type arrays for use with IpatchSample interfaces */
int ipatch_sample_loop_types_standard[] = {
  IPATCH_SAMPLE_LOOP_NONE,
  IPATCH_SAMPLE_LOOP_STANDARD,
  IPATCH_SAMPLE_LOOP_TYPE_TERM	/* terminator */
};

int ipatch_sample_loop_types_standard_release[] = {
  IPATCH_SAMPLE_LOOP_NONE,
  IPATCH_SAMPLE_LOOP_STANDARD,
  IPATCH_SAMPLE_LOOP_RELEASE,
  IPATCH_SAMPLE_LOOP_TYPE_TERM	/* terminator */
};


static void ipatch_sample_interface_init (IpatchSampleIface *sample_iface);


GType
ipatch_sample_get_type (void)
{
  static GType itype = 0;

  if (!itype)
    {
      static const GTypeInfo info =
	{
	  sizeof (IpatchSampleIface),
	  NULL,			/* base_init */
	  NULL,			/* base_finalize */
	  (GClassInitFunc) ipatch_sample_interface_init,
	  (GClassFinalizeFunc) NULL
	};

      itype = g_type_register_static (G_TYPE_INTERFACE, "IpatchSample", &info, 0);
      g_type_interface_add_prerequisite (itype, IPATCH_TYPE_ITEM);
    }

  return (itype);
}

static void
ipatch_sample_interface_init (IpatchSampleIface *sample_iface)
{
  g_object_interface_install_property (sample_iface,
	ipatch_sample_new_property_param_spec ("sample-data",
					       G_PARAM_READABLE));
  g_object_interface_install_property (sample_iface,
	ipatch_sample_new_property_param_spec ("sample-size",
					       G_PARAM_READABLE));
  g_object_interface_install_property (sample_iface,
	ipatch_sample_new_property_param_spec ("sample-format",
					       G_PARAM_READABLE));
  g_object_interface_install_property (sample_iface,
	ipatch_sample_new_property_param_spec ("sample-rate",
					       G_PARAM_READABLE));
  g_object_interface_install_property (sample_iface,
	ipatch_sample_new_property_param_spec ("loop-type",
					       G_PARAM_READABLE));
  g_object_interface_install_property (sample_iface,
	ipatch_sample_new_property_param_spec ("loop-start",
					       G_PARAM_READABLE));
  g_object_interface_install_property (sample_iface,
	ipatch_sample_new_property_param_spec ("loop-end",
					       G_PARAM_READABLE));
  g_object_interface_install_property (sample_iface,
	ipatch_sample_new_property_param_spec ("root-note",
					       G_PARAM_READABLE));
  g_object_interface_install_property (sample_iface,
	ipatch_sample_new_property_param_spec ("fine-tune",
					       G_PARAM_READABLE));
}

/**
 * ipatch_sample_get_loop_types: (skip)
 * @sample: Object with #IpatchSample interface
 *
 * Get an array of supported loop type enums for a sample object.
 *
 * Returns: -1 terminated array of #IpatchSampleLoopType values.  If no loop
 *   types are supported, then %NULL is returned.  Array is internal and should
 *   not be modified or freed.
 */
int *
ipatch_sample_get_loop_types (IpatchSample *sample)
{
  GType type;
  g_return_val_if_fail (IPATCH_IS_SAMPLE (sample), NULL);

  type = G_OBJECT_TYPE (sample);
  return (ipatch_sample_type_get_loop_types (type));
}

/**
 * ipatch_sample_type_get_loop_types: (skip)
 * @type: A GType that has a #IpatchItem interface
 *
 * Like ipatch_sample_get_loop_types() but retrieves the supported loop types
 * from an object type rather than an instance of an object.
 *
 * Returns: -1 terminated array of #IpatchSampleLoopType values.  If no loop
 *   types are supported, then %NULL is returned.  Array is internal and should
 *   not be modified or freed.
 */
int *
ipatch_sample_type_get_loop_types (GType type)
{
  GObjectClass *obj_class;
  IpatchSampleIface *iface;

  g_return_val_if_fail (g_type_is_a (type, IPATCH_TYPE_SAMPLE), NULL);

  obj_class = g_type_class_ref (type);
  iface = g_type_interface_peek (obj_class, IPATCH_TYPE_SAMPLE);
  g_type_class_unref (obj_class);

  return (iface->loop_types);
}

/**
 * ipatch_sample_get_loop_types_len: (rename-to ipatch_sample_get_loop_types)
 * @sample: Object with #IpatchSample interface
 * @len: (out) (optional): Location to store number of indeces in returned array
 *   (not including -1 terminator), can be %NULL to ignore
 *
 * Get an array of supported loop type enums for a sample object.
 *
 * Returns: (array length=len): -1 terminated array of #IpatchSampleLoopType values.
 *   If no loop types are supported, then %NULL is returned.  Array is internal and should
 *   not be modified or freed.
 *
 * Since: 1.1.0
 */
int *
ipatch_sample_get_loop_types_len (IpatchSample *sample, int *len)
{
  GType type;

  g_return_val_if_fail (IPATCH_IS_SAMPLE (sample), NULL);

  type = G_OBJECT_TYPE (sample);
  return (ipatch_sample_type_get_loop_types_len (type, len));
}

/**
 * ipatch_sample_type_get_loop_types_len:
 * @type: A GType that has a #IpatchItem interface
 * @len: (out) (optional): Location to store number of indeces in returned array
 *   (not including -1 terminator), can be %NULL to ignore
 *
 * Like ipatch_sample_get_loop_types_len() but retrieves the supported loop types
 * from an object type rather than an instance of an object.
 *
 * Returns: (array length=len): -1 terminated array of #IpatchSampleLoopType values.
 *   If no loop types are supported, then %NULL is returned.  Array is internal and should
 *   not be modified or freed.
 *
 * Since: 1.1.0
 */
int *
ipatch_sample_type_get_loop_types_len (GType type, int *len)
{
  GObjectClass *obj_class;
  IpatchSampleIface *iface;
  int *tp;

  g_return_val_if_fail (g_type_is_a (type, IPATCH_TYPE_SAMPLE), NULL);

  obj_class = g_type_class_ref (type);
  iface = g_type_interface_peek (obj_class, IPATCH_TYPE_SAMPLE);
  g_type_class_unref (obj_class);

  if (!iface->loop_types)
    return (NULL);

  if (len)
    for (*len = 0, tp = iface->loop_types; *tp != -1; *len = *len + 1);

  return (iface->loop_types);
}

/**
 * ipatch_sample_set_format:
 * @sample: Sample to set format of
 * @format: Sample format to assign to sample (see #IpatchSampleWidth, etc)
 * 
 * Set sample format of a new sample.  Should only be assigned once.  Same as
 * assigning to a sample's "sample-format" property.
 */
void
ipatch_sample_set_format (IpatchSample *sample, int format)
{
  g_return_if_fail (IPATCH_IS_SAMPLE (sample));
  g_object_set (sample, "sample-format", format, NULL);
}

/**
 * ipatch_sample_get_format:
 * @sample: Sample to get format of
 *
 * Get the sample format of a sample.  Same as getting a sample's "sample-format"
 * property.
 * 
 * Returns: Sample format integer (see #IpatchSampleWidth, etc).
 */
int
ipatch_sample_get_format (IpatchSample *sample)
{
  int format;

  g_return_val_if_fail (IPATCH_IS_SAMPLE (sample), 0);
  g_object_get (sample, "sample-format", &format, NULL);

  return (format);
}

/**
 * ipatch_sample_set_size:
 * @sample: Sample to set size of
 * @size: Size to assign (in frames)
 *
 * Set the size of a sample.  Should be done once, and only once when created.
 */
void
ipatch_sample_set_size (IpatchSample *sample, guint size)
{
  g_return_if_fail (IPATCH_IS_SAMPLE (sample));
  g_object_set (sample, "sample-size", size, NULL);
}

/**
 * ipatch_sample_get_size:
 * @sample: Sample to get size of
 * @bytes: (out) (optional): Location to store sample size in
 *   bytes (size * frame size) or %NULL to ignore
 *
 * Get the size of a sample.  Same as getting a sample's "sample-size"
 * property.
 * 
 * Returns: Sample size (in frames)
 */
guint
ipatch_sample_get_size (IpatchSample *sample, guint *bytes)
{
  guint size;

  g_return_val_if_fail (IPATCH_IS_SAMPLE (sample), 0);
  g_object_get (sample, "sample-size", &size, NULL);

  if (bytes) *bytes = size * ipatch_sample_get_frame_size (sample);

  return (size);
}

/**
 * ipatch_sample_get_frame_size:
 * @sample: Sample to get data frame size of
 * 
 * A convenience function to get size of a single sample frame for a given
 * @sample.  This is useful for determining buffer allocation sizes when
 * reading or writing data.
 *
 * Returns: Size in bytes of a single sample frame
 */
int
ipatch_sample_get_frame_size (IpatchSample *sample)
{
  g_return_val_if_fail (IPATCH_IS_SAMPLE (sample), 0);
  return (ipatch_sample_format_size (ipatch_sample_get_format (sample)));
}

/**
 * ipatch_sample_get_sample_data:
 * @sample: Sample to get sample data from
 *
 * Get sample data object from a sample.  Not every sample object supports this
 * property, in which case %NULL is returned.
 *
 * Returns: (transfer full): Sample data object of the sample or %NULL if not set or unsupported
 *   by this sample type.  Caller owns a reference to the returned object.
 */
IpatchSampleData *
ipatch_sample_get_sample_data (IpatchSample *sample)
{
  IpatchSampleData *sampledata;

  g_return_val_if_fail (IPATCH_IS_SAMPLE (sample), NULL);
  g_object_get (sample, "sample-data", &sampledata, NULL);      /* ++ ref */

  return (sampledata);  /* !! caller takes over ref */
}

/**
 * ipatch_sample_set_sample_data:
 * @sample: Sample to set sample data of
 *
 * Set sample data object of a sample.  Not every sample object supports writing
 * to this property, in which case %FALSE will be returned.
 *
 * Returns: %TRUE if the sample supports this property and it was assigned,
 *   %FALSE otherwise.
 */
gboolean
ipatch_sample_set_sample_data (IpatchSample *sample, IpatchSampleData *sampledata)
{
  GParamSpec *pspec;

  g_return_val_if_fail (IPATCH_IS_SAMPLE (sample), FALSE);
  g_return_val_if_fail (!sampledata || IPATCH_IS_SAMPLE_DATA (sampledata), FALSE);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (sample), "sample-data");
  if (!(pspec->flags & G_PARAM_WRITABLE)) return (FALSE);

  g_object_set (sample, "sample-data", sampledata, NULL);
  return (TRUE);
}

/**
 * ipatch_sample_read: (skip)
 * @sample: Sample to read from
 * @offset: Offset in frames to read from
 * @frames: Number of frames to read
 * @buf: Buffer to store sample data in (should be at least @frames *
 *   sizeof (frame), the frame size can be had from
 *   ipatch_sample_get_frame_size()).
 * @err: Location to store error info or %NULL
 *
 * Read sample data from a sample.  This is a convenience function which
 * opens/reads/closes a #IpatchSampleHandle and is therefore not as efficient
 * when making multiple accesses.  Sample data transform
 * is also not handled (see ipatch_sample_read_transform()).
 *
 * Returns: %TRUE on success, %FALSE on error (in which case
 *   @err may be set).
 */
gboolean
ipatch_sample_read (IpatchSample *sample, guint offset, guint frames,
                    gpointer buf, GError **err)
{
  IpatchSampleHandle handle;
  gpointer retval;

  g_return_val_if_fail (IPATCH_IS_SAMPLE (sample), FALSE);

  if (!ipatch_sample_handle_open (sample, &handle, 'r', 0, 0, err))
    return (FALSE);

  retval = ipatch_sample_handle_read (&handle, offset, frames, buf, err);

  ipatch_sample_handle_close (&handle);

  return (retval != NULL);
}

/**
 * ipatch_sample_read_size: (rename-to ipatch_sample_read)
 * @sample: Sample to read from
 * @offset: Offset in frames to read from
 * @size: Size of data to read in bytes
 * @err: Location to store error info or %NULL
 *
 * Read sample data from a sample. Like ipatch_sample_read() but
 * is designed to be GObject introspection friendly and returned buffer is allocated.
 *
 * Returns: (array length=size) (element-type guint8) (transfer full): Newly
 *   allocated buffer with read data, %NULL on error (in which case
 *   @err may be set). Free the buffer with g_free() when finished with it.
 *
 * Since: 1.1.0
 */
gpointer
ipatch_sample_read_size (IpatchSample *sample, guint offset, guint size, GError **err)
{
  int frame_size;
  gpointer buf;

  g_return_val_if_fail (IPATCH_IS_SAMPLE (sample), NULL);
  g_return_val_if_fail (size > 0, NULL);

  frame_size = ipatch_sample_get_frame_size (sample);
  g_return_val_if_fail (frame_size > 0, NULL);
  g_return_val_if_fail (size % frame_size == 0, NULL);

  buf = g_malloc (size);        // ++ alloc buf

  if (!ipatch_sample_read (sample, offset, size / frame_size, buf, err))
  {
    g_free (buf);               // -- free buf on error
    return (NULL);
  }

  return (buf);         // !! caller takes over
}

/**
 * ipatch_sample_write: (skip)
 * @sample: Sample to write to
 * @offset: Offset in frames to write to
 * @frames: Number of frames to write
 * @buf: Buffer of sample data to write (should be at least @frames *
 *   sizeof (frame), the frame size can be had from
 *   ipatch_sample_get_frame_size()).
 * @err: Location to store error info or %NULL
 *
 * Write sample data to a sample.  This is a convenience function which
 * opens/writes/closes a #IpatchSampleHandle and is therefore not as efficient
 * when making multiple accesses.  Sample data transform
 * is also not handled (see ipatch_sample_write_transform()).
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_sample_write (IpatchSample *sample, guint offset, guint frames,
                     gconstpointer buf, GError **err)
{
  IpatchSampleHandle handle;
  gboolean retval;

  g_return_val_if_fail (IPATCH_IS_SAMPLE (sample), FALSE);

  if (!ipatch_sample_handle_open (sample, &handle, 'w', 0, 0, err))
    return (FALSE);

  retval = ipatch_sample_handle_write (&handle, offset, frames, buf, err);

  ipatch_sample_handle_close (&handle);

  return (retval);
}

/**
 * ipatch_sample_write_size: (rename-to ipatch_sample_write)
 * @sample: Sample to write to
 * @offset: Offset in frames to write to
 * @buf: (array length=size) (element-type guint8) (transfer none): Buffer of
 *   sample data to write
 * @size: Size of buffer (must be multiple of audio frame size)
 * @err: Location to store error info or %NULL
 *
 * Write sample data to a sample.  Like ipatch_sample_write() but is designed
 * to be GObject Inspection friendly.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 *
 * Since: 1.1.0
 */
gboolean
ipatch_sample_write_size (IpatchSample *sample, guint offset,
                          gconstpointer buf, guint size, GError **err)
{
  int frame_size;

  g_return_val_if_fail (IPATCH_IS_SAMPLE (sample), FALSE);
  g_return_val_if_fail (size > 0, FALSE);

  frame_size = ipatch_sample_get_frame_size (sample);
  g_return_val_if_fail (frame_size > 0, FALSE);
  g_return_val_if_fail (size % frame_size == 0, FALSE);

  return (ipatch_sample_write (sample, offset, size / frame_size, buf, err));
}

/**
 * ipatch_sample_read_transform: (skip)
 * @sample: Sample to read from
 * @offset: Offset in frames to read from
 * @frames: Number of frames to read
 * @buf: Buffer to store sample data in (should be at least @frames *
 *   ipatch_sample_format_size() of @format).
 * @format: Format to transform sample data to (if its the same as the native
 *   format of @sample no transformation occurs)
 * @channel_map: Channel mapping if @format is set (set to 0 otherwise), use
 *   #IPATCH_SAMPLE_UNITY_CHANNEL_MAP for 1 to 1 channel mapping
 *   (see ipatch_sample_get_transform_funcs() for details).
 * @err: Location to store error info or %NULL
 *
 * Like ipatch_sample_read() but allows for sample transformation.
 *
 * Returns: %TRUE on success, %FALSE on error (in which case
 *   @err may be set).
 */
gboolean
ipatch_sample_read_transform (IpatchSample *sample, guint offset, guint frames,
                              gpointer buf, int format, guint32 channel_map,
                              GError **err)
{
  IpatchSampleHandle handle;
  gpointer retval;

  g_return_val_if_fail (IPATCH_IS_SAMPLE (sample), FALSE);

  if (!ipatch_sample_handle_open (sample, &handle, 'r', format, channel_map, err))
    return (FALSE);

  retval = ipatch_sample_handle_read (&handle, offset, frames, buf, err);

  ipatch_sample_handle_close (&handle);

  return (retval != NULL);
}

/**
 * ipatch_sample_read_transform_size: (rename-to ipatch_sample_read_transform)
 * @sample: Sample to read from
 * @offset: Offset in frames to read from
 * @size: Size of sample data to read (in bytes) after conversion
 * @format: Format to transform sample data to (if its the same as the native
 *   format of @sample no transformation occurs)
 * @channel_map: Channel mapping if @format is set (set to 0 otherwise), use
 *   #IPATCH_SAMPLE_UNITY_CHANNEL_MAP for 1 to 1 channel mapping
 *   (see ipatch_sample_get_transform_funcs() for details).
 * @err: Location to store error info or %NULL
 *
 * Like ipatch_sample_read_transform() but is GObject Introspection friendly
 * and audio buffer is allocated.
 *
 * Returns: (array length=size) (element-type guint8) (transfer full): Newly
 *   allocated buffer containing sample data or %NULL on error (in which case
 *   @err may be set).
 *
 * Since: 1.1.0
 */
gpointer
ipatch_sample_read_transform_size (IpatchSample *sample, guint offset, guint size,
                                   int format, guint32 channel_map, GError **err)
{
  int frame_size;
  gpointer buf;

  g_return_val_if_fail (IPATCH_IS_SAMPLE (sample), NULL);
  g_return_val_if_fail (size > 0, NULL);

  frame_size = ipatch_sample_format_size (format);
  g_return_val_if_fail (frame_size > 0, NULL);
  g_return_val_if_fail (size % frame_size == 0, NULL);

  buf = g_malloc (size);        // ++ alloc buf

  if (!ipatch_sample_read_transform (sample, offset, size / frame_size,
                                     buf, format, channel_map, err))
  {
    g_free (buf);               // -- free buf on error
    return (NULL);
  }

  return (buf);         // !! caller takes over
}

/**
 * ipatch_sample_write_transform: (skip)
 * @sample: Sample to write to
 * @offset: Offset in frames to write to
 * @frames: Number of frames to write
 * @buf: Buffer of sample data to write (should be at least @frames *
 *   ipatch_sample_format_size() of @format).
 * @format: Format to transform sample data from (if its the same as the native
 *   format of @sample no transformation occurs)
 * @channel_map: Channel mapping if @format is set (set to 0 otherwise), use
 *   #IPATCH_SAMPLE_UNITY_CHANNEL_MAP for 1 to 1 channel mapping
 *   (see ipatch_sample_get_transform_funcs() for details).
 * @err: Location to store error info or %NULL
 *
 * Like ipatch_sample_write() but allows for sample transformation.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_sample_write_transform (IpatchSample *sample, guint offset, guint frames,
                               gconstpointer buf, int format, guint32 channel_map,
                               GError **err)
{
  IpatchSampleHandle handle;
  gboolean retval;

  g_return_val_if_fail (IPATCH_IS_SAMPLE (sample), FALSE);

  if (!ipatch_sample_handle_open (sample, &handle, 'w', format, channel_map, err))
    return (FALSE);

  retval = ipatch_sample_handle_write (&handle, offset, frames, buf, err);

  ipatch_sample_handle_close (&handle);

  return (retval);
}

/**
 * ipatch_sample_write_transform_size: (rename-to ipatch_sample_write_transform)
 * @sample: Sample to write to
 * @offset: Offset in frames to write to
 * @buf: Buffer of sample data to write
 * @size: Size of data in @buf (must be a multiple of @format frame size)
 * @format: Format to transform sample data from (if its the same as the native
 *   format of @sample no transformation occurs)
 * @channel_map: Channel mapping if @format is set (set to 0 otherwise), use
 *   #IPATCH_SAMPLE_UNITY_CHANNEL_MAP for 1 to 1 channel mapping
 *   (see ipatch_sample_get_transform_funcs() for details).
 * @err: Location to store error info or %NULL
 *
 * Like ipatch_sample_write() but allows for sample transformation.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 *
 * Since: 1.1.0
 */
gboolean
ipatch_sample_write_transform_size (IpatchSample *sample, guint offset,
                                    gconstpointer buf, guint size, int format,
                                    guint32 channel_map, GError **err)
{
  int frame_size;

  g_return_val_if_fail (IPATCH_IS_SAMPLE (sample), FALSE);
  g_return_val_if_fail (size > 0, FALSE);

  frame_size = ipatch_sample_format_size (format);
  g_return_val_if_fail (frame_size != 0, FALSE);
  g_return_val_if_fail (size % frame_size == 0, FALSE);

  return (ipatch_sample_write_transform (sample, offset, size / frame_size,
          buf, format, channel_map, err));
}

/**
 * ipatch_sample_copy:
 * @dest_sample: Destination sample to copy data to
 * @src_sample: Source sample to copy data from
 * @channel_map: Channel mapping, use #IPATCH_SAMPLE_UNITY_CHANNEL_MAP for 1 to 1
 *   channel mapping (see ipatch_sample_get_transform_funcs() for details).
 * @err: Location to store error information or %NULL
 *
 * Copy sample data from one sample to another.  The two samples may differ
 * in format, in which case the sample data will be converted.  The
 * @dest_sample must either be the same size in frames as @src_sample or not
 * yet assigned a size.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_sample_copy (IpatchSample *dest_sample, IpatchSample *src_sample,
		    guint32 channel_map, GError **err)
{
  IpatchSampleHandle dest_handle, src_handle;
  IpatchSampleTransform *transform;
  int dest_size, src_size, thissize;
  gpointer buf;
  int src_format;
  int sizeleft, ofs;
  gboolean retval = FALSE;

  g_return_val_if_fail (IPATCH_IS_SAMPLE (dest_sample), FALSE);
  g_return_val_if_fail (IPATCH_IS_SAMPLE (src_sample), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  dest_size = ipatch_sample_get_size (dest_sample, NULL);
  src_size = ipatch_sample_get_size (src_sample, NULL);
  g_return_val_if_fail (src_size != 0, FALSE);

  /* If destination size not yet set, assign it */
  if (dest_size == 0)
  {
    dest_size = src_size;
    ipatch_sample_set_size (dest_sample, dest_size);
  }

  g_return_val_if_fail (dest_size == src_size, FALSE);

  src_format = ipatch_sample_get_format (src_sample);
  if (!ipatch_sample_handle_open (dest_sample, &dest_handle, 'w', src_format,
                                  channel_map, err))
    return (FALSE);

  if (!ipatch_sample_handle_open (src_sample, &src_handle, 'r', 0, 0, err))
  {
    ipatch_sample_handle_close (&dest_handle);
    return (FALSE);
  }

  transform = ipatch_sample_handle_get_transform (&dest_handle);  /* ++ ref */

  /* Transform should always be set, since we passed a format to ipatch_sample_handle_open */
  g_return_val_if_fail (transform != NULL, FALSE);

  thissize = ipatch_sample_transform_get_max_frames (transform);
  ipatch_sample_transform_get_buffers (transform, &buf, NULL);

  sizeleft = src_size;
  ofs = 0;

  while (sizeleft > 0)
  {
    if (thissize > sizeleft) thissize = sizeleft;

    if (!ipatch_sample_handle_read (&src_handle, ofs, thissize, buf, err))
      goto err;

    if (!ipatch_sample_handle_write (&dest_handle, ofs, thissize, buf, err))
      goto err;

    ofs += thissize;
    sizeleft -= thissize;
  }

  retval = TRUE;

err:

  ipatch_sample_handle_close (&src_handle);     /* -- close source handle */
  ipatch_sample_handle_close (&dest_handle);    /* -- close destination handle */

  return (retval);
}

/* FIXME-GIR: @sub_format is a dynamic GEnum or -1 */

/**
 * ipatch_sample_save_to_file:
 * @sample: Sample to save to file
 * @filename: File name to save to
 * @file_format: (type IpatchSndFileFormat): A value from the dynamic GEnum "IpatchSndFileFormat".
 * @sub_format: A value from the dynamic GEnum "IpatchSndFileSubFormat" or -1
 *   to calculate optimal value based on the format of @sample.
 * @err: Location to store error info or %NULL to ignore
 *
 * Convenience function to save a sample to a file using libsndfile.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
ipatch_sample_save_to_file (IpatchSample *sample, const char *filename,
                            int file_format, int sub_format, GError **err)
{
  IpatchSample *store;
  int channels, samplerate, sample_format;
  int loop_type, loop_start, loop_end, fine_tune, root_note;

  g_return_val_if_fail (IPATCH_IS_SAMPLE (sample), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  g_object_get (sample,
                "sample-format", &sample_format,
                "sample-rate", &samplerate,
                NULL);

  channels = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (sample_format);
  sub_format = ipatch_snd_file_sample_format_to_sub_format (sample_format, file_format);

  if (sub_format == -1)
  {
    g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_PROGRAM,
                 "Invalid libsndfile format parameters");
    return (FALSE);
  }

  store = ipatch_sample_store_snd_file_new (filename); /* ++ ref new store */
  if (!ipatch_sample_store_snd_file_init_write (IPATCH_SAMPLE_STORE_SND_FILE (store),
                                                file_format, sub_format,
                                                IPATCH_SND_FILE_ENDIAN_FILE,
                                                channels, samplerate))
  {
    g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_PROGRAM,
                 "Invalid libsndfile format parameters");
    g_object_unref (store);     /* -- unref store */
    return (FALSE);
  }

  g_object_get (sample,
                "loop-type", &loop_type,
                "loop-start", &loop_start,
                "loop-end", &loop_end,
                "root-note", &root_note,
                "fine-tune", &fine_tune,
                NULL);

  g_object_set (store,
                "loop-type", loop_type,
                "loop-start", loop_start,
                "loop-end", loop_end,
                "root-note", root_note,
                "fine-tune", fine_tune,
                NULL);

  if (!ipatch_sample_copy (store, sample, IPATCH_SAMPLE_UNITY_CHANNEL_MAP, err))
  {
    g_object_unref (store);     /* -- unref store */
    return (FALSE);
  }

  g_object_unref (store);     /* -- unref store */

  return (TRUE);
}

/**
 * ipatch_sample_handle_open:
 * @sample: Sample to open a handle to
 * @handle: (out): Caller supplied structure to initialize
 * @mode: Access mode to sample, 'r' for reading and 'w' for writing
 * @format: Sample format to convert to/from (0 for no conversion or to assign
 *   a transform object with ipatch_sample_handle_set_transform()).
 * @channel_map: Channel mapping if @format is set (set to 0 otherwise), use
 *   #IPATCH_SAMPLE_UNITY_CHANNEL_MAP for 1 to 1 channel mapping
 *   (see ipatch_sample_get_transform_funcs() for details).
 * @err: Location to store error information
 *
 * Open a handle to a sample for reading or writing sample data.  Can optionally
 * provide data conversion if @format is set.  If it is desirable to have more
 * control over the transform object and buffer allocation, the transform object
 * can be assigned with ipatch_sample_handle_set_transform().  Note that a sample
 * transform is acquired if @format is set, even if the format is identical to
 * the @sample format, as a convenience to always provide a data buffer.
 *
 * Returns: %TRUE on success, %FALSE on failure (in which case @err may be set)
 */
gboolean
ipatch_sample_handle_open (IpatchSample *sample, IpatchSampleHandle *handle,
			   char mode, int format, guint32 channel_map,
			   GError **err)
{
  IpatchSampleIface *iface;
  int sample_format;
  guint size;

  g_return_val_if_fail (IPATCH_IS_SAMPLE (sample), FALSE);
  g_return_val_if_fail (handle != NULL, FALSE);
  g_return_val_if_fail (mode == 'r' || mode == 'w', FALSE);
  g_return_val_if_fail (!format || ipatch_sample_format_verify (format), FALSE);

  /* Verify sample format was set */
  g_object_get (sample, "sample-format", &sample_format, NULL);
  g_return_val_if_fail (ipatch_sample_format_verify (sample_format), FALSE);

  /* Verify transform formats and channel mapping, if format is set */
  if (format)
  {
    if (mode == 'r')
      g_return_val_if_fail (ipatch_sample_format_transform_verify (sample_format, format, channel_map), FALSE);
    else g_return_val_if_fail (ipatch_sample_format_transform_verify (format, sample_format, channel_map), FALSE);
  }

  /* Verify sample size is set */
  g_object_get (sample, "sample-size", &size, NULL);
  g_return_val_if_fail (size != 0, FALSE);

  memset (handle, 0, sizeof (IpatchSampleHandle));
  handle->sample = g_object_ref (sample); /* ++ ref sample interface object */
  handle->read_mode = mode == 'r';
  handle->format = format ? format : sample_format;
  handle->channel_map = format ? channel_map : IPATCH_SAMPLE_UNITY_CHANNEL_MAP;

  /* Was format specified? */
  if (format != 0)
  { /* Acquire sample data transform in the proper direction */
    if (handle->read_mode)  /* ++ grab transform */
      handle->transform = ipatch_sample_transform_pool_acquire (sample_format, format,
                                                                channel_map);
    else handle->transform = ipatch_sample_transform_pool_acquire (format, sample_format,
                                                                   channel_map);

    handle->release_transform = TRUE;    /* Indicate that transform came from pool */
  }

  iface = IPATCH_SAMPLE_GET_IFACE (sample);

  handle->read = iface->read;
  handle->write = iface->write;
  handle->close = iface->close;

  /* call interface open method (if any) */
  if (iface->open)
  {
    if (iface->open (handle, err))
      return (TRUE);

    /* Error occurred */

    if (handle->transform)  /* -- release transform */
      ipatch_sample_transform_pool_release (handle->transform);

    g_object_unref (handle->sample);    /* -- unref sample */

    handle->transform = NULL;
    handle->sample = NULL;

    return (FALSE);
  }
  else return (TRUE);   /* No open method, assume success */
}

/**
 * ipatch_sample_handle_close:
 * @handle: Sample handle to close
 *
 * Close a handle previously opened with ipatch_sample_handle_open().
 */
void
ipatch_sample_handle_close (IpatchSampleHandle *handle)
{
  IpatchSampleIface *iface;

  g_return_if_fail (handle != NULL);
  g_return_if_fail (IPATCH_IS_SAMPLE (handle->sample));

  iface = IPATCH_SAMPLE_GET_IFACE (handle->sample);

  /* call interface close method (if any) */
  if (iface->close) iface->close (handle);

  if (handle->transform)
  { /* If transform came from pool, release it, unref otherwise (user assigned) */
    if (handle->release_transform)
      ipatch_sample_transform_pool_release (handle->transform); /* -- release transform */
    else ipatch_sample_transform_free (handle->transform);  /* -- free transform */
  }

  g_object_unref (handle->sample);    /* -- unref sample */

  handle->transform = NULL;
  handle->sample = NULL;
}

/**
 * ipatch_sample_handle_get_transform:
 * @handle: Sample handle to get transform from
 *
 * Get sample transform from a sample handle.  Only exists if sample
 * data conversion is taking place or even if formats are the same but was
 * implicitly supplied to ipatch_sample_handle_open().  Transform should not be
 * modified unless it was assigned via ipatch_sample_handle_set_transform().
 *
 * Returns: (transfer none): Sample transform or %NULL if none.
 */
IpatchSampleTransform *
ipatch_sample_handle_get_transform (IpatchSampleHandle *handle)
{
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (IPATCH_IS_SAMPLE (handle->sample), NULL);

  return (handle->transform);
}

/**
 * ipatch_sample_handle_set_transform:
 * @handle: Sample handle to set transform of
 * @transform: (nullable): Transform to assign, source format must match that of
 *   the handle's sample (read mode) or destination format must match (write mode),
 *   can be %NULL to de-activate sample transformation for @handle.
 *
 * Assign a sample transform to a sample handle.  Provided for added
 * control over @transform allocation.  A transform can also be automatically
 * created and assigned with ipatch_sample_handle_open().  Sample transform
 * allocation is taken over by @handle.
 */
void
ipatch_sample_handle_set_transform (IpatchSampleHandle *handle,
                                    IpatchSampleTransform *transform)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (!transform || transform->buf1);

  if (handle->transform)
  { /* If transform came from pool, release it, free otherwise (user assigned) */
    if (handle->release_transform)
      ipatch_sample_transform_pool_release (handle->transform); /* -- release transform */
    else ipatch_sample_transform_free (handle->transform);  /* -- free transform */
  }

  if (transform) handle->transform = transform;
  else handle->transform = NULL;

  handle->release_transform = FALSE;
}

/**
 * ipatch_sample_handle_get_format:
 * @handle: Sample handle to get format of
 *
 * Get the sample format of a sample handle.  May differ from the #IpatchSample
 * format of the handle, if it was opened with a different format and is
 * therefore being converted.
 * 
 * Returns: Sample format integer (see #IpatchSampleWidth, etc).
 */
int
ipatch_sample_handle_get_format (IpatchSampleHandle *handle)
{
  g_return_val_if_fail (handle != NULL, 0);
  g_return_val_if_fail (IPATCH_IS_SAMPLE (handle->sample), 0);

  if (handle->transform)
    return (handle->read_mode ? handle->transform->dest_format
	    : handle->transform->src_format);
  else return (ipatch_sample_get_format (handle->sample));
}

/**
 * ipatch_sample_handle_get_frame_size:
 * @handle: Sample handle to get data frame size of
 * 
 * A convenience function to get size of a single sample frame for a given
 * sample @handle.  This is useful for determining buffer allocation sizes when
 * reading or writing data.
 *
 * Returns: Size in bytes of a single sample frame
 */
int ipatch_sample_handle_get_frame_size (IpatchSampleHandle *handle)
{
  return (ipatch_sample_format_size (ipatch_sample_handle_get_format (handle)));
}

/**
 * ipatch_sample_handle_get_max_frames:
 * @handle: Sample handle to get max transform frames of
 *
 * A convenience function to get the maximum transform frames that can fit
 * in the sample transform of @handle.
 *
 * Returns: Maximum frames that can be read or written using the sample
 *   transform buffers.  0 if no sample transform is assigned.
 */
guint
ipatch_sample_handle_get_max_frames (IpatchSampleHandle *handle)
{
  g_return_val_if_fail (handle != NULL, 0);
  g_return_val_if_fail (IPATCH_IS_SAMPLE (handle->sample), 0);

  if (!handle->transform) return 0;

  return (ipatch_sample_transform_get_max_frames (handle->transform));
}

/**
 * ipatch_sample_handle_read: (skip)
 * @handle: Sample handle
 * @offset: Offset in frames to read from
 * @frames: Number of frames to read
 * @buf: Buffer to store sample data in (should be at least @frames *
 *   sizeof (frame), the frame size can be had from
 *   ipatch_sample_handle_get_frame_size()).  Can be %NULL if transforming
 *   audio data with not more than the maximum frames that can be transformed
 *   at a time, in which case the internal transform buffer pointer will be
 *   returned.
 * @err: Location to store error info or %NULL
 * 
 * Read sample data from a sample handle.  If the number of
 * frames read is within the sample transform buffer size and @buf is %NULL
 * then the transform buffer will be returned (extra copy not needed).
 * 
 * Returns: Pointer to sample data on success, %NULL on error (in which case
 *   @err may be set).  The internal transform buffer will only be returned
 *   if the @buf parameter is %NULL.
 */
gpointer
ipatch_sample_handle_read (IpatchSampleHandle *handle, guint offset,
			   guint frames, gpointer buf, GError **err)
{
  IpatchSampleTransform *trans;
  guint readframes, framesize, readbytes;
  gpointer transbuf, outbuf, bufptr;
  guint size;

  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (IPATCH_IS_SAMPLE (handle->sample), NULL);
  g_return_val_if_fail (handle->read_mode, NULL);
  g_return_val_if_fail (!err || !*err, NULL);

  g_return_val_if_fail (handle->read != NULL, NULL);

  /* Make sure read does not exceed the sample size */
  size = ipatch_sample_get_size (handle->sample, NULL);
  g_return_val_if_fail (offset + frames <= size, FALSE);

  trans = handle->transform;

  if (trans && !handle->manual_transform)	/* transforming audio data? */
  {
    readframes = trans->max_frames;
    transbuf = trans->buf1;

    /* buffer pointer not supplied? */
    if (!buf)
    {
      /* extra descriptive for debugging purposes */
      g_return_val_if_fail (buf || frames <= readframes, NULL);

      /* read the sample data */
      if (!handle->read (handle, offset, frames, transbuf, err)) return (NULL);

      /* transform the sample data and return - we done! */
      return (ipatch_sample_transform_convert_single (trans, frames));
    }

    bufptr = buf;
    framesize = ipatch_sample_format_size (trans->dest_format);
    readbytes = readframes * framesize;

    while (frames > 0)	/* must be transformed in blocks */
    {
      if (readframes > frames)
      {
	readframes = frames;
	readbytes = readframes * framesize;
      }

      /* read the sample data */
      if (!handle->read (handle, offset, readframes, transbuf, err)) return (NULL);

      /* transform the sample data */
      outbuf = ipatch_sample_transform_convert_single (trans, readframes);

      /* copy to caller's buffer */
      memcpy (bufptr, outbuf, readbytes);

      frames -= readframes;
      offset += readframes;
      bufptr += readbytes;
    }
  }
  else   /* not transforming, do it all in one go */
  {
    g_return_val_if_fail (buf != NULL, NULL);

    if (!handle->read (handle, offset, frames, buf, err)) return (NULL);
  }

  return (buf);
}

/**
 * ipatch_sample_handle_read_size: (rename-to ipatch_sample_handle_read)
 * @handle: Sample handle
 * @offset: Offset in frames to read from
 * @size: Size of data to read (in bytes), must be a multiple of sample frame size
 * @err: Location to store error info or %NULL
 * 
 * Read sample data from a sample handle.  Like ipatch_sample_handle_read() but
 * is GObject Introspection friendly and allocates returned buffer.
 *
 * Returns: (array length=size) (element-type guint8) (transfer full): Newly allocated
 *   sample data or %NULL on error (in which case @err may be set)
 *
 * Since: 1.1.0
 */
gpointer
ipatch_sample_handle_read_size (IpatchSampleHandle *handle, guint offset,
                                guint size, GError **err)
{
  gpointer buf;
  int frame_size;

  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (IPATCH_IS_SAMPLE (handle->sample), NULL);
  g_return_val_if_fail (size > 0, NULL);

  frame_size = ipatch_sample_handle_get_frame_size (handle);
  g_return_val_if_fail (frame_size > 0, NULL);
  g_return_val_if_fail (size % frame_size == 0, NULL);

  buf = g_malloc (size);        // ++ alloc buf

  if (!ipatch_sample_handle_read (handle, offset, size / frame_size, buf, err))
  {
    g_free (buf);               // -- free buf on error
    return (NULL);
  }

  return (buf);         // !! caller takes over allocation
}

/**
 * ipatch_sample_handle_write: (skip)
 * @handle: Sample handle
 * @offset: Offset in frames to write to
 * @frames: Number of frames to write
 * @buf: Buffer of sample data to write (should be at least @frames *
 *   sizeof (frame), the frame size can be had from
 *   ipatch_sample_handle_get_frame_size()).  Can be %NULL, in which case it is
 *   assumed that the sample data has been loaded into the first buffer of the
 *   handle's sample transform.
 * @err: Location to store error info or %NULL
 * 
 * Write sample data to a sample handle.
 * 
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_sample_handle_write (IpatchSampleHandle *handle, guint offset, guint frames,
			    gconstpointer buf, GError **err)
{
  IpatchSampleTransform *trans;
  guint writeframes, framesize, writebytes;
  gpointer transbuf, outbuf;
  gconstpointer bufptr;
  guint size;

  g_return_val_if_fail (handle != NULL, FALSE);
  g_return_val_if_fail (IPATCH_IS_SAMPLE (handle->sample), FALSE);
  g_return_val_if_fail (!handle->read_mode, FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  g_return_val_if_fail (handle->write != NULL, FALSE);

  /* Make sure write does not exceed the sample size */
  size = ipatch_sample_get_size (handle->sample, NULL);
  g_return_val_if_fail (offset + frames <= size, FALSE);

  trans = handle->transform;

  if (trans && !handle->manual_transform)	/* transforming audio data? */
  {
    writeframes = trans->max_frames;
    transbuf = trans->buf1;

    /* buffer pointer not supplied or its the transform buffer? */
    if (!buf || buf == transbuf)
    {
      /* extra descriptive for debugging purposes */
      g_return_val_if_fail (buf || frames <= writeframes, FALSE);

      outbuf = ipatch_sample_transform_convert_single (trans, frames);

      /* write the sample data and return - we's done! */
      return (handle->write (handle, offset, frames, outbuf, err));
    }

    bufptr = buf;
    framesize = ipatch_sample_format_size (trans->src_format);
    writebytes = writeframes * framesize;

    while (frames > 0)	/* must be transformed in blocks */
    {
      if (writeframes > frames)
      {
	writeframes = frames;
	writebytes = writeframes * framesize;
      }

      /* copy the block of sample data to transform */
      memcpy (transbuf, bufptr, writebytes);

      /* transform the sample data */
      outbuf = ipatch_sample_transform_convert_single (trans, writeframes);

      /* write the transformed sample data */
      if (!handle->write (handle, offset, writeframes, outbuf, err)) return (FALSE);

      frames -= writeframes;
      offset += writeframes;
      bufptr += writebytes;
    }
  }
  else   /* not transforming, do it all in one go */
  {
    g_return_val_if_fail (buf != NULL, FALSE);

    if (!handle->write (handle, offset, frames, buf, err)) return (FALSE);
  }

  return (TRUE);
}

/**
 * ipatch_sample_handle_write_size: (rename-to ipatch_sample_handle_write)
 * @handle: Sample handle
 * @offset: Offset in frames to write to
 * @buf: (array length=size) (element-type guint8) (transfer none): Buffer of
 *   sample data to write
 * @size: Size of @buf in bytes (must be a multiple of sample frame size)
 * @err: Location to store error info or %NULL
 * 
 * Write sample data to a sample handle.  Like ipatch_sample_handle_write() but
 * is GObject Introspection friendly.
 * 
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 *
 * Since: 1.1.0
 */
gboolean
ipatch_sample_handle_write_size (IpatchSampleHandle *handle, guint offset,
                                 gconstpointer buf, guint size, GError **err)
{
  int frame_size;

  g_return_val_if_fail (handle != NULL, FALSE);
  g_return_val_if_fail (IPATCH_IS_SAMPLE (handle->sample), FALSE);

  frame_size = ipatch_sample_handle_get_frame_size (handle);
  g_return_val_if_fail (frame_size != 0, FALSE);
  g_return_val_if_fail (size % frame_size == 0, FALSE);

  return (ipatch_sample_handle_write (handle, offset, size / frame_size, buf, err));
}

/**
 * ipatch_sample_handle_cascade_open: (skip)
 * @handle: Already open handle
 * @sample: The cascade sample containing the actual data
 * @err: Location to store error information
 *
 * This can be called from #IpatchSampleIface.open methods
 * for objects which contain a pointer to an #IpatchSample that contains the
 * sample's data.
 *
 * Returns: %TRUE on success, %FALSE on failure (in which case @err may be set)
 */
gboolean
ipatch_sample_handle_cascade_open (IpatchSampleHandle *handle,
                                   IpatchSample *sample, GError **err)
{
  IpatchSampleIface *iface;

  g_return_val_if_fail (handle != NULL, FALSE);
  g_return_val_if_fail (IPATCH_IS_SAMPLE (sample), FALSE);

  iface = IPATCH_SAMPLE_GET_IFACE (sample);

  g_object_unref (handle->sample);              /* -- unref old sample */
  handle->sample = g_object_ref (sample);       /* ++ ref cascaded sample for new handle */

  handle->read = iface->read;
  handle->write = iface->write;
  handle->close = iface->close;

  /* call interface open method (if any) */
  if (iface->open)
    return (iface->open (handle, err));
  else return (TRUE);   /* No open method, assume success */
}

/**
 * ipatch_sample_install_property: (skip)
 * @oclass: Object class to install #IpatchSample property
 * @property_id: Property ID for set/get property class method
 * @property_name: #IpatchSample property name to install
 * 
 * A helper function for objects that have an #IpatchSample interface.
 * Installs a #IpatchSample interface property for the given object class.
 * The parameter will be #G_PARAM_READWRITE.
 *
 * Returns: The newly created and installed parameter spec.
 */
GParamSpec *
ipatch_sample_install_property (GObjectClass *oclass, guint property_id,
				const char *property_name)
{
  GParamSpec *pspec;

  g_return_val_if_fail (G_IS_OBJECT_CLASS (oclass), NULL);
  g_return_val_if_fail (property_id != 0, NULL);

  pspec = ipatch_sample_new_property_param_spec (property_name,
						 G_PARAM_READWRITE);
  g_return_val_if_fail (pspec != NULL, NULL);

  g_object_class_install_property (oclass, property_id, pspec);

  return (pspec);
}

/**
 * ipatch_sample_install_property_readonly: (skip)
 * @oclass: Object class to install #IpatchSample property
 * @property_id: Property ID for set/get property class method
 * @property_name: #IpatchSample property name to install
 * 
 * A helper function for objects that have an #IpatchSample interface.
 * Identical to ipatch_sample_install_property() but installs the property
 * as readonly and uses g_object_class_override_property() instead of
 * creating a new #GParamSpec.
 *
 * Returns: The newly created and installed parameter spec (GParamSpecOverride).
 */
GParamSpec *
ipatch_sample_install_property_readonly (GObjectClass *oclass,
					 guint property_id,
					 const char *property_name)
{
  g_return_val_if_fail (G_IS_OBJECT_CLASS (oclass), NULL);
  g_return_val_if_fail (property_id != 0, NULL);

  g_object_class_override_property (oclass, property_id, property_name);
  return (g_object_class_find_property (oclass, property_name));
}

/**
 * ipatch_sample_new_property_param_spec: (skip)
 * @property_name: Name of a #IpatchSample property
 * @flags: Flags to use for the new #GParamSpec
 * 
 * Seldom used function that creates a new GParamSpec that is identical to
 * a #IpatchSample property by the name @property_name, except the flags
 * can differ.
 * 
 * Returns: New GParamSpec.
 */
GParamSpec *
ipatch_sample_new_property_param_spec (const char *property_name,
				       GParamFlags flags)
{
  if (strcmp (property_name, "sample-data") == 0)
    return g_param_spec_object ("sample-data", _("Sample data"), _("Sample data"),
			        IPATCH_TYPE_SAMPLE_DATA, flags);
  else if (strcmp (property_name, "sample-size") == 0)
    return g_param_spec_uint ("sample-size", _("Size"),
			      _("Size in frames"),
			      0, G_MAXUINT, 0,
			      flags);
  else if (strcmp (property_name, "sample-format") == 0)
    return g_param_spec_int ("sample-format", _("Sample format"),
			     _("Sample format"),
			     0, G_MAXINT, IPATCH_SAMPLE_FORMAT_DEFAULT,
			     flags);
  else if (strcmp (property_name, "sample-rate") == 0)
    return g_param_spec_int ("sample-rate", _("Sample rate"),
			     _("Sampling rate in Hertz"),
			     IPATCH_SAMPLE_RATE_MIN, IPATCH_SAMPLE_RATE_MAX,
			     IPATCH_SAMPLE_RATE_DEFAULT, flags);
  else if (strcmp (property_name, "loop-type") == 0)
    return g_param_spec_enum ("loop-type", _("Loop type"),
			      _("Loop method type"),
			      IPATCH_TYPE_SAMPLE_LOOP_TYPE,
			      IPATCH_SAMPLE_LOOP_NONE,
			      flags);
  else if (strcmp (property_name, "loop-start") == 0)
    return g_param_spec_uint ("loop-start", _("Loop start"),
			      _("Start of loop in frames"),
			      0, G_MAXUINT, 0,
			      flags);
  else if (strcmp (property_name, "loop-end") == 0)
    return g_param_spec_uint ("loop-end", _("Loop end"),
			      _("Loop end in frames (after loop)"),
			      0, G_MAXUINT, 0,
			      flags);
  else if (strcmp (property_name, "root-note") == 0)
    return g_param_spec_int ("root-note", _("Root note"),
			     _("Root MIDI note"),
			     0, 127, IPATCH_SAMPLE_ROOT_NOTE_DEFAULT,
			     flags);
  else if (strcmp (property_name, "fine-tune") == 0)
    return g_param_spec_int ("fine-tune", _("Fine tuning"),
			     _("Fine tuning in cents"),
			     -99, 99, 0,
			     flags);
  else return (NULL);
}
