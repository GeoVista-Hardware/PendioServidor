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

// Headers Principais
#include <Arduino.h>
#include <HardwareSerial.h>
#include <EEPROM.h>

// Headers de Configuração
#include "system_definitions.h"
#include "comm/credentials.h"
#include "utils/Logger.h"
#include "hardware/Sensores.h"

// Headers do Sistema
#include "core/system_utils.h"
#include "core/system_init.h"
#include "core/state_machine.h"

// Headers de Comunicação
#include "comm/CommunicationHandler.h"
#include "comm/LoRaHandler.h"
#include "comm/WiFiHandler.h"

//*****************************************************************************************
//  DEFINIÇÕES GLOBAIS E VARIÁVEIS
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

#ifdef COMMUNICATION_MODE_WIFI

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

// Instância do handler de comunicação
CommunicationHandler* commHandler = nullptr;

// Estrutura de Dados dos Sensores
CPendio_LoRa_Sensor_Data_Type CPendio_LoRa_Sensor_Data;

// Configurações da EEPROM
uint8_t NVM_LoRaWAN_Cycle_Time = 0;
bool NVM_LoRaWAN_Use_Cfm = false;

// Variáveis de Controle de Tempo
unsigned long timeout   = 0;
unsigned long timenow   = 0;
unsigned long timecycle = 0;
unsigned long sendTime  = 0;  // Tempo quando a mensagem foi enviada

// Variáveis de Controle
bool joined     = false;
int nack_count  = 0;
int err_count   = 0;
int LedState    = LOW;

// Máquina de Estados
uint16_t State = STATE_NOT_JOINED;

// Ponteiro para reset
void (*reset_function)(void) = 0;

//*****************************************************************************************
//  SETUP
//*****************************************************************************************
void setup() {

  // 1. Inicialização do Hardware e Sensores
  initializeHardware();
  initializeSerialInterfaces();
  initializeSensors();
  initializeSensorData();

  // 2. Inicialização da Comunicação
  commHandler = initializeCommunicationHandler();

  // 3. Inicia Conexão
  delay(500);
  ToggleLed();
  LOGI("COMM", "Iniciando conexão de rede...");
  commHandler->connect();

  // 4. Inicializa Timers
  initializeTimers();

}

//*****************************************************************************************
//  LOOP de EXECUÇÃO da MÁQUINA DE ESTADOS
//*****************************************************************************************
void loop() {

  timenow = millis();

  if(((unsigned long)(timeout - timenow)) > ((unsigned long)(-timecycle))) {
    switch(State) {
      case STATE_NOT_JOINED:
        State = process_state_not_joined(commHandler, joined);
        timecycle = JOIN_TIMEOUT_VALUE;
        break;

      case STATE_READY:
        State = process_state_ready(commHandler);
        if (State == STATE_WAIT_CFM) {
          // Marca o tempo de envio para calcular ciclo total depois
          sendTime = timenow;
          // Usa timeout de ACK (máximo de espera)
          unsigned long ackTimeout = commHandler->getConfirmationTimeout();
          timecycle = ackTimeout;
        }
        break;

      case STATE_WAIT_CFM:
        State = process_state_wait_cfm(commHandler);
        if (State == STATE_READY) {
          // Calcula tempo restante para completar o ciclo total
          unsigned long totalCicloMs = (unsigned long)NVM_LoRaWAN_Cycle_Time * 60000;
          if (totalCicloMs == 0) totalCicloMs = 180000; // Mínimo 3 minutos
          
          unsigned long elapsedSinceSend = timenow - sendTime;
          unsigned long remainingMs = (elapsedSinceSend < totalCicloMs) ? 
                                       (totalCicloMs - elapsedSinceSend) : 0;
          
          LOGD("SYSTEM", "Ciclo: Enviado há %lu ms, restam %lu ms (total %lu ms)", 
               elapsedSinceSend, remainingMs, totalCicloMs);
          
          timecycle = remainingMs;
        } else {
          timecycle = 5000;  // Se ainda aguardando, tenta de novo em 5s
        }
        break;

      default:
        reset_state_machine();
        timecycle = JOIN_TIMEOUT_VALUE;
        break;
    }

    // Processamento contínuo (necessário para o Firebase manter token vivo)
    commHandler->process();

    timeout = timenow + timecycle;

  }

}