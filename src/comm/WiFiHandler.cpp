/**
 * @file WiFiHandler.cpp
 * @brief Implementação do handler Wi-Fi para Oracle APEX
 */

#include "comm/WiFiHandler.h"
#include "utils/Logger.h"
#include "system_definitions.h"
#include <ctype.h>
#include <esp_task_wdt.h>

// Função auxiliar (mantida do original)
uint8_t hexToByte(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

struct ParsedUrl {
    String host;
    String path;
    uint16_t port;
};

ParsedUrl parseUrl(const String& url) {
    ParsedUrl result;
    result.port = 443; // default HTTPS

    String temp = url;

    if (temp.startsWith("https://")) {
        temp.remove(0, 8);
    } else if (temp.startsWith("http://")) {
        temp.remove(0, 7);
        result.port = 80;
    }

    int slashIndex = temp.indexOf('/');
    if (slashIndex >= 0) {
        result.host = temp.substring(0, slashIndex);
        result.path = temp.substring(slashIndex);
    } else {
        result.host = temp;
        result.path = "/";
    }

    return result;
}

WiFiHandler::WiFiHandler(const WiFiConfig& cfg)
    : config(cfg),
      currentState(ConnectionState::DISCONNECTED),
      _isConfirmed(false) {
}

bool WiFiHandler::begin() {
    LOGI("WiFi", "Inicializando WiFiHandler (Modo Oracle APEX)...");
    currentState = ConnectionState::DISCONNECTED;
    return true;
}

void WiFiHandler::end() {
    WiFi.disconnect(true);
    currentState = ConnectionState::DISCONNECTED;
    LOGI("WiFi", "WiFi Desconectado.");
}

bool WiFiHandler::connect() {
    if (isConnected()) return true;

    currentState = ConnectionState::CONNECTING;
    LOGI("WiFi", "Conectando ao SSID: %s", config.ssid);
    
    WiFi.begin(config.ssid, config.password);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < config.connectTimeout) {
        delay(500);
        Serial.print(".");
        esp_task_wdt_reset();
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        LOGI("WiFi", "Conectado! IP: %s", WiFi.localIP().toString().c_str());
        currentState = ConnectionState::CONNECTED;
        return true;
    } 

    LOGE("WiFi", "Timeout: Falha ao conectar no Wi-Fi");
    currentState = ConnectionState::ERROR;
    return false;
}

bool WiFiHandler::isConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

SendResult WiFiHandler::send(uint8_t port, const uint8_t* data, uint16_t length) {
    if (!isConnected()) return SendResult::NOT_CONNECTED;

    // 1. Processamento do Payload 
    size_t hexLen = strnlen((const char*)data, length);
    if (hexLen % 2 != 0) hexLen--; 
    
    size_t binLen = hexLen / 2;
    uint8_t* binBuffer = new uint8_t[binLen];

    for (size_t i = 0; i < binLen; i++) {
        char high = (char)data[2 * i];
        char low = (char)data[2 * i + 1];
        binBuffer[i] = (hexToByte(high) << 4) | hexToByte(low);
    }

    String base64Payload = base64::encode(binBuffer, binLen);
    delete[] binBuffer;

    // 2. Montagem do JSON
    String jsonPayload = "{";
    jsonPayload += "\"meta\": {";
    jsonPayload += "\"time\": " + String(millis()) + ",";
    jsonPayload += "\"packet_id\": " + String(millis()/1000) + ",";
    jsonPayload += "\"device_name\": \"PendioSensor\",";
    jsonPayload += "\"device\": \"" + String(config.deviceId) + "\"";
    jsonPayload += "},";
    jsonPayload += "\"params\": {";
    jsonPayload += "\"payload\": \"" + base64Payload + "\",";
    jsonPayload += "\"encrypted_payload\": \"" + base64Payload + "\",";
    jsonPayload += "\"duplicate\": \"false\"";
    jsonPayload += "}";
    jsonPayload += "}";

    LOGD("WiFi", "Enviando JSON para Oracle...");

    // 3. Configuração e Envio

    ParsedUrl url = parseUrl(config.apexUrl);
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(config.connectTimeout);

    if (!client.connect(url.host.c_str(), url.port)) {
        LOGE("WiFi", "Falha ao conectar no servidor %s", url.host.c_str());
        return SendResult::FAILED;
    }

    String request =
        "POST " + url.path + " HTTP/1.1\r\n"
        "Host: " + url.host + "\r\n"
    #ifdef WIFI_USE_API_KEY
        "X-API-Key: " + String(config.apiKey) + "\r\n"
    #endif
        "Content-Type: application/json\r\n"
        "Content-Length: " + String(jsonPayload.length()) + "\r\n"
        "Connection: close\r\n\r\n" +
        jsonPayload;

    client.print(request);

    String statusLine = client.readStringUntil('\n');
    statusLine.trim();
    LOGD("WiFi", "HTTP Status: %s", statusLine.c_str());

    LOGD("WiFi", "HTTP Status: %s", statusLine.c_str());

    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r" || line.length() == 0) {
            break; // fim dos headers
        }
    }

    String responseBody;
    unsigned long start = millis();

    while (client.connected() && millis() - start < 3000) {
        while (client.available()) {
            responseBody += (char)client.read();
        }
    }

    responseBody.trim();

    String cleanBody;
    for (size_t i = 0; i < responseBody.length(); i++) {
        char c = responseBody[i];
        if (c >= 32 && c <= 126) {  // ASCII imprimível
            cleanBody += c;
        }
    }

    responseBody = cleanBody;

    LOGD("WiFi", "HTTP Body: %s", responseBody.c_str());

    if (responseBody.length() == 0) {
        LOGE("WiFi", "Resposta vazia do servidor");
        client.stop();
        return SendResult::FAILED;
    }

    if (responseBody.indexOf("\"erro\"") >= 0) {
        LOGE("WiFi", "Erro retornado pelo servidor: %s", responseBody.c_str());
        client.stop();
        return SendResult::FAILED;
    }

    LOGI("WiFi", "POST aceito pelo servidor");
    client.stop();

    _isConfirmed = true;
    currentState = ConnectionState::WAITING_CONFIRMATION;
    return SendResult::SUCCESS;

}

bool WiFiHandler::isConfirmed() {
    if (_isConfirmed) {
        _isConfirmed = false;
        currentState = ConnectionState::CONNECTED;
        return true;
    }
    return false;
}

ReceiveResult WiFiHandler::receive(DownlinkMessage& message) {
    // Implementação futura de downlink via GET se necessário
    return ReceiveResult::NO_MESSAGE;
}

ConnectionState WiFiHandler::getConnectionState() {
    return currentState;
}

const char* WiFiHandler::getStateString() {
    switch (currentState) {
        case ConnectionState::DISCONNECTED:         return "WIFI_DISCONNECTED";
        case ConnectionState::CONNECTING:           return "WIFI_CONNECTING";
        case ConnectionState::CONNECTED:            return "WIFI_CONNECTED";
        case ConnectionState::WAITING_CONFIRMATION: return "WIFI_WAIT_CFM";
        case ConnectionState::ERROR:                return "WIFI_ERROR";
        default:                                    return "UNKNOWN";
    }
}

void WiFiHandler::process() {
    updateState();
}

void WiFiHandler::updateState() {
    if (WiFi.status() != WL_CONNECTED) {
        if (currentState != ConnectionState::DISCONNECTED) {
            LOGW("WiFi", "Conexão perdida!");
            currentState = ConnectionState::DISCONNECTED;
        }
    } else if (currentState == ConnectionState::DISCONNECTED) {
        currentState = ConnectionState::CONNECTED;
    }
}

String WiFiHandler::bufferToBase64(const uint8_t* data, uint16_t length) {
    return base64::encode(data, length);
}

unsigned long WiFiHandler::getConfirmationTimeout() {
    return 10000;
}