#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_private/wifi.h>


#define PEER_MAC_ADDR   "24:0A:C4:59:D3:7C"
#define millisll() (esp_timer_get_time() / 1000ULL)

static String tmp = "";
static bool txReady = false;
static esp_now_peer_info_t peerInfo;

bool getMacAddr(const char* str, uint8_t* buf_mac_addr) {
    uint8_t mac_addr_tmp[6] = {0}, mac_addr_filled = 0;
    for (const char* i = str; *i != '\0' && mac_addr_filled < 12U; i++) {
        const char ch = *i;
        if (ch == ':') continue;
        uint8_t tmpval = 0U;
        if (ch >= '0' && ch <= '9') {
            tmpval = (uint8_t)(ch - '0');
        } else if (ch >= 'a' && ch <= 'f') {
            tmpval = (uint8_t)(ch - 'a' + 10);
        } else if (ch >= 'A' && ch <= 'F') {
            tmpval = (uint8_t)(ch - 'A' + 10);
        }
        tmpval &= 0x0fU;
        tmpval <<= (4U * (!(mac_addr_filled & 1U)));
        mac_addr_tmp[mac_addr_filled / 2U] |= tmpval;
        mac_addr_filled++;
    }
    if (mac_addr_filled != 12U) return false;
    memcpy(buf_mac_addr, mac_addr_tmp, 6U);
    return true;
}

static int sent_status = -1;

void onDataSent(const uint8_t* mac, esp_now_send_status_t status) {
    if (status != ESP_NOW_SEND_SUCCESS) {
        Serial.println("[ERROR]: ESP-NOW Transmission Error");
    }
    
    while (sent_status != -1) {
        vTaskDelay(1);
    }
    sent_status = status;
}

void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
    uint32_t flags;
    uint32_t pos;
    memcpy(&flags, &data[0], sizeof(flags));
    memcpy(&pos, &data[4], sizeof(pos));
    Serial.println("Received " + String(flags) + " " + String(pos));
}

esp_err_t esp_now_send_packet(const uint8_t* peer_addr, const uint8_t* data, size_t len, uint32_t timeout) {
    static bool lock = false;
    int64_t time = millisll();

    while (lock) 
        vTaskDelay(1);
    lock = true;

    do {
        sent_status = -1;
        esp_err_t err_send_fn = esp_now_send(peer_addr, data, len);
        while (sent_status == -1) {
            vTaskDelay(1);
        }
    } while (sent_status != 0 && (millisll() - time) < timeout);

    lock = false;
    return sent_status;
}

esp_err_t esp_now_send_blocking(
            const uint8_t* peer_addr, 
            const uint8_t* data, 
            size_t len, 
            size_t* transmitted, 
            uint32_t timeout) {
    
    size_t i = 0;
    int64_t resend_timeout = millisll();
    for(; i < len;) {
        int txLen = ((len - i) > 242)? 242 : (len - i);
        uint8_t packet[250] = {0};
        *(uint32_t*)&packet[0] = ((i + txLen) == len);
        *(uint32_t*)&packet[0] <<= 2U;
        *(uint32_t*)&packet[0] |= (1U << (i > 0));
        *(uint32_t*)&packet[4] = (uint32_t)i + 1;
        memcpy(&packet[8], data + i, txLen);
        esp_err_t err = esp_now_send_packet(peer_addr, packet, txLen + 8, 1000UL);
        if (err != ESP_OK) {
            if (millisll() - resend_timeout < timeout) {
                Serial.println("[ERROR] ESP-NOW Transmission Error, Resending Packet");
                continue;
            }
            else return ESP_ERR_TIMEOUT;
        }
        i += txLen;
        *transmitted = i;
    }
    return ESP_OK;
}

void getInputTask(void* param) {
    while (true) {
        while (Serial.available()) {
            char c = Serial.read();
            Serial.write(c);

            if (c == '\033') {
                while (txReady)
                    vTaskDelay(1);
                txReady = true;
            }
            else tmp += c;
        }
        vTaskDelay(1);
    }
    vTaskDelete(NULL);
}

void setup() {
    vTaskDelay(100);
    Serial.begin(115200);
    xTaskCreate(getInputTask, "uart input", 4096, NULL, 0, NULL);
    pinMode(19, INPUT_PULLUP);
    WiFi.mode(WIFI_MODE_STA);
    Serial.println(WiFi.getTxPower());
    ESP_ERROR_CHECK(esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_11M_S));

    while (esp_now_init() != ESP_OK) {
        Serial.println("[ESP_NOW]: Init ERROR");
        Serial.println("[ESP_NOW]: Retrying");
        vTaskDelay(1000);
    }

    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataRecv);
    getMacAddr(PEER_MAC_ADDR, peerInfo.peer_addr);
    peerInfo.channel = 0U;
    peerInfo.encrypt = false;

    while (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        vTaskDelay(1000);
    }

}

void loop() {
    if (txReady) {
        // Serial.println("\033[2H\033[2J");
        // Serial.println("HeapSize : " + String(ESP.getHeapSize()));
        // Serial.println("FreeHeap : " + String(ESP.getFreeHeap()));
        // Serial.println("MinFreeHeap : " + String(ESP.getMinFreeHeap()));
        // Serial.println("MaxAllocHeap : " + String(ESP.getMaxAllocHeap()));

        // Serial.println("PsramSize : " + String(ESP.getPsramSize()));
        // Serial.println("FreePsram : " + String(ESP.getFreePsram()));
        // Serial.println("MinFreePsram : " + String(ESP.getMinFreePsram()));
        // Serial.println("MaxAllocPsram : " + String(ESP.getMaxAllocPsram()));
        // Serial.println("\n\n");
        size_t transmitted = 0UL;
        int64_t txTime = esp_timer_get_time();
        esp_err_t err = esp_now_send_blocking(peerInfo.peer_addr, (uint8_t*)tmp.c_str(), tmp.length(), &transmitted, 5000);
        txTime = esp_timer_get_time() - txTime;
        char buf[1000];
        esp_err_to_name_r(err, buf, 1000);
        tmp.clear();
        txReady = false;
        Serial.println(buf);
        Serial.println("Transmitted: " + String(transmitted));
        Serial.println("Speed: " + String((double)transmitted * 1000000.0 / txTime));
        vTaskDelay(1000);
    }
    vTaskDelay(100);
}