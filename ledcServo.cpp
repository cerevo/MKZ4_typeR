#include "ledcServo.h"
#include <algorithm>

LEDCServo::LEDCServo(uint8_t ledcChannel, uint8_t ledcTimerBitNum):
    ledcChannel(ledcChannel),
    ledcTimerBitNum(ledcTimerBitNum),
    maxDuty((1LU << ledcTimerBitNum) - 1)
{
    //50Hz => T = 20ms
    ledcSetup(ledcChannel, 50, ledcTimerBitNum);
}

void LEDCServo::write(int angle) {
    angle = std::max(std::min(angle, 180), 0);
    float pulseMS = 1 + angle / 180.0;
    uint32_t duty = maxDuty * (pulseMS / 20);

    ledcWrite(ledcChannel, duty);
}

void LEDCServo::attach(uint8_t pin) {
    ledcAttachPin(pin, ledcChannel);
    this->pin = pin;
}

void LEDCServo::detach() {
    ledcDetachPin(pin);
}
