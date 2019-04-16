/*
Copyright (c) 2017, Ubidots.
Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:
The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
Original Maker: Mateo Velez - Metavix for Ubidots Inc
Modified and Maintened by: María Carlina Hernández ---- Developer at Ubidots Inc
                           https://github.com/mariacarlinahernandez
                           Jose Garcia ---- Developer at Ubidots Inc
                           https://github.com/jotathebest
*/

#include "UbidotsCC3200.h"
/**
 * Constructor.
 * Default deviceLabel is "cc3200"
 */
Ubidots::Ubidots(const char* token, const char* server) {
  _token = token;
  _server = server;
  _maxValues = 5;
  _currentValue = 0;
  _deviceLabel = DEFAULT_DEVICE_LABEL;
  val = (Value *)malloc(_maxValues*sizeof(Value));
}

/***************************************************************************
FUNCTIONS TO RETRIEVE DATA
***************************************************************************/
/**
 * This function is to get last value from the Ubidots API with the device label
 * and variable label
 * @arg device_label the label of the device
 * @arg variable_label the label of the variable
 * @return num the last value received in the variable, if any error occurs
    the function returns ERRORs messages
 */
float Ubidots::getValue(const char* device_label, const char* variable_label) {
  /* Assigns the constans as global on the function */
  bool flag = false;
  char* res = (char *) malloc(sizeof(char) * 250);
  char* value = (char *) malloc(sizeof(char) * 20);
  char* parsed = (char *) malloc(sizeof(char) * 20);
  float num;
  uint8_t index = 0;
  uint8_t l = 0;
  uint8_t timeout = 0;
  uint8_t max_retries = 0;

  /* Builds the request GET - Please reference this link to know all the request's structures https://ubidots.com/docs/api/ */
  char* data = (char *) malloc(sizeof(char) * 220);
  sprintf(data, "GET /api/v1.6/devices/%s/%s/lv", device_label, variable_label);
  sprintf(data, "%s HTTP/1.1\r\n", data);
  sprintf(data, "%sHost: things.ubidots.com\r\n", data);
  sprintf(data, "%sUser-Agent: %s/%s\r\n", data, USER_AGENT, VERSION);
  sprintf(data, "%sX-Auth-Token: %s\r\n", data, _token);
  sprintf(data, "%sConnection: close\r\n\r\n", data);

  /* Initial Connection */
  _client.connect(SERVER, PORT);
  /* Reconnect the client when is disconnected */
  while (!_client.connected()) {
    if (_debug) {
      Serial.println(F("Attemping to connect"));
    }

    if (_client.connect(SERVER, PORT)) {
      break;
    }

    max_retries++;
    if (max_retries > 5) {
      if (_debug) {
        Serial.println(F("Could not connect to server"));
      }
      return ERROR_VALUE;
    }
    delay(5000);
  }

  if (_debug) {
    Serial.println(F("Getting your variable with request: \r\n"));
    Serial.println(data);
  }

  /* Make the HTTP request to the server*/
  _client.print(data);
  free(data);

  /* Reach timeout when the server is unavailable */
  while (!_client.available() && timeout < 2000) {
    timeout++;
    delay(1);
    if (timeout >= 2000) {
      if (_debug) {
        Serial.println(F("Error, max timeout reached"));
      }
      _client.stop();
      return ERROR_VALUE;
    }
  }

  /* Reads the response from the server */
  while (_client.available()) {
    char c = _client.read();
    if (_debug) {
      Serial.write(c);
    }
    if (c == -1) {
      if (_debug) {
        Serial.println(F("Error reading data from server"));
      }
      _client.stop();
      return ERROR_VALUE;
    }
    res[l++] = c;
  }

  /* Extracts the string with the value */
  /* Expected result string : {length_of_request}\r\n{value}\r\n{length_of_answer} */
  int len = dataLen(res); // Length of the answer char array from the server

  for (int i = 0; i < len - 4; i++) {
    if ((res[i] == '\r') && (res[i+1] == '\n') && (res[i+2] == '\r') && (res[i+3] == '\n')) {
        strncpy(parsed, res+i+3, 20);  // Copies the result to the parsed
        parsed[20] = '\0';
        break;
    }
  }

  /* Extracts the the value */

  for (int k = 0; k < 20; k++){
    if ((parsed[k] == '\r') && (parsed[k+1] == '\n')) {
      while((parsed[k+2] != '\r')){
        value[index] = parsed[k+2];
        k++;
        index++;
      }
      break;
    }
  }
  /* Converts the value obtained to a float */
  num = atof(value);
  /* Free memory*/
  free(res);
  free(parsed);
  free(value);
  /* Removes any buffered incoming serial data */
  _client.flush();
  /* Disconnects the client */
  _client.stop();
  return num;
}

/***************************************************************************
FUNCTIONS TO SEND DATA
***************************************************************************/
/**
 * Add a value of variable to save
 * @arg variable_id variable id or name to save in a struct
 * @arg value variable value to save in a struct
 * @arg ctext [optional] is the context that you will save, default
 * @arg timestamp_val [optional] is the timestamp for the actual value
 * is NULL
 */
void Ubidots::add(const char * variable_label, float value) {
  return add(variable_label, value, '\0', '\0');
}

void Ubidots::add(const char * variable_label, float value, char* ctext) {
  return add(variable_label, value, ctext, '\0');
}

void Ubidots::add(const char * variable_label, float value, unsigned long timestamp_val) {
  return add(variable_label, value, '\0', timestamp_val);
}

void Ubidots::add(const char * variable_label, float value, char* ctext, unsigned long timestamp_val ) {
  (val+_currentValue)->varLabel = variable_label;
  (val+_currentValue)->varValue = value;
  (val+_currentValue)->context = ctext;
  (val+_currentValue)->timestamp_val = timestamp_val;
  _currentValue++;
  if (_currentValue>=_maxValues) {
    Serial.println(F("You are sending more than 5 consecutives variables, you just could send 5 variables. Then other variables will be deleted!"));
    _currentValue = _maxValues - 1;
  }
}

/**
 * Send all data of all variables that you saved
 * @return true upon success, false upon error.
 */
bool Ubidots::sendAll() {
  /* Assigns the constans as global on the function */
  char* str_val = (char *) malloc(sizeof(char) * 30);
  uint8_t max_retries = 0;
  uint8_t timeout = 0;
  uint8_t i = 0;
  #if !defined(UBI_WIN)
  float _value;
  #endif

  /* Verify the variables invoked */
  if (_currentValue == 0) {
    Serial.println("Invoke a variable to be send using the method \"add\"");
    return false;
  }

  /* Builds the JSON to be send */
  char* body = (char *) malloc(sizeof(char) * 150);
  sprintf(body, "{");
  for (i = 0; i < _currentValue;) {
    _value = (val+i)->varValue; // float variable value
    
    /* Saves variable value in str */
    //dtostrf(_value, 4, 3, str_val); // String variable value
    int _ivalue = (int)_value;
    sprintf(str_val, "%d.%d", _ivalue, (int)((_value -_ivalue)*1000)); // Precision of 3 decimal places
    
    sprintf(body, "%s\"%s\":", body, (val + i)->varLabel);
    if ((val + i)->context != '\0') {
      sprintf(body, "%s{\"value\":%s, \"context\":{%s}}", body, str_val, (val + i)->context );
    } else if ((val + i)->timestamp_val != '\0') {
      sprintf(body, "%s{\"value\":%s, \"timestamp\":%lu%s}", body, str_val, (val + i)->timestamp_val, "000");
    }  else {
      sprintf(body, "%s%s", body, str_val);
    }
    i++;
    if (i < _currentValue) {
      sprintf(body, "%s, ", body);
    }
  }
  _currentValue = 0;
  sprintf(body, "%s}", body);
  /* Builds the HTTP request to be POST */
  char* data = (char *) malloc(sizeof(char) * 300);
  sprintf(data, "POST /api/v1.6/devices/%s", _deviceLabel);
  sprintf(data, "%s HTTP/1.1\r\n", data);
  sprintf(data, "%sHost: things.ubidots.com\r\n", data);
  sprintf(data, "%sUser-Agent: %s/%s\r\n", data, USER_AGENT, VERSION);
  sprintf(data, "%sX-Auth-Token: %s\r\n", data, _token);
  sprintf(data, "%sConnection: close\r\n", data);
  sprintf(data, "%sContent-Type: application/json\r\n", data);
  sprintf(data, "%sContent-Length: %d\r\n\r\n", data, dataLen(body));
  sprintf(data, "%s%s\r\n\r\n", data, body);
  /* Initial connection */
  _client.connect(SERVER,PORT);
  /* Reconnect the client when is disconnected */
  while (!_client.connected()) {
    if (_debug) {
      Serial.println("Attemping to connect");
    }

    if (_client.connect(SERVER, PORT)) {
      break;
    }

    max_retries++;
    if (max_retries > 5) {
      if (_debug) {
        Serial.println("Could not connect to server");
      }
      free(data);
      return 0;
    }
    delay(5000);
  }

  if (_debug) {
    Serial.println(F("Posting your variable with request: \r\n"));
    Serial.println(data);
  }

  /* Make the HTTP request to the server*/
  _client.println(data);
  /* Reach timeout when the server is unavailable */
  while (!_client.available() && timeout < 5000) {
    timeout++;
    delay(1);
    if (timeout >= 5000) {
      if (_debug) {
        Serial.println(F("Error, max timeout reached"));
      }
      _client.stop();
      return 0;
    }
  }

  /* Reads the response from the server */
  while (_client.available()) {
    char c = _client.read();
    Serial.write(c);
    if (c == -1) {
      if (_debug) {
        Serial.println(F("Error reading data from server"));
      }
      _client.stop();
      free(data);
      return 0;
    }
  }
  /* free memory */
  free(str_val);
  free(data);
  free(body);
  /* Removes any buffered incoming serial data */
  _client.flush();
  /* Disconnects the client */
  _client.stop();
  return true;
}

/***************************************************************************
AUXILIAR FUNCTIONS
***************************************************************************/
/**
 * Assigns a new device label
 * @arg new_device_name new label that you want to assign to your device
 */
void Ubidots::setDeviceLabel(const char * new_device_label) {
  _deviceLabel = new_device_label;
}

/**
 * Gets the length of a variable
 * @arg variable a variable of type char
 * @return dataLen the length of the variable
 */
int Ubidots::dataLen(char* variable) {
  uint8_t dataLen = 0;
  for (int i = 0; i <= 250; i++) {
    if (variable[i] != '\0') {
      dataLen++;
    } else {
      break;
    }
  }
  return dataLen;
}

/**
 * Connects to the WiFi service
 * @arg ssid the ssid of the WiFi
 * @arg pass the pass of the WiFi
 */
void Ubidots::wifiConnection(char* ssid, char* pass) {
  /* Print the network name (SSID) */
  Serial.print("Attempting to connect to: ");
  Serial.println(ssid);
  /* Connect to WPA/WPA2 network. Change this line if using open or WEP network */
  WiFi.begin(ssid, pass);
  while ( WiFi.status() != WL_CONNECTED) {
    /* Print dots while we wait to connect */
    Serial.print(".");
    delay(300);
  }
  /* Prints the successful connection */
  Serial.println("\nConnected!");
  /* Wating for an IP adrress  */
  while (WiFi.localIP() == INADDR_NONE) {
    /* Prints dots while we wait for an ip addresss */
    Serial.print(".");
    delay(300);
  }
  /* Save the IP address assigned */
  IPAddress ip = WiFi.localIP();
  /* Prints your WiFi shield's IP address */
  Serial.print("\nIP Address: ");
  Serial.println(ip);
}

/**
 * Turns on or off debug messages
 * @debug is a bool flag to activate or deactivate messages
 */
void Ubidots::setDebug(bool debug) {
  _debug = debug;
}

/*
 * © Francesco Potortì 2013 - GPLv3 - Revision: 1.13
 *
 * Send an NTP packet and wait for the response, return the Unix time
 *
 * To lower the memory footprint, no buffers are allocated for sending
 * and receiving the NTP packets.  Four bytes of memory are allocated
 * for transmision, the rest is random garbage collected from the data
 * memory segment, and the received packet is read one byte at a time.
 * The Unix time is returned, that is, seconds from 1970-01-01T00:00.
 */
unsigned long Ubidots::ntpUnixTime () {
  /*  Open socket on arbitrary port */
  static int udpInited = udp.begin(123);

  /* Only the first four bytes of an outgoing NTP packet need
    to be set appropriately, the rest can be whatever. */
  const long ntpFirstFourBytes = 0xEC0600E3; // NTP request header

  /* Fail if WiFiUdp.begin() could not init a socket */
  if (! udpInited)
    return 0;

  /* Clear received data from possible stray received packets */
  udp.flush();

  /* Send an NTP request */
  if (! (udp.beginPacket(TIME_SERVER, 123) // 123 is the NTP port
    && udp.write((byte *)&ntpFirstFourBytes, 48) == 48
    && udp.endPacket()))
    return 0; // sending request failed

  /* Wait for response; check every pollIntv ms up to maxPoll times */
  const int pollIntv = 150; // poll every this many ms
  const byte maxPoll = 15; // poll up to this many times
  int pktLen; // received packet length
  for (byte i = 0; i < maxPoll; i++) {
    if ((pktLen = udp.parsePacket()) == 48)
      break;
    delay(pollIntv);
  }
  if (pktLen != 48)
    return 0; // no correct packet received

  /* Read and discard the first useless bytes
   Set useless to 32 for speed; set to 40 for accuracy */
  const byte useless = 40;
  for (byte i = 0; i < useless; ++i)
    udp.read();

  /* Read the integer part of sending time */
  unsigned long time = udp.read(); // NTP time
  for (byte i = 1; i < 4; i++)
    time = time << 8 | udp.read();

  /**
   * Round to the nearest second if we want accuracy
   * The fractionary part is the next byte divided by 256: if it is
   * greater than 500ms we round to the next second; we also account
   * for an assumed network delay of 50ms, and (0.5-0.05)*256=115;
   * additionally, we account for how much we delayed reading the packet
   * since its arrival, which we assume on average to be pollIntv/2.
   */
  time += (udp.read() > 115 - pollIntv/8);
  /* Discard the rest of the packet */
  udp.flush();
  return time - 2208988800ul; // convert NTP time to Unix time
}
