/**
 * @file system_utils.h
 * @brief Funções auxiliares do sistema (LED, exceções, validações)
 * @copyright Copyright (c) 2026
 */

#ifndef _SYSTEM_UTILS_H
#define _SYSTEM_UTILS_H

#include <Arduino.h>

struct SystemContext;

// Configurações de Erro e Retentativa
constexpr int ERROR_RESTART   = 0; // Limpa erros (reinicia o contador)
constexpr int ERROR_LORAWAN   = 1; // Erro de comunicação no LoRaWAN
constexpr int RESTART_REQUEST = 2; // Solicitação remota de reinício (imediata)
constexpr int ERROR_MAX_SEQ   = 5; // Máximo de erros antes do reset forçado

/**
 * @brief Alterna o estado do LED de status.
 */
void ToggleLed(void);

/**
 * @brief Gerencia erros críticos e reinícios do sistema.
 * @param Exception_code Código do erro (ERROR_RESTART, ERROR_LORAWAN, etc).
 */
void exception_handling(int Exception_code);

/**
 * @brief Valida o tempo de ciclo lido da EEPROM.
 * @param ct Tempo em minutos.
 * @return uint8_t Tempo validado.
 */
uint8_t Validate_Cycle_Time(uint8_t ct);

/**
 * @brief Valida e armazena configurações na EEPROM.
 * @param st Byte de configurações.
 * @return uint8_t Configurações validadas.
 */
uint8_t Validate_Settings(uint8_t st);

/**
 * @brief Calcula quanto tempo falta para fechar o ciclo de envio.
 */
unsigned long calculate_next_cycle(SystemContext* ctx);

#endif /* _SYSTEM_UTILS_H */
