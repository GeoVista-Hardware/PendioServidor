/**
 * @file state_machine.h
 * @brief Máquina de estados principal do sistema
 * @copyright Copyright (c) 2025
 */

#ifndef _STATE_MACHINE_H
#define _STATE_MACHINE_H

#include "comm/CommunicationHandler.h"

/**
 * @enum SystemState
 * @brief Estados da máquina de estados principal
 */
enum SystemState {
    STATE_NOT_JOINED = 0,   /**< Aguardando conexão com a rede (Join Accept) */
    STATE_READY,            /**< Conectado, pronto para leitura e envio */
    STATE_WAIT_CFM          /**< Pacote enviado, aguardando ACK/Downlink */
};

/**
 * @brief Processa o estado STATE_NOT_JOINED.
 * @details Tenta conectar à rede e transiciona para STATE_READY quando bem-sucedido.
 * @param commHandler Ponteiro para o handler de comunicação.
 * @param joined Referência para a flag de conexão.
 * @return SystemState Próximo estado.
 */
SystemState process_state_not_joined(CommunicationHandler* commHandler, bool& joined);

/**
 * @brief Processa o estado STATE_READY.
 * @details Lê sensores, envia dados e transiciona para STATE_WAIT_CFM se bem-sucedido.
 * @param commHandler Ponteiro para o handler de comunicação.
 * @return SystemState Próximo estado.
 */
SystemState process_state_ready(CommunicationHandler* commHandler);

/**
 * @brief Processa o estado STATE_WAIT_CFM.
 * @details Aguarda confirmação (ACK) e processa downlinks se houver.
 * @param commHandler Ponteiro para o handler de comunicação.
 * @return SystemState Próximo estado.
 */
SystemState process_state_wait_cfm(CommunicationHandler* commHandler);

/**
 * @brief Redefine a máquina de estados para o estado inicial.
 */
void reset_state_machine(void);

#endif /* _STATE_MACHINE_H */
