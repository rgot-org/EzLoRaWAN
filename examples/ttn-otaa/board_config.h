#pragma once
/*
Example for LyLigo T3S3
Arduino IDE options :
T3-S3 				Value
Board 				ESP32S3 Dev Module
Port 				Your port
USB CDC On Boot 	Enable
CPU Frequency 		240MHZ(WiFi)
Core Debug Level 	None
USB DFU On Boot 	Disable
Erase All Flash  	Disable
Events Run On 		Core1
Flash Mode			QIO 80MHZ
Flash Size 			4MB(32Mb)
Arduino Runs On 	Core1
USB FW MSC On Boot 	Disable
Partition Scheme 	Default 4M Flash with spiffs(1.2M APP/1.5MB SPIFFS)
PSRAM 				QSPI PSRAM
Upload Mode 		UART0/Hardware CDC
Upload Speed 		921600
USB Mode 			CDC and JTAG
Programmer 			Esptool
*/
// T3S3 with sx1262
#define BRD_sx1262_radio 1
#define RADIO_SCLK_PIN              5
#define RADIO_MISO_PIN              3
#define RADIO_MOSI_PIN              6
#define RADIO_CS_PIN                7
#define RADIO_DIO1_PIN              33      //SX1280 DIO1 = IO9
#define RADIO_BUSY_PIN              34      //SX1280 BUSY = IO36
#define RADIO_RST_PIN               8
 // T3S3 with other LoRaWAN chip see : https://github.com/Xinyuan-LilyGO/LilyGo-LoRa-Series/blob/master/examples/T3S3Factory/utilities.h

const lmic_pinmap lmic_pins = {
	// NSS input pin for SPI communication (required)
	.nss = RADIO_CS_PIN,
	// If needed, these pins control the RX/TX antenna switch (active
	// high outputs). When you have both, the antenna switch can
	// powerdown when unused. If you just have a RXTX pin it should
	// usually be assigned to .tx, reverting to RX mode when idle).
	//
	// The SX127x has an RXTX pin that can automatically control the
	// antenna switch (if internally connected on the transceiver
	// board). This pin is always active, so no configuration is needed
	// for that here.
	// On SX126x, the DIO2 can be used for the same thing, but this is
	// disabled by default. To enable this, set .tx to
	// LMIC_CONTROLLED_BY_DIO2 below (this seems to be common and
	// enabling it when not needed is probably harmless, unless DIO2 is
	// connected to GND or VCC directly inside the transceiver board).
	.tx = LMIC_CONTROLLED_BY_DIO2, // this constant is defined automaticaly when BRD_sx1262_radio is defined
	.rx = LMIC_UNUSED_PIN,
	// Radio reset output pin (active high for SX1276, active low for
	// others). When omitted, reset is skipped which might cause problems.
	.rst = RADIO_RST_PIN,
	// DIO input pins.
	//   For SX127x, LoRa needs DIO0 and DIO1, FSK needs DIO0, DIO1 and DIO2
	//   For SX126x, Only DIO1 is needed (so leave DIO0 and DIO2 as LMIC_UNUSED_PIN)
	.dio = {/* DIO0 */ LMIC_UNUSED_PIN, /* DIO1 */ RADIO_DIO1_PIN, /* DIO2 */ LMIC_UNUSED_PIN},
	// Busy input pin (SX126x only). When omitted, a delay is used which might
	// cause problems.
	.busy = RADIO_BUSY_PIN,
	// TCXO oscillator enable output pin (active high).
	//
	// For SX127x this should be an I/O pin that controls the TCXO, or
	// LMIC_UNUSED_PIN when a crystal is used instead of a TCXO.
	//
	// For SX126x this should be LMIC_CONTROLLED_BY_DIO3 when a TCXO is
	// directly connected to the transceiver DIO3 to let the transceiver
	// start and stop the TCXO, or LMIC_UNUSED_PIN when a crystal is
	// used instead of a TCXO. Controlling the TCXO from the MCU is not
	// supported.
	.tcxo = LMIC_CONTROLLED_BY_DIO3, //this constant is defined automaticaly when BRD_sx1262_radio is defined
};

