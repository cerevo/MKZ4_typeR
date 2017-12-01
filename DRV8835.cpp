#include <Arduino.h>
#include "DRV8835.h"
#include <algorithm>

DRV8835::DRV8835(
        double freq,
        uint8_t ledcChannel1, uint8_t ledcChannel2,
        uint8_t ledcTimerBitNum
        ):
    ledcChannel1(ledcChannel1), ledcChannel2(ledcChannel2),
    ledcTimerBitNum(ledcTimerBitNum),
    maxDuty((1LU << ledcTimerBitNum) - 1)
{
    ledcSetup(ledcChannel1, freq, ledcTimerBitNum);
    ledcSetup(ledcChannel2, freq, ledcTimerBitNum);
}

void DRV8835::drive(float rate) {
    uint32_t duty = (uint32_t)(maxDuty * std::abs(rate));
    if(rate > 0) {
        ledcWrite(ledcChannel2, 0);
        ledcWrite(ledcChannel1, duty);
    } else {
        ledcWrite(ledcChannel1, 0);
        ledcWrite(ledcChannel2, duty);
    }
}

void DRV8835::stop() {
    ledcWrite(ledcChannel1, maxDuty);
    ledcWrite(ledcChannel2, maxDuty);
}

void DRV8835::coast() {
    ledcWrite(ledcChannel1, 0);
    ledcWrite(ledcChannel2, 0);
}

void DRV8835::attach(uint8_t IN1Pin, uint8_t IN2Pin) {
    ledcAttachPin(IN1Pin, ledcChannel1);
    ledcAttachPin(IN2Pin, ledcChannel2);

    this->IN1Pin = IN1Pin;
    this->IN2Pin = IN2Pin;
}

void DRV8835::detach() {
    ledcDetachPin(IN1Pin);
    ledcDetachPin(IN2Pin);
}
