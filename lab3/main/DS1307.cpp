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

uint8_t uint8ToBCD(uint8_t uint8)
{
    return ((uint8 / 10) << 4) | (uint8 % 10);
}

uint8_t bcdToUint8(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0f);
}

char *timeToString(struct tm tm)
{
    char *str = (char *)malloc(72);
    sprintf(str, "%02d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    return str;
}

time_t timeToUnix(struct tm tm, int utc_offset)
{
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

int DS1307::getRegister(int reg)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (this->address << 1) | I2C_MASTER_WRITE, I2C_MASTER_NACK);
    i2c_master_write_byte(cmd, reg, 1);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (this->address << 1) | I2C_MASTER_READ, I2C_MASTER_NACK);
    uint8_t *data = (uint8_t *)malloc(1);
    i2c_master_read_byte(cmd, data, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return bcdToUint8(*data);
}

struct tm DS1307::getTime()
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (this->address << 1) | I2C_MASTER_WRITE, I2C_MASTER_NACK);
    i2c_master_write_byte(cmd, 0x0, 1);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (this->address << 1) | I2C_MASTER_READ, I2C_MASTER_NACK);
    uint8_t data[7];
    i2c_master_read(cmd, data, 7, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    struct tm tm;
    tm.tm_sec = bcdToUint8(data[0]);
    tm.tm_min = bcdToUint8(data[1]);
    tm.tm_hour = bcdToUint8(data[2]);
    tm.tm_mday = bcdToUint8(data[4]);
    tm.tm_mon = bcdToUint8(data[5]) - 1; // 0-11
    tm.tm_year = bcdToUint8(data[6]) + 100; // since 1900
    return tm;
}

void DS1307::setRegister(int reg, int value)
{
    ESP_LOGI(this->LogName, "Register %d to: %d", reg, value);

    esp_err_t errRc;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (this->address << 1) | I2C_MASTER_WRITE, I2C_MASTER_NACK);
    i2c_master_write_byte(cmd, reg, 1);
    i2c_master_write_byte(cmd, uint8ToBCD(value), 1);
    i2c_master_stop(cmd);
    errRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    if (errRc != 0)
    {
        ESP_LOGE(this->LogName, "i2c_master_cmd_begin: %d", errRc);
    }
    i2c_cmd_link_delete(cmd);
}

void DS1307::setTime(struct tm tm)
{
    ESP_LOGI(this->LogName, "Set time");

    esp_err_t errRc;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (this->address << 1) | I2C_MASTER_WRITE, I2C_MASTER_NACK);
    i2c_master_write_byte(cmd, 0x0, 1);
    i2c_master_write_byte(cmd, uint8ToBCD(tm.tm_sec), 1);
    i2c_master_write_byte(cmd, uint8ToBCD(tm.tm_min), 1);
    i2c_master_write_byte(cmd, uint8ToBCD(tm.tm_hour), 1);
    i2c_master_write_byte(cmd, uint8ToBCD(tm.tm_wday) + 1, 1); // 1-7 since Sunday
    i2c_master_write_byte(cmd, uint8ToBCD(tm.tm_mday), 1);
    i2c_master_write_byte(cmd, uint8ToBCD(tm.tm_mon + 1), 1); // 1-12
    i2c_master_write_byte(cmd, uint8ToBCD(tm.tm_year - 100), 1); // since 2000
    i2c_master_stop(cmd);
    errRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    if (errRc != 0)
    {
        ESP_LOGE(this->LogName, "i2c_master_cmd_begin: %d", errRc);
    }
    i2c_cmd_link_delete(cmd);
}
