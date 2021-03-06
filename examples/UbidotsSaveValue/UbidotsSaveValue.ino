/********************************
 * Libraries included
 *******************************/
 #include <SPI.h>
 #include <WiFi.h>
 //#define UBI_WIN 1 // Uncomment if you are using Windows
 #include <UbidotsCC3200.h>

/********************************
 * Constants and objects
 *******************************/
 namespace {
  char SSID_NAME[] = "Assign_your_SSID_name"; // Assign your SSID Name
  char SSID_PASS[] = "Assign_your_SSID_password"; // Assign your SSID Password
  const char * TOKEN = "Assign_your_Ubidots_token"; // Assign your Ubidots TOKEN
  const char * VARIABLE_LABEL= "temperature"; // Assign the Ubidots' Variable label
}

/* initialize the instance */
Ubidots client(TOKEN);

/********************************
 * Main Functions
 *******************************/
void setup() {
  Serial.begin(115200);
  client.wifiConnection(SSID_NAME, SSID_PASS);
  //client.setDeviceLabel("my-new-device"); // Uncomment this line to change the default device name
  //client.setDebug(true); // Uncomment this line to set DEBUG on
}

void loop() {
  /* Sensors readings */
  float value = analogRead(2);
  /* Sending value to Ubidots */
  client.add(VARIABLE_LABEL, value);
  client.sendAll();
  delay(5000);
}
