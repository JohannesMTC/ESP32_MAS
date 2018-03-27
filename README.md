# ESP32_MAS ESP32 Model Audio System [![Build Status](https://travis-ci.org/JohannesMTC/ESP32_MAS.svg?branch=master)](https://travis-ci.org/JohannesMTC/ESP32_MAS)

## 3 Cannel audio player for the ESP32. 
This Arduino library allows you to play, sequenz and loop sound- files through a DAC or on chip DAC using Espressif's ESP32. The sound system supports 3 channels mono which can be controlled separately in the volume and pitch. The sound output is realized via Core 0. **Please do not start any further tasks on Core 0!** *Because this is completely exhausted with the special edition.* There is no "vTaskDelay"
within this function. The task is accessed via pointers without exception.

The class controlling methods run on Core 1. Thus, calls to the class variables via pointers are also directly possible.
  
*This library is optimized for use in model and robotic construction. If you are looking for optimal sound quality or want to play MP3 or WAVE files and do without an exact loop function, please use the "esp8266audio"library from Earle F. Philhower!*
https://github.com/earlephilhower/ESP8266Audio
 
## The files to be played must be:
* aiff formated,
* stored in the SPIFF,
* filename max 16 char
* PCM signed 8 bit,
* 22050 sample / sec,
  
## Use of the SPIFF:
Currently, the "ESP32 data upload tool" has an error. Your sketch will not have access to the files after the upload. To fix this bug, please load the example sketch "SPIFF_Test" on your ESP32 and reset the ESP32 twice! Then your sketch can easily access the files. Please note that the file name must always be complete with the path name and the file extension!
````
Example: "/testfile.aiff"
````
## Include the following libraries:  

* "esp_task.h"
* "SPIFFS.h"
* "ESP32_MAS.h"

## In the head of your sketch:

**"ESP32_MAS()"**  Initiaise a instance of the sound system.

## In the "setup" function of your sketch:  
*Methods may only be executed before the method "ESP32_MAS.startDAC()".*

**"ESP32_MAS.setPort(uint8_t * port)"** Changes the defauld port of the IS2 output.

port = Port 0 or 1 for the ESP_MAS  Defauld = 0

**"ESP32_MAS.setOut(uint8_t bck, uint8_t ws, uint8_t data)"**  Changes the defauld pin configuration of the IS2 output.

Defauld Pin assignment of the DAC:  BCK = 26,  WS = 25,  DATA = 22

**"ESP32_MAS.setDAC(bool dac)"**  Switches on the internal DAC of the ESP32 to realize the sound output by means of an audio amplifier.
dac = true sets the variable "I2S_noDAC" = true

Defauld assignment of the DAC:  I2S_noDAC = false   Allows output via an external IS2 DAC such as the "Adafruit I2S 3W Class D Amplifier".
  
In the "setup" function of your sketch:
---------------------------------------------------------------------------------------------
(Subsequent changes to the port, pin or DAC functions are no longer taken into account.)

**"ESP32_MAS.startDAC()"** Starts the IS2 output with the predefined or defauld configuration.
  
In any function:
---------------------------------------------------------------------------------------------
(Methods can be called any number of times.)
  
**"ESP32_MAS.setVolume(uint8_t volume)"** Sets the master volume of the output.

volume = Master volume of the DAC. 0-255, 0 = mute, 255 = 0dB  Defauld assignment:  Volume = 255
  
**"ESP32_MAS.playFile(uint8_t channel, String filname)"**  Send a new file to the sound system for playback.

channel = channel to play the file. (0 - 2)  filename = full path of the file to be played
The output starts immediately (delay approx. 2 ms) and stops at the end of the file. If the channel is running a file, this file will be attached to the active file.
  
**"ESP32_MAS.loopFile(uint8_t channel, String filname)"**  Loads a file into the loop buffer and repeats it continuously.

channel = channel to play the file. (0 - 2)  filename = full path of the file to be played
If the channel is running a file, this file will be attached to the active file.
  
**"ESP32_MAS.outChan(uint8_t channel)"** Lets the looped channel run to end of file and stoped file output.

channel = loop channel that should leak. (0 - 2)

**"ESP32_MAS.setGain(uint8_t channel, uint8_t gain)"**  Sets the volume of the respective channel.

channel = channel whose volume is to be changed. (0 - 2)  gain = desired volume (0 = mute, 255 = 1dB)
  
**"ESP32_MAS.setPitch(uint8_t channel, float pitch)"**

channel = channel whose playback speed is to be changed. (0 - 2)
pitch = desired acceleration of the channel. (0 = 0, 1 = doubble speed)
  
**"ESP32_MAS.stopCan(uint8_t channel)"**  Stops the output of the channel immediately.
channel = channel to be stopped. (0 - 2)
  
**"String ESP32_MAS.getCan(uint8_t channel)"**  Queries the state of the respective channel.
  channel = channel whose state is to be queried. (0 - 2)
  Return:
  PLAY = File ready to play and goto out.
  LOOP File ready to loop and goto run.
  RUN = File be looped.
  OUT = File is being ripped off.
  STOP = No output on this channel.
  BRAKE = Channel stoped file uotput and wait for run or out.
  
**"uint8_t ESP32_MAS.getGain(uint8_t channel)"**
  
  channel = channel whose state is to be queried. (0 - 2)
  Return:  Gain of the queried channel.
  
**"float ESP32_MAS.getPitch(uint8_t channel)**
  
  channel = channel whose state is to be queried. (0 - 2)
  Return:  Pitch of the queried channel.
