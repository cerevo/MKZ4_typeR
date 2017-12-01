#ifndef LEDCSERVO_HEADER
#define LEDCSERVO_HEADER

#include <Arduino.h>

//LEDCでサーボをコントロールする
//PWM周期 = 20ms
//PWMパルス長 = 1ms ~ 2ms

class LEDCServo {
public:
    LEDCServo(uint8_t ledcChannel, uint8_t ledcTimerBitNum);

    //angle [0:180]
    void write(int angle);

    void attach(uint8_t pin);
    void detach();

private:
    uint8_t pin;
    const uint8_t ledcChannel;
    const uint8_t ledcTimerBitNum;
    const uint32_t maxDuty;
};

#endif // LEDCSERVO_HEADER
