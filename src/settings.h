// MQTT settings
const char* mqtt_server = "MQTT_BROKER_IP";
const char* mqtt_pub_topic = "homeassistant/smartmeter"; // Change to the topic you want to publish to

// Hardware settings
const bool invertRxPin = true; // In case no HW transistor for inverting the Rx signal is used, invert the signal here
int uart2_tx_gpio = 17;

// Network settings for static IP
IPAddress local_IP(192, 168, 8, 47);
IPAddress gateway(192, 168, 8, 1);
IPAddress subnet(255, 255, 255, 0);

// NTP Settings
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

const byte start_byte = 0x7E; 
const byte stop_byte = 0x7E;

const int max_wait_time_for_reading_data = 1100;
int delay_before_reading_data = 1000;

const int message_length = 121; // Message length for ISKRA is 121. Message length for Siemens is 123.
byte message[message_length];
byte buffer[90];

bool use_test_data = false;
byte testData[123] = {0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                      0x00, 0x00, 0x7e};
