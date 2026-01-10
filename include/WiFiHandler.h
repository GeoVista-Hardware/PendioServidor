/**
 * @file WiFiHandler.h
 * @brief Handler de comunicação Wi-Fi com implementação Firebase Realtime Database.
 * @details Este arquivo define a classe WiFiHandler, que implementa a interface 
 * CommunicationHandler. Ele utiliza o Firebase RTDB para simular o envio 
 * de pacotes de telemetria (análogo ao LoRaWAN) via Wi-Fi.
 * @copyright Copyright (c) 2026
 */

#ifndef _WIFI_HANDLER_H
#define _WIFI_HANDLER_H

// Inclui a interface base
#include "CommunicationHandler.h"

// Includes do Framework e Wi-Fi
#include <Arduino.h>
#include <WiFi.h>

// Includes da Biblioteca Firebase (Mobizt)
#include <Firebase_ESP_Client.h>

// Codificação Base64 (para conversão de payloads)
#include <base64.h>

/**
 * @struct WiFiConfig
 * @brief Estrutura de configuração para o Handler Wi-Fi/Firebase.
 * @details Contém todas as credenciais e parâmetros necessários para estabelecer
 * a conexão Wi-Fi e autenticar no projeto Firebase.
 */
struct WiFiConfig {
    const char* ssid;               /**< @brief SSID (Nome) da rede Wi-Fi. */
    const char* password;           /**< @brief Senha da rede Wi-Fi. */
    const char* apiKey;             /**< @brief API Key do projeto Firebase. */
    const char* databaseUrl;        /**< @brief URL do Realtime Database (ex: https://x.firebaseio.com). */
    const char* deviceId;           /**< @brief Identificador do dispositivo (simula o DevEUI) para organizar os dados no banco. */
    unsigned long connectTimeout;   /**< @brief Tempo máximo (ms) para aguardar a conexão Wi-Fi antes de dar timeout. */
};

/**
 * @class WiFiHandler
 * @brief Implementação concreta do CommunicationHandler via Wi-Fi + Firebase.
 * @details Esta classe permite que o firmware, originalmente desenhado para LoRaWAN,
 * funcione sobre Wi-Fi sem alterar a lógica de negócios principal.
 * Os dados enviados via `send()` são convertidos para Hexadecimal e 
 * armazenados no caminho `/devices/{deviceId}/uplinks` do Firebase.
 */
class WiFiHandler : public CommunicationHandler {
private:

    // --- Configurações e Estado ---

    WiFiConfig config;              /**< @brief Cópia local das configurações. */
    ConnectionState currentState;   /**< @brief Estado atual da máquina de estados de comunicação. */
    bool _isConfirmed;              /**< @brief Flag que indica se o último envio recebeu ACK (HTTP 200 OK). */

    // --- Objetos da Biblioteca Firebase ---

    FirebaseData fbdo;              /**< @brief Objeto de dados principal para operações do Firebase (Request/Response). */
    FirebaseAuth auth;              /**< @brief Objeto de autenticação (sessão do usuário). */
    
    /**
     * @brief Objeto de configuração da biblioteca Firebase.
     * @note O tipo é 'FirebaseConfig' (definido pela biblioteca), não confundir com WiFiConfig.
     */
    FirebaseConfig fconfig;         

    // --- Métodos Auxiliares Privados ---

    /**
     * @brief Converte um buffer de bytes cru para uma String Hexadecimal.
     * @details Utilizado para simular o payload LoRa no formato que os Network Servers geralmente exibem.
     * @param data Ponteiro para o array de bytes.
     * @param length Tamanho do array.
     * @return String Representação em string (ex: "010AF3").
     */
    String bufferToBase64(const uint8_t* data, uint16_t length);

    /**
     * @brief Atualiza o estado interno da conexão.
     * @details Verifica se o Wi-Fi caiu ou se o Firebase precisa renovar token.
     */
    void updateState();

public:

    /**
     * @brief Construtor do WiFiHandler.
     * @param cfg Estrutura contendo as credenciais e configurações iniciais.
     */
    explicit WiFiHandler(const WiFiConfig& cfg);

    /**
     * @brief Destrutor padrão.
     * @details Encerra a conexão Wi-Fi se necessário.
     */
    ~WiFiHandler() override = default;

    /**
     * @brief Inicializa as configurações da biblioteca Firebase.
     * @details Configura API Key, URL e callbacks de token. Não conecta no Wi-Fi ainda.
     * @return bool Retorna true se a configuração inicial foi aceita.
     */
    bool begin() override;

    /**
     * @brief Finaliza o handler e desconecta o Wi-Fi.
     */
    void end() override;

    /**
     * @brief Inicia a conexão com a rede Wi-Fi.
     * @details Bloqueia (com timeout) até obter IP ou falhar. Inicializa o Firebase após conectar.
     * @return bool true se conectado com sucesso (Wi-Fi + Firebase Ready).
     */
    bool connect() override;

    /**
     * @brief Verifica se a conexão está ativa e pronta para envio.
     * @return bool true se Wi-Fi está conectado E Firebase está pronto.
     */
    bool isConnected() override;

    /**
     * @brief Envia um pacote de dados (Uplink).
     * @details Converte os dados para Hex e faz um PUSH no Firebase.
     * @param port Porta da aplicação (fPort no LoRaWAN).
     * @param data Ponteiro para os dados.
     * @param length Quantidade de bytes.
     * @return SendResult SUCCESS se o servidor Firebase aceitou o dado (HTTP 200).
     */
    SendResult send(uint8_t port, const uint8_t* data, uint16_t length) override;

    /**
     * @brief Verifica se o último envio foi confirmado.
     * @details No contexto HTTP/Firebase, é síncrono, então retorna true logo após um envio com sucesso.
     * @return bool true se confirmado.
     */
    bool isConfirmed() override;

    /**
     * @brief Verifica se há mensagens de descida (Downlink).
     * @note Implementação atual é um Mock (retorna NO_MESSAGE), mas pode ser expandida para ler do banco.
     * @param message Estrutura onde a mensagem será gravada.
     * @return ReceiveResult Resultado da operação.
     */
    ReceiveResult receive(DownlinkMessage& message) override;

    /**
     * @brief Obtém o enum do estado atual.
     * @return ConnectionState (CONNECTED, DISCONNECTED, ERROR, etc).
     */
    ConnectionState getConnectionState() override;

    /**
     * @brief Função de "Housekeeping".
     * @details Deve ser chamada no loop principal. Mantém o token do Firebase renovado.
     */
    void process() override;

    /**
     * @brief Retorna uma string legível do estado atual (para Logs).
     * @return const char* Ex: "WIFI_CONNECTED".
     */
    const char* getStateString() override;
    
};

#endif /* _WIFI_HANDLER_H */