/**
 * @file LoRaHandler.h
 * @brief Handler de comunicação para LoRaWAN (SMW_SX1262M0)
 * @details Implementa a interface CommunicationHandler para LoRaWAN
 * @copyright Copyright (c) 2025
 */

#ifndef _LORA_HANDLER_H
#define _LORA_HANDLER_H

#include "comm/CommunicationHandler.h"
#include <RoboCore_SMW_SX1262M0.h>
#include <HardwareSerial.h>

/**
 * @struct LoRaConfig
 * @brief Configuração do handler LoRa
 */
struct LoRaConfig {
    HardwareSerial* serial;                 // Serial UART para o módulo LoRa
    const uint8_t* appEUI;                  // Application EUI
    const uint8_t* appKey;                  // Application Key
    bool useConfirmation;                   // Usar confirmação (CFM)
    bool useADR;                            // Usar Adaptive Data Rate
    uint8_t fixedDR;                        // Data Rate fixo (se ADR desabilitado)
    unsigned long joinTimeout;              // Timeout para join (ms)
    unsigned long confirmTimeout;           // Timeout para confirmação (ms)
    uint8_t maxRetries;                     // Máximo de tentativas de envio
};

/**
 * @class LoRaHandler
 * @brief Handler de comunicação para LoRaWAN
 */
class LoRaHandler : public CommunicationHandler {
private:
    SMW_SX1262M0 lorawan;                   // Driver do módulo LoRa
    LoRaConfig config;                      // Configuração
    ConnectionState currentState;           // Estado atual
    bool confirmed;                         // Flag de confirmação
    DownlinkMessage lastDownlink;           // Última mensagem recebida
    unsigned long lastSendTime;             // Tempo do último envio
    uint8_t retryCount;                     // Contador de tentativas

public:
    /**
     * @brief Construtor do LoRaHandler
     * @param cfg Configuração inicial
     */
    explicit LoRaHandler(const LoRaConfig& cfg);

    /**
     * @brief Destrutor
     */
    ~LoRaHandler() override = default;

    /**
     * @brief Inicializa o handler LoRa
     * @return bool true se sucesso
     */
    bool begin() override;

    /**
     * @brief Finaliza o handler LoRa
     */
    void end() override;

    /**
     * @brief Conecta à rede LoRaWAN (OTAA Join)
     * @return bool true se conectado
     */
    bool connect() override;

    /**
     * @brief Verifica se está conectado
     * @return bool true se conectado
     */
    bool isConnected() override;

    /**
     * @brief Envia dados via LoRa
     * @param port Port/FPort para envio
     * @param data Dados a enviar
     * @param length Tamanho dos dados
     * @return SendResult Resultado
     */
    SendResult send(uint8_t port, const uint8_t* data, uint16_t length) override;

    /**
     * @brief Verifica confirmação do último envio
     * @return bool true se confirmado
     */
    bool isConfirmed() override;

    /**
     * @brief Recebe mensagem downlink
     * @param message Estrutura para armazenar
     * @return ReceiveResult Resultado
     */
    ReceiveResult receive(DownlinkMessage& message) override;

    /**
     * @brief Obtém estado da conexão
     * @return ConnectionState Estado atual
     */
    ConnectionState getConnectionState() override;

    /**
     * @brief Processa eventos LoRa
     */
    void process() override;

    /**
     * @brief Obtém descrição do estado
     * @return const char* String descritiva
     */
    const char* getStateString() override;

    /**
     * @brief Obtém DevEUI do módulo
     * @param buffer Buffer para armazenar (mínimo 16 bytes)
     * @return bool true se sucesso
     */
    bool getDevEUI(char* buffer);

    /**
     * @brief Atualiza configuração de confirmação (CFM)
     * @param enabled true para ativar CFM
     * @return bool true se sucesso
     */
    bool setConfirmation(bool enabled);

    /**
     * @brief Atualiza ADR
     * @param enabled true para ativar ADR
     * @return bool true se sucesso
     */
    bool setADR(bool enabled);

    /**
     * @brief Define Data Rate fixo
     * @param dr Data Rate (0-7)
     * @return bool true se sucesso
     */
    bool setDataRate(uint8_t dr);

private:
    /**
     * @brief Atualiza o estado da conexão
     */
    void updateState();

    /**
     * @brief Traduz CommandResponse para SendResult
     */
    SendResult commandToSendResult(CommandResponse response);
};

#endif /* _LORA_HANDLER_H */
