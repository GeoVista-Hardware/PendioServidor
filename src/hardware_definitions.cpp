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
//
void iniHWPorts(void) {
  // configure Wemos ESP32 GPIOs pins:
  pinMode(pSCL_SHTU, INPUT_PULLUP);
  pinMode(pSDA_SHTU, INPUT_PULLUP);
  pinMode(WLED, OUTPUT);                    // Led Wemos
  pinMode(LLED, OUTPUT);                    // LED_LoRa
  pinMode(nRE, OUTPUT);
  pinMode(pDE, OUTPUT);
  pinMode(RXD2_RS485, INPUT_PULLUP);        // RXD2__RS485 pullup
  pinMode(nChuva, INPUT);                   // Sensor Chuva
  pinMode(aVBat, INPUT);                    // tensão da bateria

  // define estado inicial:
  digitalWrite(nRE, HIGH);                  // RS485 RX desabilitado
  digitalWrite(pDE, LOW);                   // RS485 TX desabilitado
  digitalWrite(WLED, LOW);                  // WLed desligado
  digitalWrite(LLED, LOW);                  // LLED desligado
}

//------------------------------------------------------------------------------
//  iniHW - Inicializa HW
//
void iniHW(void) {
  iniHWPorts();
}
