// CButton.h

#ifndef _CButton_h
#define _CButton_h
 
// Pointer to event handling methods
extern "C" {
    typedef void (*ButtonEventHandler)(void);
}
// void my_singeClick_function(){}

class CButton{
    public:
        CButton(int port);
        void attachClick(ButtonEventHandler method){click = method;};

        void tick();

    private:
        ButtonEventHandler click = NULL;
        gpio_num_t m_pinNumber;
        const char *LogName = "CButton";
        int64_t  m_lastPress = 0;
        int64_t  m_lastRelease = 0;
        int m_lastState = 0;
        int click_counter = 0;
};


#endif