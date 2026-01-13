/**
 * @file system_init.cpp
 * @brief Implementação das funções de inicialização do sistema
 * @copyright Copyright (c) 2025
 */

#include "core/system_init.h"
#include "core/system_utils.h"
#include "system_definitions.h"
#include "comm/credentials.h"
#include "utils/Logger.h"
#include "comm/LoRaHandler.h"
#include "comm/WiFiHandler.h"
#include <EEPROM.h>
#include <HardwareSerial.h>

// Declarações externas
extern HardwareSerial loraSerial;
extern LoRaConfig loraConfig;
extern uint8_t NVM_LoRaWAN_Cycle_Time;
extern bool NVM_LoRaWAN_Use_Cfm;
extern CPendio_LoRa_Sensor_Data_Type CPendio_LoRa_Sensor_Data;

#ifdef COMMUNICATION_MODE_WIFI
extern WiFiConfig wifiConfig;
#endif

/**
 * @brief Inicializa o hardware básico (pinos, LED, etc).
 */
void initializeHardware(void) {
  // Configura os pinos (HW.cpp)
  iniHW();

  // Inicializa o LED da placa LoRaWAN
  pinMode(MODULE_LED_PIN, OUTPUT);
  ToggleLed();

  LOGI("INIT", "Hardware inicializado");
}

/**
 * @brief Inicializa as interfaces seriais.
 */
void initializeSerialInterfaces(void) {
  // Inicializa logger (Serial)
  Logger::begin(115200);

  // Comunicação UART para o módulo LoRa
  loraSerial.begin(9600, SERIAL_8N1, RXD1_LoRa, TXD1_LoRa);

  // Comunicação UART para os sensores SPendio (RS485)
  Serial2.setRxBufferSize(64);
  Serial2.setTimeout(100);
  Serial2.begin(4800, SERIAL_8N1, RXD2_RS485, TXD2_RS485);

  LOGI("INIT", "Interfaces seriais inicializadas");
}

/**
 * @brief Inicializa os sensores I2C (AHT, BMP280).
 */
void initializeSensors(void) {
  // Sensor AHT (Temp/Umid)
  if (!aht.begin()) {
    LOGE("SENSOR", "AHT10/20 não encontrado.");
  } else {
    LOGI("SENSOR", "AHT10/20 detectado");
  }

  // Sensor BMP (Pressão)
  if (!bmp.begin(END_BMP)) {
    LOGE("SENSOR", "BMP280 não encontrado");
    g_bBMPPresente = false;
  } else {
    LOGI("SENSOR", "BMP280 detectado");
    bmp.setSampling(
      Adafruit_BMP280::MODE_NORMAL,
      Adafruit_BMP280::SAMPLING_X2,
      Adafruit_BMP280::SAMPLING_X16,
      Adafruit_BMP280::FILTER_X16,
      Adafruit_BMP280::STANDBY_MS_500
    );
    g_bBMPPresente = true;
  }

  delay(1000);
}

/**
 * @brief Inicializa a estrutura de dados de sensores.
 */
void initializeSensorData(void) {
  LOGI("SYSTEM", "=== PENDIO SERVIDOR - INICIANDO ===");
  LOGI("SYSTEM", "Versão: %s", Versao);
  LOGI("SYSTEM", "Data: %s", Data);

  iniSensores(CPendio_LoRa_Sensor_Data.d);
}

/**
 * @brief Cria e inicializa o handler de comunicação.
 * @return CommunicationHandler* Ponteiro para o handler criado.
 */
CommunicationHandler* initializeCommunicationHandler(void) {
  LOGI("COMM", "Inicializando handler de comunicação...");
  LOGI("COMM", "Frame size: %u", (unsigned)sizeof(CPendio_LoRa_Sensor_Data));

  // Leitura da EEPROM (comum aos dois modos)
  #ifdef USE_EEPROM
    NVM_LoRaWAN_Cycle_Time = EEPROM.read(0);
    NVM_LoRaWAN_Use_Cfm = (NVM_SETTINGS_CFM_BIT == (EEPROM.read(1) & NVM_SETTINGS_CFM_BIT));
  #else
    NVM_LoRaWAN_Cycle_Time = 4;  // 4 minutos para acomodar até 3min de ACK do LoRa
    NVM_LoRaWAN_Use_Cfm = true;
  #endif

  NVM_LoRaWAN_Cycle_Time = Validate_Cycle_Time(NVM_LoRaWAN_Cycle_Time);
  LOGI("SYSTEM", "Tempo de Ciclo Configurado: %d min", NVM_LoRaWAN_Cycle_Time);

  // Seleção do modo
  CommunicationHandler* handler = nullptr;

  #ifdef COMMUNICATION_MODE_WIFI
    LOGI("COMM", "MODO WIFI");
    handler = new WiFiHandler(wifiConfig);
  #else
    LOGI("COMM", "MODO LORAWAN");
    loraConfig.useConfirmation = NVM_LoRaWAN_Use_Cfm;
    handler = new LoRaHandler(loraConfig);
  #endif

  // Inicializar handler
  if (!handler->begin()) {
    LOGE("COMM", "Falha ao inicializar handler de comunicação");
    while(1) { delay(1000); }
  }

  // Bloco específico para LoRa (DevEUI)
  #ifndef COMMUNICATION_MODE_WIFI
    LoRaHandler* loraSpecific = static_cast<LoRaHandler*>(handler);
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

  return handler;
}

/**
 * @brief Inicializa os timers do sistema.
 */
void initializeTimers(void) {
  // Define TIMERS iniciais com base no JOIN
  extern unsigned long timeout;
  extern unsigned long timecycle;

  timeout = millis() + JOIN_TIMEOUT_VALUE;
  timecycle = JOIN_TIMEOUT_VALUE;

  LOGI("INIT", "Timers inicializados");
}
