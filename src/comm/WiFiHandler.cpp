/**
 * @file WiFiHandler.cpp
 * @brief Implementação do handler Wi-Fi usando Firebase Realtime Database
 * @details Converte payloads binários em strings HEX e envia para o Firebase,
 * simulando um uplink LoRaWAN.
 * @copyright Copyright (c) 2026
 */

// Includes da aplicação

#include "comm/WiFiHandler.h"
#include "utils/Logger.h" 

// Addons da biblioteca Firebase

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Funções de manipulação de caracteres

#include <ctype.h>

// Função auxiliar

uint8_t hexToByte(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

// Construtor de WiFiHandler

WiFiHandler::WiFiHandler(const WiFiConfig& cfg)
    : config(cfg),
      currentState(ConnectionState::DISCONNECTED),
      _isConfirmed(false) {
}

// Inicialização das configs da lib (sem conectar ainda)
bool WiFiHandler::begin() {

    LOGI("WiFi", "Inicializando WiFiHandler (Modo Firebase)...");

    // Configurações obrigatórias da biblioteca Firebase
    fconfig.api_key = config.apiKey;
    fconfig.database_url = config.databaseUrl;

    // Callback para status do token
    fconfig.token_status_callback = tokenStatusCallback; 

    // Login do usuário no Firebase (pode ser anônimo ou com email/senha)
    auth.user.email = "admin@pendio.com";
    auth.user.password = "pendio123";

    // Realiza o sign-up (registro) se necessário
    Firebase.signUp(&fconfig, &auth, "", "");

    // Registra o estado inicial
    currentState = ConnectionState::DISCONNECTED;

    // Sucesso
    return true;

}

// Encerramento
void WiFiHandler::end() {

    // Desconecta do Firebase
    WiFi.disconnect(true);

    // Atualiza o estado
    currentState = ConnectionState::DISCONNECTED;

    // Informa o encerramento
    LOGI("WiFi", "WiFi Desconectado.");

}

// Conexão efetiva (Wi-Fi + Firebase)
bool WiFiHandler::connect() {

    // Se já estiver tudo pronto, retorna true
    if (isConnected()) return true;

    // Atualiza o estado para conexão Wi-Fi
    currentState = ConnectionState::CONNECTING;
    LOGI("WiFi", "Conectando ao SSID: %s", config.ssid);
    WiFi.begin(config.ssid, config.password);

    // Loop de espera com timeout
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < config.connectTimeout) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    // Verifica se conectou à rede com sucesso
    if (WiFi.status() == WL_CONNECTED) {
        LOGI("WiFi", "Conectado! IP: %s", WiFi.localIP().toString().c_str());
        
        // Inicializa o objeto Firebase
        Firebase.begin(&fconfig, &auth);
        
        // Mantém a conexão Wi-Fi ativa automaticamente
        Firebase.reconnectWiFi(true);
        
        // Atualiza o estado para conectado
        currentState = ConnectionState::CONNECTED;
        return true;

    } 

    // Falha na conexão informada por timeout
    LOGE("WiFi", "Timeout: Falha ao conectar no Wi-Fi");

    // Estado de erro na conexão Wi-Fi
    currentState = ConnectionState::ERROR;
    return false;

}

// Verifica status
bool WiFiHandler::isConnected() {

    // Consideramos conectado se temos Wi-Fi e a lib do Firebase está pronta
    return (WiFi.status() == WL_CONNECTED && Firebase.ready());

}

// Envio de Dados (Uplink)
SendResult WiFiHandler::send(uint8_t port, const uint8_t* data, uint16_t length) {

    // Verifica se a conexão está ativa
    if (!isConnected()) return SendResult::NOT_CONNECTED;

    // Caminho no Realtime Database para uplinks
    String path = "/devices/";
    path += config.deviceId;
    path += "/uplinks";

    // --- CONFIGURAÇÃO DO FORMATO DO PAYLOAD ---
    // O 'data' que chega aqui é uma String Hex (ex: "0100...").
    // É convertido de volta para Binário antes de gerar o Base64.
    
    // Calcular o tamanho real em bytes (cada 2 chars hex = 1 byte)
    // Usamos strnlen para ignorar o null terminator se houver
    size_t hexLen = strnlen((const char*)data, length);
    if (hexLen % 2 != 0) hexLen--; // Garante paridade
    
    size_t binLen = hexLen / 2;
    uint8_t* binBuffer = new uint8_t[binLen];

    // Converter Hex String -> Binário (Decode)
    for (size_t i = 0; i < binLen; i++) {
        char high = (char)data[2 * i];
        char low = (char)data[2 * i + 1];
        binBuffer[i] = (hexToByte(high) << 4) | hexToByte(low);
    }

    // Gerar o Base64 a partir do Binário (Encode)
    String base64Payload = base64::encode(binBuffer, binLen);
    
    // Limpa a memória temporária
    delete[] binBuffer;
    
    // --- REALIZA O ENVIO PARA O FIREBASE ---

    // Monta o JSON para envio
    FirebaseJson json;
    json.set("port", port);
    json.set("payload", base64Payload); 
    
    // Campos de metadados
    json.set("fcnt", (int)(millis()/1000));
    json.set("timestamp", millis());
    json.set("rssi", -50);
    json.set("snr", 9.5);

    // Informa o envio (com debug)
    LOGD("WiFi", "Enviando %d bytes (Base64: %s)", binLen, base64Payload.c_str());

    // Realiza o push no Firebase (uplink)
    if (Firebase.RTDB.pushJSON(&fbdo, path.c_str(), &json)) {

        LOGI("WiFi", "Envio Sucesso! Chave: %s", fbdo.pushName().c_str());
        _isConfirmed = true; 
        currentState = ConnectionState::WAITING_CONFIRMATION;
        return SendResult::SUCCESS;

    } else {

        // Caso de erro no envio para o Firebase
        LOGE("WiFi", "Erro no envio: %s", fbdo.errorReason().c_str());
        return SendResult::FAILED;

    }

}

// Verifica confirmação (ACK)
bool WiFiHandler::isConfirmed() {

    // Como o envio HTTP é síncrono, se _isConfirmed está true, é porque já foi confirmado.
    if (_isConfirmed) {

        _isConfirmed = false; // Limpa a flag
        currentState = ConnectionState::CONNECTED; // Volta ao estado pronto
        return true;

    }

    return false;

}

// Recepção de Downlinks (Mock)
ReceiveResult WiFiHandler::receive(DownlinkMessage& message) {

    // TODO: Futuramente, pode implementar um listener (stream) no caminho:
    // /devices/{deviceId}/downlinks
    
    // Por enquanto, retorna vazio para não travar o loop
    return ReceiveResult::NO_MESSAGE;

}

// Getters de Estado
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

// Processamento recorrente
void WiFiHandler::process() {
    updateState();
}

// Atualização de estado baseada no hardware
void WiFiHandler::updateState() {

    // Se o Wi-Fi cair, atualiza o estado para forçar reconexão se necessário
    if (WiFi.status() != WL_CONNECTED) {
        if (currentState != ConnectionState::DISCONNECTED) {
            LOGW("WiFi", "Conexão perdida!");
            currentState = ConnectionState::DISCONNECTED;
        }
    } 
    else if (currentState == ConnectionState::DISCONNECTED && Firebase.ready()) {
        currentState = ConnectionState::CONNECTED;
    }
    
}

// Implementação do Base64 usando a lib nativa
String WiFiHandler::bufferToBase64(const uint8_t* data, uint16_t length) {
    // Wrapper simples para a função da biblioteca
    return base64::encode(data, length);
}