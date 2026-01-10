/**
 * @file Sensores.h
 * @brief Definições e estruturas para gestão de sensores do Projeto Pendio.
 * * @details Este ficheiro contém as estruturas de dados para o payload LoRaWAN
 * e a classe de gestão dos sensores AHT, BMP e RS485.
 * * @author Eng. Nuncio Perrella, MSc
 * * @author Eng. André Maiolini
 * * @author Eng. Arnaldo
 * * @copyright Copyright (c) 2025 - Todos os direitos reservados.
 */

#ifndef _SENSORES_H
#define _SENSORES_H

#include "system_definitions.h"

struct SPendio_Data_Type {        // 3+3+3+5= 14 bytes
  char acx[3];                            // acc X
  char acy[3];                            // acc Y
  char acz[3];                            // acc Z
  char solo[5];                           // solo
};

struct CPendio_Sensor_Data_Type { // 2+14+14+14+2+2+5+4+3+1=60 + 1 bytes
  char frmFmtV[2];                        // Frame Format Identifier Version
  union {
    SPendio_Data_Type SPendio_Data_B;
    char bytes_B[sizeof(SPendio_Data_Type)];
  };
  union {
    SPendio_Data_Type SPendio_Data_M;
    char bytes_M[sizeof(SPendio_Data_Type)];
  };
  union {
    SPendio_Data_Type SPendio_Data_T;
    char bytes_T[sizeof(SPendio_Data_Type)];
  };
  char temp[2];
  char umid[2];
  char pressao[5];
  char pluv[4];
  char bat[3];
  char final;
};
// Exemplo
// 01 1FB 1FF 256 03A7E 1F8 1F2 276 F423F 1F4 1F8 269 F423F 21 2B 15BC6 0026 0B5
//
union CPendio_LoRa_Sensor_Data_Type {
  char Bytes[sizeof(CPendio_Sensor_Data_Type)];
  CPendio_Sensor_Data_Type d;
};

typedef enum {
  E_CHUVA_REPOUSO = 0,
  E_CHUVA_INICIA,
  E_CHUVA_TEM,
  E_CHUVA_ESTAB,
  E_CHUVA_ANALISE,
} t_eChuvaEstado;

#define Nib(x)      ((x > 9)?('A' + x - 0xa):('0' + x))

#define DEBPmax      20
#define DEBDmax      20
#define TEMPO_DIAG 1000
#define FATOR_VBAT  21

//------------------------------------------------------------------------------
//  Variáveis e Classes Globais
//
global Adafruit_AHTX0 aht;
global Adafruit_BMP280 bmp;
global bool g_bBMPPresente;
global bool g_bDiag;

void vTaskVarreSensorChuva(void *pvParameters);
void iniSensores(CPendio_Sensor_Data_Type &dado);
void varrSensores(CPendio_Sensor_Data_Type &data);

#endif /* _SENSORES_H */
