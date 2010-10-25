/*
 * libInstPatch
 * Copyright (C) 1999-2010 Joshua "Element" Green <jgreen@users.sourceforge.net>
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
 * SECTION: IpatchDLSFile
 * @short_description: DLS file object and functions
 * @see_also: 
 * @stability: Stable
 *
 * Object type for DLS files and other constants and functions dealing with
 * them.
 */
#ifndef __IPATCH_DLS_FILE_H__
#define __IPATCH_DLS_FILE_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchFile.h>
#include <libinstpatch/IpatchRiff.h>

typedef struct _IpatchDLSFile IpatchDLSFile;
typedef struct _IpatchDLSFileClass IpatchDLSFileClass;

#define IPATCH_TYPE_DLS_FILE   (ipatch_dls_file_get_type ())
#define IPATCH_DLS_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_DLS_FILE, IpatchDLSFile))
#define IPATCH_DLS_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_DLS_FILE, \
   IpatchDLSFileClass))
#define IPATCH_IS_DLS_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_DLS_FILE))
#define IPATCH_IS_DLS_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_DLS_FILE))
#define IPATCH_DLS_FILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_DLS_FILE, \
   IpatchDLSFileClass))

/* DLS file object (derived from IpatchFile) */
struct _IpatchDLSFile
{
  IpatchFile parent_instance;
};

/* DLS file class (derived from IpatchFile) */
struct _IpatchDLSFileClass
{
  IpatchFileClass parent_class;
};


/* RIFF chunk FOURCC guint32 integers */
#define IPATCH_DLS_FOURCC_DLS   IPATCH_FOURCC ('D','L','S',' ')
#define IPATCH_DLS_FOURCC_COLH  IPATCH_FOURCC ('c','o','l','h')
#define IPATCH_DLS_FOURCC_WVPL  IPATCH_FOURCC ('w','v','p','l')
#define IPATCH_DLS_FOURCC_DWPL  IPATCH_FOURCC ('d','w','p','l')
#define IPATCH_DLS_FOURCC_PTBL  IPATCH_FOURCC ('p','t','b','l')
#define IPATCH_DLS_FOURCC_PATH  IPATCH_FOURCC ('p','a','t','h')
#define IPATCH_DLS_FOURCC_WAVE  IPATCH_FOURCC ('w','a','v','e')
#define IPATCH_DLS_FOURCC_LINS  IPATCH_FOURCC ('l','i','n','s')
#define IPATCH_DLS_FOURCC_INS   IPATCH_FOURCC ('i','n','s',' ')
#define IPATCH_DLS_FOURCC_INSH  IPATCH_FOURCC ('i','n','s','h')
#define IPATCH_DLS_FOURCC_LRGN  IPATCH_FOURCC ('l','r','g','n')
#define IPATCH_DLS_FOURCC_RGN   IPATCH_FOURCC ('r','g','n',' ')
#define IPATCH_DLS_FOURCC_RGNH  IPATCH_FOURCC ('r','g','n','h')
#define IPATCH_DLS_FOURCC_LART  IPATCH_FOURCC ('l','a','r','t')
#define IPATCH_DLS_FOURCC_ART1  IPATCH_FOURCC ('a','r','t','1')
#define IPATCH_DLS_FOURCC_WLNK  IPATCH_FOURCC ('w','l','n','k')
#define IPATCH_DLS_FOURCC_WSMP  IPATCH_FOURCC ('w','s','m','p')
#define IPATCH_DLS_FOURCC_VERS  IPATCH_FOURCC ('v','e','r','s')
#define IPATCH_DLS_FOURCC_RGN2  IPATCH_FOURCC ('r','g','n','2')
#define IPATCH_DLS_FOURCC_LAR2  IPATCH_FOURCC ('l','a','r','2')
#define IPATCH_DLS_FOURCC_ART2  IPATCH_FOURCC ('a','r','t','2')
#define IPATCH_DLS_FOURCC_CDL   IPATCH_FOURCC ('c','d','l',' ')
#define IPATCH_DLS_FOURCC_DLID  IPATCH_FOURCC ('d','l','i','d')
#define IPATCH_DLS_FOURCC_INFO  IPATCH_FOURCC ('I','N','F','O')
#define IPATCH_DLS_FOURCC_FMT   IPATCH_FOURCC ('f','m','t',' ')
#define IPATCH_DLS_FOURCC_DATA  IPATCH_FOURCC ('d','a','t','a')

/* INFO FOURCC ids (user friendly defines in IpatchDLS2Info.h) */
#define IPATCH_DLS_FOURCC_IARL  IPATCH_FOURCC ('I','A','R','L')
#define IPATCH_DLS_FOURCC_IART  IPATCH_FOURCC ('I','A','R','T')
#define IPATCH_DLS_FOURCC_ICMS  IPATCH_FOURCC ('I','C','M','S')
#define IPATCH_DLS_FOURCC_ICMT  IPATCH_FOURCC ('I','C','M','T')
#define IPATCH_DLS_FOURCC_ICOP  IPATCH_FOURCC ('I','C','O','P')
#define IPATCH_DLS_FOURCC_ICRD  IPATCH_FOURCC ('I','C','R','D')
#define IPATCH_DLS_FOURCC_IENG  IPATCH_FOURCC ('I','E','N','G')
#define IPATCH_DLS_FOURCC_IGNR  IPATCH_FOURCC ('I','G','N','R')
#define IPATCH_DLS_FOURCC_IKEY  IPATCH_FOURCC ('I','K','E','Y')
#define IPATCH_DLS_FOURCC_IMED  IPATCH_FOURCC ('I','M','E','D')
#define IPATCH_DLS_FOURCC_INAM  IPATCH_FOURCC ('I','N','A','M')
#define IPATCH_DLS_FOURCC_IPRD  IPATCH_FOURCC ('I','P','R','D')
#define IPATCH_DLS_FOURCC_ISBJ  IPATCH_FOURCC ('I','S','B','J')
#define IPATCH_DLS_FOURCC_ISFT  IPATCH_FOURCC ('I','S','F','T')
#define IPATCH_DLS_FOURCC_ISRC  IPATCH_FOURCC ('I','S','R','C')
#define IPATCH_DLS_FOURCC_ISRF  IPATCH_FOURCC ('I','S','R','F')
#define IPATCH_DLS_FOURCC_ITCH  IPATCH_FOURCC ('I','T','C','H')


/* file chunk sizes */
#define IPATCH_DLS_VERS_SIZE        8 /* version chunk size */
#define IPATCH_DLS_INSH_SIZE        12 /* instrument header chunk size */
#define IPATCH_DLS_RGNH_SIZE        12 /* region header size */
#define IPATCH_DLS_RGNH_LAYER_SIZE  14 /* with optional Layer field */
#define IPATCH_DLS_WLNK_SIZE        12 /* wave link chunk size */
#define IPATCH_DLS_WSMP_HEADER_SIZE 20	/* sample info chunk without loops */
#define IPATCH_DLS_WSMP_LOOP_SIZE   16 /* sample loop size */
#define IPATCH_DLS_ART_HEADER_SIZE  8 /* articulator header size */
#define IPATCH_DLS_CONN_SIZE        12 /* connection block size */
#define IPATCH_DLS_PTBL_HEADER_SIZE 8 /* default pool table header size */
#define IPATCH_DLS_POOLCUE_SIZE     4 /* size of a pool cue offset */
#define IPATCH_DLS_WAVE_FMT_SIZE    16	/* PCM wave fmt chunk size */
#define IPATCH_DLS_DLID_SIZE        16 /* DLID unique ID chunk size */

#define IPATCH_DLS_INSH_BANK_MASK 0x3FFF
#define IPATCH_DLS_INSH_BANK_PERCUSSION (1 << 31)

#define IPATCH_DLS_RGNH_OPTION_SELF_NON_EXCLUSIVE 0x0001

#define IPATCH_DLS_WLNK_PHASE_MASTER  0x0001
#define IPATCH_DLS_WLNK_MULTI_CHANNEL 0x0002

#define IPATCH_DLS_WSMP_NO_TRUNCATION  0x0001
#define IPATCH_DLS_WSMP_NO_COMPRESSION 0x0002

#define IPATCH_DLS_WSMP_LOOP_FORWARD  0x0000
#define IPATCH_DLS_WSMP_LOOP_RELEASE  0x0001

GType ipatch_dls_file_get_type (void);
IpatchDLSFile *ipatch_dls_file_new (void);

#endif
