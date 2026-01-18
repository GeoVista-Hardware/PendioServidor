/**
 * @file system_utils.cpp
 * @brief Implementação das funções auxiliares do sistema
 * @copyright Copyright (c) 2026
 */

#include "core/system_utils.h"
#include "core/system_context.h"
#include "system_definitions.h"
#include "utils/Logger.h"
#include "hardware_definitions.h"
#include <EEPROM.h>

/**
 * @brief Alterna o estado do LED de status.
 */
void ToggleLed(void) {
  sysContext.ledState = !sysContext.ledState; 
  digitalWrite(MODULE_LED_PIN, sysContext.ledState);
}

/**
 * @brief Gerencia erros críticos e reinícios do sistema.
 * @param Exception_code Código do erro (ERROR_RESTART, ERROR_LORAWAN, etc).
 */
void exception_handling(int Exception_code) {
  switch (Exception_code) {
    case ERROR_RESTART:
      // Sucesso: Zera contador de erros
      sysContext.errorCount = 0;
      break;

    case ERROR_LORAWAN:
      // Processa um erro adicional LoRaWAN
      LOGW("SYSTEM", "Error Code: %d", Exception_code);
      sysContext.errorCount++;
      // Caso o contador de erros exceder o limite de erros consecutivos, força reinício
      if (sysContext.errorCount > ERROR_MAX_SEQ) {
        LOGE("SYSTEM", "Forced Reset in 30s due to repeated LoRa errors");
        delay(30000);
        ESP.restart();
      }
      break;

    case RESTART_REQUEST:
      LOGW("SYSTEM", "Immediate Reset Requested - rebooting in 30s");
      delay(30000);
      ESP.restart();
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
    case 0:               
      ret = CYCLE_DEBUG_MIN; // 1 min
      break; 
      
    // Adicionado o caso de 3 minutos que você precisava
    case CYCLE_FAST_MIN:    // 3 min
    case CYCLE_SHORT_MIN:   // 5 min
    case CYCLE_MEDIUM_MIN:  // 10 min
    case CYCLE_DEFAULT_MIN: // 15 min
    case CYCLE_LONG_MIN:    // 30 min
    case CYCLE_XLONG_MIN:   // 60 min
      ret = ct; 
      break;
      
    default: 
      // Se vier qualquer coisa diferente (ex: 255 ou 7), joga para o default de 15
      ret = CYCLE_DEFAULT_MIN; 
      break; 
  }
  
  #ifdef USE_EEPROM
    EEPROM.write(0, ret); // Grava o valor validado de volta para garantir integridade
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
    EEPROM.write(1, ret); // Settings must be updated in EEPROM.
  #endif
  return(ret);
}

/**
 * @brief Calcula quanto tempo falta para fechar o ciclo de envio.
 */
unsigned long calculate_next_cycle(SystemContext* ctx) {
    unsigned long totalCicloMs = (unsigned long)ctx->cycleTimeMinutes * 60000;
    if (totalCicloMs == 0) totalCicloMs = 180000; // Proteção mínima 3 min
    
    unsigned long elapsed = ctx->timenow - ctx->sendTime;
    
    if (elapsed < totalCicloMs) {
        return totalCicloMs - elapsed;
    }
    return 0; // Já estourou o tempo, envia já
}