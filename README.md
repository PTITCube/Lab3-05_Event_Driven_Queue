# Lab3-05_Event_Driven_Queue

**Kiến trúc Hướng sự kiện (Event-Driven) & Hàng đợi FreeRTOS Queue**

Bài thực hành thuộc Phân hệ C&DH (Lab 3) trong hệ thống PTITCube.

## 1. Mục tiêu
- Hiểu và ứng dụng kiến trúc **Hướng sự kiện (Event-Driven)** thay thế cho kỹ thuật thăm dò vòng lặp tuần tự (Busy Polling) nhằm tiết kiệm năng lượng cho CubeSat.
- Sử dụng **FreeRTOS Queue** (`QueueHandle_t`) để truyền nhận dữ liệu bất đồng bộ một cách an toàn giữa nhiều nguồn phát (Event Producers) và tác vụ xử lý trung tâm (Event Consumer).
- Thực hành đẩy sự kiện từ ngắt phần cứng ISR (`xQueueSendFromISR`) và từ các tác vụ phần mềm (`xQueueSend`).
- Đưa tác vụ xử lý vào trạng thái ngủ sâu (`portMAX_DELAY` - Blocked State) khi không có sự kiện, giảm thiểu 100% thời gian CPU nhàn rỗi.

## 2. Phần cứng sử dụng
- **ESP32 NodeMCU OBC**
- **LED báo hiệu:** GPIO 2 (Lab 1).
- **Nút nhấn sự kiện ngắt:** GPIO 4 (`INPUT_PULLUP`, Lab 1).
- **Serial Monitor:** Cổng USB UART (Baudrate: `115200`).

## 3. Các nguồn phát sự kiện (Producers) & Xử lý (Consumer)
1. **Producer 1 (Ngắt phần cứng nút nhấn GPIO 4):** Bắt sườn xuống (Falling Edge), đóng gói `EVENT_BUTTON_INTERRUPT` đẩy vào Queue qua ISR.
2. **Producer 2 (Viễn trắc giả lập):** Tác vụ chạy chu kỳ 1000ms sinh số liệu nhiệt độ mạch C&DH ngẫu nhiên (20-35°C). Nếu vượt ngưỡng > 30°C, đẩy `EVENT_TELEMETRY_ALERT` vào Queue.
3. **Producer 3 (Lệnh nối tiếp UART):** Nhận ký tự gõ từ Serial Monitor, đóng gói `EVENT_COMMAND_RECEIVED` đẩy vào Queue.
4. **Consumer (Event Dispatcher Task):** Chờ nhận sự kiện từ Queue, giải mã và thực thi hành vi tương ứng (đảo LED, nháy LED cảnh báo, xác nhận lệnh).
