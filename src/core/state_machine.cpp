/**
 * @file state_machine.cpp
 * @brief Implementação da máquina de estados principal
 * @copyright Copyright (c) 2025
 */

#include "core/state_machine.h"
#include "core/system_utils.h"
#include "core/system_context.h"
#include "system_definitions.h"
#include "utils/Logger.h"
#include "hardware/Sensores.h"

/**
 * @brief Processa o estado STATE_NOT_JOINED.
 */
SystemState process_state_not_joined(CommunicationHandler* commHandler, SystemContext* ctx) {
  if (commHandler->isConnected()) {
    if (!ctx->joined) {
      LOGI("COMM", "Rede Conectada!");
      ctx->joined = true;
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
SystemState process_state_ready(CommunicationHandler* commHandler, SystemContext* ctx) {
  // Leitura dos Sensores
  varrSensores(ctx->sensorData.d);
  ctx->nackCount = 0;

  // Enviar dados
  LOGD("COMM", "Payload size: %u", (unsigned)sizeof(ctx->sensorData));

  SendResult sendResult = commHandler->send(
    1,
    (const uint8_t*)ctx->sensorData.Bytes,
    sizeof(ctx->sensorData)
  );

  if (sendResult == SendResult::SUCCESS) {

    LOGI("COMM", "Envio aceito pelo Handler");
    exception_handling(ERROR_RESTART);

    #ifdef COMMUNICATION_MODE_WIFI
      // HTTP 200 já é o ACK -> não espera confirmação
      unsigned long totalCicloMs = (unsigned long)ctx->cycleTimeMinutes * 60000;
      if (totalCicloMs == 0) totalCicloMs = 180000;
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
SystemState process_state_wait_cfm(CommunicationHandler* commHandler, SystemContext* ctx) {
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
          ctx->cycleTimeMinutes = (downlink.data[2] - '0') * 16 + (downlink.data[3] - '0');
          LOGI("COMM", "Novo Ciclo: %u min", ctx->cycleTimeMinutes);
        }
      }
      ToggleLed();
    }

    // Calculo do ciclo usando contexto
    unsigned long totalCicloMs = (unsigned long)ctx->cycleTimeMinutes * 60000;
    if (totalCicloMs == 0) totalCicloMs = 180000;

    LOGI("SYSTEM", "Ciclo aguardando até %lu ms para próximo envio", totalCicloMs);
    return STATE_READY;

  } else {
    // Se estourar o tempo sem ACK
    LOGW("COMM", "Sem ACK - Timeout");
    ctx->nackCount++;

    if (ctx->nackCount> 3) {
      ctx->nackCount = 0;
      return STATE_READY;
    }
    return STATE_WAIT_CFM;
  }
}

/**
 * @brief Redefine a máquina de estados.
 */
void reset_state_machine(SystemContext* ctx) {
  ctx->state = STATE_NOT_JOINED;
  exception_handling(ERROR_LORAWAN);
}

/**
 * @brief Executa a lógica da FSM e retorna o tempo para o próximo ciclo.
 */
unsigned long fsm_dispatch(SystemContext* ctx, CommunicationHandler* commHandler) {
    unsigned long nextDelay = JOIN_TIMEOUT_VALUE;

    switch (ctx->state) {
        
        case STATE_NOT_JOINED:
            ctx->state = process_state_not_joined(commHandler, ctx);
            nextDelay = JOIN_TIMEOUT_VALUE; 
            break;

        case STATE_READY:
            ctx->state = process_state_ready(commHandler, ctx);
            
            // Se mudou para WAIT_CFM (LoRa Confirmado)
            if (ctx->state == STATE_WAIT_CFM) {
                ctx->sendTime = ctx->timenow;
                nextDelay = commHandler->getConfirmationTimeout();
            }
            // Se continuou em READY (WiFi ou LoRa Sem Confirmação -> Sucesso imediato)
            else if (ctx->state == STATE_READY) {
                 ctx->sendTime = ctx->timenow; // Marca o envio agora
                 nextDelay = calculate_next_cycle(ctx); // Calcula sono até o próximo ciclo
            }
            break;

        case STATE_WAIT_CFM:
            ctx->state = process_state_wait_cfm(commHandler, ctx);
            
            if (ctx->state == STATE_READY) {
                // Recebeu ACK, calcula quanto tempo resta do ciclo
                nextDelay = calculate_next_cycle(ctx);
            } else {
                // Ainda sem ACK, tenta verificar novamente em breve
                nextDelay = 5000; 
            }
            break;

        default:
            reset_state_machine(ctx);
            nextDelay = JOIN_TIMEOUT_VALUE;
            break;
    }

    return nextDelay;
}