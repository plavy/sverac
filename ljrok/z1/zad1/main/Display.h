#ifndef _Display_h
#define _Display_h

class Display
{
public:
    Display(int dig_pins[2], int segment_pins[7]);

    void tick();
    void setNumber(int *integers);
    int *getNumber();

private:
    void select_and_set(int i);
    void unselect(int i);
    void set_digit(int integer);
    gpio_num_t m_dig_pins[2];
    gpio_num_t m_segment_pins[7];
    int m_number[2] = {0, 0};
    const char *LogName = "Display";
};

#endif