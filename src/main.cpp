/**
 * LAB 3 - VÍ DỤ 05: KIẾN TRÚC HƯỚNG SỰ KIỆN (EVENT-DRIVEN) & HÀNG ĐỢI FREERTOS QUEUE
 */
#include <Arduino.h>
#include <PTITCube.h>

PTIT_CDH cdh;

#define LED_PIN    2
#define BUTTON_PIN 4

enum EventType {
    EVENT_BUTTON_PRESSED,
    EVENT_TELEMETRY_ALERT,
    EVENT_SERIAL_COMMAND
};

struct SystemEvent {
    EventType type;
    uint32_t timestamp;
    int32_t param;
};

static QueueHandle_t systemEventQueue = NULL;

void IRAM_ATTR buttonISR() {
    SystemEvent evt;
    evt.type = EVENT_BUTTON_PRESSED;
    evt.timestamp = millis();
    evt.param = 0;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(systemEventQueue, &evt, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void telemetryMonitorTask(void* pvParameters) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        int simTemp = random(20, 35);
        if (simTemp > 30) {
            SystemEvent evt;
            evt.type = EVENT_TELEMETRY_ALERT;
            evt.timestamp = millis();
            evt.param = simTemp;
            xQueueSend(systemEventQueue, &evt, 0);
        }
    }
}

void serialCommandTask(void* pvParameters) {
    for (;;) {
        if (cdh.available() > 0) {
            char cmd = cdh.read();
            if (cmd != '\r' && cmd != '\n') {
                SystemEvent evt;
                evt.type = EVENT_SERIAL_COMMAND;
                evt.timestamp = millis();
                evt.param = (int32_t)cmd;
                xQueueSend(systemEventQueue, &evt, 0);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void eventDispatcherTask(void* pvParameters) {
    SystemEvent receivedEvent;
    bool ledState = false;

    for (;;) {
        if (xQueueReceive(systemEventQueue, &receivedEvent, portMAX_DELAY) == pdTRUE) {
            switch (receivedEvent.type) {
                case EVENT_BUTTON_PRESSED:
                    ledState = !ledState;
                    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
                    cdh.printf("[EVENT] Button pressed (t=%lums) -> LED: %s\n",
                               receivedEvent.timestamp, ledState ? "ON" : "OFF");
                    break;

                case EVENT_TELEMETRY_ALERT:
                    cdh.printf("[EVENT] Temp Alert: %d C (t=%lums)\n",
                               receivedEvent.param, receivedEvent.timestamp);
                    break;

                case EVENT_SERIAL_COMMAND:
                    cdh.printf("[EVENT] Cmd: '%c' (t=%lums)\n",
                               (char)receivedEvent.param, receivedEvent.timestamp);
                    break;
            }
        }
    }
}

void setup() {
    cdh.begin(115200);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

    systemEventQueue = xQueueCreate(10, sizeof(SystemEvent));
    if (systemEventQueue == NULL) {
        cdh.println("[ERROR] Queue creation failed!");
        return;
    }

    cdh.println("[CDH] Event-Driven Queue Ready.");

    xTaskCreatePinnedToCore(eventDispatcherTask,  "EventDispatcher", 2048, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(telemetryMonitorTask, "TelemMonitor",    2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(serialCommandTask,    "SerialCmd",       2048, NULL, 2, NULL, 1);
}

void loop() {
}
