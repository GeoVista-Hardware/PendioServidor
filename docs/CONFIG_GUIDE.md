# ⚙️ Guia de Configuração - Pendio

Referência completa das configurações do sistema Pendio definidas em `include/system_definitions.h`.

---

## 📋 Localização das Configurações

```
include/system_definitions.h      ← PRINCIPAL (edite aqui)
include/hardware_definitions.h    ← Hardware (raramente muda)
include/comm/credentials.h        ← AppEUI, AppKey, Firebase (git ignored)
platformio.ini                    ← Ambiente de build
```

---

## 🎯 Seção 1: Modo de Operação

### Selecionar Comunicação

```cpp
// Descomente para ativar modo Wi-Fi + Firebase (prototipagem)
// Comente para usar LoRaWAN (padrão - produção)
#define COMMUNICATION_MODE_WIFI
```

| Modo | Config | Uso | Consumo |
|------|--------|-----|---------|
| **LoRaWAN** | Comentado | Produção em campo | Muito baixo |
| **Wi-Fi + Firebase** | Descomentado | Desenvolvimento/Testes | Alto |

---

## 📊 Seção 2: Logging

```cpp
// Ativar ou desativar logging
#define ENABLE_LOGGING              1       // 1=ativo, 0=inativo

// Nível padrão de logging
#define LOG_LEVEL_DEFAULT           LOG_LEVEL_INFO
// Valores: LOG_LEVEL_DEBUG (0), LOG_LEVEL_INFO (1), 
//          LOG_LEVEL_WARN (2), LOG_LEVEL_ERROR (3)
```

### Exemplo de Output

```
[00:01:23.456] [INFO][SYSTEM] Sistema iniciado
[00:02:45.789] [WARN][COMM] Tentando rejoin...
[00:03:12.345] [ERROR][SENSOR] Erro ao ler AHT10
```

---

## 🔌 Seção 3: LoRaWAN - Timeouts

| Configuração | Valor | Unidade | Descrição |
|--------------|-------|---------|-----------|
| `JOIN_TIMEOUT_VALUE` | 10000 | ms | Timeout para OTAA Join |
| `CFM_TIMEOUT_VALUE` | 180000 | ms | Aguardar ACK (3 minutos) |
| `NEXT_MSG_TIMEOUT_VALUE` | 20000 | ms | Intervalo entre mensagens |

### Cenários Comuns

#### Teste/Desenvolvimento
```cpp
#define JOIN_TIMEOUT_VALUE      10000       // 10s
#define CFM_TIMEOUT_VALUE       6000        // 6s
#define NEXT_MSG_TIMEOUT_VALUE  20000       // 20s entre mensagens
```
✅ Join rápido, testes frequentes

#### Produção/Campo
```cpp
#define JOIN_TIMEOUT_VALUE      30000       // 30s
#define CFM_TIMEOUT_VALUE       180000      // 3 min (respire de bateria)
#define NEXT_MSG_TIMEOUT_VALUE  1800000     // 30 min (duty cycle)
```
✅ Economiza bateria, respeita duty cycle (1% airtime)

---

## 📡 Seção 4: LoRaWAN - Transmissão

```cpp
// Data Rate fixo (0-7), se ADR desabilitado
#define LORA_FIXED_DR                 5

// Usar Adaptive Data Rate
#define LORA_ADR_ON                   1       // 1=ativo, 0=inativo

// Usar confirmação (pedir ACK ao gateway)
#define LORA_USE_CONFIRMATION         0       // 1=ativo, 0=inativo

// Máximo de retentativas de envio (NACK)
#define LORA_MAX_NACK_RETRIES         9
```

### Data Rates Explicado

| DR | SF | BW | Throughput | Alcance | Velocidade |
|----|----|----|-----------|---------|-----------|
| 0 | 12 | 125 | Mínimo | **Máximo** | Lenta |
| 2 | 10 | 125 | Médio | Bom | Médio |
| **5** | **7** | **125** | **Alto** | Mínimo | **Rápida** |
| 7 | 7 | 250 | Máximo | Mínimo | Máxima |

**Recomendação**: Use DR=5 para testes locais, DR=0-2 para campo.

### Confirmação (CFM)

```cpp
#define LORA_USE_CONFIRMATION  0    // Não confirmado (mais rápido)
#define LORA_USE_CONFIRMATION  1    // Confirmado (garante entrega)
```

| Config | Vantagem | Desvantagem |
|--------|----------|------------|
| **CFM=0** | Rápido, economiza bateria | Pode perder pacote |
| **CFM=1** | Garantia de entrega | Mais lento, mais consumo |

---

## 🌡️ Seção 5: Sensores

```cpp
// Habilitar ou desabilitar sensores instalados
#define SENSOR_AHT_ENABLED          1       // Temperatura/Umidade
#define SENSOR_BMP_ENABLED          1       // Pressão
#define SENSOR_SPENDIO_ENABLED      1       // Sensores RS485
#define SENSOR_RAIN_ENABLED         1       // Pluviômetro
#define SENSOR_BATTERY_ENABLED      1       // Monitor de bateria
```

**Se sensor não está instalado**: Mude para `0` para evitar erros de inicialização.

---

## 📌 Seção 6: Pinos e Hardware

```cpp
// LED de status (GPIO2 é o LED interno do Wemos)
#define PIN_LED                     2

// Serial para comunicação com LoRa (UART1)
#define LORA_SERIAL_PORT            1
// RX=GPIO5, TX=GPIO23 (configurados em hardware_definitions.h)

// Potência TX do módulo LoRa
#define LORA_TX_POWER               20      // dBm (2-20)
```

**Não altere sem consultar [docs/HARDWARE.md](./HARDWARE.md)**

---

## 🎓 Casos Práticos de Ajuste

### 1️⃣ Modo Teste (Desenvolvimento Local)

```cpp
#define COMMUNICATION_MODE_WIFI         // Use Wi-Fi
#define ENABLE_LOGGING              1
#define LOG_LEVEL_DEFAULT           LOG_LEVEL_DEBUG

#define LORA_FIXED_DR               5   // DR rápido
#define LORA_ADR_ON                 1
#define LORA_USE_CONFIRMATION       0   // Sem CFM
#define NEXT_MSG_TIMEOUT_VALUE      20000   // 20s
```

✅ Muitos logs, mensagens frequentes, sem confirmação

---

### 2️⃣ Modo Produção (Wi-Fi + Firebase)

```cpp
#define COMMUNICATION_MODE_WIFI         // ✅ Use este modo
#define ENABLE_LOGGING              1
#define LOG_LEVEL_DEFAULT           LOG_LEVEL_INFO

// Configurações LoRa são ignoradas neste modo
```

✅ Dados em tempo real no Firebase, testes rápidos

---

### 3️⃣ Modo Produção (Campo com LoRa)

```cpp
// #define COMMUNICATION_MODE_WIFI   // ❌ Comentado
#define ENABLE_LOGGING              1
#define LOG_LEVEL_DEFAULT           LOG_LEVEL_INFO

#define LORA_FIXED_DR               0    // DR longo alcance
#define LORA_ADR_ON                 1    // ADR automático
#define LORA_USE_CONFIRMATION       1    // Confirmar entrega
#define CFM_TIMEOUT_VALUE           180000  // 3 min
#define NEXT_MSG_TIMEOUT_VALUE      1800000 // 30 min
#define LORA_MAX_NACK_RETRIES       9
```

✅ Máxima economia de bateria, confirmação de entrega, logs mínimos

---

### 4️⃣ Sensor Específico Ausente

Se o **BMP280 não está instalado**:

```cpp
#define SENSOR_BMP_ENABLED          0   // Desabilitar
```

O sistema ignora erros de inicialização e continua funcionando.

---

### 5️⃣ Aumentar Confirmação (Garantia de Entrega)

```cpp
#define LORA_USE_CONFIRMATION       1        // Pedir ACK
#define CFM_TIMEOUT_VALUE           180000   // Aguardar 3 min
#define LORA_MAX_NACK_RETRIES       9        // 9 tentativas
```

⚠️ Aumenta consumo de energia. Use apenas se crítico.

---

## 🔑 Credenciais (credentials.h)

**Nunca** coloque credenciais em `system_definitions.h`. Use arquivo separado:

```cpp
// include/comm/credentials.h (git ignored)

// LoRaWAN
const char APPEUI[] = "26e7cc9af428bec1";
const char APPKEY[] = "cfeebad46ac8638d69fa23c5789926f3";

// Wi-Fi + Firebase (opcional)
const char WIFI_SSID[] = "minha_rede";
const char WIFI_PASSWORD[] = "minha_senha";
const char FIREBASE_API_KEY[] = "AIzaSyD...";
const char FIREBASE_DB_URL[] = "meu-projeto.firebaseio.com";
const char DEVICE_ID[] = "PENDIO_001";
```

---

## ✅ Validação de Configuração

Ao compilar, o sistema valida automaticamente:

```cpp
#if LORA_FIXED_DR < 0 || LORA_FIXED_DR > 7
    #error "LORA_FIXED_DR deve estar entre 0 e 7"
#endif

#if CFM_TIMEOUT_VALUE < 1000
    #error "CFM_TIMEOUT_VALUE deve ser >= 1000 ms"
#endif
```

**Se erro de compilação**: Ajuste `system_definitions.h` e tente novamente.

---

## 🔗 Referências

- [ARCHITECTURE.md](./ARCHITECTURE.md) - Como a arquitetura funciona
- [COMMUNICATION_HANDLERS.md](./COMMUNICATION_HANDLERS.md) - Detalhes dos handlers
- [HARDWARE.md](./HARDWARE.md) - Mapeamento de pinos
- [PROTOCOLO.md](./PROTOCOLO.md) - Formato de payload

---
