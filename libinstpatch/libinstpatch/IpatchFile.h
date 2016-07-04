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
#ifndef __IPATCH_FILE_H__
#define __IPATCH_FILE_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/compat.h>

/* forward type declarations */

typedef struct _IpatchFile IpatchFile;
typedef struct _IpatchFileClass IpatchFileClass;
typedef struct _IpatchFileIOFuncs IpatchFileIOFuncs;
typedef struct _IpatchFileHandle IpatchFileHandle;

#include <libinstpatch/IpatchItem.h>

#define IPATCH_TYPE_FILE (ipatch_file_get_type ())
#define IPATCH_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_FILE, IpatchFile))
#define IPATCH_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_FILE, IpatchFileClass))
#define IPATCH_IS_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_FILE))
#define IPATCH_IS_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_FILE))
#define IPATCH_FILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_FILE, IpatchFileClass))

/* IO function table for IpatchFile instances */
struct _IpatchFileIOFuncs
{
  gboolean (*open)(IpatchFileHandle *handle, const char *mode, GError **err);
  void (*close)(IpatchFileHandle *handle);
  GIOStatus (*read)(IpatchFileHandle *handle, gpointer buf, guint size,
		    guint *bytes_read, GError **err);
  GIOStatus (*write)(IpatchFileHandle *handle, gconstpointer buf, guint size,
		     GError **err);
  GIOStatus (*seek)(IpatchFileHandle *handle, int offset, GSeekType type, GError **err);
  int (*getfd)(IpatchFileHandle *handle); /* optional get file descriptor method */
  int (*get_size)(IpatchFile *file, GError **err);  /* optional get size method */
};

/* File object */
struct _IpatchFile
{
  IpatchItem parent_instance;

  /*< private >*/

  IpatchFileIOFuncs *iofuncs;	/* per instance I/O methods */
  char *file_name;		/* not always set */
  GIOChannel *iochan;           /* assigned directly with ipatch_file_set_(fd/iochan) */
  GHashTable *ref_hash;         /* registered objects referencing this file: objectPtr -> GWeakRef */
  guint open_count;             /* count of open file handles */
};

/* File object class */
struct _IpatchFileClass
{
  IpatchItemClass parent_class;

  /* File identify method */
  gboolean (*identify)(IpatchFile *file, IpatchFileHandle *handle, GError **err);
  int identify_order;   /* Identify execution order (see #IpatchFileIdentifyOrder, 0 = default) */
};

/**
 * IpatchFileFlags:
 * @IPATCH_FILE_FLAG_SWAP: Swap multi-byte numbers?
 * @IPATCH_FILE_FLAG_BIG_ENDIAN: Big endian file?
 * @IPATCH_FILE_FLAG_FREE_IOFUNCS: Should ->iofuncs be freed?
 */
typedef enum
{
  IPATCH_FILE_FLAG_SWAP = 1 << (IPATCH_ITEM_UNUSED_FLAG_SHIFT),
  IPATCH_FILE_FLAG_BIG_ENDIAN = 1 << (IPATCH_ITEM_UNUSED_FLAG_SHIFT + 1),
  IPATCH_FILE_FLAG_FREE_IOFUNCS = 1 << (IPATCH_ITEM_UNUSED_FLAG_SHIFT + 2)
} IpatchFileFlags;

/**
 * IpatchFileIdentifyOrder:
 * @IPATCH_FILE_IDENTIFY_ORDER_LAST: Execute last (toward the end of the list)
 * @IPATCH_FILE_IDENTIFY_ORDER_DEFAULT: Default execution order (no preference)
 * @IPATCH_FILE_IDENTIFY_ORDER_FIRST: Execute first (toward the start of the list)
 *
 * Some helpful constants for the identify_order #IpatchFileClass field.  Note
 * that any value can be used and this enum just provides some helpful values.
 * This value determines in what order file identification methods are called.
 * Higher values are executed first.
 */
typedef enum
{
  IPATCH_FILE_IDENTIFY_ORDER_LAST = -10,
  IPATCH_FILE_IDENTIFY_ORDER_DEFAULT = 0,
  IPATCH_FILE_IDENTIFY_ORDER_FIRST = 10
} IpatchFileIdentifyOrder;

/**
 * IPATCH_FILE_UNUSED_FLAG_SHIFT: (skip)
 */
/* reserve 6 flags (3 for expansion) */
#define IPATCH_FILE_UNUSED_FLAG_SHIFT  (IPATCH_ITEM_UNUSED_FLAG_SHIFT + 6)

/* macro to check if multi-byte numbers in file require swapping */
#define IPATCH_FILE_NEED_SWAP(file) \
  (ipatch_item_get_flags(file) & IPATCH_FILE_FLAG_SWAP)
/* macro to check if file is big endian */
#define IPATCH_FILE_BIG_ENDIAN(file) \
  (ipatch_item_get_flags(file) & IPATCH_FILE_FLAG_BIG_ENDIAN)

/* runtime byte swapping macros for convenient loading of cross endian
   files, takes a pointer to the value to swap (like in a buffer) */
#define IPATCH_FILE_SWAP16(file, p16) \
 (IPATCH_FILE_NEED_SWAP (file) ? \
  GUINT16_SWAP_LE_BE (*(guint16 *)(p16)) : *(guint16 *)(p16))
#define IPATCH_FILE_SWAP32(file, p32) \
 (IPATCH_FILE_NEED_SWAP (file) ? \
  GUINT32_SWAP_LE_BE (*(guint32 *)(p32)) : *(guint32 *)(p32))
#define IPATCH_FILE_SWAP64(file, p64) \
 (IPATCH_FILE_NEED_SWAP (file) ? \
  GUINT64_SWAP_LE_BE (*(guint64 *)(p64)) : *(guint64 *)(p64))

#define IPATCH_IS_FILE_HANDLE(handle) \
  ((handle) && IPATCH_IS_FILE ((handle)->file))

/**
 * IpatchFileHandle:
 *
 * #IpatchFile handle for opening a file and reading/writing from/to it.
 */
struct _IpatchFileHandle
{
  IpatchFile *file;		/* Parent file object */
  guint position;               /* Current file position */
  GByteArray *buf;              /* For buffered reads/writes */
  guint buf_position;           /* Current position in buffer */
  GIOChannel *iochan;		/* glib IO channel (default methods) */
  gpointer data;                /* iofuncs defined data */
};


GType ipatch_file_get_type (void);
GType ipatch_file_handle_get_type (void);
IpatchFile *ipatch_file_new (void);
IpatchFile *ipatch_file_pool_new (const char *file_name, gboolean *created);
IpatchFile *ipatch_file_pool_lookup (const char *file_name);

void ipatch_file_ref_from_object (IpatchFile *file, GObject *object);
void ipatch_file_unref_from_object (IpatchFile *file, GObject *object);
gboolean ipatch_file_test_ref_object (IpatchFile *file, GObject *object);
IpatchList *ipatch_file_get_refs (IpatchFile *file);
IpatchList *ipatch_file_get_refs_by_type (IpatchFile *file, GType type);

void ipatch_file_set_name (IpatchFile *file, const char *file_name);
char *ipatch_file_get_name (IpatchFile *file);
gboolean ipatch_file_rename (IpatchFile *file, const char *new_name, GError **err);
gboolean ipatch_file_unlink (IpatchFile *file, GError **err);
gboolean ipatch_file_replace (IpatchFile *newfile, IpatchFile *oldfile, GError **err);
IpatchFileHandle *ipatch_file_open (IpatchFile *file, const char *file_name,
                                    const char *mode, GError **err);
void ipatch_file_assign_fd (IpatchFile *file, int fd, gboolean close_on_finalize);
void ipatch_file_assign_io_channel (IpatchFile *file, GIOChannel *iochan);
GIOChannel *ipatch_file_get_io_channel (IpatchFileHandle *handle);
int ipatch_file_get_fd (IpatchFileHandle *handle);
void ipatch_file_close (IpatchFileHandle *handle);

guint ipatch_file_get_position (IpatchFileHandle *handle);
void ipatch_file_update_position (IpatchFileHandle *handle, guint offset);

gboolean ipatch_file_read (IpatchFileHandle *handle, gpointer buf, guint size,
			   GError **err);
GIOStatus ipatch_file_read_eof (IpatchFileHandle *handle, gpointer buf, guint size,
				guint *bytes_read, GError **err);
gboolean ipatch_file_write (IpatchFileHandle *handle, gconstpointer buf,
                            guint size, GError **err);
#define ipatch_file_skip(handle, offset, err) \
  ipatch_file_seek (handle, offset, G_SEEK_CUR, err)
gboolean ipatch_file_seek (IpatchFileHandle *handle, int offset, GSeekType type,
			   GError **err);
GIOStatus ipatch_file_seek_eof (IpatchFileHandle *handle, int offset,
                                GSeekType type, GError **err);
int ipatch_file_get_size (IpatchFile *file, GError **err);
GType ipatch_file_identify (IpatchFile *file, GError **err);
GType ipatch_file_identify_name (const char *filename, GError **err);
GType ipatch_file_identify_by_ext (IpatchFile *file);
IpatchFileHandle *ipatch_file_identify_open (const char *file_name, GError **err);
IpatchFile *ipatch_file_identify_new (const char *file_name, GError **err);

void ipatch_file_set_little_endian (IpatchFile *file);
void ipatch_file_set_big_endian (IpatchFile *file);

gboolean ipatch_file_read_u8 (IpatchFileHandle *handle, guint8 *val, GError **err);
gboolean ipatch_file_read_u16 (IpatchFileHandle *handle, guint16 *val, GError **err);
gboolean ipatch_file_read_u32 (IpatchFileHandle *handle, guint32 *val, GError **err);
gboolean ipatch_file_read_u64 (IpatchFileHandle *handle, guint64 *val, GError **err);
gboolean ipatch_file_read_s8 (IpatchFileHandle *handle, gint8 *val, GError **err);
gboolean ipatch_file_read_s16 (IpatchFileHandle *handle, gint16 *val, GError **err);
gboolean ipatch_file_read_s32 (IpatchFileHandle *handle, gint32 *val, GError **err);
gboolean ipatch_file_read_s64 (IpatchFileHandle *handle, gint64 *val, GError **err);

gboolean ipatch_file_write_u8 (IpatchFileHandle *handle, guint8 val, GError **err);
gboolean ipatch_file_write_u16 (IpatchFileHandle *handle, guint16 val, GError **err);
gboolean ipatch_file_write_u32 (IpatchFileHandle *handle, guint32 val, GError **err);
gboolean ipatch_file_write_u64 (IpatchFileHandle *handle, guint64 val, GError **err);
gboolean ipatch_file_write_s8 (IpatchFileHandle *handle, gint8 val, GError **err);
gboolean ipatch_file_write_s16 (IpatchFileHandle *handle, gint16 val, GError **err);
gboolean ipatch_file_write_s32 (IpatchFileHandle *handle, gint32 val, GError **err);
gboolean ipatch_file_write_s64 (IpatchFileHandle *handle, gint64 val, GError **err);

gboolean ipatch_file_buf_load (IpatchFileHandle *handle, guint size, GError **err);
void ipatch_file_buf_read (IpatchFileHandle *handle, gpointer buf, guint size);
void ipatch_file_buf_write (IpatchFileHandle *handle, gconstpointer buf,
			    guint size);
#define ipatch_file_buf_zero(filebuf, size) \
  ipatch_file_buf_memset(filebuf, 0, size)
void ipatch_file_buf_memset (IpatchFileHandle *handle, char c, guint size);
void ipatch_file_buf_set_size (IpatchFileHandle *handle, guint size);
gboolean ipatch_file_buf_commit (IpatchFileHandle *handle, GError **err);
#define ipatch_file_buf_skip(filebuf, offset) \
  ipatch_file_buf_seek (filebuf, offset, G_SEEK_CUR)
void ipatch_file_buf_seek (IpatchFileHandle *handle, int offset, GSeekType type);

guint8 ipatch_file_buf_read_u8 (IpatchFileHandle *handle);
guint16 ipatch_file_buf_read_u16 (IpatchFileHandle *handle);
guint32 ipatch_file_buf_read_u32 (IpatchFileHandle *handle);
guint64 ipatch_file_buf_read_u64 (IpatchFileHandle *handle);
gint8 ipatch_file_buf_read_s8 (IpatchFileHandle *handle);
gint16 ipatch_file_buf_read_s16 (IpatchFileHandle *handle);
gint32 ipatch_file_buf_read_s32 (IpatchFileHandle *handle);
gint64 ipatch_file_buf_read_s64 (IpatchFileHandle *handle);

void ipatch_file_buf_write_u8 (IpatchFileHandle *handle, guint8 val);
void ipatch_file_buf_write_u16 (IpatchFileHandle *handle, guint16 val);
void ipatch_file_buf_write_u32 (IpatchFileHandle *handle, guint32 val);
void ipatch_file_buf_write_u64 (IpatchFileHandle *handle, guint64 val);
void ipatch_file_buf_write_s8 (IpatchFileHandle *handle, gint8 val);
void ipatch_file_buf_write_s16 (IpatchFileHandle *handle, gint16 val);
void ipatch_file_buf_write_s32 (IpatchFileHandle *handle, gint32 val);
void ipatch_file_buf_write_s64 (IpatchFileHandle *handle, gint64 val);

void ipatch_file_set_iofuncs_static (IpatchFile *file,
				     IpatchFileIOFuncs *funcs);
void ipatch_file_set_iofuncs (IpatchFile *file,
			      const IpatchFileIOFuncs *funcs);
void ipatch_file_get_iofuncs (IpatchFile *file, IpatchFileIOFuncs *out_funcs);
void ipatch_file_set_iofuncs_null (IpatchFile *file);

gboolean ipatch_file_default_open_method (IpatchFileHandle *handle, const char *mode,
					  GError **err);
void ipatch_file_default_close_method (IpatchFileHandle *handle);
GIOStatus ipatch_file_default_read_method (IpatchFileHandle *handle, gpointer buf,
					   guint size, guint *bytes_read,
					   GError **err);
GIOStatus ipatch_file_default_write_method (IpatchFileHandle *handle,
					    gconstpointer buf,
					    guint size, GError **err);
GIOStatus ipatch_file_default_seek_method (IpatchFileHandle *handle, int offset,
					   GSeekType type, GError **err);
int ipatch_file_default_getfd_method (IpatchFileHandle *handle);
int ipatch_file_default_get_size_method (IpatchFile *file, GError **err);

#endif
