#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <espnow.h>

#include <Wire.h>

const int rightMotor1 = 12;
const int rightMotor2 = 13;
const int leftMotor1 = 1;
const int leftMotor2 = 3;

const int activeBuzzer = 15;
const int blueLed = 5;
const int redLed = 16;

const int outputPin[7] = {
    rightMotor1,
    rightMotor2,
    leftMotor1,
    leftMotor2,
    activeBuzzer,
    blueLed,
    redLed};

const int waterSensor = 4;

int16_t ax, ay, az, gx, gy, gz, Temp;

unsigned long lastTime = 0;
unsigned long timerDelay = 50; // send readings timer

int rightMotorPower = 0;
int leftMotorPower = 0;
int accel = 4;
int maximumPower = 200;

void gpioSetup()
{
    for (int k = 0; k <= 6; k++)
    {
        pinMode(outputPin[k], OUTPUT);
        digitalWrite(outputPin[k], 0);
    }
}

void getGy521Value()
{
    // スタートアドレスの設定とデータの要求処理
    Wire.beginTransmission(0x68);     // アドレス0x68指定でMPU-6050を選択、送信処理開始
    Wire.write(0x3B);                 // ACCEL_XOUT_Hレジスタのアドレス指定
    Wire.endTransmission(false);      // false設定で接続維持
    Wire.requestFrom(0x68, 14, true); // MPU-6050に対して14byte分データを要求、I2Cバス開放
    // シフト演算と論理和で16bitのデータを変数に格納
    // ax～gzまで、16bit(2byte) × 7 = 14byte
    ax = Wire.read() << 8 | Wire.read();      // x軸の加速度の読み取り 16bit
    ay = Wire.read() << 8 | Wire.read();      // y軸の加速度の読み取り 16bit
    az = Wire.read() << 8 | Wire.read();      // z軸の加速度の読み取り 16bit
    Temp = Wire.read() << 8 | Wire.read();    // 温度の読み取り 16bit
    Temp = int((Temp / 340.00 + 32.93) * 10); // 10倍の値として送信
    gx = Wire.read() << 8 | Wire.read();      // x軸の角速度の読み取り 16bit
    gy = Wire.read() << 8 | Wire.read();      // y軸の角速度の読み取り 16bit
    gz = Wire.read() << 8 | Wire.read();      // z軸の角速度の読み取り 16bit
}

typedef struct struct_message
{
    int up;
    int down;
    int right;
    int left;
    int buzzer;
    int temp;
    int battery;
    int water;
} struct_message;

// Create a struct_message called controllerData
struct_message controllerData;
struct_message shipData;

// REPLACE WITH THE MAC Address of your receiver
uint8_t broadcastAddress[] = {0x30, 0x83, 0x98, 0xE4, 0xAF, 0x60};

void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len)
{
    memcpy(&controllerData, incomingData, sizeof(controllerData));
}

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus)
{
    // Serial.print("Last Packet Send Status: ");
    if (sendStatus == 0)
    {
        // Serial.println("Delivery success");
        digitalWrite(redLed, 0);
        digitalWrite(blueLed, 1);
    }
    else
    {
        // Serial.println("Delivery fail");
        digitalWrite(redLed, 1);
        digitalWrite(blueLed, 0);
        controllerData.up = 1;
        controllerData.down = 1;
        controllerData.right = 1;
        controllerData.left = 1;
        controllerData.buzzer = 1;
    }
}

void inputData()
{
    shipData.up = 1;
    shipData.down = 1;
    shipData.right = 1;
    shipData.left = 1;
    shipData.buzzer = 1;
    shipData.temp = Temp;
    shipData.battery = int(analogRead(A0) - 30.25);
    shipData.water = digitalRead(waterSensor);
}

void fireBuzzer()
{
    if (controllerData.buzzer == 0)
    {
        digitalWrite(activeBuzzer, 1);
    }
    else
    {
        digitalWrite(activeBuzzer, 0);
    }
}

void motorPower()
{
    if (controllerData.up == 0)
    {
        rightMotorPower += accel;
        leftMotorPower += accel; // 約3秒後に最大
    }
    if (controllerData.down == 0)
    {
        if (rightMotorPower > 0 || leftMotorPower > 0)
        { // 前進or取り舵or面舵
            rightMotorPower = 0;
            leftMotorPower = 0;
        }
        rightMotorPower -= accel;
        leftMotorPower -= accel;
    }
    if (controllerData.right == 0)
    {
        rightMotorPower -= accel;
        leftMotorPower += accel;
    }
    if (controllerData.left == 0)
    {
        rightMotorPower += accel;
        leftMotorPower -= accel;
    }
    if (controllerData.up * controllerData.down * controllerData.left * controllerData.right == 1)
    {
/*         if (rightMotorPower > 0)
        {
            rightMotorPower -= accel / 2;
        }
        else if (rightMotorPower < 0)
        {
            rightMotorPower += accel / 2;
        }
        if (leftMotorPower > 0)
        {
            leftMotorPower -= accel / 2;
        }
        else if (leftMotorPower < 0)
        {
            leftMotorPower += accel / 2;
        } */
       rightMotorPower = 0;
       leftMotorPower = 0;
    }

    if (rightMotorPower > maximumPower)
    {
        rightMotorPower = maximumPower;
    }
    if (rightMotorPower < -maximumPower)
    {
        rightMotorPower = -maximumPower;
    }
    if (leftMotorPower > maximumPower)
    {
        leftMotorPower = maximumPower;
    }
    if (leftMotorPower < -maximumPower)
    {
        leftMotorPower = -maximumPower;
    }
}
void motorDrive()
{
    if (rightMotorPower >= 0)
    {
        analogWrite(rightMotor1, rightMotorPower);
        analogWrite(rightMotor2, 0);
    }
    else
    {
        analogWrite(rightMotor1, 0);
        analogWrite(rightMotor2, rightMotorPower * (-1));
    }
    if (leftMotorPower >= 0)
    {
        analogWrite(leftMotor1, leftMotorPower);
        analogWrite(leftMotor2, 0);
    }
    else
    {
        analogWrite(leftMotor1, 0);
        analogWrite(leftMotor2, leftMotorPower * (-1));
    }
}

void setup()
{
    analogWriteFreq(30000);
    gpioSetup();
    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != 0)
    {
        // Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    // GY-521用
    Wire.begin(2, 14);            // I2C通信開始
    Wire.beginTransmission(0x68); // アドレス0x68指定でMPU-6050を選択、送信処理開始
    Wire.write(0x6B);             // MPU6050_PWR_MGMT_1レジスタのアドレス指定
    Wire.write(0x00);             // 0を書き込むことでセンサ動作開始
    Wire.endTransmission();       // スレーブデバイスに対する送信を完了
}

void loop()
{
    if ((millis() - lastTime) > timerDelay)
    {
        fireBuzzer();
        motorPower(); // モーターの出力決定
        // たとえばここにimuからの値をもとにパラメーターを調整
        motorDrive(); // ここで出力
        // ここから送信用
        getGy521Value();
        inputData();
        esp_now_send(broadcastAddress, (uint8_t *)&shipData, sizeof(shipData));
        // ここまで
        lastTime = millis(); // プログラム実行から経過した時間
    }
}