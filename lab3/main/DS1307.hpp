#ifndef _DS1307_h
#define _DS1307_h


uint8_t uint8ToBCD(uint8_t num);

uint8_t bcdToUint8(uint8_t bcd);

char *timeToString(struct tm tm);

time_t timeToUnix(struct tm tm, int utc_offset);

class DS1307{
    public:
        DS1307(int sda_io, int scl_io, int address, int clk_speed);
        int getRegister(int reg);
        struct tm getTime();
        void setRegister(int reg, int value);
        void setTime(struct tm);

    private:
        int address;
        const char *LogName = "DS1307";
};


#endif