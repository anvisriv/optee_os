// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2017-2020, Linaro Limited
 */

#include <assert.h>
#include <pkcs11_ta.h>
#include <tee_api_defines.h>
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <util.h>

#include "attributes.h"
#include "object.h"
#include "pkcs11_token.h"
#include "processing.h"

/*
 * DER encoded EC parameters generated with script:
 *   ta/pkcs11/scripts/dump_ec_curve_params.sh
 */

static const uint8_t prime192v1_name_der[] = {
	0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03,
	0x01, 0x01,
};

static const uint8_t secp224r1_name_der[] = {
	0x06, 0x05, 0x2b, 0x81, 0x04, 0x00, 0x21,
};

static const uint8_t prime256v1_name_der[] = {
	0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03,
	0x01, 0x07,
};

static const uint8_t secp384r1_name_der[] = {
	0x06, 0x05, 0x2b, 0x81, 0x04, 0x00, 0x22,
};

static const uint8_t secp521r1_name_der[] = {
	0x06, 0x05, 0x2b, 0x81, 0x04, 0x00, 0x23,
};

static const uint8_t prime192v1_oid_der[] = {
	0x30, 0x81, 0xc7, 0x02, 0x01, 0x01, 0x30, 0x24,
	0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x01,
	0x01, 0x02, 0x19, 0x00, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0x30, 0x4b, 0x04, 0x18,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc,
	0x04, 0x18, 0x64, 0x21, 0x05, 0x19, 0xe5, 0x9c,
	0x80, 0xe7, 0x0f, 0xa7, 0xe9, 0xab, 0x72, 0x24,
	0x30, 0x49, 0xfe, 0xb8, 0xde, 0xec, 0xc1, 0x46,
	0xb9, 0xb1, 0x03, 0x15, 0x00, 0x30, 0x45, 0xae,
	0x6f, 0xc8, 0x42, 0x2f, 0x64, 0xed, 0x57, 0x95,
	0x28, 0xd3, 0x81, 0x20, 0xea, 0xe1, 0x21, 0x96,
	0xd5, 0x04, 0x31, 0x04, 0x18, 0x8d, 0xa8, 0x0e,
	0xb0, 0x30, 0x90, 0xf6, 0x7c, 0xbf, 0x20, 0xeb,
	0x43, 0xa1, 0x88, 0x00, 0xf4, 0xff, 0x0a, 0xfd,
	0x82, 0xff, 0x10, 0x12, 0x07, 0x19, 0x2b, 0x95,
	0xff, 0xc8, 0xda, 0x78, 0x63, 0x10, 0x11, 0xed,
	0x6b, 0x24, 0xcd, 0xd5, 0x73, 0xf9, 0x77, 0xa1,
	0x1e, 0x79, 0x48, 0x11, 0x02, 0x19, 0x00, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0x99, 0xde, 0xf8, 0x36, 0x14,
	0x6b, 0xc9, 0xb1, 0xb4, 0xd2, 0x28, 0x31, 0x02,
	0x01, 0x01,
};

static const uint8_t secp224r1_oid_der[] = {
	0x30, 0x81, 0xdf, 0x02, 0x01, 0x01, 0x30, 0x28,
	0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x01,
	0x01, 0x02, 0x1d, 0x00, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x30, 0x53, 0x04, 0x1c, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
	0x04, 0x1c, 0xb4, 0x05, 0x0a, 0x85, 0x0c, 0x04,
	0xb3, 0xab, 0xf5, 0x41, 0x32, 0x56, 0x50, 0x44,
	0xb0, 0xb7, 0xd7, 0xbf, 0xd8, 0xba, 0x27, 0x0b,
	0x39, 0x43, 0x23, 0x55, 0xff, 0xb4, 0x03, 0x15,
	0x00, 0xbd, 0x71, 0x34, 0x47, 0x99, 0xd5, 0xc7,
	0xfc, 0xdc, 0x45, 0xb5, 0x9f, 0xa3, 0xb9, 0xab,
	0x8f, 0x6a, 0x94, 0x8b, 0xc5, 0x04, 0x39, 0x04,
	0xb7, 0x0e, 0x0c, 0xbd, 0x6b, 0xb4, 0xbf, 0x7f,
	0x32, 0x13, 0x90, 0xb9, 0x4a, 0x03, 0xc1, 0xd3,
	0x56, 0xc2, 0x11, 0x22, 0x34, 0x32, 0x80, 0xd6,
	0x11, 0x5c, 0x1d, 0x21, 0xbd, 0x37, 0x63, 0x88,
	0xb5, 0xf7, 0x23, 0xfb, 0x4c, 0x22, 0xdf, 0xe6,
	0xcd, 0x43, 0x75, 0xa0, 0x5a, 0x07, 0x47, 0x64,
	0x44, 0xd5, 0x81, 0x99, 0x85, 0x00, 0x7e, 0x34,
	0x02, 0x1d, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0x16, 0xa2, 0xe0, 0xb8, 0xf0, 0x3e, 0x13,
	0xdd, 0x29, 0x45, 0x5c, 0x5c, 0x2a, 0x3d, 0x02,
	0x01, 0x01,
};

static const uint8_t prime256v1_oid_der[] = {
	0x30, 0x81, 0xf7, 0x02, 0x01, 0x01, 0x30, 0x2c,
	0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x01,
	0x01, 0x02, 0x21, 0x00, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0x30, 0x5b, 0x04, 0x20,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc,
	0x04, 0x20, 0x5a, 0xc6, 0x35, 0xd8, 0xaa, 0x3a,
	0x93, 0xe7, 0xb3, 0xeb, 0xbd, 0x55, 0x76, 0x98,
	0x86, 0xbc, 0x65, 0x1d, 0x06, 0xb0, 0xcc, 0x53,
	0xb0, 0xf6, 0x3b, 0xce, 0x3c, 0x3e, 0x27, 0xd2,
	0x60, 0x4b, 0x03, 0x15, 0x00, 0xc4, 0x9d, 0x36,
	0x08, 0x86, 0xe7, 0x04, 0x93, 0x6a, 0x66, 0x78,
	0xe1, 0x13, 0x9d, 0x26, 0xb7, 0x81, 0x9f, 0x7e,
	0x90, 0x04, 0x41, 0x04, 0x6b, 0x17, 0xd1, 0xf2,
	0xe1, 0x2c, 0x42, 0x47, 0xf8, 0xbc, 0xe6, 0xe5,
	0x63, 0xa4, 0x40, 0xf2, 0x77, 0x03, 0x7d, 0x81,
	0x2d, 0xeb, 0x33, 0xa0, 0xf4, 0xa1, 0x39, 0x45,
	0xd8, 0x98, 0xc2, 0x96, 0x4f, 0xe3, 0x42, 0xe2,
	0xfe, 0x1a, 0x7f, 0x9b, 0x8e, 0xe7, 0xeb, 0x4a,
	0x7c, 0x0f, 0x9e, 0x16, 0x2b, 0xce, 0x33, 0x57,
	0x6b, 0x31, 0x5e, 0xce, 0xcb, 0xb6, 0x40, 0x68,
	0x37, 0xbf, 0x51, 0xf5, 0x02, 0x21, 0x00, 0xff,
	0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbc,
	0xe6, 0xfa, 0xad, 0xa7, 0x17, 0x9e, 0x84, 0xf3,
	0xb9, 0xca, 0xc2, 0xfc, 0x63, 0x25, 0x51, 0x02,
	0x01, 0x01,
};

static const uint8_t secp384r1_oid_der[] = {
	0x30, 0x82, 0x01, 0x57, 0x02, 0x01, 0x01, 0x30,
	0x3c, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d,
	0x01, 0x01, 0x02, 0x31, 0x00, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff,
	0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xff, 0xff, 0xff, 0xff, 0x30, 0x7b, 0x04,
	0x30, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xfe, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0xfc, 0x04, 0x30, 0xb3, 0x31, 0x2f, 0xa7, 0xe2,
	0x3e, 0xe7, 0xe4, 0x98, 0x8e, 0x05, 0x6b, 0xe3,
	0xf8, 0x2d, 0x19, 0x18, 0x1d, 0x9c, 0x6e, 0xfe,
	0x81, 0x41, 0x12, 0x03, 0x14, 0x08, 0x8f, 0x50,
	0x13, 0x87, 0x5a, 0xc6, 0x56, 0x39, 0x8d, 0x8a,
	0x2e, 0xd1, 0x9d, 0x2a, 0x85, 0xc8, 0xed, 0xd3,
	0xec, 0x2a, 0xef, 0x03, 0x15, 0x00, 0xa3, 0x35,
	0x92, 0x6a, 0xa3, 0x19, 0xa2, 0x7a, 0x1d, 0x00,
	0x89, 0x6a, 0x67, 0x73, 0xa4, 0x82, 0x7a, 0xcd,
	0xac, 0x73, 0x04, 0x61, 0x04, 0xaa, 0x87, 0xca,
	0x22, 0xbe, 0x8b, 0x05, 0x37, 0x8e, 0xb1, 0xc7,
	0x1e, 0xf3, 0x20, 0xad, 0x74, 0x6e, 0x1d, 0x3b,
	0x62, 0x8b, 0xa7, 0x9b, 0x98, 0x59, 0xf7, 0x41,
	0xe0, 0x82, 0x54, 0x2a, 0x38, 0x55, 0x02, 0xf2,
	0x5d, 0xbf, 0x55, 0x29, 0x6c, 0x3a, 0x54, 0x5e,
	0x38, 0x72, 0x76, 0x0a, 0xb7, 0x36, 0x17, 0xde,
	0x4a, 0x96, 0x26, 0x2c, 0x6f, 0x5d, 0x9e, 0x98,
	0xbf, 0x92, 0x92, 0xdc, 0x29, 0xf8, 0xf4, 0x1d,
	0xbd, 0x28, 0x9a, 0x14, 0x7c, 0xe9, 0xda, 0x31,
	0x13, 0xb5, 0xf0, 0xb8, 0xc0, 0x0a, 0x60, 0xb1,
	0xce, 0x1d, 0x7e, 0x81, 0x9d, 0x7a, 0x43, 0x1d,
	0x7c, 0x90, 0xea, 0x0e, 0x5f, 0x02, 0x31, 0x00,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xc7, 0x63, 0x4d, 0x81, 0xf4, 0x37, 0x2d, 0xdf,
	0x58, 0x1a, 0x0d, 0xb2, 0x48, 0xb0, 0xa7, 0x7a,
	0xec, 0xec, 0x19, 0x6a, 0xcc, 0xc5, 0x29, 0x73,
	0x02, 0x01, 0x01,
};

static const uint8_t secp521r1_oid_der[] = {
	0x30, 0x82, 0x01, 0xc3, 0x02, 0x01, 0x01, 0x30,
	0x4d, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d,
	0x01, 0x01, 0x02, 0x42, 0x01, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x30, 0x81,
	0x9f, 0x04, 0x42, 0x01, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xfc, 0x04, 0x42, 0x00,
	0x51, 0x95, 0x3e, 0xb9, 0x61, 0x8e, 0x1c, 0x9a,
	0x1f, 0x92, 0x9a, 0x21, 0xa0, 0xb6, 0x85, 0x40,
	0xee, 0xa2, 0xda, 0x72, 0x5b, 0x99, 0xb3, 0x15,
	0xf3, 0xb8, 0xb4, 0x89, 0x91, 0x8e, 0xf1, 0x09,
	0xe1, 0x56, 0x19, 0x39, 0x51, 0xec, 0x7e, 0x93,
	0x7b, 0x16, 0x52, 0xc0, 0xbd, 0x3b, 0xb1, 0xbf,
	0x07, 0x35, 0x73, 0xdf, 0x88, 0x3d, 0x2c, 0x34,
	0xf1, 0xef, 0x45, 0x1f, 0xd4, 0x6b, 0x50, 0x3f,
	0x00, 0x03, 0x15, 0x00, 0xd0, 0x9e, 0x88, 0x00,
	0x29, 0x1c, 0xb8, 0x53, 0x96, 0xcc, 0x67, 0x17,
	0x39, 0x32, 0x84, 0xaa, 0xa0, 0xda, 0x64, 0xba,
	0x04, 0x81, 0x85, 0x04, 0x00, 0xc6, 0x85, 0x8e,
	0x06, 0xb7, 0x04, 0x04, 0xe9, 0xcd, 0x9e, 0x3e,
	0xcb, 0x66, 0x23, 0x95, 0xb4, 0x42, 0x9c, 0x64,
	0x81, 0x39, 0x05, 0x3f, 0xb5, 0x21, 0xf8, 0x28,
	0xaf, 0x60, 0x6b, 0x4d, 0x3d, 0xba, 0xa1, 0x4b,
	0x5e, 0x77, 0xef, 0xe7, 0x59, 0x28, 0xfe, 0x1d,
	0xc1, 0x27, 0xa2, 0xff, 0xa8, 0xde, 0x33, 0x48,
	0xb3, 0xc1, 0x85, 0x6a, 0x42, 0x9b, 0xf9, 0x7e,
	0x7e, 0x31, 0xc2, 0xe5, 0xbd, 0x66, 0x01, 0x18,
	0x39, 0x29, 0x6a, 0x78, 0x9a, 0x3b, 0xc0, 0x04,
	0x5c, 0x8a, 0x5f, 0xb4, 0x2c, 0x7d, 0x1b, 0xd9,
	0x98, 0xf5, 0x44, 0x49, 0x57, 0x9b, 0x44, 0x68,
	0x17, 0xaf, 0xbd, 0x17, 0x27, 0x3e, 0x66, 0x2c,
	0x97, 0xee, 0x72, 0x99, 0x5e, 0xf4, 0x26, 0x40,
	0xc5, 0x50, 0xb9, 0x01, 0x3f, 0xad, 0x07, 0x61,
	0x35, 0x3c, 0x70, 0x86, 0xa2, 0x72, 0xc2, 0x40,
	0x88, 0xbe, 0x94, 0x76, 0x9f, 0xd1, 0x66, 0x50,
	0x02, 0x42, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xfa, 0x51, 0x86, 0x87, 0x83,
	0xbf, 0x2f, 0x96, 0x6b, 0x7f, 0xcc, 0x01, 0x48,
	0xf7, 0x09, 0xa5, 0xd0, 0x3b, 0xb5, 0xc9, 0xb8,
	0x89, 0x9c, 0x47, 0xae, 0xbb, 0x6f, 0xb7, 0x1e,
	0x91, 0x38, 0x64, 0x09, 0x02, 0x01, 0x01,
};

/*
 * Edwards curves may be specified in two flavours:
 * - as a PrintableString 'edwards25519' or 'edwards448'
 * - as an OID, DER encoded ASN.1 Object
 */

static const uint8_t ed25519_name_der[] = {
	0x13, 0x0c, 'e', 'd', 'w', 'a', 'r', 'd', 's',
	'2', '5', '5', '1', '9',
};

static const uint8_t ed25519_oid_der[] = {
	0x06, 0x09, 0x2b, 0x06, 0x01, 0x04, 0x01, 0xda,
	0x47, 0x0f, 0x01,
};

struct supported_ecc_curve {
	const uint8_t *oid_der;
	size_t oid_size;
	const uint8_t *name_der;
	size_t name_size;
	size_t key_size;
	uint32_t tee_id;
	const char *label;
	size_t label_size;
};

#define ECC_CURVE(_tee_id, _key_size, _label)			\
	{							\
		.tee_id = (_tee_id),				\
		.key_size = (_key_size),			\
		.oid_der = _label ## _oid_der,			\
		.oid_size = sizeof(_label ## _oid_der),		\
		.name_der = _label ## _name_der,		\
		.name_size = sizeof(_label ## _name_der),	\
		.label = #_label,				\
		.label_size = sizeof(#_label) - 1,		\
	}

static const struct supported_ecc_curve ec_curve_param[] = {
	ECC_CURVE(TEE_ECC_CURVE_NIST_P192, 192, prime192v1),
	ECC_CURVE(TEE_ECC_CURVE_NIST_P224, 224, secp224r1),
	ECC_CURVE(TEE_ECC_CURVE_NIST_P256, 256, prime256v1),
	ECC_CURVE(TEE_ECC_CURVE_NIST_P384, 384, secp384r1),
	ECC_CURVE(TEE_ECC_CURVE_NIST_P521, 521, secp521r1),
	ECC_CURVE(TEE_ECC_CURVE_25519, 256, ed25519),
};

static const struct supported_ecc_curve *get_curve(void *attr, size_t size)
{
	size_t idx = 0;

	/* Weak: not a real DER parser: try by params then by named curve */
	for (idx = 0; idx < ARRAY_SIZE(ec_curve_param); idx++) {
		const struct supported_ecc_curve *curve = ec_curve_param + idx;

		if (size == curve->oid_size &&
		    !TEE_MemCompare(attr, curve->oid_der, curve->oid_size))
			return curve;

		if (size == curve->name_size &&
		    !TEE_MemCompare(attr, curve->name_der, curve->name_size))
			return curve;
	}

	return NULL;
}

size_t ec_params2tee_keysize(void *ec_params, size_t size)
{
	const struct supported_ecc_curve *curve = get_curve(ec_params, size);

	if (!curve)
		return 0;

	return curve->key_size;
}

/*
 * This function intentionally panics if the curve is not found.
 * Use ec_params2tee_keysize() to check the curve is supported by
 * the internal core API.
 */
uint32_t ec_params2tee_curve(void *ec_params, size_t size)
{
	const struct supported_ecc_curve *curve = get_curve(ec_params, size);

	assert(curve);

	return curve->tee_id;
}

enum pkcs11_rc load_tee_ec_key_attrs(TEE_Attribute **tee_attrs,
				     size_t *tee_count,
				     struct pkcs11_object *obj)
{
	TEE_Attribute *attrs = NULL;
	size_t count = 0;
	enum pkcs11_rc rc = PKCS11_CKR_GENERAL_ERROR;

	assert(get_key_type(obj->attributes) == PKCS11_CKK_EC);

	switch (get_class(obj->attributes)) {
	case PKCS11_CKO_PUBLIC_KEY:
		attrs = TEE_Malloc(3 * sizeof(TEE_Attribute),
				   TEE_USER_MEM_HINT_NO_FILL_ZERO);
		if (!attrs)
			return PKCS11_CKR_DEVICE_MEMORY;

		if (pkcs2tee_load_attr(&attrs[count], TEE_ATTR_ECC_CURVE,
				       obj, PKCS11_CKA_EC_PARAMS))
			count++;

		if (pkcs2tee_load_attr(&attrs[count],
				       TEE_ATTR_ECC_PUBLIC_VALUE_X,
				       obj, PKCS11_CKA_EC_POINT))
			count++;

		if (pkcs2tee_load_attr(&attrs[count],
				       TEE_ATTR_ECC_PUBLIC_VALUE_Y,
				       obj, PKCS11_CKA_EC_POINT))
			count++;

		if (count == 3)
			rc = PKCS11_CKR_OK;

		break;

	case PKCS11_CKO_PRIVATE_KEY:
		attrs = TEE_Malloc(4 * sizeof(TEE_Attribute),
				   TEE_USER_MEM_HINT_NO_FILL_ZERO);
		if (!attrs)
			return PKCS11_CKR_DEVICE_MEMORY;

		if (pkcs2tee_load_attr(&attrs[count], TEE_ATTR_ECC_CURVE,
				       obj, PKCS11_CKA_EC_PARAMS))
			count++;

		if (pkcs2tee_load_attr(&attrs[count],
				       TEE_ATTR_ECC_PRIVATE_VALUE,
				       obj, PKCS11_CKA_VALUE))
			count++;

		/*
		 * Standard does not have CKA_EC_POINT for EC private keys
		 * but that is required by TEE internal API. First try to get
		 * hidden EC POINT and for backwards compatibility then try to
		 * get CKA_EC_POINT value.
		 */

		if (pkcs2tee_load_attr(&attrs[count],
				       TEE_ATTR_ECC_PUBLIC_VALUE_X,
				       obj, PKCS11_CKA_OPTEE_HIDDEN_EC_POINT))
			count++;
		else if (pkcs2tee_load_attr(&attrs[count],
					    TEE_ATTR_ECC_PUBLIC_VALUE_X,
					    obj, PKCS11_CKA_EC_POINT))
			count++;

		if (pkcs2tee_load_attr(&attrs[count],
				       TEE_ATTR_ECC_PUBLIC_VALUE_Y,
				       obj, PKCS11_CKA_OPTEE_HIDDEN_EC_POINT))
			count++;
		else if (pkcs2tee_load_attr(&attrs[count],
					    TEE_ATTR_ECC_PUBLIC_VALUE_Y,
					    obj, PKCS11_CKA_EC_POINT))
			count++;

		if (count == 4)
			rc = PKCS11_CKR_OK;

		break;

	default:
		assert(0);
		break;
	}

	if (rc == PKCS11_CKR_OK) {
		*tee_attrs = attrs;
		*tee_count = count;
	} else {
		TEE_Free(attrs);
	}

	return rc;
}

enum pkcs11_rc pkcs2tee_algo_ecdsa(uint32_t *tee_id,
				   struct pkcs11_attribute_head *proc_params,
				   struct pkcs11_object *obj)
{

	switch (proc_params->id) {
	case PKCS11_CKM_ECDSA:
		/*
		 * In case of ECDSA signing without hashing.
		 */
		switch (get_object_key_bit_size(obj)) {
		case 192:
			*tee_id = TEE_ALG_ECDSA_P192;
			break;
		case 224:
			*tee_id = TEE_ALG_ECDSA_P224;
			break;
		case 256:
			*tee_id = TEE_ALG_ECDSA_P256;
			break;
		case 384:
			*tee_id = TEE_ALG_ECDSA_P384;
			break;
		case 521:
			*tee_id = TEE_ALG_ECDSA_P521;
			break;
		default:
			TEE_Panic(0);
			break;
		}
		break;
	/*
	 * In CASE OF ECDSA signing with Hash
	 */
	case PKCS11_CKM_ECDSA_SHA1:
		*tee_id = TEE_ALG_ECDSA_SHA1;
		break;
	case PKCS11_CKM_ECDSA_SHA224:
		*tee_id = TEE_ALG_ECDSA_SHA224;
		break;
	case PKCS11_CKM_ECDSA_SHA256:
		*tee_id = TEE_ALG_ECDSA_SHA256;
		break;
	case PKCS11_CKM_ECDSA_SHA384:
		*tee_id = TEE_ALG_ECDSA_SHA384;
		break;
	case PKCS11_CKM_ECDSA_SHA512:
		*tee_id = TEE_ALG_ECDSA_SHA512;
		break;
	default:
		return PKCS11_CKR_GENERAL_ERROR;
    }

	return PKCS11_CKR_OK;
}

static enum pkcs11_rc tee2pkcs_ec_attributes(struct obj_attrs **pub_head,
					     struct obj_attrs **priv_head,
					     TEE_ObjectHandle tee_obj,
					     size_t tee_size)
{
	void *x_ptr = NULL;
	void *y_ptr = NULL;
	uint8_t *ecpoint = NULL;
	size_t x_size = 0;
	size_t y_size = 0;
	size_t psize = 0;
	size_t qsize = 0;
	size_t dersize = 0;
	size_t poffset = 0;
	size_t hsize = 0;
	enum pkcs11_rc rc = PKCS11_CKR_GENERAL_ERROR;

	rc = tee2pkcs_add_attribute(priv_head, PKCS11_CKA_VALUE,
				    tee_obj, TEE_ATTR_ECC_PRIVATE_VALUE);
	if (rc)
		goto out;

	rc = alloc_get_tee_attribute_data(tee_obj, TEE_ATTR_ECC_PUBLIC_VALUE_X,
					  &x_ptr, &x_size);
	if (rc)
		goto out;

	rc = alloc_get_tee_attribute_data(tee_obj, TEE_ATTR_ECC_PUBLIC_VALUE_Y,
					  &y_ptr, &y_size);
	if (rc)
		goto x_cleanup;

	psize = (tee_size + 7) / 8;
	if (x_size > psize || y_size > psize) {
		rc = PKCS11_CKR_ARGUMENTS_BAD;
		goto y_cleanup;
	}

	qsize = 1 + 2 * psize;
	if (qsize < 0x80) {
		/* DER short definitive form up to 127 bytes */
		dersize = qsize + 2;
		hsize = 2 /* der */ + 1 /* point compression */;
	} else if (qsize < 0x100) {
		/* DER long definitive form up to 255 bytes */
		dersize = qsize + 3;
		hsize = 3 /* der */ + 1 /* point compression */;
	} else {
		EMSG("Too long DER value");
		rc = PKCS11_CKR_MECHANISM_PARAM_INVALID;
		goto y_cleanup;
	}

	ecpoint = TEE_Malloc(dersize, TEE_MALLOC_FILL_ZERO);
	if (!ecpoint) {
		rc = PKCS11_CKR_DEVICE_MEMORY;
		goto y_cleanup;
	}

	if (qsize < 0x80) {
		/* DER encoding */
		ecpoint[0] = 0x04;
		ecpoint[1] = qsize & 0x7f;

		/* Only UNCOMPRESSED ECPOINT is currently supported */
		ecpoint[2] = 0x04;
	} else if (qsize < 0x100) {
		/* DER encoding */
		ecpoint[0] = 0x04;
		ecpoint[1] = 0x80 | 0x01; /* long form, one size octet */
		ecpoint[2] = qsize & 0xFF;

		/* Only UNCOMPRESSED ECPOINT is currently supported */
		ecpoint[3] = 0x04;
	}

	poffset = 0;
	if (x_size < psize)
		poffset = psize - x_size;
	TEE_MemMove(ecpoint + hsize + poffset, x_ptr, x_size);

	poffset = 0;
	if (y_size < psize)
		poffset = psize - y_size;
	TEE_MemMove(ecpoint + hsize + psize + poffset, y_ptr, y_size);

	/*
	 * Add PKCS11_CKA_OPTEE_HIDDEN_EC_POINT to private key object and
	 * standard PKCS11_CKA_EC_POINT to public key objects as
	 * TEE_PopulateTransientObject requires public x/y values
	 * for TEE_TYPE_ECDSA_KEYPAIR.
	 */
	rc = add_attribute(priv_head, PKCS11_CKA_OPTEE_HIDDEN_EC_POINT,
			   ecpoint, dersize);
	if (rc)
		goto ecpoint_cleanup;

	rc = add_attribute(pub_head, PKCS11_CKA_EC_POINT, ecpoint, dersize);

ecpoint_cleanup:
	TEE_Free(ecpoint);
y_cleanup:
	TEE_Free(y_ptr);
x_cleanup:
	TEE_Free(x_ptr);
out:
	return rc;
}

enum pkcs11_rc generate_ec_keys(struct pkcs11_attribute_head *proc_params,
				struct obj_attrs **pub_head,
				struct obj_attrs **priv_head)
{
	enum pkcs11_rc rc = PKCS11_CKR_GENERAL_ERROR;
	void *a_ptr = NULL;
	uint32_t a_size = 0;
	uint32_t tee_size = 0;
	uint32_t tee_curve = 0;
	TEE_ObjectHandle tee_obj = TEE_HANDLE_NULL;
	TEE_Attribute tee_key_attr[1] = { };
	TEE_Result res = TEE_ERROR_GENERIC;

	if (!proc_params || !*pub_head || !*priv_head)
		return PKCS11_CKR_TEMPLATE_INCONSISTENT;

	if (remove_empty_attribute(pub_head, PKCS11_CKA_EC_POINT) ||
	    remove_empty_attribute(priv_head, PKCS11_CKA_VALUE) ||
	    remove_empty_attribute(priv_head, PKCS11_CKA_EC_PARAMS)) {
		EMSG("Unexpected attribute(s) found");
		trace_attributes("public-key", *pub_head);
		trace_attributes("private-key", *priv_head);
		return PKCS11_CKR_TEMPLATE_INCONSISTENT;
	}

	if (get_attribute_ptr(*pub_head, PKCS11_CKA_EC_PARAMS,
			      &a_ptr, &a_size) || !a_ptr) {
		EMSG("No EC_PARAMS attribute found in public key");
		return PKCS11_CKR_ATTRIBUTE_TYPE_INVALID;
	}

	tee_size = ec_params2tee_keysize(a_ptr, a_size);
	if (!tee_size)
		return PKCS11_CKR_ATTRIBUTE_TYPE_INVALID;

	tee_curve = ec_params2tee_curve(a_ptr, a_size);

	TEE_InitValueAttribute(tee_key_attr, TEE_ATTR_ECC_CURVE, tee_curve, 0);

	/* Create an ECDSA TEE key: will match PKCS11 ECDSA and ECDH */
	res = TEE_AllocateTransientObject(TEE_TYPE_ECDSA_KEYPAIR, tee_size,
					  &tee_obj);
	if (res) {
		EMSG("Transient alloc failed with %#"PRIx32, res);
		return tee2pkcs_error(res);
	}

	res = TEE_RestrictObjectUsage1(tee_obj, TEE_USAGE_EXTRACTABLE);
	if (res) {
		rc = tee2pkcs_error(res);
		goto out;
	}

	res = TEE_GenerateKey(tee_obj, tee_size, tee_key_attr, 1);
	if (res) {
		rc = tee2pkcs_error(res);
		goto out;
	}

	/* Private key needs the same EC_PARAMS as used by the public key */
	rc = add_attribute(priv_head, PKCS11_CKA_EC_PARAMS, a_ptr, a_size);
	if (rc)
		goto out;

	rc = tee2pkcs_ec_attributes(pub_head, priv_head, tee_obj, tee_size);

out:
	if (tee_obj != TEE_HANDLE_NULL)
		TEE_CloseObject(tee_obj);

	return rc;
}

enum pkcs11_rc load_tee_eddsa_key_attrs(TEE_Attribute **tee_attrs,
					size_t *tee_count,
					struct pkcs11_object *obj)
{
	TEE_Attribute *attrs = NULL;
	size_t count = 0;
	enum pkcs11_rc rc = PKCS11_CKR_GENERAL_ERROR;

	assert(get_key_type(obj->attributes) == PKCS11_CKK_EC_EDWARDS);

	switch (get_class(obj->attributes)) {
	case PKCS11_CKO_PUBLIC_KEY:
		attrs = TEE_Malloc(sizeof(TEE_Attribute),
				   TEE_USER_MEM_HINT_NO_FILL_ZERO);
		if (!attrs)
			return PKCS11_CKR_DEVICE_MEMORY;

		if (pkcs2tee_load_attr(&attrs[count],
				       TEE_ATTR_ED25519_PUBLIC_VALUE,
				       obj, PKCS11_CKA_EC_POINT))
			count++;

		if (count == 1)
			rc = PKCS11_CKR_OK;

		break;

	case PKCS11_CKO_PRIVATE_KEY:
		attrs = TEE_Malloc(2 * sizeof(TEE_Attribute),
				   TEE_USER_MEM_HINT_NO_FILL_ZERO);
		if (!attrs)
			return PKCS11_CKR_DEVICE_MEMORY;

		if (pkcs2tee_load_attr(&attrs[count],
				       TEE_ATTR_ED25519_PRIVATE_VALUE,
				       obj, PKCS11_CKA_VALUE))
			count++;

		if (pkcs2tee_load_attr(&attrs[count],
				       TEE_ATTR_ED25519_PUBLIC_VALUE,
				       obj, PKCS11_CKA_EC_POINT))
			count++;

		if (count == 2)
			rc = PKCS11_CKR_OK;

		break;

	default:
		assert(0);
		break;
	}

	if (rc == PKCS11_CKR_OK) {
		*tee_attrs = attrs;
		*tee_count = count;
	} else {
		TEE_Free(attrs);
	}

	return rc;
}

enum pkcs11_rc generate_eddsa_keys(struct pkcs11_attribute_head *proc_params,
				   struct obj_attrs **pub_head,
				   struct obj_attrs **priv_head)
{
	enum pkcs11_rc rc = PKCS11_CKR_GENERAL_ERROR;
	void *a_ptr = NULL;
	uint32_t a_size = 0;
	uint32_t tee_size = 0;
	TEE_ObjectHandle tee_obj = TEE_HANDLE_NULL;
	TEE_Result res = TEE_ERROR_GENERIC;

	if (!proc_params || !*pub_head || !*priv_head)
		return PKCS11_CKR_TEMPLATE_INCONSISTENT;

	if (remove_empty_attribute(pub_head, PKCS11_CKA_EC_POINT) ||
	    remove_empty_attribute(priv_head, PKCS11_CKA_VALUE) ||
	    remove_empty_attribute(priv_head, PKCS11_CKA_EC_POINT) ||
	    remove_empty_attribute(priv_head, PKCS11_CKA_EC_PARAMS)) {
		EMSG("Unexpected attribute(s) found");
		trace_attributes("public-key", *pub_head);
		trace_attributes("private-key", *priv_head);
		return PKCS11_CKR_TEMPLATE_INCONSISTENT;
	}

	if (get_attribute_ptr(*pub_head, PKCS11_CKA_EC_PARAMS,
			      &a_ptr, &a_size) || !a_ptr) {
		EMSG("No EC_PARAMS attribute found in public key");
		return PKCS11_CKR_ATTRIBUTE_TYPE_INVALID;
	}

	tee_size = ec_params2tee_keysize(a_ptr, a_size);
	if (!tee_size)
		return PKCS11_CKR_ATTRIBUTE_TYPE_INVALID;

	res = TEE_AllocateTransientObject(TEE_TYPE_ED25519_KEYPAIR, tee_size,
					  &tee_obj);
	if (res) {
		EMSG("Transient alloc failed with %#"PRIx32, res);
		return tee2pkcs_error(res);
	}

	res = TEE_RestrictObjectUsage1(tee_obj, TEE_USAGE_EXTRACTABLE);
	if (res) {
		rc = tee2pkcs_error(res);
		goto out;
	}

	res = TEE_GenerateKey(tee_obj, tee_size, NULL, 0);
	if (res) {
		rc = tee2pkcs_error(res);
		goto out;
	}

	/* Private key needs the same EC_PARAMS as used by the public key */
	rc = add_attribute(priv_head, PKCS11_CKA_EC_PARAMS, a_ptr, a_size);
	if (rc)
		goto out;

	rc = tee2pkcs_add_attribute(priv_head, PKCS11_CKA_VALUE,
				    tee_obj, TEE_ATTR_ED25519_PRIVATE_VALUE);
	if (rc)
		goto out;

	rc = tee2pkcs_add_attribute(priv_head, PKCS11_CKA_EC_POINT,
				    tee_obj, TEE_ATTR_ED25519_PUBLIC_VALUE);
	if (rc)
		goto out;

	rc = tee2pkcs_add_attribute(pub_head, PKCS11_CKA_EC_POINT,
				    tee_obj, TEE_ATTR_ED25519_PUBLIC_VALUE);

out:
	if (tee_obj != TEE_HANDLE_NULL)
		TEE_CloseObject(tee_obj);

	return rc;
}

enum pkcs11_rc
pkcs2tee_proc_params_eddsa(struct active_processing *proc,
			   struct pkcs11_attribute_head *proc_params)
{
	enum pkcs11_rc rc = PKCS11_CKR_GENERAL_ERROR;
	uint32_t ctx_len = 0;
	uint32_t flag = 0;
	void *ctx_data = NULL;
	struct serialargs args = { };
	struct eddsa_processing_ctx *ctx = NULL;

	serialargs_init(&args, proc_params->data, proc_params->size);

	rc = serialargs_get_u32(&args, &flag);
	if (rc)
		return rc;

	rc = serialargs_get_u32(&args, &ctx_len);
	if (rc)
		return rc;

	rc = serialargs_get_ptr(&args, &ctx_data, ctx_len);
	if (rc)
		return rc;

	if (serialargs_remaining_bytes(&args))
		return PKCS11_CKR_ARGUMENTS_BAD;

	proc->extra_ctx = TEE_Malloc(sizeof(*ctx) + ctx_len,
				     TEE_USER_MEM_HINT_NO_FILL_ZERO);
	if (!proc->extra_ctx)
		return PKCS11_CKR_DEVICE_MEMORY;

	ctx = proc->extra_ctx;
	ctx->ctx_len = ctx_len;
	ctx->flag = flag;
	TEE_MemMove(ctx->ctx, ctx_data, ctx_len);

	return PKCS11_CKR_OK;
}

size_t ecdsa_get_input_max_byte_size(TEE_OperationHandle op)
{
	TEE_OperationInfo info = { };

	TEE_GetOperationInfo(op, &info);

	/*
	 *  Determining curve size in bytes with the help of
	 *  maxkeysize attribute instead of algorithm
	 */
	switch (info.maxKeySize) {
	case 192:
		return 24;
	case 224:
		return 28;
	case 256:
		return 32;
	case 384:
		return 48;
	case 521:
		return 66;
	default:
		DMSG("Unexpected ECDSA algorithm %#"PRIx32, info.algorithm);
		return 0;
	}
}

enum pkcs11_rc pkcs2tee_param_ecdh(struct pkcs11_attribute_head *proc_params,
				   void **pub_data, size_t *pub_size)
{
	enum pkcs11_rc rc = PKCS11_CKR_GENERAL_ERROR;
	struct serialargs args = { };
	uint32_t word = 0;
	uint8_t byte = 0;

	serialargs_init(&args, proc_params->data, proc_params->size);

	/* Skip KDF */
	rc = serialargs_get_u32(&args, &word);
	if (rc)
		return rc;

	/* Shared data size, shall be 0 */
	rc = serialargs_get_u32(&args, &word);
	if (rc || word)
		return rc;

	/* Public data size and content */
	rc = serialargs_get_u32(&args, &word);
	if (rc || !word)
		return rc;

	*pub_size = word;

	rc = serialargs_get(&args, &byte, sizeof(uint8_t));
	if (rc)
		return rc;

	if (byte != 0x02 && byte != 0x03 && byte != 0x04)
		return PKCS11_CKR_ARGUMENTS_BAD;

	if (byte != 0x04) {
		EMSG("DER compressed public key format not yet supported");
		return PKCS11_CKR_ARGUMENTS_BAD;
	}

	*pub_size -= sizeof(uint8_t);

	if (*pub_size >= 0x80) {
		EMSG("DER long definitive form not yet supported");
		return PKCS11_CKR_ARGUMENTS_BAD;
	}

	rc = serialargs_get_ptr(&args, pub_data, *pub_size);
	if (rc)
		return rc;

	if (serialargs_remaining_bytes(&args))
		return PKCS11_CKR_ARGUMENTS_BAD;

	return PKCS11_CKR_OK;
}

enum pkcs11_rc pkcs2tee_algo_ecdh(uint32_t *tee_id,
				  struct pkcs11_attribute_head *proc_params,
				  struct pkcs11_object *obj)
{
	enum pkcs11_rc rc = PKCS11_CKR_GENERAL_ERROR;
	struct serialargs args = { };
	uint32_t kdf = 0;

	serialargs_init(&args, proc_params->data, proc_params->size);

	rc = serialargs_get_u32(&args, &kdf);
	if (rc)
		return rc;

	/* Remaining arguments are extracted by pkcs2tee_param_ecdh */
	if (kdf != PKCS11_CKD_NULL) {
		DMSG("Only support CKD_NULL key derivation for ECDH");
		return PKCS11_CKR_MECHANISM_PARAM_INVALID;
	}

	switch (get_object_key_bit_size(obj)) {
	case 192:
		*tee_id = TEE_ALG_ECDH_P192;
		break;
	case 224:
		*tee_id = TEE_ALG_ECDH_P224;
		break;
	case 256:
		*tee_id = TEE_ALG_ECDH_P256;
		break;
	case 384:
		*tee_id = TEE_ALG_ECDH_P384;
		break;
	case 521:
		*tee_id = TEE_ALG_ECDH_P521;
		break;
	default:
		TEE_Panic(0);
		break;
	}

	return PKCS11_CKR_OK;
}
