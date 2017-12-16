#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>   // https://github.com/bblanchon/ArduinoJson
#include <WebSocketsClient.h> // be sure to use the one from the libraries dir
#include "time.h"
#include <Wire.h>
#include <SSD1306.h>
#include <OLEDDisplayUi.h>
#include "config.h"

// GVB websocket address
// wss://maps.gvb.nl:8443/stops/02113
static const char ws_host[] = "maps.gvb.nl";
//static const char ws_host[] = "134.213.137.29";
static const int ws_port = 8443;
static const char ws_query[] = "[5,\"/stops/02113\"]";
WebSocketsClient ws;
DynamicJsonBuffer jsonBuffer(4096);


// query parameters
//const unsigned long query_interval_ms = 60 * 1000; // once per minute
const unsigned long query_interval_ms = 10 * 1000; // debug
unsigned long last_query_ms;

// OLED display object definition (address, SDA, SCL)
SSD1306 display(0x3c, 4, 15);
OLEDDisplayUi ui( &display );

WiFiClient client; // wifi client object

void
ws_event(WStype_t type, uint8_t * payload, size_t len)
{
	// When using a StaticJsonBuffer you must allocate sufficient memory for the json string returned by the WU api
if(0)
{
	Serial.print("event ");
	Serial.print(type);
	Serial.print(" ");
}

	if (type == WStype_DISCONNECTED)
	{
		Serial.println("DISCONNECTED");
		return;
	}
	if (type == WStype_CONNECTED)
	{
		Serial.println("CONNECTED");
		ws.sendTXT(ws_query);
		return;
	}
	if (type != WStype_TEXT)
	{
		Serial.println("???");
		return;
	}

if(0)
{
	Serial.print("='");
	Serial.write(payload, len);
	Serial.println("'");
}

	String payload_str = "{\"root\":";
	payload_str += (const char*) payload;
	payload_str += "}";

	jsonBuffer.clear();
	JsonObject& root = jsonBuffer.parseObject(payload_str);

	if(!root.success())
	{
		Serial.println("Unable to create a root object");
		return;
	}

	// The format is '[id,url,details]', where details is a string
	// that packs another json object.
	const char * trip_str = root["root"][2];

	JsonObject& trip = jsonBuffer.parseObject(trip_str);
	if(!trip.success())
	{
		Serial.println("Unable to parse trip object");
		return;
	}

	// see response.txt for sample object
	JsonObject &journey = trip["journey"];
	const char * line_number = journey["lineNumber"];
	const char * destination = journey["destination"];

	JsonObject &arrival = trip["calls"][0];
	int delay_sec = arrival["delay"];
	const char * live_arrival = arrival["liveArrivalAt"];

	printf("%s %s: %s (%d)\n",
		line_number,
		destination,
		live_arrival,
		delay_sec
	);
}


void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state)
{
/*
	struct tm timeinfo;
	if(!getLocalTime(&timeinfo))
	{
		Serial.println("Failed to obtain time");
		return;
	}

	//See http://www.cplusplus.com/reference/ctime/strftime/
	time_str = asctime(&timeinfo); // Displays: Sat Jun 24 14:05:49 2017
	display->setFont(ArialMT_Plain_10);
	display->setTextAlignment(TEXT_ALIGN_CENTER); // The coordinates define the center of the screen!
	display->drawString(18,53,time_str.substring(4,10));
	display->drawString(107,53,time_str.substring(11,19));
*/
}

void draw_frame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
	display->setFont(ArialMT_Plain_16);

    //display->drawRect(0,23,40,33); // Icon alignment rectangle
    display->setTextAlignment(TEXT_ALIGN_RIGHT);
    //display->drawString(x+128,y+8,getCurrWeather());
    //display->drawString(x+128,y+30,getCurrC()+"°C / "+getRelHum()); // use getCurrF for fahrenheit
}


// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { draw_frame, };

// how many frames are there?
const int frameCount = sizeof(frames) / sizeof(*frames);

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { msOverlay, };
const int overlaysCount = sizeof(overlays) / sizeof(*overlays);

int Start_WiFi(const char* ssid, const char* password)
{
	int connAttempts = 0;
	Serial.println("\r\nConnecting to: "+String(ssid));
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED ) {
		delay(500);
		Serial.print(".");
		if(connAttempts > 20)
			return -5;
		connAttempts++;
	}

	Serial.println("WiFi connected\r\n");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
	return 1;
}
 

void setup() { 
  pinMode(16,OUTPUT);
  digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
  delay(50); 
  digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high

  Serial.begin(115200);

    // The ESP is capable of rendering 60fps in 80Mhz mode but that won't give you much time for anything else run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(30);
  // Customize the active and inactive symbol
//  ui.setActiveSymbol(activeSymbol);
//  ui.setInactiveSymbol(inactiveSymbol);
  // You can change this to TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);
  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);
  // You can change the transition that is used SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);
  // Add frames
  ui.setFrames(frames, frameCount);
  // Add overlays
  ui.setOverlays(overlays, overlaysCount);
  // Initialising the UI will init the display too.
  ui.init();
  display.flipScreenVertically();
  Start_WiFi(ssid,password);
  configTime(1, 3600, "pool.ntp.org");

  // force a query on the entry to the loop
  last_query_ms = 0;

	Serial.println("attempting connection");
	ws.beginSSL(ws_host, ws_port, "/", "", "BusTimeNL");
	ws.onEvent(ws_event);
	ws.setReconnectInterval(query_interval_ms);
}

 
void loop()
{
	const int remaining_ms = ui.update();
	if (remaining_ms < 0)
		return;

	ws.loop();

	unsigned long now_ms = millis();
	if (now_ms - last_query_ms < query_interval_ms)
	{
		delay(remaining_ms);
		return;
	}

	// attempt a reconnect or send a new query
	last_query_ms = now_ms;

/*
	Serial.println("sending query");
	if (!ws.sendTXT(ws_query))
		Serial.println("FAILED");
*/
}
