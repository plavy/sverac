// Wokwi Custom Chip - For information and examples see:
// https://link.wokwi.com/custom-chips-alpha
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2022 Uri Shaked / wokwi.com

#include "wokwi-api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const int ADDRESS = 0x30;

enum SefState{ENTER_PASSWD, LOCK, UNLOCK_PASSWD};

// digitCodeMap indicate which segments must be illuminated to display
// each number.
static const uint8_t digitCodeMap[] = {
  // GFEDCBA  Segments      7-segment map:
  0b00111111, // 0   "0"          AAA
  0b00000110, // 1   "1"         F   B
  0b01011011, // 2   "2"         F   B
  0b01001111, // 3   "3"          GGG
  0b01100110, // 4   "4"         E   C
  0b01101101, // 5   "5"         E   C
  0b01111101, // 6   "6"          DDD
  0b00000111, // 7   "7"
  0b01111111, // 8   "8"
  0b01101111, // 9   "9"
  0b01110111, // 65  'A'
  0b01111100, // 66  'b'
  0b00111001, // 67  'C'
  0b01011110, // 68  'd'
  0b01111001, // 69  'E'
  0b01110001, // 70  'F'
  0b00111101, // 71  'G'
  0b01110110, // 72  'H'
  0b00110000, // 73  'I'
  0b00001110, // 74  'J'
  0b01110110, // 75  'K'  Same as 'H'
  0b00111000, // 76  'L'
  0b00000000, // 77  'M'  NO DISPLAY
  0b01010100, // 78  'n'
  0b00111111, // 79  'O'
  0b01110011, // 80  'P'
  0b01100111, // 81  'q'
  0b01010000, // 82  'r'
  0b01101101, // 83  'S'
  0b01111000, // 84  't'
  0b00111110, // 85  'U'
  0b00111110, // 86  'V'  Same as 'U'
  0b00000000, // 87  'W'  NO DISPLAY
  0b01110110, // 88  'X'  Same as 'H'
  0b01101110, // 89  'y'
  0b01011011, // 90  'Z'  Same as '2'
  0b00000000, // 32  ' '  BLANK
  0b01000000, // 45  '-'  DASH
  0b10000000, // 46  '.'  PERIOD
  0b01100011, // 42 '*'  DEGREE ..
  0b00001000, // 95 '_'  UNDERSCORE
};


//Define Main data structure
typedef struct {
  pin_t dips_A;
  pin_t dips_B;
  pin_t dips_C;
  pin_t dips_D;
  pin_t dips_E;
  pin_t dips_F;
  pin_t dips_G;
  pin_t dips_DOT;
  pin_t dips_D1;
  pin_t dips_D2;
  pin_t dips_D3;
  pin_t dips_D4;

  pin_t led_R;
  pin_t led_G;
  pin_t led_B;

  char disp_buff[5];
  int disp_state;

  char rec_data;
  char rec_cmd;

  char lock_code[5];
  int lock_state;
  uint8_t lock_status;
  uint32_t threshold_attr;
} chip_state_t;

char decodeChar(char x){
    char ret;
    if (x>='0' && x<='9'){
      x -='0';
    }else if (x>='A' && x<='Z'){
      x = x - 'A' + 10;
    }else if (x>='a' && x<='z'){
      x = x - 'a' + 10;
    }else if (x==' '){
      x = 36;
    }else if (x=='-'){
      x = 37;
    }else if (x=='*'){
      x = 39;
    }else if (x=='_'){
      x = 40;
    }
    return digitCodeMap[x];
}

void display_digit(pin_t* pin_on, pin_t* pin_off, char buff, chip_state_t* data){
  //1)disable last digit
    pin_write(*pin_off, LOW);
  //2) set number
    char decode = decodeChar(buff);
    //decode = 0x01;
    decode = ~decode;
    //printf("%02X\n", decode);
    pin_write(data->dips_A, decode & 0x01);
    decode = decode >> 1;
    pin_write(data->dips_B, decode & 0x01);
    decode = decode >> 1;
    pin_write(data->dips_C, decode & 0x01);
    decode = decode >> 1;
    pin_write(data->dips_D, decode & 0x01);
    decode = decode >> 1;
    pin_write(data->dips_E, decode & 0x01);
    decode = decode >> 1;
    pin_write(data->dips_F, decode & 0x01);
    decode = decode >> 1;
    pin_write(data->dips_G, decode & 0x01);

  //3) turn on selected
    pin_write(*pin_on, HIGH);
}

enum DisplState{DD1, DD2, DD3, DD4};
//Timer Display function, runs everi 10ms
void chip_timer_callback(void *user_data) {
  /* Called when the timer fires */
  chip_state_t *data = (chip_state_t*)user_data;
  
  switch (data->disp_state) {
    case DD1:
      //printf("State DD1\n");
      display_digit(&data->dips_D1, &data->dips_D4, data->disp_buff[0], data);
      data->disp_state = DD2;
      break;
    case DD2:
      //printf("State DD2\n");
      display_digit(&data->dips_D2, &data->dips_D1, data->disp_buff[1], data);
      data->disp_state = DD3;
      break;
    case DD3:
      //printf("State DD3\n");
      display_digit(&data->dips_D3, &data->dips_D2, data->disp_buff[2], data);
      data->disp_state = DD4;
      break;
    default:
      //printf("State DD4\n");
      display_digit(&data->dips_D4, &data->dips_D3, data->disp_buff[3], data);
      data->disp_state = DD1;
      break;
  }

    
}

static bool on_i2c_connect(void *user_data, uint32_t address, bool connect);
static uint8_t on_i2c_read(void *user_data);
static bool on_i2c_write(void *user_data, uint8_t data);
static void on_i2c_disconnect(void *user_data);

void chip_init() {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  //Inti display state
  chip->disp_state = DD1;
  chip->lock_state = ENTER_PASSWD;

  chip->dips_A = pin_init("A", OUTPUT_HIGH);
  chip->dips_B = pin_init("B", OUTPUT_HIGH);
  chip->dips_C = pin_init("C", OUTPUT_HIGH);
  chip->dips_D = pin_init("D", OUTPUT_HIGH);
  chip->dips_E = pin_init("E", OUTPUT_HIGH);
  chip->dips_F = pin_init("F", OUTPUT_HIGH);
  chip->dips_G = pin_init("G", OUTPUT_HIGH);
  chip->dips_DOT = pin_init("DOT", OUTPUT_HIGH);
  chip->dips_D1 = pin_init("D1", OUTPUT_LOW);
  chip->dips_D2 = pin_init("D2", OUTPUT_LOW);
  chip->dips_D3 = pin_init("D3", OUTPUT_LOW);
  chip->dips_D4 = pin_init("D4", OUTPUT_LOW);

  chip->led_R = pin_init("LED_R", OUTPUT_HIGH);
  chip->led_G = pin_init("LED_G", OUTPUT_LOW);
  chip->led_B = pin_init("LED_B", OUTPUT_HIGH);
  

  chip->disp_buff[0] = '-';
  chip->disp_buff[1] = '-';
  chip->disp_buff[2] = '-';
  chip->disp_buff[3] = '-';
  chip->disp_buff[4] = 0;

  const i2c_config_t i2c_config = {
    .user_data = chip,
    .address = ADDRESS,
    .scl = pin_init("SCL", INPUT),
    .sda = pin_init("SDA", INPUT),
    .connect = on_i2c_connect,
    .read = on_i2c_read,
    .write = on_i2c_write,
    .disconnect = on_i2c_disconnect, // Optional
  };
  i2c_init(&i2c_config);

  // This attribute can be edited by the user. It's defined in wokwi-custom-part.json:
  chip->threshold_attr = attr_init("threshold", 127);

  const timer_config_t config = {
            .callback = chip_timer_callback,
            .user_data = chip,
          };
  timer_t timer_id = timer_init(&config);
  timer_start(timer_id, 10, true);
  // The following message will appear in the browser's DevTools console:
  printf("I2C Chip initialized!\n");
}


bool on_i2c_connect(void *user_data, uint32_t address, bool connect) {
  chip_state_t *chip = user_data;
  chip->rec_data = 0;
  chip->rec_cmd = 0;
  return true; /* Ack */
}

uint8_t on_i2c_read(void *user_data) {
  //read from senzor - return status register
  chip_state_t *chip = user_data;
  
  return chip->lock_status;
}

bool on_i2c_write(void *user_data, uint8_t data) {
  chip_state_t *chip = user_data;
  
  if(chip->rec_data == 0){
    //command
    printf("Received CMD [%02X]\n", data);
    chip->rec_cmd = data;
    chip->rec_data ++;

    if(chip->rec_cmd == 0x02){
      if(chip->lock_state == ENTER_PASSWD || chip->lock_state == UNLOCK_PASSWD){
        chip->disp_buff[0]=chip->disp_buff[1]=chip->disp_buff[2]=chip->disp_buff[3]='-';
        printf("--> CLEANING BUFFER !!! <--\n");
      }
    }else if(chip->rec_cmd == 0x03){
      if(chip->lock_state == ENTER_PASSWD){
        printf("--> LOCK !!! <--\n");
        strncpy(chip->lock_code, chip->disp_buff, 5);
        printf("D[%s] - L[%s]\n",chip->disp_buff, chip->lock_code);
        chip->lock_state = UNLOCK_PASSWD;
        chip->led_R = pin_init("LED_R", OUTPUT_LOW);
        chip->led_G = pin_init("LED_G", OUTPUT_HIGH);
        printf("--> ENTER PASSWORD TO UNLOCK !!! <--\n");        
      }else{
        printf("Command ignored!!!\n");
      }
    }else if(chip->rec_cmd == 0x04){
      if(chip->lock_state == UNLOCK_PASSWD){
        printf("D[%s] - L[%s]\n",chip->disp_buff, chip->lock_code);
        if( strncmp(chip->disp_buff, chip->lock_code, 5) == 0){
          printf("Password OK!!!\n");
          chip->lock_state = ENTER_PASSWD;
          chip->led_R = pin_init("LED_G", OUTPUT_LOW);
          chip->led_G = pin_init("LED_R", OUTPUT_HIGH);
          printf("--> ENTER NEW PASSWORD TO LOCK !!! <--\n");
        }else{
          printf("Wrong Password!!!\n");
          printf("--> ENTER PASSWORD TO UNLOCK !!! <--\n");
        }
        
      }
    }
    

  }else if(chip->rec_data == 1){
    printf("Received CMD [%02X] - DATA [%02X]\n", chip->rec_cmd, data);
    chip->rec_data++;
    
    //Execute command
    if(chip->rec_cmd == 0x01){
      if(chip->lock_state == ENTER_PASSWD || chip->lock_state == UNLOCK_PASSWD){
        chip->disp_buff[0] = chip->disp_buff[1];
        chip->disp_buff[1] = chip->disp_buff[2];
        chip->disp_buff[2] = chip->disp_buff[3];
        chip->disp_buff[3] = data;
      }
    }
  }else{
    printf("Ignore received data!\n");
  }

  return true; // Ack
}

void on_i2c_disconnect(void *user_data) {
  // Do nothing
  //printf("I2C Disconnected!!!\n");
}
