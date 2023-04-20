#ifndef _Display_h
#define _Display_h

class Display
{
public:
    Display(int dig_pins[4], int segment_pins[7], int dp, int cln);

    void tick();
    void setTime(int *integers);
    int *getTime();
    void setDot(int i);

private:
    void select_and_set(int i);
    void unselect(int i);
    void set_digit(int integer);
    void run();
    void edit();
    gpio_num_t m_dig_pins[4];
    gpio_num_t m_segment_pins[7];
    gpio_num_t m_dp;
    gpio_num_t m_cln;
    int m_dots[4] = {0, 0, 0, 0};
    int m_time[4] = {0, 0, 0, 0};
    bool m_colon_blink = true;
    int m_colon_last_change = 0;
    const char *LogName = "Display";
};

#endif