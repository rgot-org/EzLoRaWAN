//
//
//
#include "EzLoRaWAN.h"
#include "oslmic_types.h"
#include "BasicMAC/hal/hal.h"
#include "ByteArrayUtils.h"
#include <Preferences.h>
#include "esp_log.h"
#include "esp_mac.h"
#include "helper.h"

#define LOOP_LoRaWan_MS 1

/// The last sent sequence number we know, stored in RTC memory
RTC_DATA_ATTR uint32_t rtc_sequenceNumberUp = 0;
RTC_DATA_ATTR uint8_t rtc_app_session_key[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
RTC_DATA_ATTR uint8_t rtc_net_session_key[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
RTC_DATA_ATTR uint8_t rtc_dev_adr[4] = { 0, 0, 0, 0 };
RTC_DATA_ATTR bool rtc_session = false;
//#define CFG_DEBUG

// static osjobcb_t sendMsg;
EzLoRaWAN* EzLoRaWAN::_instance = 0;
Preferences prefs;

// This EUI must be in little-endian format, so least-significant-byte first.
// When copying an EUI from ttnctl output, this means to reverse the bytes.
// For TTN issued EUIs the last bytes should be 0xD5, 0xB3, 0x70.
// The order is swapped in provisioning_decode_keys().
void os_getJoinEui(uint8_t* buf) {
	EzLoRaWAN* ttn = EzLoRaWAN::getInstance();
	memcpy(buf, ttn->app_eui, 8);

}

// This should also be in little endian format, see above.
void os_getDevEui(u1_t* buf)
{
	EzLoRaWAN* ttn = EzLoRaWAN::getInstance();
	std::copy(ttn->dev_eui, ttn->dev_eui + 8, buf);
}



// This key should be in big endian format (or, since it is not really a number
// but a block of memory, endianness does not really apply). In practice, a key
// taken from ttnctl can be copied as-is.
void os_getNwkKey(uint8_t* buf) {
	EzLoRaWAN* ttn = EzLoRaWAN::getInstance();
	memcpy(buf, ttn->app_key, 16);
	
}
uint8_t os_getRegion(void) {
	EzLoRaWAN* ttn = EzLoRaWAN::getInstance();
	return LMIC_regionCode(0);
}


/************
 * Public
 ************/

EzLoRaWAN* EzLoRaWAN::getInstance()
{
	return _instance;
}

// --- Constructor
EzLoRaWAN::EzLoRaWAN() :
	dev_eui{ 0, 0, 0, 0, 0, 0, 0, 0 },
	app_eui{ 0, 0, 0, 0, 0, 0, 0, 0 },
	app_key{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	_joined{ false },
	_provisioned{ false },
	_session{ false },
	LoRaWan_task_Handle{ NULL },
	_message{ 0 },
	_length{ 1 },
	_port{ 1 },
	_confirm{ 0 },
	txInterval{ 0 },
	cyclique{ false }
{
	_instance = this;
}

void EzLoRaWAN::setPins(uint8_t nss, uint8_t rx, uint8_t tx, uint8_t rst, uint8_t dio0, uint8_t dio1, uint8_t dio2, uint8_t busy, uint8_t tcxo)
{
	

}

bool EzLoRaWAN::begin()
{
	// record self in a static so that we can dispatch events
	// ASSERT(EzLoRaWAN::pLoRaWAN == this ||EzLoRaWAN::pLoRaWAN == NULL);
	// pLoRaWAN = this;
	return begin(&lmic_pins);
}

bool EzLoRaWAN::begin(uint8_t nss, uint8_t rx, uint8_t tx, uint8_t rst, uint8_t dio0, uint8_t dio1, uint8_t dio2,uint8_t busy,uint8_t tcxo)
{

}

bool EzLoRaWAN::begin( const lmic_pinmap* pPinmap)
{
	  os_init((void*) &pPinmap);

		// Reset the MAC state. _session and pending data transfers will be discarded.
		LMIC_reset();

	return true;
}

bool EzLoRaWAN::provision(const char* appEui, const char* appKey)
{
	uint8_t mac[6];
	esp_err_t err = esp_efuse_mac_get_default(mac);
	ESP_ERROR_CHECK(err);
	dev_eui[7] = mac[0];
	dev_eui[6] = mac[1];
	dev_eui[5] = mac[2];
	dev_eui[4] = 0xff;
	dev_eui[3] = 0xfe;
	dev_eui[2] = mac[3];
	dev_eui[1] = mac[4];
	dev_eui[0] = mac[5];
#ifdef CFG_DEBUG
	Serial.print("dev EUI: ");
	for (size_t i = 0; i < 8; i++)
	{
		Serial.printf("%02X", dev_eui[7 - i]);
	}
	Serial.println();
#endif // CFG_DEBUG

	if (decode(false, nullptr, appEui, appKey))
	{
		return saveKeys();
	}
	return false;
}

bool EzLoRaWAN::provision(const char* devEui, const char* appEui, const char* appKey)
{
	if (decode(true, devEui, appEui, appKey))
	{
		return saveKeys();
	}
	return false;
}

bool EzLoRaWAN::provisionABP(const char* devAddr, const char* nwkSKey, const char* appSKey)
{
	ByteArrayUtils::hexStrToBin(nwkSKey, rtc_net_session_key, 16);
	ByteArrayUtils::hexStrToBin(appSKey, rtc_app_session_key, 16);
	ByteArrayUtils::hexStrToBin(devAddr, rtc_dev_adr, 4);
	return saveKeys();
}

bool EzLoRaWAN::join()
{
	bool success = false;

	if (!_provisioned && !_session)
	{
		restoreKeys();
	}
	// Check if this is a cold boot
	if (_session && rtc_sequenceNumberUp != 0)
	{
		Serial.println(F("Using stored _session to join"));
		devaddr_t dev_addr = rtc_dev_adr[0] << 24 | rtc_dev_adr[1] << 16 | rtc_dev_adr[2] << 8 | rtc_dev_adr[3];
		personalize(0x13, dev_addr, rtc_net_session_key, rtc_app_session_key);
		success = true;
	}
	else if (_provisioned)
	{
		Serial.println(F("Using stored keys to join"));
		//LMIC_setClockError(MAX_CLOCK_ERROR * 7 / 100);
		//LMIC_unjoin();
		LMIC_startJoining();
		xTaskCreatePinnedToCore(loopStack, "ttnTask", 2048, (void*)1, (5 | portPRIVILEGE_BIT), &LoRaWan_task_Handle, 1);
		success = true;
	}
	else
	{
		ESP_LOGW(TAG, "Device EUI, App EUI and/or App key have not been provided");
		Serial.println(F("Cannot join. No keys provided"));
	}
	return success;
}

bool EzLoRaWAN::join(const char* appEui, const char* appKey,  int8_t retries, uint32_t retryDelay)
{
	bool force = (getAppEui() != appEui) || (getAppKey() != appKey);
	if (force || !_provisioned)
	{
		provision(appEui, appKey);
	}
	return join();
}

bool EzLoRaWAN::join(const char* devEui, const char* appEui, const char* appKey, int8_t retries, uint32_t retryDelay)
{
	bool force =  (getAppEui() != appEui) || (getDevEui() != devEui) || (getAppKey() != appKey);
	if (force || !_provisioned)
	{
		ESP_LOGI(TAG, "provisionning !");
		provision(devEui, appEui, appKey);
	}
	return join();
}

bool EzLoRaWAN::personalize()
{
	bool success;
	// restoreKeys(false);
	if (!ByteArrayUtils::isAllZeros(rtc_dev_adr, sizeof(rtc_dev_adr))
		&& !ByteArrayUtils::isAllZeros(rtc_net_session_key, sizeof(rtc_net_session_key))
		&& !ByteArrayUtils::isAllZeros(rtc_app_session_key, sizeof(rtc_app_session_key)))
	{
		devaddr_t dev_addr = rtc_dev_adr[0] << 24 | rtc_dev_adr[1] << 16 | rtc_dev_adr[2] << 8 | rtc_dev_adr[3];
		personalize(0x13, dev_addr, rtc_net_session_key, rtc_app_session_key);
		success = true;
	}
	else
	{
		success = false;
	}
	return success;
}

bool EzLoRaWAN::personalize(const char* devAddr, const char* nwkSKey, const char* appSKey)
{
	ByteArrayUtils::hexStrToBin(nwkSKey, rtc_net_session_key, 16);
	ByteArrayUtils::hexStrToBin(appSKey, rtc_app_session_key, 16);
	ByteArrayUtils::hexStrToBin(devAddr, rtc_dev_adr, 4);
	devaddr_t dev_addr = rtc_dev_adr[0] << 24 | rtc_dev_adr[1] << 16 | rtc_dev_adr[2] << 8 | rtc_dev_adr[3];
#ifdef CFG_DEBUG
	ESP_LOGI(TAG, "Dev adr str: %s", devAddr);
	ESP_LOGI(TAG, "Dev adr int: %X", dev_addr);
#endif // CFG_DEBUG
	personalize(0x13, dev_addr, rtc_net_session_key, rtc_app_session_key);
	return true;
}

bool EzLoRaWAN::sendBytes(uint8_t* payload, size_t length, uint8_t port, uint8_t confirm)
{
	cyclique = false;
	return txBytes(payload, length, port, confirm);
}

void EzLoRaWAN::sendBytesAtInterval(uint8_t* payload, size_t length, uint32_t interval, uint8_t port, uint8_t confirm)
{
	cyclique = (interval != 0) ? true : false;
	txInterval = interval;
	txBytes(payload, length, port, confirm);
}

bool EzLoRaWAN::poll(uint8_t port, uint8_t confirm)
{
	return sendBytes(0, 1, port, confirm);
}

bool EzLoRaWAN::stop()
{
	ESP_LOGI(TAG, "LoRaWan_task=%d", LoRaWan_task_Handle);
	if (LoRaWan_task_Handle != NULL)
	{
		ESP_LOGI(TAG, "delete ttn task");
		vTaskDelete(LoRaWan_task_Handle);
		LoRaWan_task_Handle = NULL;
	}
	return true;
}

bool EzLoRaWAN::isRunning(void)
{
	if (LoRaWan_task_Handle == NULL)
	{
		return false;
	}
	return true;
}


void EzLoRaWAN::onMessage(void(*callback)(const uint8_t* payload, size_t size, int rssi))
{
	messageCallback = callback;
}

void EzLoRaWAN::onMessage(void(*callback)(const uint8_t* payload, size_t size, uint8_t port, int rssi))
{
	messageCallbackPort = callback;
}

void EzLoRaWAN::onConfirm(void(*callback)())
{
	confirmCallback = callback;
}


void EzLoRaWAN::onEvent(void(*callback)(const ev_t event))
{
	eventCallback = callback;
}

bool EzLoRaWAN::isJoined()
{
	return _joined;
}

bool EzLoRaWAN::isTransceiving()
{
	return LMIC.opmode & OP_TXRXPEND;
}

uint32_t EzLoRaWAN::waitForPendingTransactions()
{
	uint32_t waited = 0;
	while (isTransceiving())
	{
		waited += 100;
		delay(100);
	}
	return waited;
}

bool EzLoRaWAN::hasSession()
{
	return _session;
}

bool EzLoRaWAN::isProvisioned()
{
	return _provisioned;
}

bool EzLoRaWAN::saveKeys()
{
	bool success = false;
	if (prefs.begin(NVS_FLASH_PARTITION, false, NVS_FLASH_PARTITION))
	{
		if (prefs.putBytes(NVS_FLASH_KEY_DEV_EUI, dev_eui, sizeof(dev_eui))
			&& prefs.putBytes(NVS_FLASH_KEY_APP_EUI, app_eui, sizeof(app_eui))
			&& prefs.putBytes(NVS_FLASH_KEY_APP_KEY, app_key, sizeof(app_key)))
		{
			success = true;
		}
		prefs.end();
	}

	return success;
}

bool EzLoRaWAN::restoreKeys(bool silent)
{
	if (prefs.begin(NVS_FLASH_PARTITION, true, NVS_FLASH_PARTITION)) {
		uint8_t buf_dev_eui[8];
		uint8_t buf_app_eui[8];
		uint8_t buf_app_key[16];
		if (prefs.getBytes(NVS_FLASH_KEY_DEV_EUI, buf_dev_eui, sizeof(dev_eui))
			&& prefs.getBytes(NVS_FLASH_KEY_APP_EUI, buf_app_eui, sizeof(app_eui))
			&& prefs.getBytes(NVS_FLASH_KEY_APP_KEY, buf_app_key, sizeof(app_key)))
		{
			std::copy(buf_dev_eui, buf_dev_eui + 8, dev_eui);
			std::copy(buf_app_eui, buf_app_eui + 8, app_eui);
			std::copy(buf_app_key, buf_app_key + 16, app_key);

			checkKeys();

			if (_provisioned)
			{
				ESP_LOGI(TAG, "Dev and app EUI and app key have been restored from NVS storage");
			}
			else
			{
				ESP_LOGW(TAG, "Dev and app EUI and app key are invalid (zeroes only)");
			}
		}
		else
		{
			_provisioned = false;
		}
		prefs.end();
	}	
	return _provisioned;
}

bool EzLoRaWAN::eraseKeys()
{
	bool success = false;
	uint8_t emptyBuf[16] = { 0 };
	if (prefs.begin(NVS_FLASH_PARTITION, false, NVS_FLASH_PARTITION))
	{
		if(prefs.remove(NVS_FLASH_KEY_DEV_EUI)
			&& prefs.remove(NVS_FLASH_KEY_APP_EUI)
			&& prefs.remove(NVS_FLASH_KEY_APP_KEY)
			)
		{
			success = true;
			ESP_LOGI(TAG, "Dev EUI, app EUI and app key erased in NVS storage");
		}
		prefs.end();
	}
		return success;
}


void EzLoRaWAN::showStatus()
{
	if (poll())
	{
		char buffer[64];
		Serial.println("---------------Status--------------");
		Serial.println("Device EUI: " + getDevEui());
		Serial.println("Application EUI: " + getAppEui());
		Serial.print("netid: ");
		Serial.println(LMIC.netid, HEX);
		Serial.print("devaddr: ");
		Serial.println(LMIC.devaddr, HEX);
		Serial.print("NwkSKey: ");
		for (size_t i = 0; i < sizeof(LMIC.lceCtx.nwkSKey); ++i)
		{
			Serial.printf("%02X",LMIC.lceCtx.nwkSKey[i]);
			
		}
		Serial.print("\nAppSKey: ");
		for (size_t i = 0; i < sizeof(LMIC.lceCtx.appSKey); ++i)
		{
			Serial.printf("%02X", LMIC.lceCtx.appSKey[i]);
		}
		Serial.printf("\ndata rate: %d\n", LMIC.datarate);
		Serial.printf("tx power: %ddB\n", LMIC.txpow);
		Serial.printf("freq: %dHz\n", LMIC.freq);
		Serial.println("-----------------------------------");
	}
}

uint8_t EzLoRaWAN::getDatarate()
{
	return LMIC.datarate;
}

bool EzLoRaWAN::setDataRate(uint8_t rate)
{
	LMIC.datarate = rate;
	return true;
}

void EzLoRaWAN::setTXInterval(uint32_t interval = 60)
{
	/*_interval=INTERVAL;*/
	txInterval = interval;
}

size_t EzLoRaWAN::getAppEui(byte* buf)
{
	memcpy(buf, app_eui, 8);
	ByteArrayUtils::swapBytes(buf, 8);
	return 8;
}

size_t EzLoRaWAN::getAppKey(byte* buf)
{
	memcpy(buf, app_key, 16);
	return 16;
}
String EzLoRaWAN::getAppKey()
{
	char hexbuf[33] = { 0 };
	for (size_t i = 0; i < 16; i++)
	{
		sprintf(hexbuf + (2 * i), "%02X", app_key[i]);
	}
	return String(hexbuf);
}
String EzLoRaWAN::getAppEui()
{
	char hexbuf[17] = { 0 };
	for (size_t i = 0; i < 8; i++)
	{
		sprintf(hexbuf + (2 * i), "%02X", app_eui[7 - i]);
	}
	return String(hexbuf);
}


size_t EzLoRaWAN::getDevEui(byte* buf, bool hardwareEUI)
{
	if (hardwareEUI)
	{
		uint8_t mac[6];
		esp_err_t err = esp_efuse_mac_get_default(mac);
		ESP_ERROR_CHECK(err);
		buf[7] = mac[0];
		buf[6] = mac[1];
		buf[5] = mac[2];
		buf[4] = 0xff;
		buf[3] = 0xfe;
		buf[2] = mac[3];
		buf[1] = mac[4];
		buf[0] = mac[5];
	}
	else {
		memcpy(buf, dev_eui, 8);
		ByteArrayUtils::swapBytes(buf, 8);
	}
	return 8;
}

String EzLoRaWAN::getDevEui(bool hardwareEUI)
{
	char hexbuf[17] = { 0 };
	if (hardwareEUI)
	{
		uint8_t mac[6];

		esp_err_t err = esp_efuse_mac_get_default(mac);
		ESP_ERROR_CHECK(err);
		ByteArrayUtils::binToHexStr(mac, 6, hexbuf);
		for (size_t i = 0; i < 6; i++)
		{
			hexbuf[15 - i] = hexbuf[11 - i];
		}
		hexbuf[9] = 'E';
		hexbuf[8] = 'F';
		hexbuf[7] = 'F';
		hexbuf[6] = 'F';
	}
	else
	{
		for (size_t i = 0; i < 8; i++)
		{
			sprintf(hexbuf + (2 * i), "%02X", dev_eui[7 - i]);
		}
	}
	return String(hexbuf);
}

String EzLoRaWAN::getMac()
{
	uint8_t mac[6];
	esp_err_t err = esp_efuse_mac_get_default(mac);
	ESP_ERROR_CHECK(err);
	char buf[20];
	for (size_t i = 0; i < 5; i++)
	{
		sprintf(buf + (3 * i), "%02X:", mac[i]);
	}
	sprintf(buf + (3 * 5), "%02X", mac[5]);

	return String(buf);
}

uint8_t EzLoRaWAN::getPort()
{
	return _port;
}

uint32_t EzLoRaWAN::getFrequency()
{
	return LMIC.freq;
}

int8_t EzLoRaWAN::getTXPower()
{
	return LMIC.txpow;
}

bool EzLoRaWAN::setDevEui(byte* value)
{
	memcpy(dev_eui, value, 8);
	return true;
}

bool EzLoRaWAN::setAppEui(byte* value)
{
	memcpy(app_eui, value, 8);
	return true;
}

bool EzLoRaWAN::setAppKey(byte* value)
{
	memcpy(app_key, value, 16);
	return true;
}




/************
 * Private
 ************/

bool EzLoRaWAN::txBytes(uint8_t* payload, size_t length, uint8_t port, uint8_t confirm)
{
	_message = payload;
	_length = length;
	_port = port;
	_confirm = confirm;

	txMessage(&sendjob);
	if ((LMIC.opmode & OP_TXDATA) == 0)
	{
		return false;
	}

	return true;
}

void EzLoRaWAN::txMessage(osjob_t* job)
{
	if (_joined)
	{
		// Check if there is not a current TX/RX job running
		if (LMIC.opmode & OP_TXRXPEND)
		{
#ifdef CFG_DEBUG
			Serial.println(F("Pending transaction, not sending"));
#endif // CFG_DEBUG
		}
		else
		{
			// Prepare upstream data transmission at the next possible time.
			LMIC_setTxData2(_port, _message, _length, _confirm);
#ifdef CFG_DEBUG
			Serial.println(F("Packet queued"));
#endif // CFG_DEBUG
		}
		// Next TX is scheduled after TX_COMPLETE event.
	}
#ifdef CFG_DEBUG
	else
	{
		Serial.println(F("Not connected/joined to TTN"));
	}
#endif // CFG_DEBUG
		}

void EzLoRaWAN::personalize(u4_t netID, u4_t DevAddr, uint8_t* NwkSKey, uint8_t* AppSKey)
{
//	LMIC_setClockError(MAX_CLOCK_ERROR * 7 / 100);
	LMIC_reset();
	LMIC_setSession(netID, DevAddr, NwkSKey, AppSKey);
	enum { BAND_MILLI = 0, BAND_CENTI = 1, BAND_DECI = 2, BAND_AUX = 3 };
#if defined(CFG_eu868)
	// Set up the channels used by the Things Network, which corresponds
	// to the defaults of most gateways. Without this, only three base
	// channels from the LoRaWAN specification are used, which certainly
	// works, so it is good for debugging, but can overload those
	// frequencies, so be sure to configure the full frequency range of
	// your network here (unless your network autoconfigures them).
	// Setting up channels should happen after LMIC_setSession, as that
	// configures the minimal channel set.
	// NA-US channels 0-71 are configured automatically
	enum {
		DR_SF12 = 0,
		DR_SF11 = 1,
		DR_SF10 = 2,
		DR_SF9 = 3,
		DR_SF8 = 4,
		DR_SF7 = 5,
		DR_SF7_BW250 = 6,
		DR_FSK = 7,
	};
	LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7)); // g-band
	LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7_BW250)); // g-band
	LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7)); // g-band
	LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7)); // g-band
	LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7)); // g-band
	LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7)); // g-band
	LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7)); // g-band
	LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7)); // g-band
	LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK, DR_FSK)); // g2-band
#endif
	// Disable link check validation
	LMIC_setLinkCheckMode(0);

	// TTN uses SF9 for its RX2 window.
	LMIC.dn2Dr = DR_SF9;

	// Set data rate and transmit power for uplink
	LMIC_setDrTxpow(DR_SF7, 14);

	// Set the sequence number of the packet
	LMIC.seqnoUp=(rtc_sequenceNumberUp);

	// Start job
	_joined = true;
	xTaskCreatePinnedToCore(loopStack, "ttnTask", 2048, (void*)1, (5 | portPRIVILEGE_BIT), &LoRaWan_task_Handle, 1);
}

bool EzLoRaWAN::decode(bool includeDevEui, const char* devEui, const char* appEui, const char* appKey)
{
	uint8_t buf_dev_eui[8];
	uint8_t buf_app_eui[8];
	uint8_t buf_app_key[16];

	if (includeDevEui && (strlen(devEui) != 16 || !ByteArrayUtils::hexStrToBin(devEui, buf_dev_eui, 8)))
	{
		ESP_LOGW(TAG, "Invalid device EUI: %s", devEui);
		return false;
	}

	if (includeDevEui)
	{
		ByteArrayUtils::swapBytes(buf_dev_eui, 8);
	}

	if (strlen(appEui) != 16 || !ByteArrayUtils::hexStrToBin(appEui, buf_app_eui, 8))
	{
		ESP_LOGW(TAG, "Invalid application EUI: %s", appEui);
		return false;
	}

	ByteArrayUtils::swapBytes(buf_app_eui, 8);

	if (strlen(appKey) != 32 || !ByteArrayUtils::hexStrToBin(appKey, buf_app_key, 16))
	{
		ESP_LOGW(TAG, "Invalid application key: %s", appKey);
		return false;
	}

	if (includeDevEui)
	{
		std::copy(buf_dev_eui, buf_dev_eui + 8, dev_eui);
	}
	std::copy(buf_app_eui, buf_app_eui + 8, app_eui);
	std::copy(buf_app_key, buf_app_key + 16, app_key);

	checkKeys();

	return true;
}

void EzLoRaWAN::checkKeys()
{
	_provisioned = !ByteArrayUtils::isAllZeros(dev_eui, sizeof(dev_eui))
		//&& !ByteArrayUtils::isAllZeros(app_eui, sizeof(app_eui))
		&& !ByteArrayUtils::isAllZeros(app_key, sizeof(app_key));
	_session = !ByteArrayUtils::isAllZeros(rtc_dev_adr, sizeof(rtc_dev_adr))
		&& !ByteArrayUtils::isAllZeros(rtc_app_session_key, sizeof(rtc_app_session_key))
		&& !ByteArrayUtils::isAllZeros(rtc_net_session_key, sizeof(rtc_net_session_key)) && rtc_sequenceNumberUp != 0x00;

#ifdef CFG_DEBUG
	Serial.print(F("[checkKeys] "));
	if (_provisioned)
	{
		Serial.print(F("_provisioned, "));
	}
	else
	{
		Serial.print(F("unprovisioned, "));
	}
	if (_session)
	{
		Serial.println(F("_session "));
}
	else
	{
		Serial.println(F("no _session "));
	}
#endif
		}

void EzLoRaWAN::loopStack(void* parameter)
{
	for (;;)
	{
		os_runstep();
		vTaskDelay(LOOP_LoRaWan_MS / portTICK_PERIOD_MS);
	}
}
// this macro can be used to initalize a normal table of event strings
#define LMIC_EVENT_NAME_TABLE__INIT                                         \
    "<<zero>>",                                                             \
    "EV_SCAN_TIMEOUT", "EV_BEACON_FOUND",                                   \
    "EV_BEACON_MISSED", "EV_BEACON_TRACKED", "EV_JOINING",                  \
    "EV_JOINED", "EV_RFU1", "EV_JOIN_FAILED", "EV_REJOIN_FAILED",           \
    "EV_TXCOMPLETE", "EV_LOST_TSYNC", "EV_RESET",                           \
    "EV_RXCOMPLETE", "EV_LINK_DEAD", "EV_LINK_ALIVE", "EV_SCAN_FOUND",      \
    "EV_TXSTART", "EV_TXCANCELED", "EV_RXSTART", "EV_JOIN_TXCOMPLETE"
static const char* const eventNames[] = { LMIC_EVENT_NAME_TABLE__INIT };

void onLmicEvent(ev_t event)
{
	EzLoRaWAN* ttn = EzLoRaWAN::getInstance();
#ifdef CFG_DEBUG
	Serial.print(os_getTime());
	Serial.print(": ");
	// get event message
	if (event < sizeof(eventNames) / sizeof(eventNames[0]))
	{
		Serial.print("[Event] ");
		Serial.println((eventNames[event] + 3)); // +3 to strip "EV_"
	}
	else
	{
		Serial.print("[Event] Unknown: ");
		Serial.println(event);
	}
#endif // CFG_DEBUG
	switch (event)
	{
	case EV_JOINING:
		ttn->_joined = false;
		break;
	case EV_JOINED:
	{
		u4_t netid = LMIC.netid;
		devaddr_t devaddr = LMIC.devaddr;
		memcpy(rtc_app_session_key, LMIC.lceCtx.appSKey, sizeof(LMIC.lceCtx.appSKey));
		memcpy(rtc_net_session_key, LMIC.lceCtx.nwkSKey, sizeof(LMIC.lceCtx.nwkSKey));
		lce_loadSessionKeys(rtc_net_session_key, rtc_app_session_key);
		rtc_dev_adr[0] = (devaddr >> 24) & 0xFF;
		rtc_dev_adr[1] = (devaddr >> 16) & 0xFF;
		rtc_dev_adr[2] = (devaddr >> 8) & 0xFF;
		rtc_dev_adr[3] = devaddr & 0xFF;
		
#ifdef CFG_DEBUG
		Serial.println(F("EV_JOINED"));
		Serial.print("netid: ");
		Serial.println(netid, DEC);
		Serial.print("devAdr: ");
		Serial.println(devaddr, HEX);
		Serial.print("artKey: ");
		for (size_t i = 0; i < sizeof(artKey); ++i)
		{
			Serial.print(artKey[i], HEX);
		}
		Serial.println("");
		Serial.print("nwkKey: ");
		for (size_t i = 0; i < sizeof(nwkKey); ++i)
		{
			Serial.print(nwkKey[i], HEX);
		}
		Serial.println();
#endif // CFG_DEBUG
	}
	ttn->_joined = true;
	// Disable link check validation (automatically enabled
	// during join, but because slow data rates change max TX
	// size, we don't use it in this example.)
	LMIC_setLinkCheckMode(0);
	break;
	case EV_JOIN_FAILED:
		ttn->_joined = false;
		break;
	case EV_TXCOMPLETE:
		rtc_sequenceNumberUp = LMIC.seqnoUp;

		if (LMIC.txrxFlags & TXRX_ACK)
		{
			if (ttn->confirmCallback)
			{
				ttn->confirmCallback();
			}
#ifdef CFG_DEBUG
			Serial.printf("txrxFlags : %02X\n", LMIC.txrxFlags);
			Serial.println(F("Received ack"));
#endif // CFG_DEBUG
		}

		if (LMIC.dataLen)
		{
#ifdef CFG_DEBUG
			Serial.print(F("Received "));
			Serial.print(LMIC.dataLen);
			Serial.print(F(" bytes of payload. FPORT: "));
			Serial.println(LMIC.frame[LMIC.dataBeg-1]);
			String JSONMessage = "";
				
			for (byte i = LMIC.dataBeg; i < LMIC.dataBeg + LMIC.dataLen; i++)
			{
				JSONMessage += (char)LMIC.frame[i];
			}
			Serial.println(JSONMessage);
#endif // CFG_DEBUG

			if (ttn->messageCallback)
			{
				uint8_t downlink[LMIC.dataLen];
				uint8_t offset = LMIC.dataBeg; //9;// offset to get data.
				std::copy(LMIC.frame + offset, LMIC.frame + offset + LMIC.dataLen, downlink);
				ttn->messageCallback(downlink, LMIC.dataLen, LMIC.rssi);
			}
			if (ttn->messageCallbackPort)
			{
				uint8_t downlink[LMIC.dataLen];
				uint8_t offset = LMIC.dataBeg; //9;// offset to get data.
				std::copy(LMIC.frame + offset, LMIC.frame + offset + LMIC.dataLen, downlink);
				ttn->messageCallbackPort(downlink, LMIC.dataLen, LMIC.frame[LMIC.dataBeg-1], LMIC.rssi);
			}
		}
		// Schedule next transmission
		// if (cyclique)
		// {
		//     os_setTimedCallback(&ttn.sendjob, os_getTime() + sec2osticks(txInterval), ttn.txMessage);
		// }
		break;
	case EV_RESET:
		ttn->_joined = false;
		break;
	case EV_LINK_DEAD:
		ttn->_joined = false;
		break;

	default:
		break;
	}

	if (ttn->eventCallback)
	{
		ttn->eventCallback(event);
	}
}
