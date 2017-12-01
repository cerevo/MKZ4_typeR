#ifndef DRV8835_HEADER
#define DRV8835_HEADER

#include <Arduino.h>

//DCモータードライバDRV8835のライブラリ
//チップには2系統の入出力があるが1系統につきこのクラスのインスタンス一つを割り当てる。
class DRV8835 {
public:
    //freq: 制御PWM周期 [Hz]
    DRV8835(
            double freq,
            uint8_t ledcChannel1, uint8_t ledcChannel2,
            uint8_t ledcTimerBitNum
           );

    //rate [-1,1]
    //rate > 0: 正転
    //rate < 0: 逆転
    void drive(float rate);
    
    //ブレーキ
    void stop();

    //空転
    void coast();

    //IOピンとのattach, detach
    void attach(uint8_t IN1Pin, uint8_t IN2Pin);
    void detach();
    
private:
    uint8_t IN1Pin, IN2Pin;
    const uint8_t ledcChannel1, ledcChannel2;
    const uint8_t ledcTimerBitNum;
    const uint32_t maxDuty;
};


#endif // DRV8835_HEADER
