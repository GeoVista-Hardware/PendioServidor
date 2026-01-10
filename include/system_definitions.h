/**
 * @file system_definitions.h
 * @brief Definições globais do sistema Pendio
 * @details Este ficheiro contém definições e macros globais utilizadas em todo o
 * projeto Pendio.
 * @copyright Copyright (c) 2026
 * @note Edite este arquivo com cuidado, pois afeta todo o sistema.
 */

#ifndef _SYSTEM_DEFINITIONS_H
#define _SYSTEM_DEFINITIONS_H

// ============================================================================
// VERSÃO DO PROJETO
// ============================================================================

#define Versao "WRCPendio Wemos Robocore CPendio"
#define Data   "10/01/2024"

// ============================================================================
// MACROS GERAIS
// ============================================================================

#define ON    1
#define OFF   0
#define LIGA  1
#define DESLIGA 0

#define CR 0x0D
#define LF 0x0A

#define SPENDIO_TIMEOUT   20        // 200 ms

typedef unsigned char uchar;
typedef unsigned int  uint;
typedef unsigned short ushort;

#ifdef MAIN
 #define global
#else
 #define global extern
#endif

// ============================================================================
// INCLUDES COMUNS
// ============================================================================

#include <arduino.h>
#include <RoboCore_SMW_SX1262M0.h>
#include <HardwareSerial.h>
#include <EEPROM.h>
#include <stdint.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "hardware_definitions.h"
#include "hardware_definitions.h"
#include "hardware_definitions.h"
#include "hardware/Sensores.h"

// ============================================================================
// MODO DE OPERAÇÃO
// ============================================================================

/**
 * @section OPERATIONAL Modo Operacional
 */

/** @brief Define o modo de comunicação */
// Descomente a linha abaixo para ativar o modo Protótipo (Wi-Fi + Firebase)
// Comente para compilar a versão LoRaWAN
#define COMMUNICATION_MODE_WIFI 

/** @brief Ativa logging serial estruturado (RECOMENDADO) */
#define ENABLE_LOGGING              1

/** @brief Nível padrão de log (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR) */
#define LOG_LEVEL_DEFAULT           LOG_LEVEL_INFO

/** @brief Baudrate serial para logs */
#define SERIAL_BAUDRATE             115200

/** @brief Ativa persistência de dados em EEPROM */
// Definir para usar EEPROM para persistência de dados
// Comente para desativar
#define USE_EEPROM

/** @brief Ativa simulação de JOIN para testes sem hardware */
#define ENABLE_FAKE_JOIN            0

/** @brief Bit de CFM em EEPROM (NVM settings) */
#define NVM_SETTINGS_CFM_BIT        0x01

// ============================================================================
// LoRaWAN - TIMINGS
// ============================================================================

/**
 * @section LORA_TIMINGS Timeouts e Intervalos
 */

/** @brief Timeout para OTAA Join [ms] */
#define JOIN_TIMEOUT_VALUE          10000

/** @brief Timeout para aguardar ACK/CFM [ms] */
#define CFM_TIMEOUT_VALUE           180000                // 3 minutos

/** @brief Intervalo mínimo entre mensagens [ms] */
#define NEXT_MSG_TIMEOUT_VALUE      20000                 // 20 segundos (teste)
// #define NEXT_MSG_TIMEOUT_VALUE   1800000               // 30 minutos (produção)

/** @brief Também suportado por legado: NXTMSG_TIMEOUT_VALUE */
#define NXTMSG_TIMEOUT_VALUE        NEXT_MSG_TIMEOUT_VALUE

// ============================================================================
// LoRaWAN - CONFIGURAÇÃO DE TRANSMISSÃO
// ============================================================================

/**
 * @section LORA_TRANSMISSION Transmissão
 */

/** @brief Data Rate fixo (se ADR desabilitado): 0-12 */
#define LORA_FIXED_DR               2

/** @brief Ativa Adaptive Data Rate (recomendado) */
#define LORA_ADR_ON                 true

/** @brief Confirmação de mensagens habilitada (CFM) */
#define LORA_USE_CONFIRMATION       0

/** @brief Tamanho máximo de payload [bytes] */
#define LORA_MAX_PAYLOAD            100

/** @brief Máximo de retentativas de NACK */
#define LORA_MAX_NACK_RETRIES       9

/** @brief Também suportado por legado: LORA_MAX_NACK */
#define LORA_MAX_NACK               LORA_MAX_NACK_RETRIES

// ============================================================================
// SENSORES - AMOSTRAGEM
// ============================================================================

/**
 * @section SENSORS Sensores
 */

/** @brief Ativa AHT10/AHT20 (temperatura/umidade) */
#define SENSOR_AHT_ENABLED          1

/** @brief Ativa BMP280 (pressão) */
#define SENSOR_BMP_ENABLED          1

/** @brief Ativa RS485 SPendio (sensores customizados) */
#define SENSOR_SPENDIO_ENABLED      1

/** @brief Ativa sensor de chuva (GPIO) */
#define SENSOR_RAIN_ENABLED         1

/** @brief Ativa monitoramento de bateria */
#define SENSOR_BATTERY_ENABLED      1

// ============================================================================
// HARDWARE - PINOS
// ============================================================================

/**
 * @section HARDWARE Mapeamento de Pinos
 */

/** @brief LED do módulo (ESP32 DOIT V1: GPIO 2) */
#define PIN_LED                     2

/** @brief Serial LoRaWAN (Serial1: TX=GPIO17, RX=GPIO16) */
#define LORA_SERIAL_PORT            1

/** @brief Serial LoRaWAN TX power (dBm): 2-20 */
#define LORA_TX_POWER               20

// ============================================================================
// SISTEMA - GERENCIAMENTO
// ============================================================================

/**
 * @section SYSTEM Sistema
 */

/** @brief Stack trace em caso de erro (DEBUG) */
#define ENABLE_STACK_TRACE          0

/** @brief Watchdog timer habilitado */
#define ENABLE_WATCHDOG             0

/** @brief Intervalo watchdog [ms] */
#define WATCHDOG_TIMEOUT            30000

/** @brief Reinicia automaticamente após N erros sequenciais */
#define MAX_SEQUENTIAL_ERRORS       10

// ============================================================================
// DESENVOLVIMENTO - DEBUG
// ============================================================================

/**
 * @section DEBUG Debug e Teste
 */

/** @brief Modo debug: logging verboso */
#define DEBUG_MODE                  0

/** @brief Modo teste: desabilita certas funcionalidades */
#define TEST_MODE                   0

/** @brief Simula downlinks para testes */
#define SIMULATE_DOWNLINKS          0

// ============================================================================
// VALIDAÇÃO EM TEMPO DE COMPILAÇÃO
// ============================================================================

// Garantir que configurações críticas estão definidas
#if !defined(SERIAL_BAUDRATE)
    #error "SERIAL_BAUDRATE não definido"
#endif

#if LORA_FIXED_DR < 0 || LORA_FIXED_DR > 12
    #error "LORA_FIXED_DR inválido (0-12)"
#endif

#if LORA_MAX_PAYLOAD < 10 || LORA_MAX_PAYLOAD > 242
    #error "LORA_MAX_PAYLOAD inválido (10-242)"
#endif

// ============================================================================

#endif /* _SYSTEM_DEFINITIONS_H */