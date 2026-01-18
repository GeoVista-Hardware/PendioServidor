/**
 * @file WiFiHandler.h
 * @brief Handler de comunicação Wi-Fi para Oracle APEX (REST).
 * @details Implementa a interface CommunicationHandler para comunicação via Wi-Fi.
 *          Envia uplinks via HTTP POST formatados em JSON para a procedure GRAVA_UPLINK
 *          do Oracle APEX através do endpoint ORDS.
 * @author Sistema de Monitoramento
 * @copyright Copyright (c) 2026
 */

#ifndef _WIFI_HANDLER_H
#define _WIFI_HANDLER_H

#include "comm/CommunicationHandler.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>           ///< Biblioteca nativa para requisições HTTP
#include <base64.h>               ///< Para codificação do payload em Base64

/**
 * @struct WiFiConfig
 * @brief Estrutura de configuração para o handler Wi-Fi.
 * @details Contém todos os parâmetros necessários para configurar a conexão Wi-Fi
 *          e comunicação com o servidor Oracle APEX.
 */
struct WiFiConfig {
    const char* ssid;             ///< SSID da rede Wi-Fi a conectar.
    const char* password;         ///< Senha de autenticação da rede Wi-Fi.
    const char* apexUrl;          ///< URL completa do endpoint ORDS (ex: http://host/ords/uplink).
    const char* apiKey;           ///< API KEY para acesso ao endpoint ORDS
    const char* deviceId;         ///< Identificador único do dispositivo (DevEUI).
    unsigned long connectTimeout; ///< Timeout em milissegundos para tentativas de conexão.
};

/**
 * @class WiFiHandler
 * @brief Handler responsável pela comunicação Wi-Fi com Oracle APEX via REST API.
 * @details Implementa a interface CommunicationHandler para gerenciar conexão Wi-Fi,
 *          envio de uplinks formatados em JSON e recebimento de downlinks do servidor.
 *          Realiza conversão de dados para Base64 e montagem de payloads HTTP.
 * @see CommunicationHandler
 */
class WiFiHandler : public CommunicationHandler {
private:
    WiFiConfig config;              ///< Configuração da conexão Wi-Fi
    ConnectionState currentState;   ///< Estado atual da conexão
    bool _isConfirmed;              ///< Flag de confirmação de mensagem enviada

    /**
     * @brief Converte dados binários para string Base64.
     * @param data Ponteiro para os dados a serem codificados.
     * @param length Tamanho em bytes dos dados.
     * @return String contendo os dados codificados em Base64.
     */
    String bufferToBase64(const uint8_t* data, uint16_t length);

    /**
     * @brief Atualiza o estado atual da conexão Wi-Fi.
     * @details Verifica o status da conexão e atualiza currentState.
     * @see updateState()
     */
    void updateState();

public:
    /**
     * @brief Construtor parameterizado do WiFiHandler.
     * @param cfg Referência constante para a estrutura de configuração WiFiConfig.
     */
    explicit WiFiHandler(const WiFiConfig& cfg);

    /**
     * @brief Destrutor virtual do WiFiHandler.
     */
    ~WiFiHandler() override = default;

    /**
     * @brief Inicializa o módulo Wi-Fi.
     * @details Configura o Wi-Fi em modo estação (STA) e prepara para conexão.
     * @return true se a inicialização foi bem-sucedida, false caso contrário.
     */
    bool begin() override;

    /**
     * @brief Finaliza e desliga o módulo Wi-Fi.
     * @details Desconecta da rede e coloca o Wi-Fi em modo sleep/desligado.
     */
    void end() override;

    /**
     * @brief Conecta à rede Wi-Fi configurada.
     * @details Tenta estabelecer conexão com o SSID especificado usando as credenciais.
     * @return true se conectado com sucesso, false se falha na conexão.
     * @see WiFiConfig::connectTimeout
     */
    bool connect() override;

    /**
     * @brief Verifica se está conectado à rede Wi-Fi.
     * @return true se conectado, false caso contrário.
     */
    bool isConnected() override;
    
    /**
     * @brief Envia dados via HTTP POST para o servidor Oracle APEX.
     * @details Formata os dados em JSON com Base64 e envia via HTTP POST
     *          para a URL do endpoint ORDS configurado.
     * @param port Número da porta (utilizado para compatibilidade com interface).
     * @param data Ponteiro para os dados a serem enviados.
     * @param length Tamanho em bytes dos dados.
     * @return SendResult contendo status de envio e confirmação.
     * @see SendResult
     */
    SendResult send(uint8_t port, const uint8_t* data, uint16_t length) override;
    
    /**
     * @brief Verifica se a última mensagem foi confirmada pelo servidor.
     * @return true se confirmada, false caso contrário.
     */
    bool isConfirmed() override;

    /**
     * @brief Recebe downlinks (comandos) do servidor Oracle APEX.
     * @param message Referência para a estrutura que receberá a mensagem.
     * @return ReceiveResult contendo status e dados da mensagem recebida.
     * @see ReceiveResult, DownlinkMessage
     */
    ReceiveResult receive(DownlinkMessage& message) override;

    /**
     * @brief Obtém o estado atual da conexão.
     * @return ConnectionState representando o estado presente.
     * @see ConnectionState
     */
    ConnectionState getConnectionState() override;

    /**
     * @brief Processa tarefas assíncronas do handler Wi-Fi.
     * @details Realiza verificações periódicas de estado e reconexão se necessário.
     */
    void process() override;

    /**
     * @brief Retorna uma string descritiva do estado atual.
     * @return Ponteiro para string com nome legível do estado.
     */
    const char* getStateString() override;

    /**
     * @brief Retorna o timeout para confirmação de mensagem.
     * @return Timeout em milissegundos.
     */
    unsigned long getConfirmationTimeout() override;
};

#endif /* _WIFI_HANDLER_H */