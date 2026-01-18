# Handlers de Comunicação - Referência Completa

## Visão Geral

O sistema Pendio utiliza a interface `CommunicationHandler` como abstração para diferentes meios de comunicação. Atualmente, estão implementados:

- **LoRaHandler**: Comunicação via LoRaWAN (SMW_SX1262M0)
- **WiFiHandler**: Comunicação via Wi-Fi + Oracle Apex Database (REST/ORDS)

---

## Interface Base: CommunicationHandler

Definida em `include/comm/CommunicationHandler.h`, a interface fornece uma abstração completa para qualquer meio de comunicação.

### Enumerações

```cpp
enum class ConnectionState {
    DISCONNECTED,           // Sem conexão
    CONNECTING,             // Tentando conectar
    CONNECTED,              // Conectado e pronto
    WAITING_CONFIRMATION,   // Aguardando ACK
    ERROR                   // Erro na comunicação
};

enum class SendResult {
    SUCCESS,                // Envio aceito
    PENDING,                // Pendente
    FAILED,                 // Falhou
    NOT_CONNECTED,          // Sem conexão
    INVALID_DATA            // Dados inválidos
};

enum class ReceiveResult {
    MESSAGE_RECEIVED,       // Mensagem recebida
    NO_MESSAGE,             // Sem mensagens
    ERROR                   // Erro ao ler
};

struct DownlinkMessage {
    uint8_t port;           // Porta de recebimento (1-223)
    uint8_t data[256];      // Dados recebidos
    uint16_t length;        // Tamanho dos dados
    uint32_t timestamp;     // Timestamp do recebimento
};
```

### Interface de Métodos

```cpp
class CommunicationHandler {
public:
    virtual ~CommunicationHandler() = default;
    
    // Inicialização e finalização
    virtual bool begin() = 0;                           // Inicializar
    virtual void end() = 0;                             // Finalizar
    
    // Conectividade
    virtual bool connect() = 0;                         // Conectar
    virtual bool isConnected() = 0;                     // Verificar conexão
    virtual ConnectionState getConnectionState() = 0;   // Obter estado
    
    // Envio e confirmação
    virtual SendResult send(uint8_t port, 
                           const uint8_t* data, 
                           uint16_t length) = 0;        // Enviar dados
    virtual bool isConfirmed() = 0;                     // Verificar ACK
    
    // Recebimento
    virtual ReceiveResult receive(DownlinkMessage& msg) = 0;  // Receber
    
    // Processamento
    virtual void process() = 0;                         // Processar em background
    
    // Informações
    virtual const char* getStateString() = 0;          // Descrição do estado
};
```

---

## LoRaHandler - Comunicação LoRaWAN

### Localização

- **Header**: `include/comm/LoRaHandler.h`
- **Implementação**: `src/comm/LoRaHandler.cpp`

### Características

| Aspecto | Detalhes |
|---------|----------|
| **Hardware** | Robocore SMW_SX1262M0 (SX1262) |
| **Interface** | UART1 (GPIO5/RX, GPIO23/TX) |
| **Protocolo** | LoRaWAN OTAA (Over The Air Activation) |
| **Confirmação** | CFM (Uplink Confirmed) |
| **ADR** | Suportado (Adaptive Data Rate) |
| **Data Rates** | 0-7 (SF12 até SF7) |
| **Status** | ✅ 100% implementado |

### Configuração

```cpp
struct LoRaConfig {
    HardwareSerial* serial;           // &loraSerial
    const uint8_t* appEUI;            // 8 bytes da credencial
    const uint8_t* appKey;            // 16 bytes da credencial
    bool useConfirmation;             // false = não confirmado, true = confirmado
    bool useADR;                      // true = ADR automático
    uint8_t fixedDR;                  // 0-7 (se ADR = false)
    unsigned long joinTimeout;        // Ex: 10000 ms (10s)
    unsigned long confirmTimeout;     // Ex: 180000 ms (3 min)
    uint8_t maxRetries;               // Ex: 3 tentativas
};
```

### Exemplo de Uso

```cpp
#include "comm/LoRaHandler.h"
#include "comm/credentials.h"

// Criar serial para LoRa
HardwareSerial loraSerial(1);

// Configurar
LoRaConfig config = {
    .serial = &loraSerial,
    .appEUI = (const uint8_t*)APPEUI,
    .appKey = (const uint8_t*)APPKEY,
    .useConfirmation = false,
    .useADR = true,
    .fixedDR = 5,
    .joinTimeout = 10000,
    .confirmTimeout = 180000,
    .maxRetries = 3
};

// Instanciar
CommunicationHandler* commHandler = new LoRaHandler(config);

// Usar em setup()
void setup() {
    loraSerial.begin(115200, SERIAL_8N1, 5, 23);  // RX, TX
    
    if (!commHandler->begin()) {
        Serial.println("Erro ao inicializar LoRa");
        while(1) delay(1000);
    }
    
    if (!commHandler->connect()) {
        Serial.println("Erro ao conectar (OTAA Join failed)");
    }
}

// Usar em loop()
void loop() {
    commHandler->process();  // Processar eventos LoRa
    
    if (commHandler->isConnected()) {
        uint8_t payload[] = {0x01, 0x02, 0x03};
        SendResult result = commHandler->send(1, payload, 3);
        
        if (result == SendResult::SUCCESS) {
            Serial.println("Packet enfileirado");
            
            // Aguardar confirmação
            unsigned long start = millis();
            while (millis() - start < 6000) {
                commHandler->process();
                
                if (commHandler->isConfirmed()) {
                    Serial.println("ACK recebido!");
                    break;
                }
                delay(100);
            }
        }
    }
    
    delay(100);
}
```

### Métodos Adicionais

```cpp
// Obter DevEUI do módulo
bool getDevEUI(char* buffer);  // buffer mínimo 16 bytes

// Alterar configuração em runtime
bool setConfirmation(bool enabled);
bool setADR(bool enabled);
bool setDataRate(uint8_t dr);
```

---

## WiFiHandler - Comunicação Wi-Fi + Firebase

### Localização

- **Header**: `include/comm/WiFiHandler.h`
- **Implementação**: `src/comm/WiFiHandler.cpp`

### Características

| Aspecto | Detalhes |
|---------|----------|
| **Hardware** | ESP32 built-in Wi-Fi |
| **Interface** | Wi-Fi 802.11 b/g/n (2.4 GHz) |
| **Backend** | Firebase Realtime Database |
| **Protocolo** | HTTPS/REST API |
| **Confirmação** | HTTP 200 OK |
| **Encoding** | Base64 |
| **Status** | ✅ 100% implementado |

### Configuração

```cpp
struct WiFiConfig {
    const char* ssid;               // Nome da rede Wi-Fi
    const char* password;           // Senha Wi-Fi
    const char* apiKey;             // Firebase API Key
    const char* databaseUrl;        // URL do Firebase (ex: "seu-projeto.firebaseio.com")
    const char* deviceId;           // Identificador único (ex: "PENDIO_001")
    unsigned long connectTimeout;   // Timeout de conexão (ms)
};
```

### Exemplo de Uso

```cpp
#include "comm/WiFiHandler.h"
#include "comm/credentials.h"

// Configurar
WiFiConfig config = {
    .ssid = WIFI_SSID,
    .password = WIFI_PASSWORD,
    .apiKey = FIREBASE_API_KEY,
    .databaseUrl = FIREBASE_DB_URL,
    .deviceId = DEVICE_ID,
    .connectTimeout = 30000
};

// Instanciar
CommunicationHandler* commHandler = new WiFiHandler(config);

// Usar em setup()
void setup() {
    if (!commHandler->begin()) {
        Serial.println("Erro ao inicializar Wi-Fi");
        while(1) delay(1000);
    }
    
    if (!commHandler->connect()) {
        Serial.println("Erro ao conectar ao Wi-Fi");
    }
}

// Usar em loop()
void loop() {
    commHandler->process();
    
    if (commHandler->isConnected()) {
        uint8_t payload[] = {0x01, 0x02, 0x03};
        SendResult result = commHandler->send(1, payload, 3);
        
        if (result == SendResult::SUCCESS) {
            Serial.println("Dados enviados ao Firebase");
            
            if (commHandler->isConfirmed()) {
                Serial.println("HTTP 200 OK");
            }
        }
    }
    
    // Verificar downlinks
    DownlinkMessage msg;
    if (commHandler->receive(msg) == ReceiveResult::MESSAGE_RECEIVED) {
        Serial.printf("Downlink recebido na porta %d\n", msg.port);
        // Processar msg.data[0..msg.length-1]
    }
    
    delay(100);
}
```

### Estrutura de Dados no Firebase

Os dados são armazenados em:

```
/devices/{deviceId}/uplinks/
├─ {timestamp1}
│  ├─ port: 1
│  ├─ payload: "010AF3..." (Base64 do payload)
│  └─ timestamp: {unix_timestamp}
├─ {timestamp2}
│  └─ ...
└─ ...
```

---

## Seleção de Handler em main.cpp

```cpp
#include "comm/CommunicationHandler.h"
#include "comm/LoRaHandler.h"
#include "comm/WiFiHandler.h"
#include "comm/credentials.h"

CommunicationHandler* commHandler = nullptr;

void setup() {
    Serial.begin(115200);
    
    #ifdef COMMUNICATION_MODE_WIFI
        // Modo Wi-Fi + Firebase
        WiFiConfig wifiConfig = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .apiKey = FIREBASE_API_KEY,
            .databaseUrl = FIREBASE_DB_URL,
            .deviceId = DEVICE_ID,
            .connectTimeout = 30000
        };
        commHandler = new WiFiHandler(wifiConfig);
    
    #else
        // Modo LoRaWAN (padrão)
        HardwareSerial loraSerial(1);
        LoRaConfig loraConfig = {
            .serial = &loraSerial,
            .appEUI = (const uint8_t*)APPEUI,
            .appKey = (const uint8_t*)APPKEY,
            .useConfirmation = false,
            .useADR = LORA_ADR_ON,
            .fixedDR = LORA_FIXED_DR,
            .joinTimeout = JOIN_TIMEOUT_VALUE,
            .confirmTimeout = CFM_TIMEOUT_VALUE,
            .maxRetries = 3
        };
        commHandler = new LoRaHandler(loraConfig);
    #endif
    
    // O resto do código é agnóstico!
    if (!commHandler->begin()) {
        Serial.println("Erro ao inicializar comunicação");
        while(1) delay(1000);
    }
    
    if (!commHandler->connect()) {
        Serial.println("Erro ao conectar");
    }
}
```

---

## Implementando um Novo Handler

### Passo 1: Criar Header

```cpp
// include/comm/MeuHandler.h
#ifndef _MEU_HANDLER_H
#define _MEU_HANDLER_H

#include "CommunicationHandler.h"

struct MeuConfig {
    // Configurações específicas
    unsigned long timeout;
    const char* servidor;
};

class MeuHandler : public CommunicationHandler {
private:
    MeuConfig config;
    ConnectionState currentState;
    bool _isConfirmed;
    DownlinkMessage lastMessage;
    
public:
    explicit MeuHandler(const MeuConfig& cfg);
    ~MeuHandler() override = default;
    
    bool begin() override;
    void end() override;
    bool connect() override;
    bool isConnected() override;
    SendResult send(uint8_t port, const uint8_t* data, uint16_t length) override;
    bool isConfirmed() override;
    ReceiveResult receive(DownlinkMessage& message) override;
    ConnectionState getConnectionState() override;
    void process() override;
    const char* getStateString() override;
    
private:
    void updateState();
};

#endif
```

### Passo 2: Implementar CPP

```cpp
// src/comm/MeuHandler.cpp
#include "comm/MeuHandler.h"
#include "utils/Logger.h"

MeuHandler::MeuHandler(const MeuConfig& cfg)
    : config(cfg),
      currentState(ConnectionState::DISCONNECTED),
      _isConfirmed(false) {
    memset(&lastMessage, 0, sizeof(lastMessage));
}

bool MeuHandler::begin() {
    LOGI("MeuHandler", "Inicializando...");
    currentState = ConnectionState::DISCONNECTED;
    return true;
}

void MeuHandler::end() {
    LOGI("MeuHandler", "Finalizando");
    currentState = ConnectionState::DISCONNECTED;
}

bool MeuHandler::connect() {
    LOGI("MeuHandler", "Conectando a %s...", config.servidor);
    currentState = ConnectionState::CONNECTED;
    return true;
}

bool MeuHandler::isConnected() {
    return currentState == ConnectionState::CONNECTED;
}

SendResult MeuHandler::send(uint8_t port, const uint8_t* data, uint16_t length) {
    if (!isConnected()) {
        return SendResult::NOT_CONNECTED;
    }
    
    LOGI("MeuHandler", "Enviando %d bytes na porta %d", length, port);
    _isConfirmed = true;
    return SendResult::SUCCESS;
}

bool MeuHandler::isConfirmed() {
    return _isConfirmed;
}

ReceiveResult MeuHandler::receive(DownlinkMessage& message) {
    return ReceiveResult::NO_MESSAGE;
}

ConnectionState MeuHandler::getConnectionState() {
    return currentState;
}

void MeuHandler::process() {
    // Processar eventos
}

const char* MeuHandler::getStateString() {
    switch(currentState) {
        case ConnectionState::DISCONNECTED: return "DESCONECTADO";
        case ConnectionState::CONNECTING: return "CONECTANDO";
        case ConnectionState::CONNECTED: return "CONECTADO";
        case ConnectionState::WAITING_CONFIRMATION: return "AGUARDANDO CFM";
        case ConnectionState::ERROR: return "ERRO";
        default: return "DESCONHECIDO";
    }
}

void MeuHandler::updateState() {
    // Lógica de atualização de estado
}
```

### Passo 3: Usar em main.cpp

```cpp
#ifdef COMMUNICATION_MODE_MEU
    MeuConfig cfg = {
        .timeout = 5000,
        .servidor = "meu.servidor.com"
    };
    commHandler = new MeuHandler(cfg);
#endif
```

---

## Tratamento de Erros

### Padrão Recomendado

```cpp
ConnectionState state = commHandler->getConnectionState();

if (state == ConnectionState::ERROR) {
    LOGW("Main", "Erro na comunicação: %s", commHandler->getStateString());
    
    // Tentar reconectar
    if (commHandler->connect()) {
        LOGI("Main", "Reconectado com sucesso");
    } else {
        LOGE("Main", "Falha ao reconectar");
    }
} else if (state == ConnectionState::CONNECTED) {
    // Enviar dados
} else if (state == ConnectionState::WAITING_CONFIRMATION) {
    // Aguardar confirmação
}
```

---

## Recursos Adicionais

- [ARCHITECTURE.md](./ARCHITECTURE.md) - Diagrama da arquitetura
- [CONFIG_GUIDE.md](./CONFIG_GUIDE.md) - Configurações do sistema

---
