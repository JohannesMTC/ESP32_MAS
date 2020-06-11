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

#include <Arduino.h>
#include <FS.h>
#include "driver/i2s.h"
#include "esp_task.h"
#include "ESP32_MAS.h"
#include "SPIFFS.h"

#define CHANNELS 3

bool Audio_Player_run=false;

void Audio_Player(void *ptr) {
  Serial.println("Task Audio Player gestartet");

  unsigned int ptr_array[18];
  String *Audio_File[CHANNELS];
  uint8_t *I2S_PORT; // PORT NUM
  uint8_t *I2S_BCK; // BCK
  uint8_t *I2S_WS; // WS
  uint8_t *I2S_DATA; // DATA
  bool *I2S_noDAC; // noDAC
  uint8_t *Volume; // 0-255, 0 = mute, 255 = 0dB
  uint8_t *Channel[CHANNELS]; // 0 = STOP, 1 = BRAKE, 2 = PLAY, 3 = LOOP, 4 = RUN, 5 = OUT
  uint8_t *Gain[CHANNELS]; // 0-255, 0 = mute, 255 = 0dB
  float *Pitch[CHANNELS]; // 0 - 1, 0 = normal speed, 1 = double speed

  unsigned int is = sizeof ptr;
  int ic = 3;
  int ip = 0;

  for (int i = 0; i < 18; i++) {
    ptr_array[i] = *(unsigned int*)ptr;
    ptr += is;
  }
  for (int i = 0; i < ic; i++) {
    Audio_File[i] = (String*)ptr_array[ip];
    ip++;
  }
  I2S_PORT = (uint8_t*)ptr_array[ip];  ip++;
  I2S_BCK = (uint8_t*)ptr_array[ip];  ip++;
  I2S_WS = (uint8_t*)ptr_array[ip];  ip++;
  I2S_DATA = (uint8_t*)ptr_array[ip];  ip++;
  I2S_noDAC = (bool*)ptr_array[ip];  ip++;
  Volume = (uint8_t*)ptr_array[ip];  ip++;
  for (int i = 0; i < ic; i++) {
    Channel[i] = (uint8_t*)ptr_array[ip];
    ip++;
  }
  for (int i = 0; i < ic; i++) {
    Gain[i] = (uint8_t*)ptr_array[ip];
    ip++;
  }
  for (int i = 0; i < ic; i++) {
    Pitch[i] = (float*)ptr_array[ip];
    ip++;
  }
  int file_buf_len;
  int8_t out_buf_8[1024];         // higher buffer removes crackins when opening a new file, 1024 = max size
  int buf_len_8 = sizeof(out_buf_8);
  int16_t out_buf_16[sizeof(out_buf_8) / 2];
  int buf_len_16 = sizeof(out_buf_8) / 2;
  int8_t file_buf[3][sizeof(out_buf_8) / 2];

  float pitch[CHANNELS] = {0, 0, 0};
  float pitch_loc;
  bool end_file = false;
  File aiff_file[3];
  //-------------------------------------------------------------------------I2S-configuration
  const int I2S_PORT_NUM = (const int) * I2S_PORT;
  //--------------------------------------------------------------------------I2S-interlal DAC
  i2s_config_t i2s_config_noDAC = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
    .sample_rate = 22050,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT, // if you need both channels don't forget to update set_dac_mode as well
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_PCM),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = sizeof(out_buf_8),
    .use_apll  =  false,        // use_apll doesn't work with internal DAC + samplerate > 20kHz
    .tx_desc_auto_clear = true, // mute out if no data
    .fixed_mclk = 0 // calculate by the driver
  };
  //----------------------------------------------------------------------------I2S-extern DAC
  i2s_config_t i2s_config_DAC = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 22050,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll  =  true,
    .tx_desc_auto_clear = true, // mute out if no data
    .fixed_mclk = 0 // calculate by the driver
  };
  //-----------------------------------------------------------------------------I2S-pin-config
  i2s_pin_config_t pin_config = {
    .bck_io_num = (int) * I2S_BCK,
    .ws_io_num = (int) * I2S_WS,
    .data_out_num = (int) * I2S_DATA,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  //-----------------------------------------------------------------------------------open I2S
  if (*I2S_noDAC) {
    if(i2s_driver_install((i2s_port_t)I2S_PORT_NUM, &i2s_config_noDAC, 0, NULL) != ESP_OK) {
      Serial.println("i2s_driver_install error");
    }
    i2s_set_pin((i2s_port_t)I2S_PORT_NUM, NULL);
    i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);
  }
  else {
    if(i2s_driver_install((i2s_port_t)I2S_PORT_NUM, &i2s_config_DAC, 0, NULL) != ESP_OK) {
      Serial.println("i2s_driver_install error");
    }
  }
  i2s_set_pin((i2s_port_t)I2S_PORT_NUM, &pin_config);
  i2s_zero_dma_buffer((i2s_port_t)I2S_PORT_NUM);
  Serial.print("RUN I2S ON PORT_NUM: ");
  Serial.println(I2S_PORT_NUM);

  while (Audio_Player_run) {
    //------------------------------------------------------------------------AUDIO PLAYER LOOP
    for (int h = 0; h < ic; h++) {
      //--------------------------------------------------------------------------read channels
      if (*Channel[h] > 1) {
        pitch_loc = *Pitch[h];
        //---------------------------------------------------------------------------------play
        if (!aiff_file[h].available()) {
          //--------------------------------------------------------------------------open file
          aiff_file[h] = SPIFFS.open(*Audio_File[h], "r");
          switch (*Channel[h]) {
            case 2:
              *Channel[h] = 5;
              break;
            case 3:
              *Channel[h] = 4;
              break;
            case 4:
              *Channel[h] = 4;
              break;
          }
        }//                                                                           open file
        //------------------------------------------------------------------read file to buffer
        file_buf_len = aiff_file[h].available();
        file_buf_len =  file_buf_len - (file_buf_len * pitch_loc) / 2;
        if (file_buf_len > buf_len_16) {
          file_buf_len = buf_len_16;
          end_file = false;
        }
        else {
          end_file = true;
        }
        for (int i = 0; i < file_buf_len; i++) {
          file_buf[h][i] = aiff_file[h].read();
          pitch[h] += pitch_loc;
          if (pitch[h] >= 1) {
            file_buf[h][i] = aiff_file[h].read();
            pitch[h] -= 1;
          }
        }//                                                                 read file to buffer
        if (end_file) {
          //--------------------------------------------------------------------------file emty
          if (*Channel[h] < 5 && *Channel[h] > 1) {
            //--------------------------------------------------------------------open new file

            if(*Audio_File[h] != aiff_file[h].name()) {
              printf("open file [%s, %s]\n", Audio_File[h]->c_str(), aiff_file[h].name());
              aiff_file[h] = SPIFFS.open(*Audio_File[h], "r");
            } else {
              aiff_file[h].seek(0); // seek0 is nicht viel schneller als neues open !!!!
            }

            switch (*Channel[h]) {
              case 2:
                *Channel[h] = 5;
                break;
              case 3:
                *Channel[h] = 4;
                break;
              case 4:
                *Channel[h] = 4;
                break;
            }
            //-------------------------------------------------------read new file to to buffer
            for (int i = file_buf_len; i  < buf_len_16; i++) {
              file_buf[h][i] = aiff_file[h].read();
              pitch[h] += pitch_loc;
              if (pitch[h] >= 1) {
                file_buf[h][i] = aiff_file[h].read();
                pitch[h] -= 1;
              }
            }
          }//                                                        read new file to to buffer
          else {
            //-----------------------------------------------------------cleare rest of channel
            aiff_file[h].close();
            *Channel[h] = 0;
            for (int i = file_buf_len; i < buf_len_16; i++) {
              file_buf[h][i] = 0;
            }//for
          }//                                                        cleare rest of the channel
        }//                                                                           file emty
      }//                                                                                  play
      else {
        //---------------------------------------------------------------------------------stop
        for (int i = 0; i < buf_len_16; i++) {
          //----------------------------------------------------------------write clear channel
          file_buf[h][i] = 0;
        }//                                                                 write clear channel
      }//                                                                                  stop
    }//read channels
    //------------------------------------------------------------------------------------MIXER
    for (int i = 0; i < buf_len_16; i++) {
      out_buf_16[i] = (file_buf[0][i] * (*Gain[0]) +
                       file_buf[1][i] * (*Gain[1]) +
                       file_buf[2][i] * (*Gain[2]))
                      * float(*Volume / 255);
    }//                                                                                   MIXER
    if (*I2S_noDAC) {
      for (int i = 0; i < buf_len_16; i ++) {
        int16_t x = (out_buf_16[i] + 0x8000); // convert 16bit signed to 8bit unsigned
        //out_buf_8[i] = lowByte(x);
        out_buf_8[i*2] = 0;
        uint8_t val=highByte(x);
        out_buf_8[i*2 + 1] = val;
    } else {
      for (int i = 0; i < buf_len_8; i = i + 2) {
        //-----------------------------------------------------------------------write IS2 buffer
        out_buf_8[i] = lowByte(out_buf_16[i / 2]);
        out_buf_8[i + 1] = highByte(out_buf_16[i / 2]);
      }
    }//                                                                        write IS2 buffer
    int ret=i2s_write_bytes((i2s_port_t)I2S_PORT_NUM, (const char *)&out_buf_8, buf_len_8, 500);
    if(ret != buf_len_8) {
      printf("Error: bytes_written: %d to %d, len=%d\n", ret, I2S_PORT_NUM, buf_len_8);
    }
    vTaskDelay(10);
  }//                                                                         AUDIO PLAYER LOOP
  Serial.println("Audio_Player stopped");
  i2s_driver_uninstall((i2s_port_t)I2S_NUM_0); //stop & destroy i2s driver
  vTaskDelete(NULL);
}//                                                                           VOID AUDIO PLAYER

ESP32_MAS::ESP32_MAS() {
  int iz = 0;
  int ic = 3;
  for (int i = 0; i < ic; i++) {
    ptr_array[iz] = &Audio_File[i];
    iz++;
  }
  ptr_array[iz] = &I2S_PORT; iz++;
  ptr_array[iz] = &I2S_BCK; iz++;
  ptr_array[iz] = &I2S_WS; iz++;
  ptr_array[iz] = &I2S_DATA; iz++;
  ptr_array[iz] = &I2S_noDAC; iz++;
  ptr_array[iz] = &Volume;  iz++;
  for (int i = 0; i < ic; i++) {
    ptr_array[iz] = &Channel[i];
    iz++;
  }
  for (int i = 0; i < ic; i++) {
    ptr_array[iz] = &Gain[i];
    iz++;
  }
  for (int i = 0; i < ic; i++) {
    ptr_array[iz] = &Pitch[i];
    iz++;
  }
};
void ESP32_MAS::setPort(uint8_t port) {
  I2S_PORT = port;
};
void ESP32_MAS::setOut(uint8_t bck, uint8_t ws, uint8_t data) {
  I2S_BCK = bck;
  I2S_WS = ws;
  I2S_DATA = data;
};
void ESP32_MAS::setDAC(bool dac) {
  I2S_noDAC = dac;
};
void ESP32_MAS::startDAC() {
  if(Audio_Player_run) {
    Serial.println("already running");
  } else {
    Audio_Player_run=true;
    // pinning is *not* necessary
    // xTaskCreatePinnedToCore(Audio_Player, "Audio_Player", 10000, (void*)&ptr_array, 1, NULL, 0);
    // xTaskCreatePinnedToCore(Audio_Player, "Audio_Player", 10000, (void*)&ptr_array, tskIDLE_PRIORITY, NULL, 0);
    xTaskCreate(Audio_Player, "Audio_Player", 15000, (void*)&ptr_array, 5, NULL);
    Serial.println("Pinned AUDIO PLAYER to core 0");
  }
};

void ESP32_MAS::stopDAC() {
  Audio_Player_run=false;
}

void ESP32_MAS::setVolume(uint8_t volume) {
  Volume = volume;
};

void ESP32_MAS::stopChan(uint8_t channel) {
  assert(channel < CHANNELS);
  Channel[channel] = 0;
};

void ESP32_MAS::brakeChan(uint8_t channel) {
  assert(channel < CHANNELS);
  Channel[channel] = 1;
};

void ESP32_MAS::playFile(uint8_t channel, String audio_file) {
  assert(channel < CHANNELS);
  Audio_File[channel] = audio_file;
  Channel[channel] = 2;
};

void ESP32_MAS::loopFile(uint8_t channel, String audio_file) {
  assert(channel < CHANNELS);
  Audio_File[channel] = audio_file;
  Channel[channel] = 3;
};

void ESP32_MAS::runChan(uint8_t channel) {
  assert(channel < CHANNELS);
  Channel[channel] = 4;
};

void ESP32_MAS::outChan(uint8_t channel) {
  assert(channel < CHANNELS);
  Channel[channel] = 5;
};

void ESP32_MAS::setGain(uint8_t channel, uint8_t gain) {
  assert(channel < CHANNELS);
  Gain[channel] = gain;
};

void ESP32_MAS::setPitch(uint8_t channel, float pitch) {
  assert(channel < CHANNELS);
  if (pitch < 0) {
    pitch = 0;
  }
  if (pitch > 1) {
    pitch = 1;
  }
  Pitch[channel] = pitch;
};

String ESP32_MAS::getChan(uint8_t channel) {
  assert(channel < CHANNELS);
  String Return;
  switch (Channel[channel]) {
    case 0: //  0 = STOP, 1 = BRAKE, 2 = PLAY, 3 = LOOP, 4 = RUN, 5 = OUT
      Return = "STOP";
      break;
    case 1:
      Return = "BRAKE";
      break;
    case 2:
      Return = "PLAY";
      break;
    case 3:
      Return = "LOOP";
      break;
    case 4:
      Return = "RUN";
      break;
    case 5:
      Return = "OUT";
      break;
  }
  return Return;
};

uint8_t ESP32_MAS::getGain(uint8_t channel) {
  assert(channel < CHANNELS);
  return Gain[channel];
};

float ESP32_MAS::getPitch(uint8_t channel) {
  assert(channel < CHANNELS);
  return Pitch[channel];
};

#endif
