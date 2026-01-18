/*
--------------------------------------------------------------------------------
                                                              Inicio: 28/11/2023
        Proj.:  WCPendio
        Fonte:  HW.cpp
        Progs:  Núncio, Arnaldo
        Descr.: Rotinas de inicializacao do HW
                                                              Ultima: 28/11/2023
--------------------------------------------------------------------------------
        Diario de Bordo
        ---------------
28/11/2023 - 
*/

#include "system_definitions.h"

//------------------------------------------------------------------------------
//      iniHWPorts - Inicializa os Ports de HW

// Desc: configure Wemos ESP32 GPIOs pins

void iniHWPorts(void) {
  

  // --- I2C (Compartilhado AHT/BMP) ---
  // Mantemos ativo se qualquer um dos sensores I2C estiver habilitado
  #if SENSOR_AHT_ENABLED || SENSOR_BMP_ENABLED
    // configure Wemos ESP32 GPIOs pins for I2C:
    pinMode(pSCL_SHTU, INPUT_PULLUP);
    pinMode(pSDA_SHTU, INPUT_PULLUP);
  #endif

  // --- LEDS DE STATUS (Sistema) ---
  pinMode(LLED, OUTPUT);                      // LED_LoRa
  digitalWrite(LLED, LOW);                    // LLED desligado
  pinMode(WLED, OUTPUT);                      // Led Wemos
  digitalWrite(WLED, LOW);                    // WLed desligado

  // --- SENSOR SPENDIO (RS485) ---
  #if SENSOR_SPENDIO_ENABLED
    pinMode(nRE, OUTPUT);
    pinMode(pDE, OUTPUT);
    pinMode(RXD2_RS485, INPUT_PULLUP);        // RXD2__RS485 pullup
    digitalWrite(nRE, HIGH);                  // RS485 RX desabilitado
    digitalWrite(pDE, LOW);                   // RS485 TX desabilitado
  #endif

  // --- SENSOR CHUVA ---
  #if SENSOR_RAIN_ENABLED
    pinMode(nChuva, INPUT);                   // Sensor Chuva
  #endif


  // --- SENSOR BATERIA ---
  #if SENSOR_BATTERY_ENABLED
    pinMode(aVBat, INPUT);                    // tensão da bateria
  #endif

}

//------------------------------------------------------------------------------
//  iniHW - Inicializa HW
//
void iniHW(void) {
  iniHWPorts();
}
