///
/// Inspiracija: https://github.com/nkolban/esp32-snippets/blob/master/hardware/rtc/ds1307.c
///

#include <driver/i2c.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/sockets.h>
#include <stdio.h>
#include <time.h>
#include "DS1307.hpp"

uint8_t intToBCD(uint8_t num)
{
    return ((num / 10) << 4) | (num % 10);
}

uint8_t bcdToInt(uint8_t bcd)
{
    // 0x10
    return ((bcd >> 4) * 10) + (bcd & 0x0f);
    ;
}

char *timeToString(struct tm tm) {
    char *str = (char *)malloc(72);
    sprintf(str, "%02d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    return str;
}

time_t timeToUnix(struct tm tm, int utc_offset) {
    tm.tm_hour -= utc_offset;
    time_t unix_time = mktime(&tm);
    return unix_time;
}

DS1307::DS1307(int sda_io, int scl_io, int address, int clk_speed)
{
    this->address = address;

    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = sda_io;
    conf.scl_io_num = scl_io;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = clk_speed;
    conf.clk_flags = 0;
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}

struct tm DS1307::getTime()
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (this->address << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
    i2c_master_write_byte(cmd, 0x0, 1);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (this->address << 1) | I2C_MASTER_READ, 1 /* expect ack */);
    uint8_t data[7];
    i2c_master_read(cmd, data, 7, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    int i;
    for (i = 0; i < 7; i++)
    {
        ESP_LOGD(this->LogName, "%d: 0x%.2x", i, data[i]);
    }

    struct tm tm;
    tm.tm_sec = bcdToInt(data[0]);
    tm.tm_min = bcdToInt(data[1]);
    tm.tm_hour = bcdToInt(data[2]);
    tm.tm_mday = bcdToInt(data[4]);
    tm.tm_mon = bcdToInt(data[5]) - 1;    // 0-11
    tm.tm_year = bcdToInt(data[6]) + 100; // since 1900
    return tm;
}

void DS1307::setRegister(int reg, int value)
{
    ESP_LOGD(this->LogName, "Register %d to: %d", reg, value);

    esp_err_t errRc;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (this->address << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
    i2c_master_write_byte(cmd, 0x0, 1);
    i2c_master_write_byte(cmd, intToBCD(value), 1);        // seconds
    i2c_master_stop(cmd);
    errRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    if (errRc != 0)
    {
        ESP_LOGD(this->LogName, "i2c_master_cmd_begin: %d", errRc);
    }
    i2c_cmd_link_delete(cmd);
}
