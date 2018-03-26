/*ESP32_MAS
  Copyright (C) 2018  Johannes Schreiner Otterthal AUSTRIA
  https://github.com/JohannesMTC/ESP32_MAS.git
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
  If not, see <http://www.gnu.org/licenses/>.*
  ---------------------------------------------------------------------------------------------
  ESP32 Model Audio System
  https://github.com/JohannesMTC/ESP32_MAS.git
  ---------------------------------------------------------------------------------------------
  3 CANNEL AUDIO PLAYER FOR THE ESP32.
  This library allows you to play and loop sounds through a DAC or on chip DAC
  using Espressif's ESP32.
  The sound system supports 3 channels mono which can be controlled separately
  in the volume an pitch.
  The sound output is realized via Core 0. Please do not start any further tasks on Core 0,
  because this is completely exhausted with the special edition! There is no "vTaskDelay"
  within this function. The task is accessed via pointers without exception.
  The class controlling methods run on Core 1.
  Thus, calls to the class variables via pointers are also directly possible.

  This library is optimized for use in model and robotic construction.
  If you are looking for optimal sound quality or want to play MP3 or WAVE files and do without
  an exact loop function, please use the "esp8266audio" library from Earle F. Philhower!
  https://github.com/earlephilhower/ESP8266Audio
  ---------------------------------------------------------------------------------------------
  The files to be played must be:
  aiff formated,
  PCM signed 8 bit,
  22050 sample / sec,
  stored in the SPIFF,
  Filename max 16 char
  ---------------------------------------------------------------------------------------------
  Use of the SPIFF:
  Currently, the data upload tool for the ESP32 still has an error.
  Your sketch will not have access to the files after the upload.
  To fix this bug, please load the example sketch "SPIFF_Test" on your ESP32
  and reset the ESP32 twice!
  Then your sketch can easily access the files.
  Please note that the file name must always be complete with the path name and
  the file extension!
  Example: "/testfile.aiff"
  ---------------------------------------------------------------------------------------------
  USING THE LIBRARY
  
  Include the following libraries:
  
  "esp_task.h"
  "SPIFFS.h"
  "ESP32_MAS.h"
  ---------------------------------------------------------------------------------------------
  In the head of your sketch:

  "ESP32_MAS()"
  Initiaise a instance of the sound system.
  ---------------------------------------------------------------------------------------------
  In the "setup" function of your sketch:
  (Methods may only be executed before the method "ESP32_MAS.startDAC()".)

  "ESP32_MAS.setPort(uint8_t * port)"
  Changes the defauld port of the IS2 output.
  port = Port 0 or 1 for the ESP_MAS
  Defauld = 0

  "ESP32_MAS.setOut(uint8_t bck, uint8_t ws, uint8_t data)"
  Changes the defauld pin configuration of the IS2 output.
  Defauld Pin assignment of the DAC:
  BCK = 26
  WS = 25
  DATA = 22

  "ESP32_MAS.setDAC(bool dac)"
  Switches on the internal DAC of the ESP32 to realize the sound output by means of an
  audio amplifier.
  dac = true sets the variable "I2S_noDAC" = true
  Defauld assignment of the DAC:
  I2S_noDAC = false
  Allows output via an external IS2 DAC such as the "Adafruit I2S 3W Class D Amplifier".
  ---------------------------------------------------------------------------------------------
  Method may only be executed once.
  (Subsequent changes to the port, pin or DAC functions are no longer taken into account.)

  "ESP32_MAS.startDAC()"
  Starts the IS2 output with the predefined or defauld configuration.
  ---------------------------------------------------------------------------------------------
  In any function:
  (Methods can be called any number of times.)

  "ESP32_MAS.setVolume(uint8_t volume)"
  Sets the master volume of the output.
  volume = Master volume of the DAC. 0-255, 0 = mute, 255 = 0dB
  Defauld assignment:
  Volume = 255

  "ESP32_MAS.playFile(uint8_t channel, String filname)"
  Send a new file to the sound system for playback.
  channel = channel to play the file. (0 - 2)
  filename = full path of the file to be played
  The output starts immediately (delay approx. 2 ms) and stops at the end of the file.
  If the channel is running a file, this file will be attached to the active file.

  "ESP32_MAS.loopFile(uint8_t channel, String filname)"
  Loads a file into the loop buffer and repeats it continuously.
  channel = channel to play the file. (0 - 2)
  filename = full path of the file to be played
  If the channel is running a file, this file will be attached to the active file.

  "ESP32_MAS.outChan(uint8_t channel)"
  Lets the looped channel run to end of file and stoped file output.
  channel = loop channel that should leak. (0 - 2)

  "ESP32_MAS.setGain(uint8_t channel, uint8_t gain)"
  Sets the volume of the respective channel.
  channel = channel whose volume is to be changed. (0 - 2)
  gain = desired volume (0 = mute, 255 = 1dB)

  "ESP32_MAS.setPitch(uint8_t channel, float pitch)"
  channel = channel whose playback speed is to be changed. (0 - 2)
  pitch = desired acceleration of the channel. (0 = 0, 1 = doubble speed)

  "ESP32_MAS.stopCan(uint8_t channel)"
  Stops the output of the channel immediately.
  channel = channel to be stopped. (0 - 2)

  "String ESP32_MAS.getCan(uint8_t channel)"
  Queries the state of the respective channel.
  channel = channel whose state is to be queried. (0 - 2)
  Return:
  PLAY = File ready to play and goto out.
  LOOP File ready to loop and goto run.
  RUN = File be looped.
  OUT = File is being ripped off.
  STOP = No output on this channel.
  BRAKE = Channel stoped file uotput and wait for run or out.

  "uint8_t ESP32_MAS.getGain(uint8_t channel)"
  channel = channel whose state is to be queried. (0 - 2)
  Return:
  Gain of the queried channel.

  "float ESP32_MAS.getPitch(uint8_t channel)"
  channel = channel whose state is to be queried. (0 - 2)
  Return:
  Pitch of the queried channel.
  -------------------------------------------------------------------------------------------*/
#ifndef _ESP32_MAS_
#define _ESP32_MAS_
#include <Arduino.h>
#include <FS.h>
#include "driver/i2s.h"
#include "esp_task.h"
#include "ESP32_MAS.h"

class ESP32_MAS {
  public:
    ESP32_MAS();
    void setPort(uint8_t port);
    void setOut(uint8_t bck, uint8_t ws, uint8_t data);
    void setDAC(bool dac);
    void startDAC();
    void setVolume(uint8_t volume);
    void stopChan(uint8_t channel);
    void playFile(uint8_t channel, String audio_file);
    void loopFile(uint8_t channel, String audio_file);
    void runChan(uint8_t channel);
    void brakeChan(uint8_t channel);
    void outChan(uint8_t channel);
    void setGain(uint8_t channel, uint8_t gain);
    void setPitch(uint8_t channel, float pitch);
    String getChan(uint8_t channel);
    uint8_t getGain(uint8_t channel);
    float getPitch(uint8_t channel);
  private:
    void *ptr_array[18];
    String Audio_File[3] = {"/xxxxxxxxxxxxxxxx.aiff",
                            "/xxxxxxxxxxxxxxxx.aiff",
                            "/xxxxxxxxxxxxxxxx.aiff"
                           };
    uint8_t I2S_PORT = 0; // PORT NUM
    uint8_t I2S_BCK = 26; // BCK
    uint8_t I2S_WS = 25; // WS
    uint8_t I2S_DATA = 22; // DATA
    bool I2S_noDAC = false; // noDAC
    uint8_t Volume = 255; // 0-255, 0 = mute, 255 = 0dB
    uint8_t Channel[3] = {0, 0, 0}; // 0 = STOP, 1 = BRAKE, 2 = PLAY, 3 = LOOP, 4 = RUN, 5 = OUT
    uint8_t Gain[3] = {128, 128, 128}; // 0-255, 0 = mute, 255 = 0dB
    float Pitch[3] = {0, 0, 0}; // 0 - 1, 0 = normal speed, 1 = double speed
};
#endif
