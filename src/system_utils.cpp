/**
 * @file system_utils.cpp
 * @brief Implementação das funções auxiliares do sistema
 * @copyright Copyright (c) 2025
 */

#include "system_utils.h"
#include "config.h"
#include "Logger.h"
#include "Pendio_LoRa_Wemos_Robocore.h"
#include <EEPROM.h>

// Variável global para armazenar estado do LED
extern int LedState;
extern int err_count;

// Ponteiro para a função de reset (software)
extern void (*reset_function)(void);

/**
 * @brief Alterna o estado do LED de status.
 */
void ToggleLed(void) {
  LedState = !LedState; 
  digitalWrite(MODULE_LED_PIN, LedState);
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
    EEPROM.update(0, ret); // Cycle time must be store in EEPROM.
  #endif
  return(ret);
}

/**
 * @brief Valida e armazena configurações na EEPROM.
 * @param st Byte de configurações.
 * @return uint8_t Configurações validadas.
 */
uint8_t Validate_Settings(uint8_t st) {
  unsigned char ret = st;
  #ifdef USE_EEPROM
    EEPROM.update(1, ret); // Settings must be updated in EEPROM.
  #endif
  return(ret);
}
