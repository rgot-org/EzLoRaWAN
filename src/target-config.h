/*******************************************************************************
 * Copyright (c) 2015 Matthijs Kooijman
 *
 * --- Revised 3-Clause BSD License ---
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice,
 *       this list of conditions and the following disclaimer in the documentation
 *       and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the names of its contributors
 *       may be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SEMTECH BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file defines the configuration of the library and HAL.
 *******************************************************************************/
#ifndef _lmic_arduino_hal_config_h_
#define _lmic_arduino_hal_config_h_

// This defines the region(s) to use. You can enable more than one and
// then select the right region at runtime using os_getRegion() and/or
// 
#include <pins_arduino.h>
#ifdef REGION_EU868
#define CFG_eu868 1
#elif defined (REGION_US915)
#define CFG_us915 1
#else
// todo complete region config

////#define CFG_as923 1
////#define CFG_il915 1
////#define CFG_kr920 1
////#define CFG_au915 1
#define CFG_eu868 1
#endif // REGION_EU868


#define CFG_autojoin

#if defined (ARDUINO_HELTEC_WIFI_LORA_32_V3) || defined (WIFI_LoRa_32_V3)\
|| defined (ARDUINO_HELTEC_WIRELESS_STICK_V3) \
|| defined (ARDUINO_HELTEC_WIRELESS_STICK_LITE_V3) \
|| defined (ARDUINO_HELTEC_WIRELESS_PAPER) \
|| defined (ARDUINO_HELTEC_WIRELESS_TRACKER) \
|| defined (ARDUINO_WIFI_LoRa_32_V3) //sx1262
#define BRD_sx1262_radio 1
#define RADIO_SCLK_PIN              9 //SCK //voir pins_arduino.h
#define RADIO_MISO_PIN              11 //MISO
#define RADIO_MOSI_PIN              10 //MOSI
#define RADIO_TX_PIN				LMIC_CONTROLLED_BY_DIO2
#define RADIO_RX_PIN				LMIC_UNUSED_PIN
#define RADIO_CS_PIN                8 //SS
#define RADIO_DIO0_PIN				LMIC_UNUSED_PIN
#define RADIO_DIO1_PIN              14      //SX1280 DIO1 = IO9
#define RADIO_DIO2_PIN				LMIC_UNUSED_PIN
#define RADIO_BUSY_PIN              13 //BUSY_LoRa      //SX1280 BUSY = IO36
#define RADIO_RST_PIN               12 //RST_LoRa
#define RADIO_TCXO_PIN				LMIC_CONTROLLED_BY_DIO3
#define AUTO_PIN_MAP

#elif defined(ARDUINO_HELTEC_WIRELESS_STICK) || defined(Wireless_Stick) \
|| defined (ARDUINO_HELTEC_WIRELESS_STICK_LITE)|| defined(Wireless_Stick_Lite)\
|| defined (ARDUINO_HELTEC_WIFI_LORA_32)  || defined(WIFI_LoRa_32)\
|| defined (ARDUINO_HELTEC_WIFI_LORA_32_V2) || defined(WIFI_LoRa_32_V2) \
//sx1276
#define BRD_sx1276_radio 1
#define RADIO_SCLK_PIN              SCK //voir pins_arduino.h
#define RADIO_MISO_PIN              MISO
#define RADIO_MOSI_PIN              MOSI
#define RADIO_CS_PIN                SS
#define RADIO_DIO0_PIN				DIO0
#define RADIO_DIO1_PIN              DIO1      //SX1280 DIO1 = IO9
#define RADIO_DIO2_PIN				DIO2
#define RADIO_RST_PIN               RST_LoRa
#define RADIO_BUSY_PIN              LMIC_UNUSED_PIN      //SX1280 BUSY = IO36
#define RADIO_TCXO_PIN				LMIC_UNUSED_PIN
#define RADIO_TX_PIN				LMIC_UNUSED_PIN
#define RADIO_RX_PIN				LMIC_UNUSED_PIN
#define AUTO_PIN_MAP
#elif defined (ARDUINO_TBEAM_USE_RADIO_SX1276) 
#define BRD_sx1276_radio 1
#define RADIO_SCLK_PIN              LORA_SCK //voir pins_arduino.h
#define RADIO_MISO_PIN              LORA_MISO
#define RADIO_MOSI_PIN              LORA_MOSI
#define RADIO_CS_PIN                LORA_CS
#define RADIO_DIO0_PIN				LORA_IO0
#define RADIO_DIO1_PIN              LORA_IO1      //SX1280 DIO1 = IO9
#define RADIO_DIO2_PIN				LORA_IO2
#define RADIO_RST_PIN               LORA_RST
#define RADIO_BUSY_PIN              LMIC_UNUSED_PIN      //SX1280 BUSY = IO36
#define RADIO_TCXO_PIN				LMIC_UNUSED_PIN
#define RADIO_TX_PIN				LMIC_UNUSED_PIN
#define RADIO_RX_PIN				LMIC_UNUSED_PIN
#define AUTO_PIN_MAP
#else
//#define BRD_sx1262_radio
//#if defined(BRD_sx1261_radio) || defined(BRD_sx1262_radio)
//// Used for lmic_pinmap.tcxo only
//#define LMIC_CONTROLLED_BY_DIO3 0xff
//#define LMIC_CONTROLLED_BY_DIO2 0xfe
//#endif // defined(BRD_sx1261_radio) || defined(BRD_sx1262_radio)


#endif 
#if !defined(BRD_sx1272_radio) && !defined(BRD_sx1276_radio) && !defined(BRD_sx1261_radio) && !defined(BRD_sx1262_radio)
// This is the SX1272/SX1273 radio, which is also used on the HopeRF
// RFM92 boards.
//#define BRD_sx1272_radio 1
// This is the SX1276/SX1277/SX1278/SX1279 radio, which is also used on
// the HopeRF RFM95 boards.
//#define BRD_sx1276_radio 1
// This is the newer SX1261 radio (up to +15dBM).
//#define BRD_sx1261_radio 1
// This is the newer SX1262 radio (up to +22dBM).
#define BRD_sx1262_radio 1
#endif // !defined(BRD_sx1272_radio) && !defined(BRD_sx1276_radio) && !defined(BRD_sx1262_radio)

// 16 μs per tick
// LMIC requires ticks to be 15.5μs - 100 μs long
#define US_PER_OSTICK_EXPONENT 4
#define US_PER_OSTICK (1 << US_PER_OSTICK_EXPONENT)
#define OSTICKS_PER_SEC (1000000 / US_PER_OSTICK)

// When this is defined, some debug output will be printed and
// debug_printf(...) is available (which is a slightly non-standard
// printf implementation).
// Without this, assertion failures are *not* printed!
//#define CFG_DEBUG
// When this is defined, additional debug output is printed.
//#define CFG_DEBUG_VERBOSE
// Debug output (and assertion failures) are printed to this Stream
//->#define CFG_DEBUG_STREAM Serial
// Define these to add some TX or RX specific debug output (needs
// CFG_DEBUG)
//->#define DEBUG_TX
//->#define DEBUG_RX
// Define these to add some job scheduling specific debug output (needs
// CFG_DEBUG_VERBOSE)
//#define DEBUG_JOBS
// Uncomment to display timestamps in ticks rather than milliseconds
//#define CFG_DEBUG_RAW_TIMESTAMPS

// When this is defined, the standard libc printf function will print to
// this Stream. You should probably use CFG_DEBUG and debug_printf()
// instead, though.
//#define LMIC_PRINTF_TO Serial

// Remove/comment this to enable code related to beacon tracking.
#define DISABLE_CLASSB

// This allows choosing between multiple included AES implementations.
// Make sure exactly one of these is uncommented.
//
// This selects the original AES implementation included LMIC. This
// implementation is optimized for speed on 32-bit processors using
// fairly big lookup tables, but it takes up big amounts of flash on the
// AVR architecture.
// #define USE_ORIGINAL_AES
//
// This selects the AES implementation written by Ideetroon for their
// own LoRaWAN library. It also uses lookup tables, but smaller
// byte-oriented ones, making it use a lot less flash space (but it is
// also about twice as slow as the original).
#define USE_IDEETRON_AES

#endif // _lmic_arduino_hal_config_h_
