// Wokwi Custom Chip - For information and examples see:
// https://docs.wokwi.com/chips-api/getting-started
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2022 Uri Shaked / wokwi.com

#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

#define INTERCAL_TICK 1
#define SENS_TEMP 35
#define SENS_HUM  84

enum STATE{IDLE, START_INIT, SEND_DATA_INIT, SEND_DATA_TEMP, SEND_DATA_IDLE, SEND_DATA_HUM};

typedef struct {
  pin_t pin_data_in;  
  pin_t pin_data_out;
  int state;
  uint64_t start_time; 
  uint64_t send_start_time;
  uint64_t send_start_time_hum;
  uint32_t attr_temp;
  uint32_t attr_hum;
} chip_state_t;


void chip_timer_callback(void *user_data) {
  /* Called when the timer fires */
  chip_state_t *data = (chip_state_t*)user_data;

  //On start wait 10ms
  if (data->state == SEND_DATA_INIT){
      if((get_sim_nanos() - data->send_start_time) > 10000){
          data->state = SEND_DATA_TEMP;
          printf("GoTo: SEND_DATA_TEMP\n");
          // Set pin 1
          pin_write(data->pin_data_out, HIGH);
          data->send_start_time = get_sim_nanos();
      }
  }else if (data->state == SEND_DATA_TEMP){
    const uint32_t attr_temp = attr_read(data->attr_temp);
    if((get_sim_nanos() - data->send_start_time) > ((10 + attr_temp *10)*(1000))){
          data->state = SEND_DATA_IDLE;
          printf("GoTo: SEND_DATA_IDLE\n");
          // Set pin O
          pin_write(data->pin_data_out, LOW);
          data->send_start_time = get_sim_nanos();
    }
  }else if (data->state == SEND_DATA_IDLE){
    if((get_sim_nanos() - data->send_start_time) > 10000){
          data->state = SEND_DATA_HUM;
          printf("GoTo: SEND_DATA_HUM\n");
          pin_write(data->pin_data_out, HIGH);
          data->send_start_time = get_sim_nanos(); 
    }
  }else if (data->state == SEND_DATA_HUM){
    const uint32_t attr_hum = attr_read(data->attr_hum);
    if((get_sim_nanos() - data->send_start_time) > ((15 + attr_hum *10)*(1000))){
          data->state = IDLE;
          printf("GoTo: IDLE\n");
          // Set pin O
          pin_write(data->pin_data_out, LOW);
    }  
  }
}


static void chip_pin_change(void *user_data, pin_t pin, uint32_t value) {
    chip_state_t *data = (chip_state_t*)user_data;
  
    if(data->state==IDLE){
        printf("GoTo START_INIT\n");
        data->start_time = get_sim_nanos();
        data->state = START_INIT;
    }else if(data->state==START_INIT){
        uint64_t current_time = get_sim_nanos(); 
        if((current_time - data->start_time) > 1000){
          printf("GoTo: SEND_DATA_INIT\n");
          data->state=SEND_DATA_INIT;
          data->send_start_time = get_sim_nanos();
          //Init timer
          const timer_config_t config = {
            .callback = chip_timer_callback,
            .user_data = user_data,
          };
          timer_t timer_id = timer_init(&config);
          timer_start(timer_id, INTERCAL_TICK, true);          

        }else{
          printf("Init signal to short, must be 1ms!!!\n");
        }
    }
}

//Init chip
void chip_init() {  
  printf("Init CHIP\n");
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->pin_data_in = pin_init("DATA_IN", INPUT);  
  chip->pin_data_out = pin_init("DATA_OUT", OUTPUT_LOW);
  chip->state = IDLE;

  const pin_watch_config_t watch_config = {
    .edge = BOTH,
    .pin_change = chip_pin_change,
    .user_data = chip,
  };
  pin_watch(chip->pin_data_in, &watch_config);  

  chip->attr_temp = attr_init("temp", 23);
  chip->attr_hum = attr_init("hum", 90);
  
}
