#include <LoRaWan_CayenneLPP.h>
#include <LoRaWan_esp32.h>
#include "LoRaWan_config.h"
/***************************************************************************
 *  Go to your TTN console register a device then the copy fields
 *  and replace the CHANGE_ME strings below
 ****************************************************************************/
const char* devEui = config_devEui; // Change to TTN Device EUI
const char* appEui = config_appEui; // Change to TTN Application EUI
const char* appKey = config_appKey; // Chaneg to TTN Application Key

LoRaWan_esp32 ttn;
LoRaWan_CayenneLPP lpp;
#ifndef AUTO_PIN_MAP // AUTO_PIN_MAP is set if board is defined in the file target-config.h
#include "board_config.h"
#endif // !AUTO_PINS
void message(const uint8_t* payload, size_t size, uint8_t port, int rssi)
{
	Serial.println("-- MESSAGE");
	Serial.printf("Received %d bytes on port %d (RSSI=%ddB) :", size, port, rssi);
	for (int i = 0; i < size; i++)
	{
		Serial.printf(" %02X", payload[i]);
	}
	Serial.println();
}

void setup()
{
	Serial.begin(115200);
	Serial.println("Starting");
#ifndef AUTO_PIN_MAP
	SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
#endif // !AUTO_PIN_MAP
	ttn.begin();
	ttn.onMessage(message); // Declare callback function for handling downlink
	// messages from server
	ttn.join(devEui, appEui, appKey);
	Serial.print("Joining TTN ");
	while (!ttn.isJoined())
	{
		Serial.print(".");
		delay(500);
	}
	Serial.println("\njoined !");
	ttn.showStatus();
}

void loop()
{
	static float nb = 18.2;
	nb += 0.1;
	lpp.reset();
	lpp.addTemperature(1, nb);
	if (ttn.sendBytes(lpp.getBuffer(), lpp.getSize()))
	{
		Serial.printf("Temp: %f TTN_CayenneLPP: %d %x %02X%02X\n", nb, lpp.getBuffer()[0], lpp.getBuffer()[1],
			lpp.getBuffer()[2], lpp.getBuffer()[3]);
	}
	delay(100000);
}