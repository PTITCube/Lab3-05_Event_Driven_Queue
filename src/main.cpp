/**
 * VÍ DỤ 3-05: KIẾN TRÚC HƯỚNG SỰ KIỆN (EVENT-DRIVEN) VỚI FREERTOS QUEUE
 */
#include <Arduino.h>
#include <PTITCube.h>

PTIT_CDH cdh;

#define LED_PIN    2
#define BUTTON_PIN 4

enum EventType {
    EVENT_BUTTON,
    EVENT_TEMP_ALERT,
    EVENT_COMMAND
};

struct SystemEvent {
    EventType type;
    uint32_t timestamp;
    int32_t value;
};

static QueueHandle_t eventQueue = NULL;

void IRAM_ATTR buttonISR() {
    SystemEvent evt = { EVENT_BUTTON, millis(), 0 };
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(eventQueue, &evt, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
}

void sensorTask(void* pvParameters) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        int temp = random(20, 35);
        if (temp > 30) {
            SystemEvent evt = { EVENT_TEMP_ALERT, millis(), temp };
            xQueueSend(eventQueue, &evt, 0);
        }
    }
}

void serialTask(void* pvParameters) {
    for (;;) {
        if (cdh.available() > 0) {
            String str = cdh.readStringUntil('\n');
            str.trim();
            if (str.length() > 0) {
                SystemEvent evt = { EVENT_COMMAND, millis(), str[0] };
                xQueueSend(eventQueue, &evt, 0);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void dispatcherTask(void* pvParameters) {
    SystemEvent evt;
    bool ledState = false;

    for (;;) {
        if (xQueueReceive(eventQueue, &evt, portMAX_DELAY) == pdTRUE) {
            switch (evt.type) {
                case EVENT_BUTTON:
                    ledState = !ledState;
                    digitalWrite(LED_PIN, ledState);
                    cdh.printf("Event: Button (t=%lums) -> LED: %s\n", evt.timestamp, ledState ? "ON" : "OFF");
                    break;
                case EVENT_TEMP_ALERT:
                    cdh.printf("Event: Temp Alert: %d C (t=%lums)\n", evt.value, evt.timestamp);
                    break;
                case EVENT_COMMAND:
                    cdh.printf("Event: Cmd: '%c' (t=%lums)\n", (char)evt.value, evt.timestamp);
                    break;
            }
        }
    }
}

void setup() {
    cdh.begin(115200);

    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

    eventQueue = xQueueCreate(10, sizeof(SystemEvent));
    cdh.println("Event: Queue Ready.");

    xTaskCreatePinnedToCore(dispatcherTask, "Dispatcher", 2048, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(sensorTask,     "Sensor",     2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(serialTask,     "Serial",     2048, NULL, 2, NULL, 1);
}

void loop() {
}
