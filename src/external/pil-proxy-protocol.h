/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Nikita Travkin <nikita@trvn.ru> */

#ifndef PIL_PROXY_PROTOCOL_H
#define PIL_PROXY_PROTOCOL_H

#include <efi.h>

/*
 * The "PIL Proxy" protocol is the protocol defined by the
 * PILProxyDxe driver in Qualcomm EFI. The driver casts
 * "RPMh proxy votes" for remoteprocs that are booted by the
 * firmware.
 *
 * The protocol is simple and consists of a single function
 * taking a single argument - whether user wants to cast a vote
 * or take the vote back. Casting a vote has an immediate effect,
 * but taking a vote back is asynchronous and will be delayed by
 * 0.3-5 seconds by the PILProxyDxe driver.
 *
 * Access to the protocol is made by discovering it on one of
 * "well-known" GUIDs which, in original firmware, are configured
 * in PIL configuration file or, on newer platforms, firmware FDT.
 * This header provides a list of known GUIDs and their expected
 * meaning.
 */

typedef struct {
#define PIL_PROXY_REVISION 0x10000
	UINT64 Revision;
#define PIL_PROXY_MODE_VOTE   1
#define PIL_PROXY_MODE_UNVOTE 2
	EFI_STATUS (*Vote)(UINT32 mode);
} PIL_PROXY_PROTOCOL;

/* pas_id=1  8c3 x1e */
#define PIL_PROXY_ADSP_GUID \
	{ 0x36fe27e1, 0x33e9, 0x45ad, {0x98, 0xda, 0xc8, 0x84, 0x38, 0xca, 0x88, 0x16} }

/* pas_id=4  8c3 x1e */
#define PIL_PROXY_MODEM_GUID \
	{ 0x61513695, 0xe0c6, 0x4f07, {0xbf, 0x41, 0xa5, 0x1a, 0x77, 0x70, 0x64, 0x0e} }

/* pas_id=6      x1e */
#define PIL_PROXY_WPSS_GUID \
	{ 0x1caaeca0, 0x978b, 0x4472, {0x8b, 0x36, 0x1a, 0xcd, 0x7f, 0x06, 0xb7, 0x13} }

/* pas_id=12 8c3 x1e */
#define PIL_PROXY_SLPI_GUID \
	{ 0xb2dcfc34, 0xb2a0, 0x4bb7, {0xbd, 0xb0, 0x31, 0xdb, 0x01, 0xe3, 0xcc, 0x2a} }

/* pas_id=18 8c3 x1e */
#define PIL_PROXY_CDSP_GUID \
	{ 0x45e14c04, 0xd134, 0x4ee4, {0xac, 0x13, 0x70, 0x98, 0xf0, 0xa9, 0xf2, 0x61} }

/* pas_id=23 8c3 */
#define PIL_PROXY_UNK23_GUID \
	{ 0x45654140, 0x778e, 0x4384, {0x8e, 0xec, 0x70, 0x3f, 0xda, 0xce, 0x31, 0x72} }

/* pas_id=30 8c3 */
#define PIL_PROXY_CDSP1_GUID \
	{ 0x64d08b73, 0xa56f, 0x4db7, {0xa2, 0x4a, 0x79, 0x41, 0x61, 0x38, 0x17, 0x7e} }

/* pas_id=36     x1e */
#define PIL_PROXY_ADSP_DTB_GUID \
	{ 0x7860b2df, 0x3a7b, 0x45ce, {0xaf, 0xe4, 0x14, 0x71, 0x01, 0x2c, 0xfc, 0x08} }

/* pas_id=37     x1e */
#define PIL_PROXY_CDSP_DTB_GUID \
	{ 0x973f30f9, 0xb696, 0x4252, {0xa8, 0x40, 0xf4, 0xeb, 0x99, 0xfd, 0x13, 0x0f} }

/* pas_id=38     x1e */
#define PIL_PROXY_UNK38_GUID \
	{ 0x476571ca, 0x9239, 0x4daf, {0xbf, 0x65, 0xd3, 0x26, 0xec, 0xa4, 0x3a, 0xeb} }

#endif
