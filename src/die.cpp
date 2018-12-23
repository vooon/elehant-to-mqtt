
#include <common.h>

[[noreturn]] void die()
{
#ifdef ESP8266
	WiFiClient::stopAll();
#endif

	// Check what boot mode currently selected.
	pinMode(15, INPUT);
	pinMode(0, INPUT);
	pinMode(2, INPUT);

	int v = 0;
	v |= (digitalRead(15)) ? 1 << 2 : 0;
	v |= (digitalRead(0)) ?  1 << 1 : 0;
	v |= (digitalRead(2)) ?  1 << 0 : 0;

	log_i("Die. boot: %d", v);
	for (;;) {
		delay(20);
		//ESP.reset();
		ESP.restart();
	}
}
