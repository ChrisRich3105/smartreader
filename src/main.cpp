
#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <TelnetStream.h>
#include <Crypto.h>
#include <AES.h>
#include <GCM.h>
#include "time.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>

#include "secrets.h"
#include "settings.h"

// NTP Settings
WiFiUDP ntpUDP;
unsigned int localPort = 2390;      // local port to listen for UDP packets
NTPClient timeClient(ntpUDP, "0.at.pool.ntp.org");
struct tm *ntpTime;
uint8_t current_time_day;
uint8_t current_time_month;
uint16_t current_time_year;

// message date variables
uint16_t message_year;
uint8_t message_month;
uint8_t message_day;
uint8_t message_hour;
uint8_t message_minute;
uint8_t message_second;

// reading variables
uint32_t counter_reading_p_in;
uint32_t counter_reading_p_out;
uint32_t counter_reading_q_in;
uint32_t counter_reading_q_out;
uint32_t current_power_usage_in;
uint32_t current_power_usage_out;

struct Vector_GCM {
    const char *name;
    uint8_t key[16];
    uint8_t ciphertext[90];
    uint8_t authdata[16];
    uint8_t iv[12];
    uint8_t tag[12];
    size_t authsize;
    size_t datasize;
    size_t tagsize;
    size_t ivsize;
};

Vector_GCM Vector_SM;
GCM<AES128> *gcmaes128 = 0;


// MQTT
WiFiClient espClient;
PubSubClient client(espClient);

boolean getLocalTime()
{

  unsigned long epochTime = timeClient.getEpochTime();
  //Get a time structure
  ntpTime = gmtime ((time_t *)&epochTime); 
  current_time_day = ntpTime->tm_mday;
  current_time_month = ntpTime->tm_mon + 1; // Month is 0 - 11, add 1 to get a jan-dec 1-12 concept
  current_time_year = ntpTime->tm_year + 1900; // Year is # years since 1900
  return true;
}

void printLocalTime() {
  if (getLocalTime()) {
    Serial.printf("%d-%d-%d_%d:%d:%d\n", ntpTime->tm_year, ntpTime->tm_mon, ntpTime->tm_mday, ntpTime->tm_hour, ntpTime->tm_min, ntpTime->tm_sec);  
    TelnetStream.printf("%d-%d-%d_%d:%d:%d\n", ntpTime->tm_year, ntpTime->tm_mon, ntpTime->tm_mday, ntpTime->tm_hour, ntpTime->tm_min, ntpTime->tm_sec);  
  }
}

void print_array(byte array[], unsigned int len)
{
  char text_buffer[len];

  for (unsigned int i = 0; i < len; i++)
  {
      byte nib1 = (array[i] >> 4) & 0x0F;
      byte nib2 = (array[i] >> 0) & 0x0F;
      text_buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
      text_buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
  }
  text_buffer[len*2] = '\0';
  for (unsigned int i = 0; i < len; i++) {
    Serial.print(text_buffer[i]);
  }
}

uint32_t byteToUInt32(byte array[], unsigned int startByte)
{
    // https://stackoverflow.com/questions/12240299/convert-bytes-to-int-uint-in-c
    // convert 4 bytes to uint32
    uint32_t result;
    result = (uint32_t) array[startByte] << 24;
    result |=  (uint32_t) array[startByte+1] << 16;
    result |= (uint32_t) array[startByte+2] << 8;
    result |= (uint32_t) array[startByte+3];

    return result;
}

void parse_timestamp(byte array[]) {
  message_year = (array[6] << 8) + (array[7]);
  message_month = array[8];
  message_day = array[9];
  message_hour = array[11];
  message_minute = array[12];
  message_second = array[13];
}

// used to see if encrypted data are correct...
bool validate_message_date() {
  Serial.println();
  // compare date from ntp and message should be the same day!
  Serial.println();
  TelnetStream.println();
  Serial.println("======DEBUG=======");
  TelnetStream.println("======DEBUG=======");
  Serial.printf("DATE FROM NTP: %02d-%02d-%02d", current_time_year, current_time_month, current_time_day);
  TelnetStream.printf("DATE FROM NTP: %02d-%02d-%02d", current_time_year, current_time_month, current_time_day);
  Serial.println();
  TelnetStream.println();
  Serial.printf("DATE FROM MESSAGE: %02d-%02d-%02d", message_year, message_month, message_day);
  TelnetStream.printf("DATE FROM MESSAGE: %02d-%02d-%02d", message_year, message_month, message_day);
  Serial.println();
  TelnetStream.println();

  if (current_time_year == message_year and current_time_month == message_month and current_time_day == message_day){
    Serial.printf("Message Date is VALID!, ntp_date: %02d-%02d-%02d == message_date: %02d-%02d-%02d\n", current_time_year, current_time_month, current_time_day, message_year, message_month, message_day);
    TelnetStream.printf("Message Date is VALID!, ntp_date: %02d-%02d-%02d == message_date: %02d-%02d-%02d\n", current_time_year, current_time_month, current_time_day, message_year, message_month, message_day);
    Serial.println("======DEBUG=======");
    TelnetStream.println("======DEBUG=======");
    Serial.println();
    TelnetStream.println();
    return true;
  }
  else {
    Serial.printf("Message Date is INVALID!, ntp_date: %02d-%02d-%02d !=  message_date: %02d-%02d-%02d\n", current_time_year, current_time_month, current_time_day, message_year, message_month, message_day);
    TelnetStream.printf("Message Date is INVALID!, ntp_date: %02d-%02d-%02d !=  message_date: %02d-%02d-%02d\n", current_time_year, current_time_month, current_time_day, message_year, message_month, message_day);
    Serial.println("======DEBUG=======");
    TelnetStream.println("======DEBUG=======");
    Serial.println();
    TelnetStream.println();
    return false;
  }
}

int readMessage() {
    unsigned short serial_cnt = 0;
    if (use_test_data == true) {
        Serial.println("USE TEST DATA IS ACTIVE!");
        TelnetStream.println("USE TEST DATA IS ACTIVE!");
        serial_cnt = message_length;
        for (unsigned int i = 0; i < message_length; i++) {
            message[i] = testData[i];
        }
    }
    else {
        Serial.println("Try to read data from serial port.");
        TelnetStream.println("Try to read data from serial port.");
        
        memset(message, 0, message_length);

        int cnt = 0;
        int readBuffer = 250;

        Serial.swap();

        pinMode(LED_BUILTIN, OUTPUT);
        pinMode(D1, OUTPUT);
        digitalWrite(LED_BUILTIN, HIGH);
        digitalWrite(D1,HIGH);
        unsigned long requestMillis = millis();
        delay(delay_before_reading_data);
        while ((Serial.available()) && (cnt < readBuffer) && (millis()-requestMillis <= max_wait_time_for_reading_data)) {
          message[cnt] = Serial.read();
          if (message[0] != start_byte && cnt == 0) {
            continue;
          }
          else {
            cnt++;
          }
        }
        Serial.swap();

        digitalWrite(LED_BUILTIN, LOW);
        digitalWrite(D1,LOW);
    }
  Serial.println("Done with reading from from serial port.");
  TelnetStream.println("Done with reading from from serial port.");
  return (serial_cnt);
}

void init_vector(Vector_GCM *vect, const char *Vect_name, byte *key_SM) {
  vect->name = Vect_name;  // vector name
  for (unsigned int i = 0; i < 16; i++) {
    vect->key[i] = key_SM[i];
  }

  for (unsigned int i = 0; i < 90; i++) {
    vect->ciphertext[i] = message[i+ (message_length - 93)];
  }
  byte AuthData[] = {0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf}; // fixed value, i got it from the gurus director software

  for (int i = 0; i < 16; i++) {
     vect->authdata[i] = AuthData[i];
  }

  for (int i = 0; i < 8; i++) {
     vect->iv[i] = message[i + (message_length - 107)]; // manufacturer + serialnumber 8 bytes
  }
  for (int i = 8; i < 12; i++) {
    vect->iv[i] = message[i + (message_length - 105)]; // frame counter
  }

  byte tag[12]; // 12x zero
  for (int i = 0; i < 12; i++) {
    vect->tag[i] = tag[i];
  }

  vect->authsize = 16;
  vect->datasize = 90;
  vect->tagsize = 12;
  vect->ivsize  = 12;
}

void decrypt_text(Vector_GCM *vect) {
  gcmaes128 = new GCM<AES128>();
  gcmaes128->setKey(vect->key, gcmaes128->keySize());
  gcmaes128->setIV(vect->iv, vect->ivsize);
  gcmaes128->decrypt((byte*)buffer, vect->ciphertext, vect->datasize);

  // this does not work...
  // bool decryption_failed = false;
  // if (!gcmaes128->checkTag(vect->tag, vect->tagsize)) {
  //   decryption_failed = true;
  //   Serial.println("Decryption Failed!");
  // }
  // else {
  //   Serial.println("Decryption OK!");
  // }
  delete gcmaes128;
}

void parse_message(byte array[]) {
      counter_reading_p_in = byteToUInt32(array, 57);
      counter_reading_p_out = byteToUInt32(array, 62);
      counter_reading_q_in = byteToUInt32(array, 67);
      counter_reading_q_out = byteToUInt32(array, 72);
      current_power_usage_in = byteToUInt32(array, 77);
      current_power_usage_out = byteToUInt32(array, 82);

      // Serial.println(result, DEC);
      Serial.printf("counter_reading_p_in: %d\n", counter_reading_p_in);
      TelnetStream.printf("counter_reading_p_in: %d\n", counter_reading_p_in);
      Serial.printf("counter_reading_p_out: %d\n", counter_reading_p_out);
      TelnetStream.printf("counter_reading_p_out: %d\n", counter_reading_p_out);
      Serial.printf("counter_reading_q_in: %d\n", counter_reading_q_in);
      TelnetStream.printf("counter_reading_q_in: %d\n", counter_reading_q_in);
      Serial.printf("counter_reading_q_out: %d\n", counter_reading_q_out);
      TelnetStream.printf("counter_reading_q_out: %d\n", counter_reading_q_out);
      Serial.printf("current_power_usage_in: %d\n", current_power_usage_in);
      TelnetStream.printf("current_power_usage_in: %d\n", current_power_usage_in);
      Serial.printf("current_power_usage_out: %d\n", current_power_usage_out);
      TelnetStream.printf("current_power_usage_out: %d\n", current_power_usage_out);
}

void printBytesToHex(byte array[], unsigned int len) {
  
  for (unsigned int i = 0; i < len; i++) {
    Serial.printf("%02X ",array[i]);
    TelnetStream.printf("%02X ",array[i]);
  }
  Serial.print("\n");
  TelnetStream.print("\n");
}

void SerialTelnetPrint(char msg[]) {
  Serial.println(msg);
  Serial.println(msg);
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT
}

void setup() {
    Serial.begin(115200, SERIAL_8N1, SERIAL_FULL, uart2_tx_gpio, invertRxPin);
    
    //connect to WiFi
    Serial.printf("Connecting to %s ", wifi_ssid);
    // // Configures static IP address
    if (!WiFi.config(local_IP, gateway, subnet)) {
      Serial.println("STA Failed to configure");
    }
    //WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("Connection Failed! Rebooting...");
      delay(5000);
      ESP.restart();
    }

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
    
  // Port defaults to 3232
  ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("sm_reader");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    });
  ArduinoOTA
    .onEnd([]() {
      Serial.println("\nEnd");
    });
  ArduinoOTA
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
  ArduinoOTA
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });


    ArduinoOTA.begin();
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());


    // Initialize a NTPClient to get time
    timeClient.begin();
    // Set offset time in seconds to adjust for your timezone, for example:
    // GMT +1 = 3600
    // GMT +8 = 28800
    // GMT -1 = -3600
    // GMT 0 = 0
    timeClient.setTimeOffset(gmtOffset_sec);

    TelnetStream.begin();
}
void reconnect() {
  // Loop until we're reconnected
  if (!client.connected()) {
    TelnetStream.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", mqtt_auth_user, mqtt_auth_password)) {
      TelnetStream.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    } else {
      TelnetStream.print("failed, rc=");
      TelnetStream.print(client.state());
      TelnetStream.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void publishData() {
    char buffer[128];
    sprintf(buffer, "{\"counter_p_in\": %d, \"counter_p_out\": %d,\"counter_q_in\": %d, \"counter_q_out\": %d, \"current_p_in\": %d, \"current_p_out\": %d}", 
    counter_reading_p_in, counter_reading_p_out, counter_reading_q_in, counter_reading_q_out, current_power_usage_in, current_power_usage_out);
    client.publish(mqtt_pub_topic, buffer);
}

void loop() {
    ArduinoOTA.handle();

    if (!client.connected()) {
      reconnect();
    }
    client.loop();
    
    //init and get the time
    //configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org");
    timeClient.update();
    unsigned long epochTime = timeClient.getEpochTime();
    TelnetStream.print("Epoch Time: ");
    TelnetStream.println(epochTime);
    printLocalTime();
    
    Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    TelnetStream.printf("RSSI: %d dBm\n", WiFi.RSSI());
    Serial.println(WiFi.BSSIDstr());
    TelnetStream.println(WiFi.BSSIDstr());

    readMessage();
    publishData();
    if (message[0] == start_byte and message[sizeof(message)-1] == stop_byte) {
      Serial.println("Got message from meter, try to decrypt.");
      TelnetStream.println("Got message from meter, try to decrypt.");
      Serial.print("ReceivedMessage: ");
      TelnetStream.print("ReceivedMessage: ");
      printBytesToHex(message, (sizeof(message)/sizeof(message[0])));

      init_vector(&Vector_SM,"Vector_SM",sm_decryption_key); 

      // print decryption details
      Serial.print("IV: ");
      TelnetStream.print("IV: ");
      printBytesToHex(Vector_SM.iv, (sizeof(Vector_SM.iv)/sizeof(Vector_SM.iv[0])));
      Serial.print("Key: ");
      TelnetStream.print("Key: ");
      printBytesToHex(Vector_SM.key, (sizeof(Vector_SM.key)/sizeof(Vector_SM.key[0])));
      Serial.print("Authdata: ");
      TelnetStream.print("Authdata: ");
      printBytesToHex(Vector_SM.authdata, (sizeof(Vector_SM.authdata)/sizeof(Vector_SM.authdata[0])));
      Serial.print("Tag: ");
      TelnetStream.print("Tag: ");
      printBytesToHex(Vector_SM.tag, (sizeof(Vector_SM.tag)/sizeof(Vector_SM.tag[0])));
      Serial.print("Encrypted Data (Ciphertext): ");
      TelnetStream.print("Encrypted Data (Ciphertext): ");
      printBytesToHex(Vector_SM.ciphertext, (sizeof(Vector_SM.ciphertext)/sizeof(Vector_SM.ciphertext[0])));

      decrypt_text(&Vector_SM);
      Serial.print("Decrypted Data: ");
      TelnetStream.print("Decrypted Data: ");
      printBytesToHex(buffer, (sizeof(buffer)/sizeof(buffer[0])));

      Serial.print("======Decrypted Parsed Data======\n");
      TelnetStream.print("======Decrypted Parsed Data======\n");
      parse_message(buffer);
      publishData();
      Serial.print("======Decrypted Parsed Data======\n");
      TelnetStream.print("======Decrypted Parsed Data======\n");

      parse_timestamp(buffer);

      if (validate_message_date()) {
        Serial.println("Do something.");
        TelnetStream.println("Do something.");
      }
      else {
        Serial.println("Do nothing.");
        TelnetStream.println("Do nothing.");
      }

    }
    else {
      Serial.println("Message not starting/ending with 0xE7, skip this message!");
      TelnetStream.println("Message not starting/ending with 0xE7, skip this message!");
      Serial.print("Received Message: ");
      TelnetStream.print("Received Message: ");
      printBytesToHex(message, (sizeof(message)/sizeof(message[0])));
    }

    delay(1000);
    Serial.println("waiting 1 second...");
    TelnetStream.println("waiting 1 second...");
    Serial.println("reset");
    TelnetStream.println("reset");
}