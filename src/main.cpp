/* Projeto Pendio (Monitoramento de Taludes) **********************************************

Software Release para Campo (Produção)
Eng. Nuncio Perrella, MSc
Data:  26 de  Abril 2025  

******************************************************************************************/

/**
 * @file main.cpp
 * @brief Firmware principal do sistema de monitoramento Pendio (ESP32 + LoRaWAN/Wi-Fi).
 * @details Gerencia a máquina de estados, leitura de sensores e telemetria.
 * @author Eng. Nuncio Perrella, MSc
 * @author Eng. Arnaldo
 * @author Eng. André Maiolini
 * @copyright Copyright (c) 2025
 */

#define MAIN

// Headers Principais do Projeto

#include <Arduino.h>
#include <HardwareSerial.h>
#include <EEPROM.h>

// Headers de Configuração do Projeto

#include "aplic.h"
#include "config.h"
#include "credentials.h"

// Headers de Comunicação

#include "CommunicationHandler.h"
#include "LoRaHandler.h"
#include "WiFiHandler.h"
#include "Logger.h"

//*****************************************************************************************
//  SELETOR DE MODO DE OPERAÇÃO
//*****************************************************************************************

// Descomente a linha abaixo para ativar o modo Protótipo (Wi-Fi + Firebase)
// Comente para compilar a versão final (LoRaWAN)
#define PROTOTYPE_MODE_WIFI 

//*****************************************************************************************
//  DEFINIÇÕES GLOBAIS, CONSTANTES E VARIÁVEIS
//*****************************************************************************************

// Interface Serial (Serial1 para o Módulo LoRa)
HardwareSerial loraSerial(1);

// Configuração do LoRa Handler
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

#ifdef PROTOTYPE_MODE_WIFI

  // Configuração do Wi-Fi/Firebase Handler (credenciais em credentials.h)
  WiFiConfig wifiConfig = {
      .ssid = WIFI_SSID,
      .password = WIFI_PASSWORD,
      .apiKey = FIREBASE_API_KEY,
      .databaseUrl = FIREBASE_DB_URL,
      .deviceId = DEVICE_ID,
      .connectTimeout = 30000
  };

#endif

// Instância do handler de comunicação (Polimorfismo: aceita LoRa ou Wi-Fi)
CommunicationHandler* commHandler = nullptr; 

// Estrutura de Dados dos Sensores (Definida em Sensores.h/Aplic.h)
CPendio_LoRa_Sensor_Data_Type CPendio_LoRa_Sensor_Data;

// RAM variable to store the EEPROM Value stored in position 0
uint8_t NVM_LoRaWAN_Cycle_Time = 0;                              

// RAM variable to store the EEPROM Value stored in the LSbit of position 1
bool NVM_LoRaWAN_Use_Cfm = false;       

// Variáveis de Controle de Tempo
unsigned long timeout   = 0;
unsigned long timenow   = 0;
unsigned long timecycle = 0;

// Variáveis de Controle LoRa
bool joined     = false;
int nack_count  = 0;              // Contador de não-confirmações (NACK)
int err_count   = 0;              // Contador de exceções sequenciais

// Variável de Estado do LED
int LedState = LOW;

/* Estados da Máquina de Estados Principal ---------------------------------------*/

// Definição dos estados
enum SystemState {
    STATE_NOT_JOINED = 0,          // Aguardando conexão com a rede (Join Accept)
    STATE_READY,                   // Conectado, pronto para leitura e envio
    STATE_WAIT_CFM                 // Pacote enviado, aguardando ACK/Downlink
};

// Inicializa a variável de estado
uint16_t State = STATE_NOT_JOINED; 

/* Configurações de Erro e Retentativa -------------------------------------------*/

constexpr int ERROR_RESTART   = 0; // Limpa erros (reinicia o contador)
constexpr int ERROR_LORAWAN   = 1; // Erro de comunicação no LoRaWAN
constexpr int RESTART_REQUEST = 2; // Solicitação remota de reinício (imediata)
constexpr int ERROR_MAX_SEQ   = 5; // Máximo de erros antes do reset forçado

// ---------------------------------------------------------------------------
// Protótipos - funções auxiliares (assinam com as implementações abaixo)
// ---------------------------------------------------------------------------

void ToggleLed(void);
void exception_handling(int Exception_code);
uint8_t Validate_Cycle_Time(uint8_t ct);

// Ponteiro para a função de reset (software)

void (*reset_function)(void) = 0;

//*****************************************************************************************
//  IMPLEMENTAÇÃO
//*****************************************************************************************
                                                
/**
 * @brief Alterna o estado do LED de status.
 */
void ToggleLed(void) {
  LedState = !LedState; 
  digitalWrite(MODULE_LED_PIN,LedState);
}

/**
 * @brief Gerencia erros críticos e reinícios do sistema.
 * @param Exception_code Código do erro (ERROR_RESTART, ERROR_LORAWAN, etc).
 */
void exception_handling(int Exception_code) {

  switch (Exception_code) {
    case ERROR_RESTART:
      // Sucesso: Zera contador de erros
      err_count = 0;
      break;

    case ERROR_LORAWAN:
      // Processa um erro adicional LoRaWAN
      LOGW("SYSTEM", "Error Code: %d", Exception_code);
      err_count++;
      // Caso o contador de erros exceder o limite de erros consecutivos, força reinício
      if (err_count > ERROR_MAX_SEQ) {
        LOGE("SYSTEM", "Forced Reset in 30s due to repeated LoRa errors");
        delay(30000);
        reset_function();
      }
      break;

    case RESTART_REQUEST:
      LOGW("SYSTEM", "Immediate Reset Requested - rebooting in 30s");
      delay(30000);
      reset_function();
      break;

    default:
      LOGW("SYSTEM", "Undefined Exception - ignored (%d)", Exception_code);
      break;
  }
}

/**
 * @brief Valida o tempo de ciclo lido da EEPROM.
 * @param ct Tempo em minutos.
 * @return uint8_t Tempo validado.
 */
uint8_t Validate_Cycle_Time(uint8_t ct) {
  unsigned char ret;
  switch (ct) {
    case 0: ret = 1; break; // Modo Debug (1 min)
    case 5: case 10: case 15: case 30: case 60: ret = ct; break;
    default: ret = 15; break; // Padrão (15 min)
  }
  #ifdef USE_EEPROM
    EEPROM.update(0,ret); // Cycle time must be store in EEPROM.
  #endif
    return(ret);
}

// --------------------------------------------------
// - Validate (and Store) Settings (BYTE 1 of EEPROM)
// --------------------------------------------------
uint8_t Validate_Settings(uint8_t st)
{
  unsigned char ret = st;
#ifdef USE_EEPROM
  EEPROM.update(1,ret); // Settings must be updated in EEPROM.
#endif
  return(ret);
}

//*****************************************************************************************
//  SETUP
//*****************************************************************************************
void setup() {
  // 1. Inicialização do Hardware Básico

  // Configura os pinos (HW.cpp)
  iniHW();                                   

  // Inicializa o LED da placa LoRaWAN
  pinMode(MODULE_LED_PIN,OUTPUT); 
  ToggleLed();

  // 2. Inicialização das Interfaces Seriais

  // Inicializa logger (Serial)
  Logger::begin(115200);
  
  // Comunicação UART para o módulo LoRa
  loraSerial.begin(9600, SERIAL_8N1, RXD1_LoRa, TXD1_LoRa);

  // Comunicação UART para os sensores SPendio (RS485)
  Serial2.begin(4800, SERIAL_8N1, RXD2_RS485, TXD2_RS485); // RS485
  Serial2.setRxBufferSize(64);
  Serial2.setTimeout(100);

  // 3. Inicialização dos Sensores I2C
  
  // Sensor AHT (Temp/Umid)
  if (!aht.begin()) LOGE("SENSOR", "AHT10/20 não encontrado. Verifique conexões.");
  else LOGI("SENSOR", "AHT10/20 detectado");

  // Sensor BMP (Pressão)
  if (!bmp.begin(END_BMP)) {
    LOGE("SENSOR", "BMP280 não encontrado");
    g_bBMPPresente = false;
  } else {
    LOGI("SENSOR", "BMP280 detectado");
    bmp.setSampling(
      Adafruit_BMP280::MODE_NORMAL,     // Operating Mode. 
      Adafruit_BMP280::SAMPLING_X2,     // Temp. oversampling 
      Adafruit_BMP280::SAMPLING_X16,    // Pressure oversampling 
      Adafruit_BMP280::FILTER_X16,      // Filtering. 
      Adafruit_BMP280::STANDBY_MS_500   // Standby time. 
    ); 
    g_bBMPPresente = true;
  }

  // Delay para estabilização
  delay(1000);

  // 4. Mensagem de Boas-vindas
  LOGI("SYSTEM", "=== PENDIO SERVIDOR - INICIANDO ===");
  LOGI("SYSTEM", "Versão: %s", Versao);
  LOGI("SYSTEM", "Data: %s", Data);
  
  // Inicializa estruturas de dados dos sensores
  iniSensores(CPendio_LoRa_Sensor_Data.d);

  // 5. Configuração do Handler de Comunicação
  LOGI("COMM", "Inicializando handler de comunicação...");
  LOGI("COMM", "Frame size: %u", (unsigned)sizeof(CPendio_LoRa_Sensor_Data));

  // --- LÓGICA DE SELEÇÃO DE MODO (LoRa vs WiFi) ---
  #ifdef PROTOTYPE_MODE_WIFI

    LOGI("COMM", "MODO PROTOTIPO ATIVO: Inicializando Wi-Fi + Firebase...");
    commHandler = new WiFiHandler(wifiConfig);

    // Ajustes opcionais para protótipo
    NVM_LoRaWAN_Cycle_Time = 1; // Força ciclo rápido para testes

  #else

    LOGI("COMM", "MODO PRODUCAO: Inicializando LoRaWAN...");
    // Carrega informações da EEPROM
    #ifdef USE_EEPROM
      NVM_LoRaWAN_Cycle_Time = EEPROM.read(0);
      NVM_LoRaWAN_Use_Cfm = (NVM_SETTINGS_CFM_BIT == (EEPROM.read(1) & NVM_SETTINGS_CFM_BIT));
    #else
      NVM_LoRaWAN_Cycle_Time = 0;
      NVM_LoRaWAN_Use_Cfm = true;
    #endif
    NVM_LoRaWAN_Cycle_Time = Validate_Cycle_Time(NVM_LoRaWAN_Cycle_Time);

    // Atualizar configuração com valores da EEPROM
    loraConfig.useConfirmation = NVM_LoRaWAN_Use_Cfm;

    // Criar instância do handler LoRa
    commHandler = new LoRaHandler(loraConfig);

  #endif

  // Inicializar handler (comum para ambos)
  if (!commHandler->begin()) {
    LOGE("COMM", "Falha ao inicializar handler de comunicação");
    while(1) { delay(1000); }
  }

  // --- BLOCO ESPECÍFICO LORAWAN (DevEUI) ---
  #ifndef PROTOTYPE_MODE_WIFI

    // O método getDevEUI não existe na interface genérica, então se precisa 
    // garantir que só é rodado se for LoRaHandler
    // Cast seguro pois sabemos que estamos no #else do modo WiFi
    LoRaHandler* loraSpecific = static_cast<LoRaHandler*>(commHandler);
    
    char deveui[16];
    if (loraSpecific->getDevEUI(deveui)) {
      char hexstr[33];
      for (int i = 0; i < 16; ++i) {
        sprintf(&hexstr[i*2], "%02X", (uint8_t)deveui[i]);
      }
      hexstr[32] = '\0';
      LOGI("COMM", "DevEUI: %s", hexstr);
    }
  #endif

  // Inicia Conexão (JOIN no LoRa ou Connect WiFi)
  delay(500);
  ToggleLed();
  LOGI("COMM", "Iniciando conexão de rede...");
  commHandler->connect();

  // Define TIMERS iniciais
  timeout = millis() + JOIN_TIMEOUT_VALUE; // Timeout para o processo de Join
  timecycle = JOIN_TIMEOUT_VALUE;          // Timecycle para comparação posterior
}

//*****************************************************************************************
//  LOOP (O Loop é agnóstico, funciona igual para os dois modos)
//*****************************************************************************************
void loop() {
  DownlinkMessage downlink;

  timenow = millis();     // sample running time only here for all uses
  
  if(((unsigned long)(timeout - timenow))>((unsigned long)(-timecycle))) {
    switch(State) {
      case STATE_NOT_JOINED:          // IF NOT JOINED YET...
        if(commHandler->isConnected()) {
          if(!joined){ LOGI("COMM", "Rede Conectada!"); joined = true; }
          State = STATE_READY;
        } else {
          LOGI("COMM", "Tentando conectar novamente...");
          commHandler->connect();
        }
        timecycle = JOIN_TIMEOUT_VALUE;
      break;

      case STATE_READY:               // IF ALREADY JOINED OR TX + RX COMPLETE...
        // Leitura dos Sensores
        varrSensores(CPendio_LoRa_Sensor_Data.d);     // Varre Sensores
        nack_count = 0;

        // Enviar dados
        {
          LOGD("COMM", "Payload size: %u", (unsigned)sizeof(CPendio_LoRa_Sensor_Data));

          SendResult sendResult = commHandler->send(1, (const uint8_t*)CPendio_LoRa_Sensor_Data.Bytes,
                                                    sizeof(CPendio_LoRa_Sensor_Data));

          if(sendResult == SendResult::SUCCESS) {
            State = STATE_WAIT_CFM;
            timecycle = CFM_TIMEOUT_VALUE;
            timenow = millis();
            LOGI("COMM", "Envio aceito pelo Handler");
            exception_handling(ERROR_RESTART);
          }
          else if(sendResult == SendResult::PENDING) {
            LOGW("COMM", "Envio pendente");
          }
          else {
            State = STATE_NOT_JOINED;
            timecycle = JOIN_TIMEOUT_VALUE;
            LOGE("COMM", "Envio negado/falha - reiniciando conexão");
            exception_handling(ERROR_LORAWAN);
          }
        }
      break;

      case STATE_WAIT_CFM:
        // Verifica confirmação (ACK)
        // No modo Firebase, isso é quase instantâneo (HTTP OK)
        // No modo LoRa, espera o RX1/RX2
        if (commHandler->isConfirmed()) {
            LOGI("COMM", "Confirmação Recebida (ACK)");
            exception_handling(ERROR_RESTART);
            
            // Verifica se tem mensagem de descida (Downlink)
            if(commHandler->receive(downlink) == ReceiveResult::MESSAGE_RECEIVED) {
                LOGI("COMM", "Downlink recebido! (port=%u)", (unsigned)downlink.port);
                // ... Lógica de processamento de downlink (mantida igual) ...
                if (downlink.length >= 5 && downlink.data[0] == '8') {
                    // Exemplo: Atualizar tempo de ciclo
                    if (downlink.data[1] == '0') {
                        NVM_LoRaWAN_Cycle_Time = (downlink.data[2]-'0')*16 + (downlink.data[3]-'0');
                        LOGI("COMM", "Novo Ciclo: %u min", NVM_LoRaWAN_Cycle_Time);
                    }
                }
                ToggleLed();
            }
            
            State = STATE_READY;
            // Define o tempo para o próximo envio (Ciclo)
            // Convertendo minutos da EEPROM para milissegundos
            unsigned long cicloMs = (unsigned long)NVM_LoRaWAN_Cycle_Time * 60000;
            if (cicloMs == 0) cicloMs = 60000; // Mínimo 1 min
            
            timecycle = cicloMs; 
            LOGI("SYSTEM", "Dormindo por %lu ms...", timecycle);

        } else {
            // Se estourar o tempo sem ACK
            LOGW("COMM", "Sem ACK - Timeout");
            // Lógica de retentativa ou volta para READY
            // ... (simplificado aqui para manter a lógica original)
             nack_count++;
             if (nack_count > 3) { // Exemplo
                State = STATE_READY;
                nack_count = 0;
             }
             timecycle = 5000; // Tenta de novo em 5s
        }
      break;

      default:
        State = STATE_NOT_JOINED;
        timecycle = JOIN_TIMEOUT_VALUE;
        exception_handling(ERROR_LORAWAN);
      break;
    }
    
    // Processamento contínuo (necessário para o Firebase manter token vivo)
    commHandler->process();
    
    timeout = timenow + timecycle;
  }
}