/*
 * This is the header file for the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * IpatchMD5 structure, pass it to ipatch_md5_init, call ipatch_md5_update as
 * needed on buffers full of bytes, and then call ipatch_md5_final, which
 * will fill a supplied 16-byte array with the digest.
 *
 * Changed so as no longer to depend on Colin Plumb's `usual.h'
 * header definitions; now uses stuff from dpkg's config.h
 *  - Ian Jackson <ijackson@nyx.cs.du.edu>.
 * Still in the public domain.
 *
 * Josh Coalson: made some changes to integrate with libFLAC.
 * Still in the public domain.
 *
 * Josh Green: made some changes to integrate with libInstPatch.
 * Still in the public domain.
 */
#ifndef __IPATCH__MD5_H__
#define __IPATCH__MD5_H__

#include <glib.h>

/**
 * IpatchMD5: (skip)
 */
typedef struct
{
  guint32 buf[4];
  guint32 bytes[2];
  guint32 in[16];
} IpatchMD5;

void ipatch_md5_init (IpatchMD5 *ctx);
void ipatch_md5_update (IpatchMD5 *ctx, guint8 const *buf, unsigned len);
void ipatch_md5_final (IpatchMD5 *ctx, guint8 digest[16]);

#endif /* !MD5_H */
