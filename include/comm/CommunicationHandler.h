/**
 * @file CommunicationHandler.h
 * @brief Interface abstrata para diferentes métodos de comunicação (LoRa, Wi-Fi, etc).
 * @details Define a interface que deve ser implementada por qualquer método de comunicação.
 * @copyright Copyright (c) 2025
 */

#ifndef _COMMUNICATION_HANDLER_H
#define _COMMUNICATION_HANDLER_H

#include <Arduino.h>
#include <stdint.h>

/**
 * @enum ConnectionState
 * @brief Estados possíveis da conexão
 */
enum class ConnectionState {
    DISCONNECTED,           // Desconectado
    CONNECTING,             // Tentando se conectar
    CONNECTED,              // Conectado e pronto
    WAITING_CONFIRMATION,   // Aguardando confirmação do envio
    ERROR                   // Erro na comunicação
};

/**
 * @enum SendResult
 * @brief Resultado de uma tentativa de envio
 */
enum class SendResult {
    SUCCESS,                // Envio aceito
    PENDING,                // Envio pendente
    FAILED,                 // Envio falhou
    NOT_CONNECTED,          // Não está conectado
    INVALID_DATA            // Dados inválidos
};

/**
 * @enum ReceiveResult
 * @brief Resultado de leitura de mensagem
 */
enum class ReceiveResult {
    MESSAGE_RECEIVED,       // Mensagem recebida
    NO_MESSAGE,             // Nenhuma mensagem
    ERROR                   // Erro ao ler
};

/**
 * @struct DownlinkMessage
 * @brief Estrutura para armazenar mensagens recebidas
 */
struct DownlinkMessage {
    uint8_t port;                           // Porta de recebimento
    uint8_t data[256];                      // Dados recebidos
    uint16_t length;                        // Tamanho dos dados
    uint32_t timestamp;                     // Timestamp do recebimento
};

/**
 * @class CommunicationHandler
 * @brief Interface abstrata para handlers de comunicação
 */
class CommunicationHandler {
public:
    virtual ~CommunicationHandler() = default;

    /**
     * @brief Inicializa o handler de comunicação
     * @return bool true se inicialização bem-sucedida, false caso contrário
     */
    virtual bool begin() = 0;

    /**
     * @brief Finaliza o handler de comunicação
     */
    virtual void end() = 0;

    /**
     * @brief Tenta se conectar ao servidor/rede
     * @return bool true se conectado, false caso contrário
     */
    virtual bool connect() = 0;

    /**
     * @brief Verifica se está conectado
     * @return bool true se conectado, false caso contrário
     */
    virtual bool isConnected() = 0;

    /**
     * @brief Envia dados
     * @param port Porta/Canal de envio
     * @param data Ponteiro para os dados a enviar
     * @param length Tamanho dos dados
     * @return SendResult Resultado do envio
     */
    virtual SendResult send(uint8_t port, const uint8_t* data, uint16_t length) = 0;

    /**
     * @brief Verifica se o último envio foi confirmado
     * @return bool true se confirmado, false caso contrário
     */
    virtual bool isConfirmed() = 0;

    /**
     * @brief Lê uma mensagem recebida
     * @param message Estrutura para armazenar a mensagem
     * @return ReceiveResult Resultado da leitura
     */
    virtual ReceiveResult receive(DownlinkMessage& message) = 0;

    /**
     * @brief Obtém o estado atual da conexão
     * @return ConnectionState Estado atual
     */
    virtual ConnectionState getConnectionState() = 0;

    /**
     * @brief Processa eventos de comunicação (deve ser chamado regularmente)
     */
    virtual void process() = 0;

    /**
     * @brief Obtém descrição textual do estado
     * @return const char* Descrição do estado
     */
    virtual const char* getStateString() = 0;
};

#endif /* _COMMUNICATION_HANDLER_H */
