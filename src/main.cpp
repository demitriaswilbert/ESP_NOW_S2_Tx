#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_private/wifi.h>

#define PMK_STRING "Never Gonna Give"
#define LMK_STRING "You Up Never Gon"
#define PEER_MAC_ADDR   "24:0A:C4:59:D3:7C"
#define millisll() (esp_timer_get_time() / 1000ULL)

static esp_now_peer_info_t peerInfo;

static String send_str = "", recv_str = "";
static bool txReady = false, rxReady = false;


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

String getMacString(const uint8_t* buf) {
    String tmp;
    const char symbols[] = "0123456789ABCDEF";
    for (int i = 0; i < 6; i++) {
        if (i > 0) send_str += ':';
        send_str += symbols[(buf[i] >> 4U) & 0x0fU];
        send_str += symbols[buf[i] & 0x0fU];
    }
    return tmp;
}

void setMacAddr(const char* addr, esp_now_peer_info_t* peerInfo) {
    uint8_t tmp_addr[6];
    bool success = getMacAddr(addr, tmp_addr);
    if (!success) return;
    esp_now_del_peer(peerInfo->peer_addr);
    memcpy(peerInfo->peer_addr, tmp_addr, 6);
    esp_now_add_peer(peerInfo);
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

    if (rxReady)
        return;

    memcpy(&flags, &data[0], sizeof(flags));
    memcpy(&pos, &data[4], sizeof(pos));


    if (flags & 1U) {
        recv_str.clear();
    }
    for (int i = 8; i < data_len; i++) {
        recv_str += (char) data[i];
    }

    if (flags & 4U) {
        Serial.println("Received " + String(recv_str.length()) + " Bytes");
        rxReady = true;
    } else {
        Serial.println("Received " + String(flags) + " " + String(pos));
    }
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

        *(uint32_t*)&packet[0] = ((uint32_t)((i + txLen) == len)) << 2U;
        *(uint32_t*)&packet[0] |= (1U << (i > 0));
        *(uint32_t*)&packet[4] = (uint32_t)i + 1;

        memcpy(&packet[8], data + i, txLen);

        esp_err_t err = esp_now_send_packet(peer_addr, packet, txLen + 8, 1000UL);

        if (err != ESP_OK) {
            if (millisll() - resend_timeout < timeout) {
                Serial.println(
                    "[ERROR] ESP-NOW Transmission Error, Resending Packet");
                continue;
            } else return ESP_ERR_TIMEOUT;
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

            if (c == '\033') {

                if (send_str.startsWith("SetMac ")) {
                    setMacAddr(send_str.c_str() + 7, &peerInfo);
                    Serial.println(getMacString(peerInfo.peer_addr));
                    send_str.clear();
                    continue;
                } else if (send_str.startsWith("Encr 0")) {

                    esp_now_del_peer(peerInfo.peer_addr);
                    peerInfo.encrypt = false;
                    esp_now_add_peer(&peerInfo);
                    
                    Serial.println("Disabled Encryption");
                    send_str.clear();
                    continue;

                } else if (send_str.startsWith("Encr 1")) {

                    esp_now_del_peer(peerInfo.peer_addr);
                    peerInfo.encrypt = true;
                    esp_now_add_peer(&peerInfo);

                    Serial.println("Enabled Encryption");
                    send_str.clear();
                    continue;
                }

                while (txReady)
                    vTaskDelay(1);

                txReady = true;
                
            } else if (c == '\b' && !send_str.isEmpty()) {
                send_str.remove(send_str.length() - 1);
                Serial.write("\b \b");
            } else {
                send_str += c;
                Serial.write(c);
            } 
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
    ESP_ERROR_CHECK(esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_11M_S));

    while (esp_now_init() != ESP_OK) {
        Serial.println("[ESP_NOW]: Init ERROR");
        Serial.println("[ESP_NOW]: Retrying");
        vTaskDelay(1000);
    }

    esp_now_set_pmk((const uint8_t*)PMK_STRING);
    
    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataRecv);

    getMacAddr(PEER_MAC_ADDR, peerInfo.peer_addr);
    Serial.println(WiFi.macAddress());
    peerInfo.channel = 0U;
    memcpy(peerInfo.lmk, LMK_STRING, 16);
    peerInfo.encrypt = true;

    while (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        vTaskDelay(1000);
    }

}

void loop() {
    if (txReady) {
        size_t transmitted = 0UL;

        esp_err_t err = ESP_OK;
        int64_t txTime = esp_timer_get_time();
        err = esp_now_send_blocking(peerInfo.peer_addr, (uint8_t*)send_str.c_str(), send_str.length(), &transmitted, 5000);
        txTime = esp_timer_get_time() - txTime;
        send_str.clear();

        char buf[1000];
        esp_err_to_name_r(err, buf, 1000);
        txReady = false;

        while (!Serial.availableForWrite())
            vTaskDelay(1);

        Serial.println(buf);
        Serial.println("Transmitted: " + String(transmitted));
        Serial.println("Speed: " + String((double)transmitted * 1000000.0 / txTime));
        vTaskDelay(1000);
    }
    if (rxReady) {
        Serial.println(recv_str);
        rxReady = false;
    }
    
    vTaskDelay(100);
}