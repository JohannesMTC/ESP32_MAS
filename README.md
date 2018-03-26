# ESP8266Audio - now supporting ESP32 as well! [![Build Status](https://travis-ci.org/earlephilhower/ESP8266Audio.svg?branch=master)](https://travis-ci.org/earlephilhower/ESP8266Audio)
Arduino library for parsing and decoding MOD, WAV, MP3, FLAC, MIDI, and AAC files and playing them on an I2S DAC or even using a software-simulated delta-sigma DAC with dynamic 32x oversampling.

ESP8266 is fully supported and most mature, but we've also just introduced support for the ESP32.

## Disclaimer
All this code is released under the GPL, and all of it is to be used at your own risk.  If you find any bugs, please let me know via the GitHub issue tracker or drop me an email.  The MOD and MP3 routines were taken from StellaPlayer and libMAD respectively.  The software I2S delta-sigma 32x oversampling DAC was my own creation, and sounds quite good if I do say so myself.

The AAC decode code is from the Helix project and licensed under RealNetwork's RSPL license.  For commercial use you're still going to need the usual AAC licensing from Via Licensing (http://www.via-corp.com/us/en/licensing/aac/overview.html).

On the ESP32, AAC-SBR is supported (many webradio stations use this to reduce bandwidth even further).  The ESP8266, however, does not support it due to a lack of onboard RAM.

MIDI decoding comes from a highly ported MIDITONES (https://github.com/LenShustek/miditones) combined with a massively memory-optimized TinySoundFont (https://github.com/schellingb/TinySoundFont), see the respective source files for more information.

## Prerequisites and Installation
First, make sure you are running the 2.4 or GIT head version of the Arduino libraries for ESP8266, or the latest ESP32 SDK from Espressif.

You can use GIT to pull right from GitHub: see https://github.com/esp8266/Arduino/blob/master/README.md#using-git-version for detailed instructions.

Install the library and the SPI driver library in your ~/Arduino/libraries
````
mkdir -p ~/Arduino/libraries
cd ~/Arduino/libraries
git clone https://github.com/earlephilhower/ESP8266Audio
git clone https://github.com/Gianbacchio/ESP8266_Spiram
````

When in the IDE please select the following options on the ESP8266:
````
Tools->lwIP Variant->v1.4 Open Source, or V2 Prebuilt (MSS=1460)
Tools->CPU Frequency->160MHz
````

## Usage
Create an AudioInputXXX source pointing to your input file, an AudioOutputXXX sink as either an I2S, I2S-sw-DAC, or as a "SerialWAV" which simply writes a WAV file to the Serial port which can be dumped to a file on your development system, and an AudioGeneratorXXX to actually take that input and decode it and send to the output.

After creation, you need to call the AudioGeneratorXXX::loop() routine from inside your own main loop() one or more times.  This will automatically read as much of the file as needed and fill up the I2S buffers and immediately return.  Since this is not interrupt driven, if you have large delay()s in your code, you may end up with hiccups in playback.  Either break large delays into very small ones with calls to AudioGenerator::loop(), or reduce the sampling rate to require fewer samples per second.

## Example
See the examples directory for some simple examples, but the following snippet can play an MP3 file over the simulated I2S DAC:
````
#include <Arduino.h>
#include "AudioFileSourceSPIFFS.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"

AudioGeneratorMP3 *mp3;
AudioFileSourceSPIFFS *file;
AudioOutputI2SNoDAC *out;
void setup()
{
  Serial.begin(115200);
  delay(1000);
  SPIFFS.begin();
  file = new AudioFileSourceSPIFFS("/jamonit.mp3");
  out = new AudioOutputI2SNoDAC();
  mp3 = new AudioGeneratorMP3();
  mp3->begin(file, out);
}

void loop()
{
  if (mp3->isRunning()) {
    if (!mp3->loop()) mp3->stop(); 
  } else {
    Serial.printf("MP3 done\n");
    delay(1000);
  }
}
````

## AudioFileSource classes
AudioFileSource:  Base class which implements a very simple read-only "file" interface.  Required because it seems everyone has invented their own filesystem on the Arduino with their own unique twist.  Using this wrapper lets that be abstracted and makes the AudioGenerator simpler as it only calls these simple functions.

AudioFileSourceSPIFFS:  Reads a file from the SPIFFS filesystem

AudioFileSourcePROGMEM:  Reads a file from a PROGMEM array.  Under UNIX you can use "xxd -i file.mp3 > file.h" to get the basic format, then add "const" and "PROGMEM" to the generated array and include it in your sketch.  See the example .h files for a concrete example.

AudioFileSourceHTTPStream:  Simple implementation of a streaming HTTP reader for ShoutCast-type MP3 streaming.  Not yet resilient, and at 44.1khz 128bit stutters due to CPU limitations, but it works more or less.

## AudioFileSourceBuffer - Double buffering, useful for HTTP streams
AudioFileSourceBuffer is an input source that simpy adds an additional RAM buffer of the output of any other AudioFileSource.  This is particularly useful for web streaming where you need to have 1-2 packets in memory to ensure hiccup-free playback.

Simply create your standard input file source, create the buffer with the original source as its input, and pass this buffer object to the generator.
````
...
AudioGeneratorMP3 *mp3;
AudioFileSourceHTTPStream *file;
AudioFileSourceBuffer *buff;
AudioOutputI2SNoDAC *out;
...
  // Create the HTTP stream normally
  file = new AudioFileSourceHTTPStream("http://your.url.here/mp3");
  // Create a buffer using that stream
  buff = new AudioFileSourceBuffer(file, 2048);
  out = new AudioOutputI2SNoDAC();
  mp3 = new AudioGeneratorMP3();
  // Pass in the *buffer*, not the *http stream* to enable buffering
  mp3->begin(buff, out);
...

````

## AudioFileSourceID3 - ID3 stream parser filter with a user-speficied callback
This class, which takes as input any other AudioFileSource and outputs an AudioFileSource suitable for any decoder, automatically parses out ID3 tags from MP3 files.  You need to specify a callback function, which will be called as tags are decoded and allow you to update your UI state with this information.  See the PlayMP3FromSPIFFS example for more information.

## AudioGenerator classes
AudioGenerator:  Base class for all file decoders.  Takes a AudioFileSource and an AudioOutput object to get the data from and to write decoded samples to.  Call its loop() function as often as you can to ensure the buffers are always kept full and your music won't skip.

AudioGeneratorWAV:  Reads and plays Microsoft WAVE (.WAV) format files of 8 or 16 bits.

AudioGeneratorMOD:  Reads and plays Amiga ModTracker files (.MOD).  Use a 160MHz clock as this requires tons of SPIFFS reads (which are painfully slow) to get raw instrument sample data for every output sample.  See https://modarchive.org for many free MOD files.

AudioGeneratorMP3:  Reads and plays MP3 format files (.MP3) using a ported libMAD library.  Use a 160MHz clock to ensure enough compute power to decode 128KBit 44.1KHz without hiccups.  For complete porting history with the gory details, look at https://github.com/earlephilhower/libmad-8266

AudioGeneratorFLAC:  Plays FLAC files via ported libflac-1.3.2.  On the order of 30KB heap and minimal stack required as-is.

AudioGeneratorMIDI:  Plays a MIDI file using a wavetable synthesizer and a SoundFont2 wavetable input.  Theoretically up to 16 simultaneous notes available, but depending on the memory needed for the SF2 structures you may not be able to get that many before hitting OOM.

AudioGeneratorAAC:  Requires about 30KB of heap and plays a mono or stereo AAC file using the Helix fixed-point AAC decoder.

## AudioOutput classes
AudioOutput:  Base class for all output drivers.  Takes a sample at a time and returns true/false if there is buffer space for it.  If it returns false, it is the calling object's (AudioGenerator's) job to keep the data that didn't fit and try again later.

AudioOutputI2S: Interface for any I2S 16-bit DAC.  Sends stereo or mono signals out at whatever frequency set.  Tested with Adafruit's I2SDAC and a Beyond9032 DAC from eBay.  Tested up to 44.1KHz.

AudioOutputI2SNoDAC:  Abuses the I2S interface to play music without a DAC.  Turns it into a 32x (or higher) oversampling delta-sigma DAC.  Use the schematic below to drive a speaker or headphone from the I2STx pin (i.e. Rx).  Note that with this interface, depending on the transistor used, you may need to disconnect the Rx pin from the driver to perform serial uploads.  Mono-only output, of course.

AudioOutputSerialWAV:  Writes a binary WAV format with headers to the Serial port.  If you capture the serial output to a file you can play it back on your development system.

AudioOutputSPIFFSWAV:  Writes a binary WAV format with headers to a SPIFFS filesystem.  Ensure the FS is mounted and SPIFFS is started before calling.  USe the SetFilename() call to pick the output file before starting.

AudioOutputNull:  Just dumps samples to /dev/null.  Used for speed testing as it doesn't artificially limit the AudioGenerator output speed since there are no buffers to fill/drain.

## Software I2S Delta-Sigma DAC (i.e. playing music with a single transistor and speaker)
For the best fidelity, and stereo to boot, spend the money on a real I2S DAC.  Adafruit makes a great mono one with amplifier, and you can find stereo unamplified ones on eBay or elsewhere quite cheaply.  However, thanks to the software delta-sigma DAC with 32x oversampling (up to 128x if the audio rate is low enough) you can still have pretty good sound!

Use the AudioOutputI2S*No*DAC object instead of the AudioOutputI2S in your code, and the following schematic to drive a 2-3W speaker using a single $0.05 NPN 2N3904 transistor:

````
                            2N3904 (NPN)
                            +---------+
                            |         |     +-|
                            | E  B  C |    / S|
                            +-|--|--|-+    | P|
                              |  |  +------+ E|
                              |  |         | A|
ESP8266-GND ------------------+  |  +------+ K| 
                                 |  |      | E|
ESP8266-I2SOUT (Rx) -------------+  |      \ R|
                                    |       +-|
USB 5V -----------------------------+

You may also want to add a 220uF cap from USB5V to GND just to help filter out any voltage droop during high volume playback.
````
If you don't have a 5V source available on your ESP model, you can use the 5V from your USB serial adapter, or even the 3V from the ESP8266 (but it'll be lower volume).  Don't try and drive the speaker without the transistor, the ESP8266 pins can't give enough current to drive even a headphone well and you may end up damaging your device.

Connections are as a follows:
````
ESP8266-RX(I2S tx) -- 2N3904 Base
ESP8266-GND        -- 2N3904 Emitter
USB-5V             -- Speaker + Terminal
2N3904-Collector   -- Speaker - Terminal
````

Basically the transistor acts as a switch and requires only a drive of 1/beta (~1/1000 for the transistor specified) times the speaker current.  As shown you've got a max current of (5-0.7)/8=540mA and a power of 0.54^2 * 8 = ~2.3W into the speaker.

When using the software delta-sigma DAC, even though our playback circuit is not using the LRCLK or BCLK pins, the ESP8266 internal hardware *will* be driving them.  So these pins cannot be used as outputs in your application.  However, you can use the LRCLK and BCLK pins as *inputs*.  Simply start playback, then use the standard pinMode(xxx, INPUT/INPUT_PULLUP) Arduino commands and you can, for example, use those two pins to read a button or sensor.

## Using external SPI RAM to increase buffer
A class allows you to use a 23lc1024 SPI RAM from Microchip as input buffer. This chip connects to ESP8266 HSPI port and use external SPI RAM library (https://github.com/Gianbacchio/ESP8266_Spiram).
You need to choose another pin than GPIO15 for Cs as this pin is already used by the I2S port. Here is an example with the Cs pin plugged to GPIO00 on NodeMCU board.

![Example of SPIRAM Schematic](examples/StreamMP3FromHTTP_SPIRAM/Schema_Spiram.png)

## Notes for using SD cards and ESP8266Audio on Wemos shields
I've been told the Wemos SD card shield uses GPIO15 as the SD chip select.  This needs to be changed because GPIO15 == I2SBCLK, and is driven even if you're using the NoDAC option.  Once you move the CS to another pin and update your program it should work fine.

## Porting to other microcontrollers
There's no ESP8266-specific code in the AudioGenerator routines, so porting to other controllers should be relatively easy assuming they have the same endianness as the Xtensa core used.  Drop me a line if you're doing this, I may be able to help point you in the right direction.

## Thanks
Thanks to the authors of StellaPlayer and libMAD for releasing their code freely, and to the maintainers and contributors to the ESP8266 Arduino port.

Also, big thanks to @tueddy for getting the initial ESP32 porting into the tree!

-Earle F. Philhower, III
 earlephilhower@yahoo.com

