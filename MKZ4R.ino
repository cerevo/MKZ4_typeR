/*
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <algorithm>

#include "DRV8835.h"
#include "MutexWrapper.h"
#include "driver/ledc.h"
#include "ledcServo.h"

namespace Config {
    namespace ServoAngle {
        float left = 0;
        float right = 180;
        float center = 90;
    }
    float maxMotorDuty = 0.8;
}
using namespace Config;

//Configの値を保存する
const char *configFileName = "/config.json";

//デバッグ用
//リセット回数をカウントする
const char *countFileName = "/count.txt";

//jsonに格納されたConfigパラメータをパースして適用する
void applyConfig(const JsonObject &json) {
    if(json.containsKey("ServoAngle")) {
        const JsonObject &jsonServoAngle = json["ServoAngle"];
        if(jsonServoAngle.containsKey("left")) {
            ServoAngle::left = jsonServoAngle["left"];
        }
        if(jsonServoAngle.containsKey("right")) {
            ServoAngle::right =jsonServoAngle["right"];
        }
        if(jsonServoAngle.containsKey("center")) {
            ServoAngle::center = jsonServoAngle["center"];
        }
    }
    if(json.containsKey("maxMotorDuty")) {
        maxMotorDuty = json["maxMotorDuty"];
    }
}

//現在のConfigパラメータをjsonに変換したものを返す
String makeJsonConfig() {
    StaticJsonBuffer<256> jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    JsonObject &servo = root.createNestedObject("ServoAngle");
    servo["left"] = ServoAngle::left;
    servo["right"] = ServoAngle::right;
    servo["center"] = ServoAngle::center;
    root["maxMotorDuty"] = maxMotorDuty;

    String ret;
    root.printTo(ret);
    return ret;
}


/* Set these to your desired credentials. */
const char * const AP_SSID = "MKZ4_ESP32";
//password: min 8 characters, max 63 characters
const char * AP_PASSWORD = "";

AsyncWebServer server(80);

AsyncWebSocket ws("/ws");

ledc_timer_bit_t LEDCTimerBitNum = LEDC_TIMER_15_BIT;

const int ServoPin = 18;
const int MotorPWMForwardPin = 32;
const int MotorPWMReversePin = 25;

LEDCServo servo(2, LEDCTimerBitNum);

//サーボを操作する
//rate: [-1, 1]
//-1: left, 1: right, 0: center
MutexWrapper<float> currentServoRate = 0;
void servoControl(float rate) {
    if(rate > 0) {
        servo.write((ServoAngle::right - ServoAngle::center) * rate + ServoAngle::center);
    } else {
        servo.write((ServoAngle::left - ServoAngle::center) * (-rate) + ServoAngle::center);
    }
    currentServoRate.set(rate);
}


DRV8835 motorDriver(
        100,
        0, 1,
        LEDCTimerBitNum);

MutexWrapper<float> currentMotorRate = 0;
//モーターを駆動する
//rate: [-1, 1]
//この引数のrateにConfig::maxMotorDutyをかけたものが実際のデューティー比になる
void driveMotor(float rate) {
    if(rate * currentMotorRate.get() < 0) {
        //回転の正負が入れ替わるときはブレーキを挟む
        //（本当に必要なのかは自信ないけど）
        motorDriver.stop();
        delay(10);
    }

    motorDriver.drive(rate * maxMotorDuty);
    currentMotorRate.set(rate);
}

struct ControllerCommand {
    float motorRate;
    float servoRate;
};

QueueHandle_t controllerQueueHandle;
const size_t controllerQueueLength = 1;
xTaskHandle controllerTaskHandle;

//driveMotorが内部でdelay()を使うので、AsyncWebSocketから直接呼べない
//そのためdriveMotorを呼ぶタスクを別で回す
void controllerTaskFunction(void *) {
    ControllerCommand command;

    while (1) {
        delay(1);
        portBASE_TYPE ret = xQueueReceive(controllerQueueHandle, &command, portMAX_DELAY);
        if (ret == pdFALSE) {
            continue;
        }
        Serial.print("Command: motor = ");
        Serial.print(command.motorRate);
        Serial.print(", servo = ");
        Serial.println(command.servoRate);

        driveMotor(command.motorRate);
        servoControl(command.servoRate);
    }
}

//driveMotorを呼ぶタスクと通信するための関数
//これはすぐに制御を返すのでAsyncWebSocketから呼んでいい
portBASE_TYPE sendControllerCommand(float motorRate, float servoRate) {
    ControllerCommand command;
    //queueを空にする
    while(xQueueReceive(controllerQueueHandle, &command, 0)){
        //nop
    }

    command = {motorRate, servoRate};
    return xQueueSend(controllerQueueHandle, &command, 0);
}


//デバッグ用
//wifiの接続確立などのイベントで呼ばれるコールバック
void wifiEventHandler(WiFiEvent_t event) {
    Serial.printf("[WiFi-event] event: %d\n", event);
    switch (event) {
        case SYSTEM_EVENT_AP_STACONNECTED:
            Serial.println("a station was connected");
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            Serial.println("a station was disconnected");
            if (WiFi.softAPgetStationNum() == 0) {
                sendControllerCommand(0, 0);
            }
            break;
        default:
            break;
    }
}

//デバッグ用
//fs内のdirnameディレクトリの中身をSerialにプリントする
void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root) {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels) {
                listDir(fs, file.name(), levels - 1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

//fs内のdirnameディレクトリ以下を削除する
void clearDir(fs::FS &fs, const char *dirname) {
    Serial.printf("Clearing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root) {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.print("clear DIR : ");
            Serial.println(file.name());
            fs.rmdir(file.name());
        } else {
            Serial.print("  FILE: ");
            Serial.println(file.name());
            fs.remove(file.name());
        }
        file = root.openNextFile();
    }
}

//config.htmlからのsubmitを処理する
//送られてきたjsonをパースしてConfigパラメータに適用し、SPIFFS内のconfigFileに書き出す
void configSetterHandler(AsyncWebServerRequest *request) {
    int servoLeft = request->getParam("servoLeft")->value().toInt();
    int servoRight = request->getParam("servoRight")->value().toInt();
    int servoCenter = request->getParam("servoCenter")->value().toInt();
    float maxMotorDuty = request->getParam("maxMotorDuty")->value().toFloat();
    ServoAngle::left = servoLeft;
    ServoAngle::right = servoRight;
    ServoAngle::center = servoCenter;
    ::maxMotorDuty = maxMotorDuty;

    Serial.println(makeJsonConfig());

    if(File configFile = SPIFFS.open(configFileName, "w")) {
        configFile.print(makeJsonConfig());
    }

    request->send(200, "text/html", "");
    Serial.println("save config...");
}

//config.htmlからのgetリクエストを処理する
//現在のConfigパラメータの値をjsonに入れて返す
template <size_t BufSize>
void configGetterHanlder(AsyncWebServerRequest *request) {
    request->send(200, "text/json", makeJsonConfig());
}

//デバッグ用
//累計リセット回数を返す
//リセットなのかただwifiが切れただけかわからないことが多いので作った
void countGetterHandler(AsyncWebServerRequest *request) {
    if(File f = SPIFFS.open(countFileName, "r")) {
        String s;
        while(f.available()) {
            s += (char)(f.read());
        }
        f.close();
        int c;
        sscanf(s.c_str(), "%d", &c);
        String message = R"(<html><head><meta charset="UTF-8"><title>count</title></head><body>)" + String(c) + "</body></html>";
        request->send(400, "text/html", message);
    } else {
        Serial.println("Could not open count file");
    }

}

//WebSocketの通信監視用なんちゃってwatchdog
//WebSocket自体のPingはjavascriptでは好きなタイミングで送れないので、
//メッセージのJSONの中にPingという名のJSONオブジェクトがあればそれをPingとして扱う
const unsigned long pingWatchdogDelay = 1000;
MutexWrapper<unsigned long> lastPingTime;

//webSocketのハンドラ
//エコーバック、コントローラへのコマンド送信と、lastPingTimeの更新を行う
void onWebSocketEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
    if(type == WS_EVT_CONNECT){
        //client connected
        Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
        client->printf("Hello Client %u :)", client->id());

        lastPingTime.set(millis() + pingWatchdogDelay);
    } else if(type == WS_EVT_DISCONNECT){
        //client disconnected
        Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id(), client->id());
        sendControllerCommand(0, 0);
    } else if(type == WS_EVT_ERROR){
        //error was received from the other end
        Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
    } else if(type == WS_EVT_PONG){
        //pong message was received (in response to a ping request maybe)
        Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
    } else if(type == WS_EVT_DATA){
        //data packet
        AwsFrameInfo * info = (AwsFrameInfo*)arg;
        static String message;
        static bool messageCompleted = false;
        if(info->final && info->index == 0 && info->len == len){
            //the whole message is in a single frame and we got all of it's data
            Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);
            if(info->opcode == WS_TEXT){
                data[len] = 0;
                Serial.printf("%s\n", (char*)data);
            } else {
                for(size_t i=0; i < info->len; i++){
                    Serial.printf("%02x ", data[i]);
                }
                Serial.printf("\n");
            }

            if(info->opcode == WS_TEXT) {
                message = (char *)data;
                messageCompleted = true;
            }
        } else {
            //message is comprised of multiple frames or the frame is split into multiple packets
            if(info->index == 0){
                if(info->num == 0)
                    Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
                Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
            }

            Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);
            if(info->message_opcode == WS_TEXT){
                data[len] = 0;
                Serial.printf("%s\n", (char*)data);
            } else {
                for(size_t i=0; i < len; i++){
                    Serial.printf("%02x ", data[i]);
                }
                Serial.printf("\n");
            }

            if((info->index + len) == info->len){
                Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
                if(info->final){
                    Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
                }
            }

            if(info->opcode == WS_TEXT) {
                message += (char *)data;
                if(info->final) {
                    messageCompleted = true;
                }
            }
        }

        if(messageCompleted) {
            client->text(message);
            StaticJsonBuffer<256> jsonBuffer;
            JsonObject &json = jsonBuffer.parseObject(message.c_str());
            if(!json.success()) {
                Serial.printf("message %s can not be parsed", message.c_str());
            } else {
                if(json.containsKey("motorRate") && json.containsKey("servoRate")) {
                    float motorRate = json["motorRate"];
                    float servoRate = json["servoRate"];
                    if(std::abs(motorRate) > 1) {
                        motorRate = motorRate > 0 ? 1: -1;
                    }
                    if(std::abs(servoRate) > 1) {
                        servoRate = servoRate > 0 ? 1: -1;
                    }
                    sendControllerCommand(motorRate, servoRate);
                }
                if(json.containsKey("Ping")) {
                    lastPingTime.set(millis());
                }
            }
            message = "";
            messageCompleted = false;
        }
    }
}


void setup() {
    delay(1000);
    Serial.begin(115200);

    while (Serial.available()) {
        Serial.read();
    }
    Serial.println();

    //SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount failed");
        while (1);
    }

    //デバッグ用
    //累計リセット回数の表示
    {
        int c = 0;
        if(File f = SPIFFS.open(countFileName, "r")) {
            String s;
            while(f.available()) {
                s += (char)(f.read());
            }
            if(s.length() > 0) {
                sscanf(s.c_str(), "%d", &c);
            }
            f.close();
        }
        if(File f = SPIFFS.open(countFileName, "w")) {
            f.printf("%d", c+1);
            f.close();
        } else {
            Serial.println("Could not open count file");
        }
    }

    //configFileの読み込み、適用
    if(File configFile = SPIFFS.open(configFileName, "r")) {
        String config;
        while(configFile.available()) {
            config += (char)(configFile.read());
        }
        StaticJsonBuffer<256> jsonBuffer;
        JsonObject &jsonConfig = jsonBuffer.parseObject(config);
        if(jsonConfig.success()) {
            applyConfig(jsonConfig);
        }
        configFile.close();
    } else {
        Serial.print("Could not read config file ");
        Serial.println(configFileName);
    }

    delay(40);

    //WiFiアクセスポイントの立ち上げ---------------------------------------------------
    Serial.println("Configuring access point...");

    WiFi.onEvent(wifiEventHandler);

    //You can remove the password parameter if you want the AP to be open.
    if (strlen(AP_PASSWORD) == 0) {
        AP_PASSWORD = NULL;
    }
    if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) {
        Serial.println("error while setting up SoftAP");
        while (1);
    }


    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    //---------------------------------------------------------------------------------

    listDir(SPIFFS, "/", 1);

    //WebSocketサーバー立ち上げ
    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);
    
    //AsyncWebServerの設定
    //server.onを使用するときはserveStaticより先に指定すること
    //そうでないと全部serveStaticに処理されてnot foundが出る
    server.on("/config.set", configSetterHandler);
    server.on("/config.get", configGetterHanlder<256>);
    server.on("/count.html", countGetterHandler);
    server.serveStatic("/", SPIFFS, "/www").setDefaultFile("index.html");

    server.begin();
    Serial.println("HTTP server started");

    delay(100);

    //モーター、サーボの設定
    motorDriver.attach(MotorPWMForwardPin, MotorPWMReversePin);
    servo.attach(ServoPin);

    //コントローラのキュー、タスク作成
    controllerQueueHandle = xQueueCreate(controllerQueueLength, sizeof(ControllerCommand));
    if (!controllerQueueHandle) {
        Serial.println("couldn't create queue for controller");
        while (1);
    }
    xTaskCreate(controllerTaskFunction, "controllerTask", 1000, NULL, 1, &controllerTaskHandle);
    if (!controllerTaskHandle) {
        Serial.println("couldn't create controller task");
        while (1);
    };

    //デフォルトの制御値を送る
    sendControllerCommand(currentMotorRate.get(), currentServoRate.get());
}

//自動停止までの制限時間 [ms]
//この時間以内に次のPingを受信しなければモーターを停止させる
const unsigned long webSocketTimeout = 5000;
void loop() {
    delay(1);
    // 一定時間以内に操縦者からのPingが来ていなかったら停止させる
    if(ws.count() > 0
            && millis() > lastPingTime.get() + webSocketTimeout) {
        if(currentMotorRate.get() != 0) {
            sendControllerCommand(0, currentServoRate.get());
        }
    }
}


