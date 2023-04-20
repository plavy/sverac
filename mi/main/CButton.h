// CButton.h

#ifndef _CButton_h
#define _CButton_h

// microseconds
#define SHORT_CLICK_AT_MOST 300000L
 
// Pointer to event handling methods
extern "C" {
    typedef void (*ButtonEventHandler)(void);
}
// void my_singeClick_function(){}

class CButton{
    public:
        CButton(int port);
        void attachSingleClick(ButtonEventHandler method){singleClick = method;};
        void attachLongPress(ButtonEventHandler method){longPress = method;};

        void tick();

    private:
        ButtonEventHandler singleClick = NULL;
        ButtonEventHandler longPress = NULL;
        gpio_num_t m_pinNumber;
        const char *LogName = "CButton";
        int64_t  m_lastPress = 0;
        int64_t  m_lastRelease = 0;
        int m_lastState = 0;
};


#endif