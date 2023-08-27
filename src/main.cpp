#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266WebServer.h>

const char* ssid = "IoT";
const char* password = "gerbangub";
const char* password_ota = "123teknik";
const char* host_ota = "esp8266";
ESP8266WebServer server(80);

uint8_t interruptPin = 13;

String request_string;
const char* host_database = "175.45.185.189";
WiFiClient client;
HTTPClient http;
String p="HTTP/1.1 \r\n";

int jumlah, dummy;
const char* x = "Suhat";

bool trig, trigpin;

unsigned long bounce, prev_bounce = 0, debounce_delay=1000, lama, waktu, prev_waktu = 0, delay_kirim;

/* Style */
String style =
"<style>#file-input,input{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}"
"input{background:#f1f1f1;border:0;padding:0 15px}body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}"
"#file-input{padding:0;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer}"
"#bar,#prgbar{background-color:#f1f1f1;border-radius:10px}#bar{background-color:#3498db;width:0%;height:10px}"
"form{background:#fff;max-width:258px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}"
".btn{background:#3498db;color:#fff;cursor:pointer}</style>";

/* Login page */
String loginIndex = 
"<form name=loginForm>"
"<h1>Counter Gerbang Login</h1>"
"<input name=userid placeholder='User ID'> "
"<input name=pwd placeholder=Password type=Password> "
"<input type=submit onclick=check(this.form) class=btn value=Login></form>"
"<script>"
"function check(form) {"
"if(form.userid.value=='admin' && form.pwd.value=='admin')"
"{window.open('/serverIndex')}"
"else"
"{alert('Error Password or Username')}"
"}"
"</script>" + style;
 
/* Server Index Page */
String serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
"<input type='file' name='update' id='file' onchange='sub(this)' style=display:none>"
"<label id='file-input' for='file'>   Choose file...</label>"
"<input type='submit' class=btn value='Update'>"
"<br><br>"
"<div id='prg'></div>"
"<br><div id='prgbar'><div id='bar'></div></div><br></form>"
"<script>"
"function sub(obj){"
"var fileName = obj.value.split('\\\\');"
"document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];"
"};"
"$('form').submit(function(e){"
"e.preventDefault();"
"var form = $('#upload_form')[0];"
"var data = new FormData(form);"
"$.ajax({"
"url: '/update',"
"type: 'POST',"
"data: data,"
"contentType: false,"
"processData:false,"
"xhr: function() {"
"var xhr = new window.XMLHttpRequest();"
"xhr.upload.addEventListener('progress', function(evt) {"
"if (evt.lengthComputable) {"
"var per = evt.loaded / evt.total;"
"$('#prg').html('progress: ' + Math.round(per*100) + '%');"
"$('#bar').css('width',Math.round(per*100) + '%');"
"}"
"}, false);"
"return xhr;"
"},"
"success:function(d, s) {"
"console.log('success!') "
"},"
"error: function (a, b, c) {"
"}"
"});"
"});"
"</script>" + style;

//OTA COM
void ota()
{
	// Port defaults to 8266
	// ArduinoOTA.setPort(8266);

	// Hostname defaults to esp8266-[ChipID]
	// ArduinoOTA.setHostname("myesp8266");

	ArduinoOTA.setPassword(password_ota);
	ArduinoOTA.onStart([]() 
	{
		Serial.println("Start");
	});
	ArduinoOTA.onEnd([]()
	{
		Serial.println("\nEnd");
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) 
	{
		Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error) 
	{
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
		else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
		else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
		else if (error == OTA_END_ERROR) Serial.println("End Failed");
	});
	ArduinoOTA.begin();
	Serial.println("Ready");
}

//Connect WIFI
void konek_wifi()
{
	// start connection
	delay(10);
	Serial.println("Connecting to ");
	Serial.println(ssid); 

	WiFi.begin(ssid, password); 
	while (WiFi.status() != WL_CONNECTED) 
	{
		digitalWrite(LED_BUILTIN, HIGH);
		delay(500);
		digitalWrite(LED_BUILTIN, LOW); 
	}
	Serial.println("");
	Serial.println("WiFi connected");
}

//Webserver
void webserverprogram()
{
	if (!MDNS.begin(host_ota)) { //http://esp8266/
		Serial.println("Error setting up MDNS responder!");
		while (1) {
			delay(1000);
		}
	}
	Serial.println("mDNS responder started");
	/*return index page which is stored in serverIndex */
	server.on("/", HTTP_GET, []() {
		server.sendHeader("Connection", "close");
		server.send(200, "text/html", loginIndex);
	});
	server.on("/serverIndex", HTTP_GET, []() {
		server.sendHeader("Connection", "close");
		server.send(200, "text/html", serverIndex);
	});
	/*handling uploading firmware file */
	server.on("/update", HTTP_POST, []() {
		server.sendHeader("Connection", "close");
		server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
		ESP.restart();
	}, []() {
		HTTPUpload& upload = server.upload();
		if (upload.status == UPLOAD_FILE_START) {
			Serial.printf("Update: %s\n", upload.filename.c_str());
			if (!Update.begin(UPDATE_ERROR_SIZE)) { //start with max available size
				Update.printError(Serial);
			}
		} else if (upload.status == UPLOAD_FILE_WRITE) {
			/* flashing firmware to ESP*/
			if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
				Update.printError(Serial);
			}
		} else if (upload.status == UPLOAD_FILE_END) {
			if (Update.end(true)) { //true to set the size to the current progress
				Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
			} else {
				Update.printError(Serial);
			}
		}
	});
	server.begin();
}

// Fungsi Kirim data
void koneksi_database()
{
	if (!client.connect(host_database,80))
	{
		Serial.println("Gagal Konek");
		return;
	}
	//if (bounce - delay_kirim >= 1000)
	request_string = "/gate/count.php?gate=";
	request_string += x;
	Serial.print("requesting URL: ");
	Serial.println(request_string);
	client.print(String("GET ") + request_string  +"\r\n" + p + "Host: " + host_database + "\r\n" + "Connection: close\r\n\r\n");
	dummy=jumlah;
	
	unsigned long timeout = bounce;
	while (client.available() == 1)
	{
		if (bounce - timeout > 5000)
		{
			Serial.println(">>> Client Timeout !");
			client.stop();
			return;
		}
	}
}

//Fungsi untuk Menghitung
void ICACHE_RAM_ATTR trigger()
{
	trig=1;
}

//Fungsi untuk Menghitung
void count()
{
	trigpin=digitalRead(interruptPin);
	bounce = millis();
	if (bounce - prev_bounce >= debounce_delay && trigpin==0)
	{
		jumlah++;
		Serial.print(jumlah);
		koneksi_database();
	}
}

void setup() 
{
	pinMode(LED_BUILTIN, OUTPUT);
	jumlah=0;
	trig=0;
	Serial.begin(115200);
	Serial.println("Booting");
	WiFi.mode(WIFI_STA);
	konek_wifi();
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
	webserverprogram();
	ota();
	// declare Infrared sensor as input
	attachInterrupt(digitalPinToInterrupt(interruptPin), trigger, FALLING);
}

void loop()
{
	digitalWrite(LED_BUILTIN, LOW);
	ArduinoOTA.handle();
	server.handleClient();
	trigpin=digitalRead(interruptPin);
	if (WiFi.waitForConnectResult() != WL_CONNECTED)
	{
		Serial.println("Disconnect");
		digitalWrite(LED_BUILTIN, HIGH);
		konek_wifi();
		Serial.print("IP address: ");
		Serial.println(WiFi.localIP());
	}
	if (trig==1)
	{
		prev_bounce = bounce;
		count();
		trig=0;
	}
	if (dummy != jumlah)
	{
		koneksi_database();
	}
}
