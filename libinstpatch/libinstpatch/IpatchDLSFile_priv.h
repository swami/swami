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
#ifndef __IPATCH_DLS_FILE_PRIV_H__
#define __IPATCH_DLS_FILE_PRIV_H__

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

#define IPATCH_DLS_INSH_BANK_MASK 0x3FFF
#define IPATCH_DLS_INSH_BANK_PERCUSSION (1 << 31)

#define IPATCH_DLS_RGNH_OPTION_SELF_NON_EXCLUSIVE 0x0001

#define IPATCH_DLS_WLNK_PHASE_MASTER  0x0001
#define IPATCH_DLS_WLNK_MULTI_CHANNEL 0x0002

#define IPATCH_DLS_WSMP_NO_TRUNCATION  0x0001
#define IPATCH_DLS_WSMP_NO_COMPRESSION 0x0002

#define IPATCH_DLS_WSMP_LOOP_FORWARD  0x0000
#define IPATCH_DLS_WSMP_LOOP_RELEASE  0x0001

#endif

