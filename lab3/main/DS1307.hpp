#ifndef _DS1307_h
#define _DS1307_h

uint8_t intToBCD(uint8_t num);

uint8_t bcdToInt(uint8_t bcd);

char *timeToString(struct tm tm);

time_t timeToUnix(struct tm tm, int utc_offset);

class DS1307{
    public:
        DS1307(int sda_io, int scl_io, int address, int clk_speed);
        void setTime(struct tm);
        struct tm getTime();
        void setRegister(int reg, int value);
        int getRegister(int reg);

    private:
        int address;
        const char *LogName = "DS1307";
};


#endif