/*Play over serrial commands
  Example to use the ESP32_MAS libery.

  Model Audio System
  3 CANNEL AUDIO PLAYER AND SEQUENZER FOR THE ESP32.
  -----------------------------------------------------------------------------------------------
  This sketch shows the application of the ESP32_MAS.
  To do this, load the files of the "data" folder into the SPIFFS.
  If you use the "ESP32 sketch data upload",
  please open the sample SPIFFS test and upload it to your ESP32.
  Reset your ESP32 several times until the serial monitor stops displaying errors.
  After that, you can upload this example.
  Then you can trigger the different actions by entering numbers 1 - 9 in the serial monitor.
  The sound system supports 3 channels which can be controlled separately in the volume.

  The files to be played must be:
  aiff format,
  PCM signed 8 bit,
  22050 sample / sec
  stored in the SPIFF

  Pin assignment of the DAC:
  26 = BCK
  25 = LRCK
  22 = DATA
  ----------------------------------------------------------------------------------------------*/
#include "esp_task.h"
#include "SPIFFS.h"
#include "ESP32_MAS.h"

ESP32_MAS Audio;
bool up = true;
float pitch = 0;

void setup() {//---------------------------------------------------------------------------SETUP
  Serial.begin(115200);
  Serial.println("Serial run");
  SPIFFS.begin();
  delay(500);
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS Mount Failed");
  }
  delay(500);
  Audio.startDAC();
  Serial.println("DAC and Setup redy");
}// SETUP

void loop() {//-----------------------------------------------------------------------------LOOP

  uint8_t income = Serial.read();
  switch (income) {
    case 48:
      for (int i = 0; i < 3; i++) {
        String Channel;
        uint8_t Gain;
        float Pitch;
        Channel = Audio.getChan(i);
        Serial.print("Channel: ");
        Serial.print(i);
        Serial.print(" too: ");
        Serial.print(Channel);
        Gain = Audio.getGain(i);
        Serial.print(" Gain: ");
        Serial.print(Gain);
        Pitch = Audio.getPitch(i);
        Serial.print(" Pitch: ");
        Serial.println(Pitch);
      }
      break;
    case 49:
      Audio.setGain(1, 150);
      Audio.playFile(1, "/makrofon.aiff");
      Serial.println("Play /makrofon.aiff");
      break;
    case 50:
      Audio.setGain(0, 150);
      Audio.loopFile(0, "/E_engine0.aiff");
      Serial.println("Loop /E_engine0.aiff");
      break;
    case 51:
      Audio.setGain(0, 150);
      Audio.loopFile(0, "/E_engine1.aiff");
      Serial.println("Loop /E_engine1.aiff");
      break;
  }
  if (up) {
    pitch = pitch + 0.01;
  }
  else {
    pitch = pitch - 0.01;
  }
  if (pitch > 1) {
    up = false;
  }
  if (pitch < 0) {
    up = true;
  }
  Audio.setPitch(0, pitch);
  vTaskDelay(100);
}// LOOP
/*Copyright (C) 2018  Johannes Schreiner Otterthal AUSTRIA
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.
  If not, see <http://www.gnu.org/licenses/>.*/
