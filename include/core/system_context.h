/**
 * @file system_context.h
 * @brief Estrutura central de rastreamento de estado, dados e temporização do sistema
 *
 * Este arquivo define a estrutura SystemContext, responsável por manter
 * todo o contexto global do sistema embarcado, incluindo:
 *  - Estado da máquina de estados (FSM)
 *  - Dados de sensores
 *  - Parâmetros de configuração
 *  - Controle de temporização
 *  - Contadores de erro e estado de hardware
 *
 * @copyright Copyright (c) 2026
 */

#ifndef _SYSTEM_CONTEXT_H
#define _SYSTEM_CONTEXT_H

// Includes
#include <Arduino.h>
#include "hardware/Sensores.h"
#include "core/state_machine.h"

/**
 * @struct SystemContext
 * @brief Contexto global do sistema
 *
 * Estrutura responsável por concentrar todas as informações de estado
 * necessárias para a execução do sistema. Atua como:
 *  - Memória de estado da FSM
 *  - Repositório de dados de sensores
 *  - Gerenciador de temporizações
 *  - Armazenamento de contadores e flags de controle
 *
 * Essa estrutura é normalmente instanciada como um objeto global.
 */
struct SystemContext {

    // =========================================================================
    // Máquina de Estados
    // =========================================================================

    /** Estado atual da máquina de estados do sistema */
    SystemState state;

    /** Indica se o dispositivo já realizou JOIN na rede (ex.: LoRaWAN) */
    bool joined;
    
    // =========================================================================
    // Dados de Sensores
    // =========================================================================

    /** Estrutura contendo os dados mais recentes dos sensores */
    CPendio_LoRa_Sensor_Data_Type sensorData;

    // =========================================================================
    // Configurações Operacionais
    // =========================================================================

    /** Tempo de ciclo do sistema, em minutos */
    uint8_t cycleTimeMinutes;

    /** Habilita envio confirmado (ACK) na comunicação */
    bool useConfirmation;

    // =========================================================================
    // Controle de Tempo
    // =========================================================================

    /**
     * Timestamp absoluto do próximo timeout programado (ms)
     */
    unsigned long timeout;

    /**
     * Timestamp atual do sistema (normalmente derivado de millis())
     */
    unsigned long timenow;

    /**
     * Intervalo de tempo utilizado para cálculo do próximo timeout (ms)
     */
    unsigned long timecycle;

    /**
     * Timestamp do último envio de dados (ms)
     */
    unsigned long sendTime;

    // =========================================================================
    // Contadores e Estado de Hardware
    // =========================================================================

    /** 
     * Contador de NACKs recebidos 
     */
    int nackCount;

    /** 
     * Contador de erros gerais do sistema 
     */
    int errorCount;

    /** 
     * Estado atual do LED de status 
     */
    int ledState;

    // =========================================================================
    // Métodos
    // =========================================================================

    /**
     * @brief Construtor padrão
     *
     * Inicializa o contexto do sistema com valores padrão seguros,
     * incluindo estado inicial da FSM, configurações e limpeza
     * dos dados de sensores.
     */
    SystemContext() {
        state = STATE_NOT_JOINED;
        joined = false;
        cycleTimeMinutes = 15;
        useConfirmation = false;
        timeout = 0;
        timenow = 0;
        timecycle = 0;
        sendTime = 0;

        nackCount = 0;
        errorCount = 0;
        ledState = LOW;
        memset(sensorData.Bytes, 0, sizeof(sensorData.Bytes));
    }

    /**
     * @brief Verifica se o temporizador atual expirou
     *
     * Implementa uma verificação segura contra overflow de `unsigned long`,
     * adequada para uso com temporização baseada em `millis()`.
     *
     * @return true se o timeout expirou
     * @return false caso contrário
     */
    bool isTimerExpired() const {
        return ((unsigned long)(timeout - timenow) > (unsigned long)(-timecycle));
    }

    /**
     * @brief Programa o próximo timeout do sistema
     *
     * Calcula e armazena o próximo instante de timeout a partir do
     * tempo atual.
     *
     * @param delayMs Intervalo de tempo até o próximo timeout (ms)
     */
    void setNextTimeout(unsigned long delayMs) {
        timecycle = delayMs;
        timeout = timenow + timecycle;
    }
};

/**
 * @brief Instância global do contexto do sistema
 */
extern SystemContext sysContext;

#endif /* _SYSTEM_CONTEXT_H */