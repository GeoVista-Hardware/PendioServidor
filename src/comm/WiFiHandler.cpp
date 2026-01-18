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

    // ========================================================================
    // Processamento do Payload (Hex String -> Binário -> Base64)
    // ========================================================================
    
    // Calcula tamanho real da string hex
    size_t hexLen = strnlen((const char*)data, length);
    if (hexLen % 2 != 0) hexLen--; 
    
    size_t binLen = hexLen / 2;
    
    // Alocação temporária para conversão (pequena e rápida, deletada logo em seguida)
    uint8_t* binBuffer = new uint8_t[binLen];

    for (size_t i = 0; i < binLen; i++) {
        char high = (char)data[2 * i];
        char low = (char)data[2 * i + 1];
        binBuffer[i] = (hexToByte(high) << 4) | hexToByte(low);
    }

    String base64Payload = base64::encode(binBuffer, binLen);
    
    delete[] binBuffer; // Limpa buffer binário imediatamente

    LOGD("WiFi", "Montando JSON...");

    // ========================================================================
    // Montagem do JSON (Zero Alocação Dinâmica)
    // ========================================================================
    
    unsigned long now = millis();
    
    // Limpa o buffer estático para garantir que não tenha lixo
    memset(txBuffer, 0, WIFI_TX_BUFFER_SIZE);

    // Formata o JSON diretamente no buffer pré-alocado
    // Nota: %.3f ou %s são usados conforme o tipo. %lu é para unsigned long.
    int len = snprintf(txBuffer, WIFI_TX_BUFFER_SIZE, 
        "{\"meta\":{\"time\":%lu,\"packet_id\":%lu,\"device_name\":\"PendioSensor\",\"device\":\"%s\"},"
        "\"params\":{\"payload\":\"%s\",\"encrypted_payload\":\"%s\",\"duplicate\":\"false\"}}",
        now,
        now / 1000,
        config.deviceId,
        base64Payload.c_str(), // Extrai o array de char da String
        base64Payload.c_str()
    );

    // Verifica se o JSON coube no buffer
    if (len < 0 || len >= WIFI_TX_BUFFER_SIZE) {
        LOGE("WiFi", "Buffer Overflow! JSON muito grande para txBuffer.");
        return SendResult::FAILED;
    }

    LOGD("WiFi", "JSON Length: %d", len);

    // ========================================================================
    // Configuração e Envio 
    // ========================================================================

    ParsedUrl url = parseUrl(config.apexUrl);
    WiFiClientSecure client;
    client.setInsecure(); 
    client.setTimeout(config.connectTimeout);

    if (!client.connect(url.host.c_str(), url.port)) {
        LOGE("WiFi", "Falha ao conectar no servidor %s", url.host.c_str());
        return SendResult::FAILED;
    }

    // Envia Headers linha a linha 

    client.print("POST ");
    client.print(url.path);
    client.println(" HTTP/1.1");
    
    client.print("Host: ");
    client.println(url.host);
    
    #ifdef WIFI_USE_API_KEY
    client.print("X-API-Key: ");
    client.println(config.apiKey);
    #endif
    
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(len); 
    client.println("Connection: close");
    client.println(); 
    
    // Envia o Corpo (JSON que está no txBuffer)
    client.print(txBuffer);

    // ========================================================================
    // Tratamento da Resposta
    // ========================================================================
    
    // Lê apenas a primeira linha para pegar o status (ex: "HTTP/1.1 200 OK")
    String statusLine = client.readStringUntil('\n');
    statusLine.trim();
    LOGD("WiFi", "HTTP Status: %s", statusLine.c_str());

    // Consome (descarta) o resto dos headers da resposta para liberar o buffer de entrada
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r" || line.length() == 0) break;
    }

    // Verifica sucesso (Códigos 200 ou 201)
    if (statusLine.indexOf("200") > 0 || statusLine.indexOf("201") > 0) {

        LOGI("WiFi", "POST aceito pelo servidor (Sucesso)");
        client.stop();
        _isConfirmed = true;
        currentState = ConnectionState::WAITING_CONFIRMATION;
        return SendResult::SUCCESS;

    } else {

        // Em caso de erro, imprime o corpo da resposta no Serial para debug
        LOGE("WiFi", "Erro no envio. Resposta do servidor:");
        
        unsigned long errorStart = millis();
        while(client.connected() && (millis() - errorStart < 2000)) {
            if(client.available()) {
                char c = client.read();
                Serial.print(c); 
            }
        }

        Serial.println(); 
        client.stop();
        return SendResult::FAILED;

    }

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