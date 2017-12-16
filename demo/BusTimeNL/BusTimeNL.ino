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
static const char *ws_queries[] = {
	"[5,\"/stops/02113\"]",
	"[5,\"/stops/02114\"]",
};
static const int ws_num_queries = sizeof(ws_queries)/sizeof(*ws_queries);
WebSocketsClient ws;


// query parameters
//const unsigned long query_interval_ms = 60 * 1000; // once per minute
const unsigned long query_interval_ms = 10 * 1000; // debug
unsigned long last_query_ms;

// linked list of train structures
struct train_t {
	train_t * next;
	train_t ** prev;
	int id;
	char status; // U or A (upcoming or arriving)
	char type; // B or T (bus or tram)
	unsigned long arrival;
	int delay_sec;
	char line_number[5];
	char destination[32];
};

static train_t * train_list;

train_t * train_find_and_remove(int id)
{
	train_t * t = train_list;
	while(t)
	{
		if(t->id != id)
		{
			t = t->next;
			continue;
		}

		// found it -- remove it from the list
		*(t->prev) = t->next;
		if (t->next)
			t->next->prev = t->prev;
		t->next = NULL;
		return t;
	}

	return t;
}

train_t * train_create(int id)
{
	train_t * t = (train_t*) calloc(1, sizeof(*t));
	t->id = id;
	t->next = NULL;
	t->prev = NULL;
	t->arrival = 0;
	t->delay_sec = 0;
	t->status = '?';
	t->type = '?';

	return t;
}


void train_insert(train_t * nt)
{
	train_t **t = &train_list;
	while(*t)
	{
		if((*t)->arrival > nt->arrival)
			break;

		t = &(*t)->next;
	}

	// found our spot or the list is empty
	nt->next = *t;
	nt->prev = t;
	*t = nt;
}


// OLED display object definition (address, SDA, SCL)
SSD1306 display(0x3c, 4, 15);
OLEDDisplayUi ui( &display );

WiFiClient client; // wifi client object

void
ws_event(WStype_t type, uint8_t * payload, size_t len)
{
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
		for(int i = 0 ; i < ws_num_queries ; i++)
			ws.sendTXT(ws_queries[i]);
		return;
	}
	if (type != WStype_TEXT)
	{
		Serial.println("???");
		return;
	}

	if (len > 1024)
	{
		Serial.print("ERROR: len=");
		Serial.println(len);
		return;
	}

if(0)
{
	Serial.print("='");
	Serial.write(payload, len);
	Serial.println("'");
}

	//StaticJsonBuffer<1024*2> jsonBuffer;
	DynamicJsonBuffer jsonBuffer(4096);
	// since we own the payload, we can modify it into an array
	//JsonObject& root = jsonBuffer.parseObject(payload_str);
	JsonArray & root = jsonBuffer.parseArray((char*) payload);

	if(!root.success())
	{
		Serial.println("Unable to create a root object");
		return;
	}

	// The format is '[id,url,details]', where details is a string
	// that packs another json object.  We can cast-away the const,
	// since we don't care if the string is modified
	const char * trip_str = root[2];

	DynamicJsonBuffer jsonBuffer2(4096);
	JsonObject& trip = jsonBuffer2.parseObject(trip_str);
	if(!trip.success())
	{
		Serial.println("Unable to parse trip object");
		jsonBuffer.clear();
		return;
	}

	// see response.txt for sample object
	JsonObject &journey = trip["journey"];
	const char * line_number = journey["lineNumber"];
	const char * destination = journey["destination"];
	const char * vehicle = journey["vehicletype"];

	// should use "id" : "gvb:3:zkg:243:2017-12-16",
	// but for now using the short number
	const int trip_id = trip["trip"]["number"];
	const char * date = trip["trip"]["operatingDate"];

	JsonObject &arrival = trip["calls"][0];
	int delay_sec = 0;
	const char * arrival_time = arrival["liveArrivalAt"];

	// if there is no live arrival time, don't fetch the delay
	// and instead just fetch the planned time
	if(arrival_time)
	{
		delay_sec = arrival["delay"];
	} else {
		arrival_time = arrival["plannedArrivalAt"];
	}

	if (!arrival_time)
		arrival_time = "N/A";

	const char * status = arrival["status"];

	struct tm tm = {};
/*
	strptime(date, "%Y-%m-%d", &tm);
	strptime(arrival_time, "%H-%M-%S", &tm);
*/
	sscanf(date, "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
	sscanf(arrival_time, "%d:%d:%d", &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
	tm.tm_year -= 1900; // years since 1900
	tm.tm_mon -= 1; // months go from 0 - 11
	time_t at = mktime(&tm);

	printf("%4d %8s %s %+4d: %-3s %c %s\n",
		trip_id,
		status,
		arrival_time,
		delay_sec,
		line_number,
		vehicle[0],
		destination
	);

	if (arrival_time[0] == 'N')
	{
		// we can't parse a nonexistant time
		return;
	}

	// figure out where to insert this into our list

	train_t * t = train_find_and_remove(trip_id);
	if (!t)
	{
		if (status[0] == 'P')
		{
			// we have never heard of this train,
			// so let's not bother adding it
			return;
		}

		t = train_create(trip_id);
		t->type = vehicle[0];
		strlcpy(t->destination, destination, sizeof(t->destination));
		strlcpy(t->line_number, line_number, sizeof(t->line_number));
	}

	if (status[0] == 'P')
	{
		// "Passed": we've already removed it from the list
		// so we release the trip
		free(t);
		return;
	} else
	if (status[0] == 'A' || status[0] == 'U')
	{
		// "Arriving", "Upcoming" or "Unknown"
		// we'll update the arrival time
	} else {
		printf("%d: Unknown status: '%s'\n",
			trip_id,
			status
		);
	}
 
	// update the arrival time
	t->status = status[0];
	t->arrival = at;
	t->delay_sec = delay_sec;

	// re-insert it into the sorted list
	train_insert(t);

	jsonBuffer.clear();
	jsonBuffer2.clear();

	
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

void draw_frame(int start, OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
	time_t now = time(NULL);
	char buf[32];

	train_t * t = train_list;

	// throw away up to the starting train
	for(int i = 0 ; t && i < start ; i++, t = t->next)
		;

	// and draw the next three
	for(int i = 0 ; t && i < 3 ; i++, t = t->next)
	{
		display->setFont(ArialMT_Plain_16);
		display->setTextAlignment(TEXT_ALIGN_RIGHT);
		display->drawString(x+25, y+i*21+2, t->line_number);

		display->setFont(ArialMT_Plain_10);
		display->setTextAlignment(TEXT_ALIGN_LEFT);
		display->drawString(x+30, y+i*21+0, t->destination);

		int delta = t->arrival - now - 3600; // fix for UTC to NL
if(1)
		snprintf(buf, sizeof(buf), "%2d:%02d",
			delta / 60,
			delta % 60
		);
else
		snprintf(buf, sizeof(buf), "%d", delta);

		if (t->status == 'A')
		{
			display->drawString(x+30, y+i*21+10, "ARRIVING");
		} else {
			display->drawString(x+30, y+i*21+10, buf);
		}

		if (t->delay_sec != 0)
		{
			snprintf(buf, sizeof(buf), "%+4d",
				t->delay_sec
			);
			display->setTextAlignment(TEXT_ALIGN_RIGHT);
			display->drawString(x+110, y+i*21+10, buf);
		}
	}
}

void draw_frame0(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
	draw_frame(0, display, state, x, y);
}

void draw_frame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
	draw_frame(3, display, state, x, y);
}

void draw_frame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
	draw_frame(6, display, state, x, y);
}


// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { draw_frame0, draw_frame1, draw_frame2, };

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
  ui.setIndicatorPosition(RIGHT);
  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);
  // You can change the transition that is used SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_UP);
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

	// dump the list
	train_t * t = train_list;
	while(t)
	{
		printf("%3d %c %d %-4d: %-3s %c %s\n",
			t->id,
			t->status,
			t->arrival,
			t->delay_sec,
			t->line_number,
			t->type,
			t->destination
		);

		t = t->next;
	}

/*
	Serial.println("sending query");
	if (!ws.sendTXT(ws_query))
		Serial.println("FAILED");
*/
}
