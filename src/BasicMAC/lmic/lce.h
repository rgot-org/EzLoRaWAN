// Copyright (C) 2016-2019 Semtech (International) AG. All rights reserved.
//
// This file is subject to the terms and conditions defined in file 'LICENSE',
// which is part of this source code package.

#ifndef _lce_h_
#define _lce_h_

#include "oslmic.h"

#ifdef __cplusplus
extern "C"{
#endif

// Some keyids:
#define LCE_APPSKEY   (-2)
#define LCE_NWKSKEY   (-1)
#define LCE_MCGRP_0   ( 0)
#define LCE_MCGRP_MAX ( 2)

// Stream cipher categories (lce_cipher(..,cat,..):
// Distinct use of the AppSKey must use different key classes
// or plain text will leak:
enum {
    LCE_SCC_UP   = 0,     // std LoRaWAN uplink frame
    LCE_SCC_DN   = 1,     // std LoRaWAN downlink frame
    LCE_SCC_FUP  = 0x40,  // file upload
    LCE_SCC_DSE  = 0x41,  // data streaming engine
    LCE_SCC_ROSE = 0x42,  // reliable octet streaming engine
};

void lce_encKey0 (u8_t* buf);
u32_t lce_micKey0 (u32_t devaddr, u32_t seqno, u8_t* pdu, int len);
bool lce_processJoinAccept (u8_t* jacc, u8_t jacclen, u16_t devnonce);
void lce_addMicJoinReq (u8_t* pdu, int len);
bool lce_verifyMic (s8_t keyid, u32_t devaddr, u32_t seqno, u8_t* pdu, int len);
void lce_addMic (s8_t keyid, u32_t devaddr, u32_t seqno, u8_t* pdu, int len);
void lce_cipher (s8_t keyid, u32_t devaddr, u32_t seqno, int cat, u8_t* payload, int len);
#if defined(CFG_lorawan11)
void lce_loadSessionKeys (const u8_t* nwkSKey, const u8_t* nwkSKeyDn, const u8_t* appSKey);
#else
void lce_loadSessionKeys (const u8_t* nwkSKey, const u8_t* appSKey);
#endif
void lce_init (void);


typedef struct lce_ctx_mcgrp {
    u8_t nwkSKeyDn[16]; // network session key for down-link
    u8_t appSKey[16];   // application session key
} lce_ctx_mcgrp_t;

typedef struct lce_ctx {
    u8_t nwkSKey[16];   // network session key (LoRaWAN1.1: up-link only)
#if defined(CFG_lorawan11)
    u8_t nwkSKeyDn[16]; // network session key for down-link
#endif
    u8_t appSKey[16];   // application session key
    lce_ctx_mcgrp_t mcgroup[LCE_MCGRP_MAX];
} lce_ctx_t;


#ifdef __cplusplus
} // extern "C"
#endif

#endif // _lce_h_
