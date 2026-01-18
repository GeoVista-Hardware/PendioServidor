/**
 * @file state_machine.cpp
 * @brief Implementação da máquina de estados principal
 * @copyright Copyright (c) 2025
 */

#include "core/state_machine.h"
#include "core/system_utils.h"
#include "system_definitions.h"
#include "utils/Logger.h"
#include "hardware/Sensores.h"

// Declarações externas
extern CPendio_LoRa_Sensor_Data_Type CPendio_LoRa_Sensor_Data;
extern uint8_t NVM_LoRaWAN_Cycle_Time;
extern uint16_t State;
extern unsigned long timeout;
extern unsigned long timenow;
extern unsigned long timecycle;
extern int nack_count;

/**
 * @brief Processa o estado STATE_NOT_JOINED.
 */
SystemState process_state_not_joined(CommunicationHandler* commHandler, bool& joined) {
  if (commHandler->isConnected()) {
    if (!joined) {
      LOGI("COMM", "Rede Conectada!");
      joined = true;
    }
    return STATE_READY;
  } else {
    LOGI("COMM", "Tentando conectar novamente...");
    commHandler->connect();
    return STATE_NOT_JOINED;
  }
}

/**
 * @brief Processa o estado STATE_READY.
 */
SystemState process_state_ready(CommunicationHandler* commHandler) {
  // Leitura dos Sensores
  varrSensores(CPendio_LoRa_Sensor_Data.d);
  nack_count = 0;

  // Enviar dados
  LOGD("COMM", "Payload size: %u", (unsigned)sizeof(CPendio_LoRa_Sensor_Data));

  SendResult sendResult = commHandler->send(
    1,
    (const uint8_t*)CPendio_LoRa_Sensor_Data.Bytes,
    sizeof(CPendio_LoRa_Sensor_Data)
  );

  if (sendResult == SendResult::SUCCESS) {

    LOGI("COMM", "Envio aceito pelo Handler");
    exception_handling(ERROR_RESTART);

    #ifdef COMMUNICATION_MODE_WIFI
      // HTTP 200 já é o ACK -> não espera confirmação
      unsigned long totalCicloMs = (unsigned long)NVM_LoRaWAN_Cycle_Time * 60000;
      if (totalCicloMs == 0) totalCicloMs = 180000; // Mínimo 3 minutos
      LOGI("SYSTEM", "Ciclo aguardando até %lu ms para próximo envio", totalCicloMs);
      return STATE_READY;
    #else
      // LoRaWAN precisa esperar ACK
      return STATE_WAIT_CFM;
    #endif

  } else if (sendResult == SendResult::PENDING) {

    LOGW("COMM", "Envio pendente");
    return STATE_READY;

  } else {

    LOGE("COMM", "Envio negado/falha - reiniciando conexão");
    exception_handling(ERROR_LORAWAN);
    return STATE_NOT_JOINED;

  }
}

/**
 * @brief Processa o estado STATE_WAIT_CFM.
 */
SystemState process_state_wait_cfm(CommunicationHandler* commHandler) {
  DownlinkMessage downlink;

  if (commHandler->isConfirmed()) {
    LOGI("COMM", "Confirmação Recebida (ACK)");
    exception_handling(ERROR_RESTART);

    // Verifica se tem mensagem de descida (Downlink)
    if (commHandler->receive(downlink) == ReceiveResult::MESSAGE_RECEIVED) {
      LOGI("COMM", "Downlink recebido! (port=%u)", (unsigned)downlink.port);

      // Lógica de processamento de downlink
      if (downlink.length >= 5 && downlink.data[0] == '8') {
        // Exemplo: Atualizar tempo de ciclo
        if (downlink.data[1] == '0') {
          NVM_LoRaWAN_Cycle_Time = (downlink.data[2] - '0') * 16 + (downlink.data[3] - '0');
          LOGI("COMM", "Novo Ciclo: %u min", NVM_LoRaWAN_Cycle_Time);
        }
      }
      ToggleLed();
    }

    // Define o tempo para o próximo envio (Ciclo)
    // Ciclo total: NVM_LoRaWAN_Cycle_Time minutos (ex: 4 min)
    // Já foi gasto tempo esperando ACK, então dormimos o restante
    unsigned long totalCicloMs = (unsigned long)NVM_LoRaWAN_Cycle_Time * 60000;
    if (totalCicloMs == 0) totalCicloMs = 180000; // Mínimo 3 minutos

    LOGI("SYSTEM", "Ciclo aguardando até %lu ms para próximo envio", totalCicloMs);
    return STATE_READY;

  } else {
    // Se estourar o tempo sem ACK
    LOGW("COMM", "Sem ACK - Timeout");
    nack_count++;

    if (nack_count > 3) {
      nack_count = 0;
      return STATE_READY;
    }
    return STATE_WAIT_CFM;
  }
}

/**
 * @brief Redefine a máquina de estados.
 */
void reset_state_machine(void) {
  State = STATE_NOT_JOINED;
  exception_handling(ERROR_LORAWAN);
}
