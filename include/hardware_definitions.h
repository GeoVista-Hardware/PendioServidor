#ifndef _HARDWARE_DEFINITIONS_H
#define _HARDWARE_DEFINITIONS_H

// ============================================================================
// Funções de Inicialização de Hardware
// ============================================================================

/** @brief Inicializa os portos de hardware */
void iniHWPorts(void);

/** @brief Inicializa o hardware específico do projeto */
void iniHW(void);

// ============================================================================
// PINOUT DEFINITIONS para Wemos D1 R32 + Robocore LoRaWAN SHIELD
// ============================================================================

#define RXD2_RS485  16              // Serial2 RS485
#define TXD2_RS485  17
#define WLED         2              // WLED Led Wemos
#define nRE         27              // Pinos nRE RS485 - Habilita Recepção
#define pDE         19              // Pinos pDE RS485 - Habilita Transmissão
#define LLED        18              // LLED Led Robocore LoRaWAN
#define pSCL_SHTU   22              // SCL Sensor temp/umidade
#define pSDA_SHTU   21              // SDA Sensor temp/umidade
#define nChuva       4
#define RXD1_LoRa    5              // Serial1 Robocore LoRaWAN
#define TXD1_LoRa   23
#define aVBat       39              // tensão da bateria

#define END_BMP    0x76             // Endereço I2C BMP280

// ============================================================================
// MACROS para controle de GPIOs
// ============================================================================

/** @brief Liga o LED Wemos */
#define ligWLED() digitalWrite(WLED, HIGH )

/** @brief Desliga o LED Wemos */
#define desWLED() digitalWrite(WLED, LOW)

/** @brief Inverte o estado do LED Wemos */
#define invWLED() digitalWrite(WLED, !digitalRead(WLED))

/** @brief Liga o LED Robocore */
#define ligLLED() digitalWrite(LLED, HIGH )

/** @brief Desliga o LED Robocore */
#define desLLED() digitalWrite(LLED, LOW)

/** @brief Inverte o estado do LED Robocore */
#define invLLED() digitalWrite(LLED, !digitalRead(LLED))

/** @brief Lê o estado do sensor de chuva */
#define leChuva() digitalRead(nChuva)

/** @brief Retorna verdadeiro se estiver chovendo */
#define temChuva() (!digitalRead(nChuva))

// ============================================================================
// HARDWARE - Mapeamento de Pinos (ESP32 DOIT DEVKIT V1)
// ============================================================================

/**
 * @brief LED do módulo
 * @details Usado para indicação visual de status do sistema
 */
#define MODULE_LED_PIN                  2

// ============================================================================
// Macros Utilitárias
// ============================================================================

/**
 * @brief Converte número (0-15) para dígito hexadecimal (0-9, A-F)
 * @param x Valor 0-15
 * @return Caractere '0'-'9' ou 'A'-'F'
 *
 * @note Útil para conversão de arrays de bytes para string hex
 */
#if !defined(Nib)
#define Nib(x)  ((x > 9) ? ('A' + x - 0x0A) : ('0' + x))
#endif


// ============================================================================

#endif /* _HARDWARE_DEFINITIONS_H */
