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
#include <esp_task_wdt.h>

// Headers de Configuração
#include "system_definitions.h"
#include "comm/credentials.h"
#include "utils/Logger.h"
#include "hardware/Sensores.h"

// Headers do Sistema
#include "core/system_utils.h"
#include "core/system_init.h"
#include "core/state_machine.h"
#include "core/system_context.h"

// Headers de Comunicação
#include "comm/CommunicationHandler.h"
#include "comm/LoRaHandler.h"
#include "comm/WiFiHandler.h"

//*****************************************************************************************
//  DEFINIÇÕES GLOBAIS E VARIÁVEIS
//*****************************************************************************************

// Contexto do Sistema (da FSM e dados)
SystemContext sysContext;

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
    .maxRetries = 3
};

#ifdef COMMUNICATION_MODE_WIFI

  // Configuração do Wi-Fi/Oracle APEX
  WiFiConfig wifiConfig = {
      .ssid = WIFI_SSID,
      .password = WIFI_PASSWORD,
      .apexUrl = DB_URL,
      .apiKey = API_KEY,
      .deviceId = DEVICE_ID,
      .connectTimeout = 30000
  };

#endif

// Instância do handler de comunicação
CommunicationHandler* commHandler = nullptr;

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

  // 5. Mensagem de sucesso
  LOGI("SYSTEM", "Setup do Sistema Concluído. Sistema Iniciado!");

}

//*****************************************************************************************
//  LOOP de EXECUÇÃO da MÁQUINA DE ESTADOS
//*****************************************************************************************
void loop() {

  // --- Alimenta o WatchDog Timer (WDT) ---
  
  #if ENABLE_WATCHDOG
    esp_task_wdt_reset();
  #endif

  // --- Rotina tradicional da FSM ---

  // Comunicação (Processamento contínuo)
  if (commHandler) commHandler->process();

  // Atualização de Tempo
  sysContext.timenow = millis();

  // Máquina de Estados (executa apenas se o timer expirou)
  if (sysContext.isTimerExpired()) {
      
      // Executa a lógica e recebe o tempo para a próxima execução
      unsigned long nextDelay = fsm_dispatch(&sysContext, commHandler);
      
      // Agenda o próximo ciclo
      sysContext.setNextTimeout(nextDelay);

  }

}