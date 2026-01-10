#ifndef _APL_H
#define _APL_H

#define Versao "WRCPendio Wemos Robocore CPendio"
#define Data   "10/01/2024"

#define ON    1
#define OFF   0
#define LIGA  1
#define DESLIGA 0

#define CR 0x0D
#define LF 0x0A

#define SPENDIO_TIMEOUT   20        // 200 ms

typedef unsigned char uchar;
typedef unsigned int  uint;
typedef unsigned short ushort;

#ifdef MAIN
 #define global
#else
 #define global extern
#endif

//------------------------------------------------------------------------------
//  includes comuns
//
#include <arduino.h>
#include <RoboCore_SMW_SX1262M0.h>
//#include <SoftwareSerial.h>
#include <HardwareSerial.h>
#include <EEPROM.h>
#include <stdint.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

//#include "Pendio_Sensor.h"
#include "Pendio_LoRa_Wemos_Robocore.h"
#include "WRCPendio.h"
#include "HW.h"
#include "Sensores.h"

//------------------------------------------------------------------------------
//  SELETOR DE MODO DE OPERAÇÃO

// Descomente a linha abaixo para ativar o modo Protótipo (Wi-Fi + Firebase)
// Comente para compilar a versão LoRaWAN
#define COMMUNICATION_MODE_WIFI 

// Definir para usar EEPROM para persistência de dados
#define USE_EEPROM

//------------------------------------------------------------------------------

#endif /* _APL_H */
