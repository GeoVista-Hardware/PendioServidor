/**
 * @file LoRaHandler.cpp
 * @brief Implementação do handler de comunicação LoRaWAN
 * @copyright Copyright (c) 2025
 */

#include "comm/LoRaHandler.h"
#include <Arduino.h>
#include "utils/Logger.h"

// Constantes internas
static const unsigned long DEFAULT_JOIN_TIMEOUT = 30000;      // 30s
static const unsigned long DEFAULT_CFM_TIMEOUT = 6000;        // 6s

/**
 * @brief Construtor
 */
LoRaHandler::LoRaHandler(const LoRaConfig& cfg)
    : lorawan(cfg.serial ? *cfg.serial : Serial1),
      config(cfg),
      currentState(ConnectionState::DISCONNECTED),
      confirmed(false),
      lastSendTime(0),
      retryCount(0) {
    
    if (!cfg.serial) {
        config.serial = &Serial1;  // Default serial if not provided
    }
    
    // Inicializa a estrutura de downlink
    lastDownlink.port = 0;
    lastDownlink.length = 0;
    lastDownlink.timestamp = 0;
    memset(lastDownlink.data, 0, sizeof(lastDownlink.data));
}

/**
 * @brief Inicializa o handler
 */
bool LoRaHandler::begin() {
    LOGI("LoRa", "Inicializando LoRaHandler...");

    // Reset do módulo
    CommandResponse response = lorawan.reset();
    if (response != CommandResponse::OK) {
        LOGE("LoRa", "Falha no reset do módulo");
        currentState = ConnectionState::ERROR;
        return false;
    }

    LOGI("LoRa", "Reset OK");

    // Configurar App EUI
    if (config.appEUI != nullptr) {
        response = lorawan.set_AppEUI((const char*)config.appEUI);
        if (response != CommandResponse::OK) {
            LOGE("LoRa", "Falha ao configurar App EUI");
            currentState = ConnectionState::ERROR;
            return false;
        }
        LOGI("LoRa", "App EUI configurado");
    }

    // Configurar App Key
    if (config.appKey != nullptr) {
        response = lorawan.set_AppKey((const char*)config.appKey);
        if (response != CommandResponse::OK) {
            LOGE("LoRa", "Falha ao configurar App Key");
            currentState = ConnectionState::ERROR;
            return false;
        }
        LOGI("LoRa", "App Key configurado");
    }

    // Configurar modo de join (OTAA)
    response = lorawan.set_JoinMode(SMW_SX1262M0_JOIN_MODE_OTAA);
    if (response != CommandResponse::OK) {
        LOGE("LoRa", "Falha ao configurar OTAA");
        currentState = ConnectionState::ERROR;
        return false;
    }
    LOGI("LoRa", "OTAA configurado");

    // Configurar confirmação
    response = lorawan.set_CFM(config.useConfirmation ? 
                                SMW_SX1262M0_CFM_ON : 
                                SMW_SX1262M0_CFM_OFF);
    if (response != CommandResponse::OK) {
        LOGE("LoRa", "Falha ao configurar CFM");
        currentState = ConnectionState::ERROR;
        return false;
    }
    LOGI("LoRa", "Confirmação: %s", config.useConfirmation ? "ON" : "OFF");

    // Configurar ADR
    if (config.useADR) {
        response = lorawan.set_ADR(SMW_SX1262M0_ADR_ON);
        if (response != CommandResponse::OK) {
            LOGE("LoRa", "Falha ao ativar ADR");
            currentState = ConnectionState::ERROR;
            return false;
        }
        LOGI("LoRa", "ADR ativado");
    } else {
        response = lorawan.set_ADR(SMW_SX1262M0_ADR_OFF);
        if (response != CommandResponse::OK) {
            LOGE("LoRa", "Falha ao desativar ADR");
            currentState = ConnectionState::ERROR;
            return false;
        }
        LOGI("LoRa", "ADR desativado");

        // Configurar DR fixo
        response = lorawan.set_DR(config.fixedDR);
        if (response != CommandResponse::OK) {
            LOGE("LoRa", "Falha ao configurar DR");
            currentState = ConnectionState::ERROR;
            return false;
        }
        LOGI("LoRa", "DR fixo: %d", config.fixedDR);
    }

    // Salvar configurações
    response = lorawan.save();
    if (response != CommandResponse::OK) {
        LOGW("LoRa", "Falha ao salvar configurações (não crítico)");
    }

    currentState = ConnectionState::DISCONNECTED;
    LOGI("LoRa", "Handler inicializado com sucesso");
    return true;
}

/**
 * @brief Finaliza o handler
 */
void LoRaHandler::end() {
    LOGI("LoRa", "Finalizando LoRaHandler");
    currentState = ConnectionState::DISCONNECTED;
}

/**
 * @brief Conecta à rede (OTAA Join)
 */
bool LoRaHandler::connect() {
    if (currentState == ConnectionState::CONNECTED) {
        return true;
    }

    LOGI("LoRa", "Tentando conectar à rede (JOIN)...");
    currentState = ConnectionState::CONNECTING;

    CommandResponse response = lorawan.join();
    if (response != CommandResponse::OK) {
        LOGE("LoRa", "Falha ao enviar JOIN");
        currentState = ConnectionState::ERROR;
        return false;
    }

    // Aguarda join com timeout
    unsigned long startTime = millis();
    while ((millis() - startTime) < (config.joinTimeout ? config.joinTimeout : DEFAULT_JOIN_TIMEOUT)) {
        delay(100);
        if (lorawan.isConnected()) {
            LOGI("LoRa", "Conectado com sucesso");
            currentState = ConnectionState::CONNECTED;
            return true;
        }
    }
    LOGE("LoRa", "TIMEOUT: Falha ao conectar");
    currentState = ConnectionState::ERROR;
    return false;
}

/**
 * @brief Verifica se está conectado
 */
bool LoRaHandler::isConnected() {
    bool connected = lorawan.isConnected();
    
    if (connected && currentState != ConnectionState::CONNECTED && 
        currentState != ConnectionState::WAITING_CONFIRMATION) {
        currentState = ConnectionState::CONNECTED;
    }
    
    return connected;
}

/**
 * @brief Envia dados
 */
SendResult LoRaHandler::send(uint8_t port, const uint8_t* data, uint16_t length) {
    // Validar entrada
    if (data == nullptr || length == 0 || length > 242) {
        return SendResult::INVALID_DATA;
    }

    // Verificar conexão
    if (!isConnected()) {
        return SendResult::NOT_CONNECTED;
    }

    // Enviar mensagem
    LOGD("LoRa", "Enviando %u bytes...", (unsigned)length);

    CommandResponse response = lorawan.sendX(port, (const char*)data);

    if (response == CommandResponse::OK) {
        lastSendTime = millis();
        retryCount = 0;
        if (config.useConfirmation) currentState = ConnectionState::WAITING_CONFIRMATION;
        else currentState = ConnectionState::CONNECTED;
        LOGI("LoRa", "Envio aceito (port=%u, len=%u)", (unsigned)port, (unsigned)length);
        return SendResult::SUCCESS;
    } else {
        LOGE("LoRa", "Envio recusado (port=%u)", (unsigned)port);
        return SendResult::FAILED;
    }
}

/**
 * @brief Verifica confirmação
 */
bool LoRaHandler::isConfirmed() {
    if (!config.useConfirmation) {
        // Se não usar confirmação, sempre retorna true após envio
        return (currentState != ConnectionState::WAITING_CONFIRMATION);
    }

    bool result = lorawan.isConfirmed();
    
    if (result && currentState == ConnectionState::WAITING_CONFIRMATION) {
        confirmed = true;
        currentState = ConnectionState::CONNECTED;
    }

    return result;
}

/**
 * @brief Recebe mensagem
 */
ReceiveResult LoRaHandler::receive(DownlinkMessage& message) {
    uint8_t port;
    Buffer buffer;

    CommandResponse response = lorawan.readX(port, buffer);
    
    if (response != CommandResponse::OK) {
        LOGD("LoRa", "readX retornou erro: %d", (int)response);
        return ReceiveResult::ERROR;
    }

    if (!buffer.available()) {
        LOGD("LoRa", "Buffer vazio - nenhuma mensagem disponível");
        return ReceiveResult::NO_MESSAGE;
    }

    // Armazenar mensagem
    message.port = port;
    message.timestamp = millis();
    message.length = 0;

    while (buffer.available() && message.length < sizeof(message.data)) {
        message.data[message.length++] = buffer.read();
    }

    // Guardar referência
    lastDownlink = message;

    LOGI("LoRa", "Downlink: porta=%u, tamanho=%u bytes", (unsigned)port, (unsigned)message.length);
    
    // Log dos dados em hex para diagnóstico
    LOGD("LoRa", "Dados (hex):");
    for (uint16_t i = 0; i < message.length && i < 64; i++) {
        LOGD("LoRa", "%02X", message.data[i]);
    }

    return ReceiveResult::MESSAGE_RECEIVED;
}

/**
 * @brief Obtém estado da conexão
 */
ConnectionState LoRaHandler::getConnectionState() {
    updateState();
    return currentState;
}

/**
 * @brief Processa eventos
 */
void LoRaHandler::process() {
    // Verifica timeouts
    if (currentState == ConnectionState::WAITING_CONFIRMATION) {
        unsigned long elapsed = millis() - lastSendTime;
        unsigned long timeout = config.confirmTimeout ? config.confirmTimeout : DEFAULT_CFM_TIMEOUT;

        if (elapsed > timeout) {
            if (retryCount++ < config.maxRetries) {
                LOGW("LoRa", "Timeout - tentando novamente...");
                currentState = ConnectionState::CONNECTED;
            } else {
                LOGE("LoRa", "Máximo de tentativas excedido");
                currentState = ConnectionState::ERROR;
                retryCount = 0;
            }
        }
    }

    // Atualizar estado
    updateState();
}

/**
 * @brief Atualiza estado
 */
void LoRaHandler::updateState() {
    if (currentState == ConnectionState::ERROR || 
        currentState == ConnectionState::DISCONNECTED) {
        return;
    }

    if (currentState != ConnectionState::WAITING_CONFIRMATION) {
        if (!lorawan.isConnected()) {
            currentState = ConnectionState::DISCONNECTED;
        }
    }
}

/**
 * @brief Traduz CommandResponse para SendResult
 */
SendResult LoRaHandler::commandToSendResult(CommandResponse response) {
    switch (response) {
        case CommandResponse::OK:
            return SendResult::SUCCESS;
        case CommandResponse::ERROR:
        default:
            return SendResult::FAILED;
    }
}

/**
 * @brief Obtém DevEUI
 */
bool LoRaHandler::getDevEUI(char* buffer) {
    if (buffer == nullptr) {
        return false;
    }

    // get_DevEUI espera uma referência de array char[16]
    char temp[16];
    CommandResponse response = lorawan.get_DevEUI(temp);
    if (response == CommandResponse::OK) {
        memcpy(buffer, temp, 16);
        return true;
    }
    return false;
}

/**
 * @brief Define confirmação
 */
bool LoRaHandler::setConfirmation(bool enabled) {
    config.useConfirmation = enabled;
    CommandResponse response = lorawan.set_CFM(enabled ? 
                                                SMW_SX1262M0_CFM_ON : 
                                                SMW_SX1262M0_CFM_OFF);
    
    if (response == CommandResponse::OK) {
        lorawan.save();
        return true;
    }
    return false;
}

/**
 * @brief Define ADR
 */
bool LoRaHandler::setADR(bool enabled) {
    config.useADR = enabled;
    CommandResponse response = lorawan.set_ADR(enabled ? 
                                               SMW_SX1262M0_ADR_ON : 
                                               SMW_SX1262M0_ADR_OFF);
    
    if (response == CommandResponse::OK) {
        lorawan.save();
        return true;
    }
    return false;
}

/**
 * @brief Define Data Rate
 */
bool LoRaHandler::setDataRate(uint8_t dr) {
    config.fixedDR = dr;
    CommandResponse response = lorawan.set_DR(dr);
    
    if (response == CommandResponse::OK) {
        lorawan.save();
        return true;
    }
    return false;
}

/**
 * @brief Obtém descrição do estado
 */
const char* LoRaHandler::getStateString() {
    switch (currentState) {
        case ConnectionState::DISCONNECTED:
            return "DISCONNECTED";
        case ConnectionState::CONNECTING:
            return "CONNECTING";
        case ConnectionState::CONNECTED:
            return "CONNECTED";
        case ConnectionState::WAITING_CONFIRMATION:
            return "WAITING_CFM";
        case ConnectionState::ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Obtém o timeout apropriado para LoRaWAN
 */
unsigned long LoRaHandler::getConfirmationTimeout() {
    // LoRaWAN é assíncrono, ACK pode levar até 3 minutos
    // O main.cpp calcula o ciclo total desde o envio, então aqui retornamos
    // o máximo de tempo que podemos esperar por ACK
    return 180000;  // 180 segundos (3 minutos)
}
