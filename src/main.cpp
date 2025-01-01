#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <espnow.h>

const int bat = 13;

unsigned long lastTime = 0;
unsigned long timerDelay = 1000; // send readings timer

typedef struct struct_message
{
    int white;
    int red;
    int yellow;
    int orange;
    int Onled;
    int buzzer;
    int battery;
    int temp;
    int water;
} struct_message;

// Create a struct_message called myData
struct_message myData;
struct_message shipData;

// REPLACE WITH THE MAC Address of your receiver
uint8_t broadcastAddress[] = {0x30, 0x83, 0x98, 0xE4, 0xAF, 0x60};

void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len)
{
    memcpy(&myData, incomingData, sizeof(myData));
    Serial.print("Bytes received: ");
    Serial.println(len);
/*     Serial.print("white:");
    Serial.println(myData.white);
    Serial.print("red:");
    Serial.println(myData.red);
    Serial.print("yellow:");
    Serial.println(myData.yellow);
    Serial.print("orange:");
    Serial.println(myData.orange);
    Serial.print("joy:");
    Serial.println(myData.joy);
    Serial.print("x:");
    Serial.println(myData.x);
    Serial.print("y:");
    Serial.println(myData.y);
    Serial.println(); */
}


void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus)
{
    Serial.print("Last Packet Send Status: ");
    if (sendStatus == 0)
    {
        Serial.println("Delivery success");
        Serial.println(shipData.battery);
    }
    else
    {
        Serial.println("Delivery fail");
        Serial.println(shipData.battery);
    }
}


void setup()
{
    Serial.begin(9600);
    pinMode(bat, INPUT);
    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != 0)
    {
        Serial.println("Error initializing ESP-NOW");
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
        shipData.battery = digitalRead(bat);
        shipData.temp = 1;
        // Send message via ESP-NOW
        esp_now_send(broadcastAddress, (uint8_t *)&shipData, sizeof(shipData));
        lastTime = millis(); // プログラム実行から経過した時間
    }
}