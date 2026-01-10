# 📡 Pendio - Monitoramento de Taludes com LoRaWAN

Este repositório contém o firmware para o sistema de monitoramento de taludes e encostas "Pendio", baseado na placa Wemos D1 R32 (ESP32) e no módulo LoRaWAN Robocore (SMW_SX1262M0).

---

## 📝 Sobre o Projeto

O sistema realiza a leitura de diversos sensores e envia os dados consolidados via rede LoRaWAN.

- **Versão do SW**: WRCPendio Wemos Robocore CPendio (10/01/2024)
- **Autores**: Eng. Nuncio Perrella, MSc e Arnaldo

---

## ⚡ Hardware Principal

- **MCU**: Wemos D1 R32 (ESP32)
- **LoRaWAN**: Módulo Robocore SMW_SX1262M0
- **Sensores**:
    - Sensores SPendio (comunicação RS485)
    - Sensor de Temp/Umidade (AHT10/AHT20)
    - Sensor de Pressão/Temp (BMP280)
    - Sensor de Chuva (Contacto seco)
    - Monitor de Bateria (Divisor de tensão)

```mermaid
graph LR
    A["ESP32<br/>(DOIT V1)"] --> B["SMW_SX1262M0<br/>(LoRaWAN)"]
    A --> C["I2C"]
    C --> D["AHT10/20<br/>(Temp/Umidade)"]
    C --> E["BMP280<br/>(Pressão)"]
    A --> F["RS485"]
    F --> G["SPendio"]
    A --> H["GPIO<br/>(Chuva)"]
    A --> I["ADC<br/>(Bateria)"]
```

Para um detalhamento completo do mapeamento de pinos, consulte [➡️ docs/HARDWARE.md](docs/HARDWARE.md).

A descrição completa do formato da mensagem enviada via LoRaWAN está documentada em [➡️ docs/PROTOCOLO.md](docs/PROTOCOLO.md).

--- 

## 📁 Estrutura

```
PendioServidor/
├── include/           # Headers (.h)
├── src/               # Implementações (.cpp)
├── lib/               # Bibliotecas externas
├── docs/              # HARDWARE, PROTOCOLO
├── README.md          # Este arquivo
├── QUICK_START.md     # Primeiros passos
├── ARCHITECTURE.md    # Design de handlers
├── HANDLERS.md        # Guia de implementação
└── INTEGRATION_CHECKLIST.md
```

---

## 📖 Documentação

| Arquivo | Informações |
|---------|---------|
| [**docs/ARCHITECTURE.md**](./docs/ARCHITECTURE.md) | Entender o design |
| [**docs/HANDLERS.md**](./docs/HANDLERS.md)| Usar/estender handlers |
| [**docs/HARDWARE.md**](./docs/HARDWARE.md)| Pinos e conexões |
| [**docs/PROTOCOLO.md**](./docs/PROTOCOLO.md) | Formato de mensagens |

---

## 👾 Upload do Projeto 

Instale o Visual Studio Code com a extensão PlatformIO e clone este repositório:

```bash
git clone https://github.com/Nyfeu/PendioServidor.git
```

Crie o ficheiro `include/credentials.h` com as chaves LoRaWAN corretas (ver `include/credentials.h.exemplo`).

Use o ambiente PIO (PlatformIO) para compilar e gravar o firmware no hardware.

```bash
git clone https://github.com/Nyfeu/PendioServidor.git
cd PendioServidor
cp include/credentials.example.h include/credentials.h
# Editar include/credentials.h com suas chaves LoRa
platformio run                  # Compilar
platformio run --target upload  # Upload
platformio device monitor       # Monitor Serial
```

---

## 🧾 Histórico de Instalações e Gravações

| Unidade | Descrição |
|---------|-----------|
| Pendio 1 | Sistema de Testes POLI Civil - Kaiene |
| Pendio 2 | Caixa de testes - Geólogos _ Igor |
| Pendio 3 | Arnaldo |
| Pendio 4 | USP |
| Pendio 5 | A ser instalado |
| Pendio 6 | A ser instalado (Teste Nuncio 14/11/2024) |
| Pendio 7 | Raia Olimpica USP |
| Pendio 8 | Raia Olimpica USP |
| Pendio 9 | Sensor 14/11/2024 |

## 🗝️ Configuração de Credenciais

### Estrutura do Arquivo `credentials.h`

O projeto utiliza um arquivo centralizado para todas as credenciais sensíveis:

```c
// LoRaWAN (produção)
const char APPEUI[] = "Seu AppEUI aqui";    // 16 caracteres HEX
const char APPKEY[] = "Sua AppKey aqui";    // 32 caracteres HEX

// Wi-Fi + Firebase (modo protótipo)
const char WIFI_SSID[] = "Nome da rede";
const char WIFI_PASSWORD[] = "Senha da rede";
const char FIREBASE_API_KEY[] = "Sua API Key";
const char FIREBASE_DB_URL[] = "seu-projeto.firebaseio.com";
const char DEVICE_ID[] = "ESP32_PENDIO_01";  // Identificador único
```

### Passo a Passo para Configurar

1. **Copiar o template:**
   ```bash
   cp include/credentials.example.h include/credentials.h
   ```

2. **Editar `include/credentials.h` com seus valores.**

3. **Verificar `.gitignore`:**
   Certifique-se que `credentials.h` está na lista de ignorados:
   ```
   include/credentials.h
   ```

### Histórico de Chaves LoRaWAN

O histórico das chaves de produção pode ser consultado em [➡️ docs/CHAVES.md](docs/CHAVES.md).

--- 
