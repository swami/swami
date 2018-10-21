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
#ifndef __IPATCH_SAMPLE_H__
#define __IPATCH_SAMPLE_H__

#include <glib.h>
#include <glib-object.h>

#include <libinstpatch/IpatchSampleTransform.h>

/* forward type declarations */

typedef struct _IpatchSample IpatchSample; /* dummy typedef */
typedef struct _IpatchSampleIface IpatchSampleIface;
typedef struct _IpatchSampleHandle IpatchSampleHandle;

#include <libinstpatch/IpatchSampleData.h>

#define IPATCH_TYPE_SAMPLE   (ipatch_sample_get_type ())
#define IPATCH_SAMPLE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SAMPLE, \
   IpatchSample))
#define IPATCH_SAMPLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SAMPLE, \
   IpatchSampleIface))
#define IPATCH_IS_SAMPLE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SAMPLE))
#define IPATCH_SAMPLE_GET_IFACE(obj) \
  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), IPATCH_TYPE_SAMPLE, \
   IpatchSampleIface))

/**
 * IpatchSampleHandleOpenFunc:
 * @handle: Caller supplied structure to initialize
 * @err: Location to store error information
 *
 * #IpatchSample interface method function type to open a sample for reading
 * or writing.  This method is optional for an #IpatchSample interface and if
 * not specified then it is assumed that the open was successful and nothing
 * additional need be done.  All fields of @handle structure are already
 * initialized, except <structfield>data1</structfield>,
 * <structfield>data2</structfield> and <structfield>data3</structfield>
 * which are available for the interface implementation.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case an error should
 *   optionally be stored in @err).
 */
typedef gboolean (*IpatchSampleHandleOpenFunc)(IpatchSampleHandle *handle,
					       GError **err);
/**
 * IpatchSampleHandleCloseFunc:
 * @handle: Sample handle to close (as returned from #IpatchSampleHandleOpenFunc)
 *
 * #IpatchSample interface method to free any resources allocated in
 * #IpatchSampleOpenFunc to @handle.  This method is optional for an
 * #IpatchSample interface and need not be specified if nothing needs to be done
 * to release any resources allocated by #IpatchSampleHandleOpenFunc.
 */
typedef void (*IpatchSampleHandleCloseFunc)(IpatchSampleHandle *handle);

/**
 * IpatchSampleHandleReadFunc:
 * @handle: Handle to read from (as returned from #IpatchSampleOpenFunc)
 * @offset: Offset in sample to start read from (in frames), use
 *   #IPATCH_SAMPLE_CUROFS to use current offset (starts at 0 if not specified)
 * @frames: Size of sample data to read (in frames)
 * @buf: Buffer to store sample data in
 * @err: Location to store error information
 *
 * #IpatchSample interface method function type to read data from a
 * sample handle.  Can be %NULL in #IpatchSampleIface if sample data is not
 * readable.  Sample data should be stored in its native format.
 *
 * Returns: Should return %TRUE on success, %FALSE otherwise (in which case
 * an error should optionally be stored in @err).
 */
typedef gboolean (*IpatchSampleHandleReadFunc)(IpatchSampleHandle *handle,
					       guint offset, guint frames,
					       gpointer buf, GError **err);
/**
 * IpatchSampleHandleWriteFunc:
 * @handle: Handle to write to (as returned from #IpatchSampleOpenFunc)
 * @offset: Offset in sample to start write to (in frames), use
 *   #IPATCH_SAMPLE_CUROFS to use current offset (starts at 0 if not specified)
 * @frames: Size of sample data to write (in frames)
 * @buf: Buffer to store sample data in
 * @err: Location to store error information
 *
 * #IpatchSample interface method function type to write data to a
 * sample handle.  Can be %NULL in #IpatchSampleIface if sample data is not
 * writable.  Sample data will be supplied in its native format.
 *
 * Returns: Should return %TRUE on success, %FALSE otherwise (in which case
 * an error should optionally be stored in @err).
 */
typedef gboolean (*IpatchSampleHandleWriteFunc)(IpatchSampleHandle *handle,
					        guint offset, guint frames,
					        gconstpointer buf, GError **err);
/* Sample interface */
struct _IpatchSampleIface
{
  GTypeInterface parent_class;

  IpatchSampleHandleOpenFunc open;
  IpatchSampleHandleCloseFunc close;
  IpatchSampleHandleReadFunc read;
  IpatchSampleHandleWriteFunc write;

  int *loop_types; /* -1 terminated array of supp. loop types (NULL = none) */
};

/* Sample handle for I/O operations */
struct _IpatchSampleHandle
{
  IpatchSample *sample;		      /* The sample which this handle applies to */
  IpatchSampleTransform *transform;   /* Set if sample is being converted */
  IpatchSampleHandleReadFunc read;    /* Read method pointer (copied from IpatchItem interface) */
  IpatchSampleHandleWriteFunc write;  /* Write method pointer (copied from IpatchItem interface) */
  IpatchSampleHandleCloseFunc close;  /* Close method pointer (copied from IpatchItem interface) */
  guint32 read_mode : 1;              /* TRUE if read mode, FALSE if write mode */
  guint32 manual_transform : 1;       /* Methods handle sample transform */
  guint32 release_transform : 1;      /* TRUE if transform should be released from transform pool */
  guint32 format : 12;                /* Format to transform to */
  guint32 reserved : 17;
  guint32 channel_map;                /* Channel map for multi-channel audio transform */
  gpointer data1;		      /* sample interface defined */
  gpointer data2;		      /* sample interface defined */
  gpointer data3;		      /* sample interface defined */
  gpointer data4;		      /* sample interface defined */
  guint32 reserved2;
};

/**
 * IpatchSampleLoopType:
 * @IPATCH_SAMPLE_LOOP_NONE: No loop.
 * @IPATCH_SAMPLE_LOOP_STANDARD: Standard loop.
 * @IPATCH_SAMPLE_LOOP_RELEASE: Loop till note release stage.
 * @IPATCH_SAMPLE_LOOP_PINGPONG: Play forward and then in reverse continously.
 *
 * Sample looping type.
 */
typedef enum
{
  IPATCH_SAMPLE_LOOP_NONE,
  IPATCH_SAMPLE_LOOP_STANDARD,
  IPATCH_SAMPLE_LOOP_RELEASE,
  IPATCH_SAMPLE_LOOP_PINGPONG
} IpatchSampleLoopType;

/**
 * IPATCH_SAMPLE_FORMAT_DEFAULT:
 *
 * Default sample format for #IpatchSample interface.
 */
#define IPATCH_SAMPLE_FORMAT_DEFAULT  (IPATCH_SAMPLE_16BIT | IPATCH_SAMPLE_MONO \
                		       | IPATCH_SAMPLE_LENDIAN | IPATCH_SAMPLE_SIGNED)

/**
 * IPATCH_SAMPLE_RATE_MIN: (skip)
 *
 * Minimum sample rate.
 * SoundFont spec says 8000 Hz is minimum guaranteed, seen lots of smaller values though.
 */
#define IPATCH_SAMPLE_RATE_MIN     100

/**
 * IPATCH_SAMPLE_RATE_MAX: (skip)
 *
 * Maximum sample rate.
 */
#define IPATCH_SAMPLE_RATE_MAX     192000

/**
 * IPATCH_SAMPLE_RATE_DEFAULT: (skip)
 *
 * Default sample rate.
 */
#define IPATCH_SAMPLE_RATE_DEFAULT 44100

/**
 * IPATCH_SAMPLE_ROOT_NOTE_DEFAULT: (skip)
 *
 * Default root note.
 */
#define IPATCH_SAMPLE_ROOT_NOTE_DEFAULT  60

/**
 * IPATCH_SAMPLE_LOOP_TYPE_TERM:
 *
 * Value used for terminating list of supported loop types.
 */
#define IPATCH_SAMPLE_LOOP_TYPE_TERM (-1)

/**
 * IPATCH_SAMPLE_HANDLE_FORMAT:
 * @handle: Sample handle
 *
 * Macro to access transform sample format of a sample handle.
 *
 * Returns: Sample transform format.
 */
#define IPATCH_SAMPLE_HANDLE_FORMAT(handle)  ((handle)->format)

/* some convenient loop_type arrays for use by IpatchSample interfaces */
extern int ipatch_sample_loop_types_standard[];
extern int ipatch_sample_loop_types_standard_release[];

GType ipatch_sample_get_type (void);
int *ipatch_sample_get_loop_types (IpatchSample *sample);
int *ipatch_sample_type_get_loop_types (GType type);
int *ipatch_sample_get_loop_types_len (IpatchSample *sample, int *len);
int *ipatch_sample_type_get_loop_types_len (GType type, int *len);

void ipatch_sample_set_format (IpatchSample *sample, int format);
int ipatch_sample_get_format (IpatchSample *sample);
void ipatch_sample_set_size (IpatchSample *sample, guint size);
guint ipatch_sample_get_size (IpatchSample *sample, guint *bytes);
int ipatch_sample_get_frame_size (IpatchSample *sample);
IpatchSampleData *ipatch_sample_get_sample_data (IpatchSample *sample);
gboolean ipatch_sample_set_sample_data (IpatchSample *sample,
                                        IpatchSampleData *sampledata);

gboolean ipatch_sample_read (IpatchSample *sample, guint offset, guint frames,
                             gpointer buf, GError **err);
gpointer ipatch_sample_read_size (IpatchSample *sample, guint offset, guint size, GError **err);
gboolean ipatch_sample_write (IpatchSample *sample, guint offset, guint frames,
                              gconstpointer buf, GError **err);
gboolean ipatch_sample_write_size (IpatchSample *sample, guint offset,
                                   gconstpointer buf, guint size, GError **err);
gboolean ipatch_sample_read_transform (IpatchSample *sample, guint offset, guint frames,
                                       gpointer buf, int format, guint32 channel_map,
                                       GError **err);
gpointer ipatch_sample_read_transform_size (IpatchSample *sample, guint offset, guint size,
                                            int format, guint32 channel_map, GError **err);
gboolean ipatch_sample_write_transform (IpatchSample *sample, guint offset, guint frames,
                                        gconstpointer buf, int format, guint32 channel_map,
                                        GError **err);
gboolean ipatch_sample_write_transform_size (IpatchSample *sample, guint offset,
                                             gconstpointer buf, guint size, int format,
                                             guint32 channel_map, GError **err);
gboolean ipatch_sample_copy (IpatchSample *dest_sample, IpatchSample *src_sample,
			     guint32 channel_map, GError **err);
gboolean ipatch_sample_save_to_file (IpatchSample *sample, const char *filename,
                                     int file_format, int sub_format, GError **err);

gboolean ipatch_sample_handle_open (IpatchSample *sample,
				    IpatchSampleHandle *handle, char mode,
				    int format, guint32 channel_map, GError **err);
void ipatch_sample_handle_close (IpatchSampleHandle *handle);
IpatchSampleTransform *ipatch_sample_handle_get_transform (IpatchSampleHandle *handle);
void ipatch_sample_handle_set_transform (IpatchSampleHandle *handle,
                                         IpatchSampleTransform *transform);
int ipatch_sample_handle_get_format (IpatchSampleHandle *handle);
int ipatch_sample_handle_get_frame_size (IpatchSampleHandle *handle);
guint ipatch_sample_handle_get_max_frames (IpatchSampleHandle *handle);
gpointer ipatch_sample_handle_read (IpatchSampleHandle *handle, guint offset, guint frames,
				    gpointer buf, GError **err);
gpointer
ipatch_sample_handle_read_size (IpatchSampleHandle *handle, guint offset,
                                guint size, GError **err);
gboolean ipatch_sample_handle_write (IpatchSampleHandle *handle, guint offset, guint frames,
				     gconstpointer buf, GError **err);
gboolean
ipatch_sample_handle_write_size (IpatchSampleHandle *handle, guint offset,
                                 gconstpointer buf, guint size, GError **err);

gboolean ipatch_sample_handle_cascade_open (IpatchSampleHandle *handle,
                                            IpatchSample *sample, GError **err);

GParamSpec *ipatch_sample_install_property (GObjectClass *oclass,
					    guint property_id,
					    const char *property_name);
GParamSpec *ipatch_sample_install_property_readonly (GObjectClass *oclass,
						     guint property_id,
						     const char *property_name);
GParamSpec *ipatch_sample_new_property_param_spec (const char *property_name,
						   GParamFlags flags);

#endif
