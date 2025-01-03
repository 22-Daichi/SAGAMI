#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <espnow.h>

const int rightMotor1 = 12;
const int rightMotor2 = 13;
const int leftMotor1 = 1;
const int leftMotor2 = 3;

const int activeBuzzer = 15;
const int blueLed = 5;
const int redLed = 16;

int outputPin[] = {
    rightMotor1,
    rightMotor2,
    leftMotor1,
    leftMotor2,
    activeBuzzer,
    blueLed,
    redLed};

const int waterSensor = 4;

unsigned long lastTime = 0;
unsigned long timerDelay = 1000; // send readings timer

void gpioSetup()
{
    for (int i = 0; 0 <= 6; i += 1)
    {
        pinMode(outputPin[i], OUTPUT);
        digitalWrite(outputPin[i], 0);
    }

    pinMode(waterSensor, INPUT);
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
    // Serial.print("Bytes received: ");
    // Serial.println(len);
    /*     Serial.print("up:");
        Serial.println(controllerData.up);
        Serial.print("red:");
        Serial.println(controllerData.down);
        Serial.print("yellow:");
        Serial.println(controllerData.right);
        Serial.print("orange:");
        Serial.println(controllerData.left);
 */
}

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus)
{
    // Serial.print("Last Packet Send Status: ");
    if (sendStatus == 0)
    {
        // Serial.println("Delivery success");
        digitalWrite(redLed, 1);
        digitalWrite(blueLed, 0);
    }
    else
    {
        // Serial.println("Delivery fail");
        digitalWrite(redLed, 0);
        digitalWrite(blueLed, 1);
    }
}

void setup()
{
    // Serial.begin(9600);
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
}

void loop()
{
    if ((millis() - lastTime) > timerDelay)
    {
        // Set values to send
        // Send message via ESP-NOW
        esp_now_send(broadcastAddress, (uint8_t *)&shipData, sizeof(shipData));
        lastTime = millis(); // プログラム実行から経過した時間
    }
}