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
#ifndef __IPATCH_GIG_FILE_PRIV_H__
#define __IPATCH_GIG_FILE_PRIV_H__

/*
 * A GigaSampler file is based on DLS2 with many proprietary extensions.
 * Descriptions of chunks below that start with DLS are part of the DLS
 * standard, while the "Gig" ones are GigaSampler specific.
 * Extensions and quirks for the GigaSampler format:
 *
 * Toplevel file <INFO> chunk is rather specific.
 * Sub chunks are listed in this order:
 * IARL IART ICMS ICMT ICOP ICRD IENG IGNR IKEY IMED INAM IPRD ISBJ ISFT
 * ISRC ISRF ITCH
 * The IARL chunk is always 256 bytes long and padded with spaces ' ' ????
 * The ICMT chunk is 1024 bytes and padded with NULLs
 * All other chunks are 128 bytes and padded with NULLs
 *
 *
 * lins->ins: DLS instrument
 *   INFO - DLS INFO LIST
 *     INAM - Name always 64 bytes "GigaSampler Instrument Editor 2.0", etc
 *     ISFT - Software always 12 bytes "Endless Wave"
 *   dlid - DLS unique ID
 *   insh - DLS instrument header
 *   lrgn - DLS Region LIST
 *     rgn - DLS instrument region (LIST)
 *       rgnh - DLS region header
 *       wsmp - DLS sample parameters
 *       wlnk - DLS wave link parameters
 *       3lnk - Gig dimension info
 *       3prg - Gig LIST chunk
 *         3ewl - Gig LIST chunk (one for each sub region)
 *           wsmp - DLS sample parameters (tuning, gain and loop)
 *           3ewa - Gig Envelop/LFO/Filter parameters (IpatchGigEffects)
 *         3ewl
 *           wsmp
 *           3ewa
 *         ...
 *       3dnl - Gig dimension names (up to 5 zero terminated strings)
 *       3ddp - Gig ???? (size 10, 2 byte words for each dimension?)
 *     rgn - next DLS region
 *       ...
 *   lart - DLS Articulation LIST
 *     3ewg - Gig global instrument parameters
 * 3gri - Gig LIST
 *   3gnl - Gig LIST
 *     3gnm - Gig sample group names
 * ptbl - DLS pool table
 * wvpl - DLS wave pool LIST
 *   wave - DLS RIFF wave file
 *     fmt  - DLS WAVE format
 *     INFO - DLS INFO list
 *       INAM - Name always 64 bytes
 *     data - DLS WAVE sample data
 *     smpl - Gig sample parameters
 *     3gix - Gig sample group number
 * einf - Unknown (perhaps to speed up loading?)
 */

/* RIFF chunk FOURCC guint32 integers - list chunks*/
#define IPATCH_GIG_FOURCC_3PRG  IPATCH_FOURCC ('3','p','r','g')
#define IPATCH_GIG_FOURCC_3EWL  IPATCH_FOURCC ('3','e','w','l')
#define IPATCH_GIG_FOURCC_3DNL  IPATCH_FOURCC ('3','d','n','l')
#define IPATCH_GIG_FOURCC_3GNL  IPATCH_FOURCC ('3','g','n','l')
#define IPATCH_GIG_FOURCC_3GRI  IPATCH_FOURCC ('3','g','r','i')

/* sub chunks */
#define IPATCH_GIG_FOURCC_SMPL  IPATCH_FOURCC ('s','m','p','l')
#define IPATCH_GIG_FOURCC_3DDP  IPATCH_FOURCC ('3','d','d','p')
#define IPATCH_GIG_FOURCC_3EWA  IPATCH_FOURCC ('3','e','w','a')
#define IPATCH_GIG_FOURCC_3EWG  IPATCH_FOURCC ('3','e','w','g')
#define IPATCH_GIG_FOURCC_3GIX  IPATCH_FOURCC ('3','g','i','x')
#define IPATCH_GIG_FOURCC_3GNM  IPATCH_FOURCC ('3','g','n','m')
#define IPATCH_GIG_FOURCC_3LNK  IPATCH_FOURCC ('3','l','n','k')
#define IPATCH_GIG_FOURCC_EINF  IPATCH_FOURCC ('e','i','n','f')

/* file chunk sizes */
#define IPATCH_GIG_SMPL_SIZE  60
#define IPATCH_GIG_3DDP_SIZE  10
#define IPATCH_GIG_3EWA_SIZE 140
#define IPATCH_GIG_3EWG_SIZE  12
#define IPATCH_GIG_3GIX_SIZE   4
#define IPATCH_GIG_3GNM_SIZE  64
#define IPATCH_GIG_3LNK_SIZE 172

/* size of instrument and sample name INFO chunk sizes */
#define IPATCH_GIG_ITEM_INAM_SIZE 64

/* fixed sizes for toplevel file INFO chunks */
#define IPATCH_GIG_MOST_INFO_SIZE 128 /* size of all chunks except 2 below */
#define IPATCH_GIG_IARL_INFO_SIZE 256 /* this one is padded with spaces ' ' */
#define IPATCH_GIG_ICMT_INFO_SIZE 1024

/* Software INFO value for GigaSampler instruments */
/* FIXME - Should we put something else there? */
#define IPATCH_GIG_INST_ISFT_VAL "Endless Wave"

#endif

