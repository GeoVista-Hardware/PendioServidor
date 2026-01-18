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
#define Data   "10/01/2026"

// ============================================================================
// MACROS GERAIS
// ============================================================================

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
// Descomente a linha abaixo para ativar o modo Wi-Fi
// Comente para compilar a versão LoRaWAN
#define COMMUNICATION_MODE_WIFI 

/** @brief Define se usará API KEY para autenticação */
// Descomente a linha abaixo para ativar a autenticação via API Key
// Comente para compilar a versão com autenticação
#define WIFI_USE_API_KEY

/** @brief Ativa logging serial estruturado (RECOMENDADO) */
#define ENABLE_LOGGING              1

/** @brief Nível padrão de log (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR) */
#define LOG_LEVEL_DEFAULT           0

/** @brief Baudrate serial para logs */
#define SERIAL_BAUDRATE             115200

/** @brief Ativa persistência de dados em EEPROM */
// Definir para usar EEPROM para persistência de dados
// Comente para desativar
#define USE_EEPROM

/** @brief Bit de CFM em EEPROM (NVM settings) */
#define NVM_SETTINGS_CFM_BIT        0x01

// ============================================================================
// LoRaWAN - TIMINGS
// ============================================================================

/**
 * @section LORA_TIMINGS Timeouts e Intervalos
 */

/** @brief Timeout para OTAA Join [ms] */
// 45 segundos - balanço entre permitir JOIN lento e não esperar demais
#define JOIN_TIMEOUT_VALUE          45000                          

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
// SISTEMA - GERENCIAMENTO
// ============================================================================

/**
 * @section WDT Sistema
 */


/** @brief Watchdog timer habilitado */
#define ENABLE_WATCHDOG             0

/** @brief Intervalo watchdog [ms] */
#define WATCHDOG_TIMEOUT            30000


// ============================================================================
// CONFIGURAÇÃO DE CICLOS (TIMING)
// ============================================================================

/**
 * @section TIMING Sistema
 */

/** @brief Tempo de ciclo para Debug (1 min) */
#define CYCLE_DEBUG_MIN     1

/** @brief Tempo de ciclo Ideal/Rápido (3 min) */
#define CYCLE_FAST_MIN      3

/** @brief Tempos intermediários permitidos */
#define CYCLE_SHORT_MIN     5
#define CYCLE_MEDIUM_MIN    10

/** @brief Tempo de ciclo Padrão/Default (15 min) */
#define CYCLE_DEFAULT_MIN   15

/** @brief Ciclos longos */
#define CYCLE_LONG_MIN      30
#define CYCLE_XLONG_MIN     60

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