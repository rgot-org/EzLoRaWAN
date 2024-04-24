// Copyright (C) 2016-2019 Semtech (International) AG. All rights reserved.
//
// This file is subject to the terms and conditions defined in file 'LICENSE',
// which is part of this source code package.

#ifndef _aes_h_
#define _aes_h_

#include "oslmic.h"

#ifdef __cplusplus
extern "C"{
#endif

// ======================================================================
// AES support
// !!Keep in sync with lorabase.hpp!!
// !!Keep in sync with bootloader/aes.c!!

#ifndef AES_ENC  // if AES_ENC is defined as macro all other values must be too
#define AES_ENC       0x00
#define AES_DEC       0x01
#define AES_MIC       0x02
#define AES_CTR       0x04
#define AES_MICNOAUX  0x08
#endif
#ifndef AESkey  // if AESkey is defined as macro all other values must be too
extern u8_t* AESkey;
extern u8_t* AESaux;
#endif
#ifndef os_aes
u32_t os_aes (u8_t mode, u8_t* buf, u16_t len);
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _aes_h_
