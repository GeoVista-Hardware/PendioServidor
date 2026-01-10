/**
 * @file system_init.h
 * @brief Funções de inicialização do sistema
 * @copyright Copyright (c) 2025
 */

#ifndef _SYSTEM_INIT_H
#define _SYSTEM_INIT_H

#include "comm/CommunicationHandler.h"

/**
 * @brief Inicializa o hardware básico (pinos, LED, etc).
 * @details Chama iniHW() e configura o LED.
 */
void initializeHardware(void);

/**
 * @brief Inicializa as interfaces seriais (UART para LoRa, sensores e Logger).
 * @details Configura Serial para debug, loraSerial para LoRa e Serial2 para RS485.
 */
void initializeSerialInterfaces(void);

/**
 * @brief Inicializa os sensores I2C (AHT, BMP280).
 * @details Detecta e configura sensores de temperatura, umidade e pressão.
 */
void initializeSensors(void);

/**
 * @brief Inicializa a estrutura de dados de sensores e exibe mensagens de boas-vindas.
 */
void initializeSensorData(void);

/**
 * @brief Cria e inicializa o handler de comunicação (LoRa ou Wi-Fi).
 * @return CommunicationHandler* Ponteiro para o handler criado.
 */
CommunicationHandler* initializeCommunicationHandler(void);

/**
 * @brief Inicializa os timers do sistema.
 */
void initializeTimers(void);

#endif /* _SYSTEM_INIT_H */
