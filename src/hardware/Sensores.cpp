/*
  --------------------------------------------------------------------------------
                                                              Início: 29/11/2023
        Proj.:  WCPendio - Sistema de monitoramento de Taludes e Encostas
        Fonte:  Sensores.cpp
        Progs:  Eng. Prof. Nuncio Perrella, MSc e Arnaldo
        Descr.: Funções de tratamento dos sensores
                Varredura Sensores T,M,S (SPendio)
                                                              Última: 16/12/2023
  --------------------------------------------------------------------------------
        Diario de Bordo
        ---------------
  28/11/2023 - Campeonato brasileiro 2023 : Vasco 2 x 4 Corinthians
  29/11/2023 - Fragmentação em arquivos
  15/12/2023 - Sensor de pressão
  15/12/2023 - Sensor de bateria
  16/12/2023 - Sensor de chuva

*/

#include "system_definitions.h"
#include "utils/Logger.h"
#include "hardware/Sensores.h"

const uchar TabHexa[] = {"0123456789ABCDEF"};
char inputBuffer[32];
int16_t contChuva;
static t_eChuvaEstado eChuvaEstado = E_CHUVA_REPOUSO;
static uint cdeb;                           // contador debounce
TaskHandle_t taskScanSensorHandle = NULL;
TaskHandle_t taskVarreSensorChuvaHandle = NULL;


//------------------------------------------------------------------------------
//  iniSensores - Inicializa sensores
//
void iniSensores(CPendio_Sensor_Data_Type &dado) {
  dado.frmFmtV[0] = '0';                  // Frame Format Version 0x01
  dado.frmFmtV[1] = '1';
  contChuva = 0;                          // Inicializa contador de chuva
  g_bDiag = false;                        // Modo diagnóstico desativado
  
  // Só cria a task de chuva se o sensor estiver habilitado
  #if SENSOR_RAIN_ENABLED
    xTaskCreate(vTaskVarreSensorChuva, "SENSOR CHUVA", configMINIMAL_STACK_SIZE + 1024, NULL, 1, &taskVarreSensorChuvaHandle);
    eChuvaEstado = E_CHUVA_INICIA;
    LOGI("SENSOR", "Task Chuva Iniciada");
  #else
    LOGI("SENSOR", "Task Chuva DESABILITADA");
  #endif

}

//------------------------------------------------------------------------------
//  vTaskVarreSensorChuva - Task de Varredura do Sensor de chuva
//
void vTaskVarreSensorChuva(void *pvParameters)
{

  #if SENSOR_RAIN_ENABLED
    int cont = 0;
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    while (1)
    {
      switch (eChuvaEstado) {
        case E_CHUVA_REPOUSO:
          break;
        case E_CHUVA_INICIA:
          cdeb = DEBDmax;
          eChuvaEstado = E_CHUVA_TEM;
          break;
        case E_CHUVA_TEM:                         // Tem chuva?
          if (!temChuva()) {
            if (cdeb) cdeb--;                     // não, aguarda depressionar
            else desWLED();
          }
          else {
            if (!cdeb) {
              cdeb = DEBPmax;                     // Sim há uma possibilidade de Chuva...
              eChuvaEstado = E_CHUVA_ESTAB;
            }
            else cdeb = DEBDmax;                  // Ruído, reinicia debouncing
          }
          break;
        case E_CHUVA_ESTAB:                       // Chuva estável?
          if (temChuva()) {
            if (cdeb) cdeb--;                     // aguarda debouncing
            else {
              ligWLED();
              eChuvaEstado = E_CHUVA_ANALISE;     // Sim...
            }
          }
          else {
            cdeb = DEBPmax;                       // Ruído, reinicia debouncing
          }
          break;
        case E_CHUVA_ANALISE:
          if (temChuva()) {
            cdeb++;
          }
          else {
            if (cdeb > TEMPO_DIAG) g_bDiag = true;  // Modo diagnóstico
            else contChuva++;                       // incrementa contador
            eChuvaEstado = E_CHUVA_INICIA;          // reinicia...
          }
          break;
      }
      vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1));
    }
  #else
  // Se a task for criada acidentalmente mas o sensor estiver desabilitado
    vTaskDelete(NULL);
  #endif
}

//------------------------------------------------------------------------------
//      int16Hex - Converte int8 em ASCII Hexa
//
void int16Hex(int16_t c, char *p) {
  uchar b;
  b = c >> 8;
  *p = Nib(b / 16);
  p++;
  *p = Nib(b % 16);
  p++;
  b = (char)c;
  *p = Nib(b / 16);
  p++;
  *p = Nib(b % 16);
}

//------------------------------------------------------------------------------
//      int8Hex - Converte int8 em ASCII Hexa
//
void int8Hex(int8_t c, char *p) {
  *p = Nib(c / 16);
  p++;
  *p = Nib((c & 0x0f));
}

//-----------------------------------------------------------------------------------------------------
//      leSenChuva - le Sensor de Chuva
//
void leSenChuva(char *p) {
  #if SENSOR_RAIN_ENABLED
    int16Hex(contChuva, p);
  #else
    // Preenche com 0000 se desabilitado
    *p = '0'; *(p+1) = '0'; *(p+2) = '0'; *(p+3) = '0';
  #endif
}

//-----------------------------------------------------------------------------------------------------
//      leSenBateria - le Sensor de Bateria
//
void leSenBateria(char *p) {

  #if SENSOR_BATTERY_ENABLED
    uchar b;
    int16_t vBat;
    int32_t soma=0;
    for (int i = 0; i < 8; i++) {
      soma += analogReadMilliVolts(aVBat);
      delay(1);                           // 1 ms
    }
    vBat = int16_t(soma >> 3);                   // média de 8
    if (vBat > 4095) vBat = 4095;                // limita 4095 (12 bits) 
    LOGD("SENSOR", "VBAT ADC=%d V=%ld mV", vBat, (long)(FATOR_VBAT * vBat));

    b = (uchar) vBat;
    vBat = vBat >> 8;
    *p     =   TabHexa[vBat & 0x0f];
    *(p + 1) = TabHexa[(b >> 4) & 0x0f];
    *(p + 2) = TabHexa[b & 0x0f];
  #else
    // Preenche com 000 se desabilitado
    *p = '0'; *(p+1) = '0'; *(p+2) = '0';
  #endif

}

//-----------------------------------------------------------------------------------------------------
//      mostraBufferLora - mostra Buffer LoRa
//
void mostraBufferLora(CPendio_Sensor_Data_Type &dado) {
  LOGD("SENSOR", "%s", dado.bytes_B);
}

//-----------------------------------------------------------------------------------------------------
//      mostraVetorAscii - mostra um vetor ASCII
//
void mostraVetorAscii(char *p, char tam) {
  for (int i = 0; i < tam; i++) {
    LOGD("SENSOR", "%c", *p);
  }
}

//-----------------------------------------------------------------------------------------------------
//      leSenTempUmid - le Sensor Temperatura e Umidade
//
void leSenTempUmid(char *t, char *u) {
  #if SENSOR_AHT_ENABLED
    sensors_event_t humidity, temperature;
    uchar tempC;
    int8_t umid;

    if (aht.getEvent(&humidity, &temperature)) {
      tempC = (char) temperature.temperature;
      umid  = (char) humidity.relative_humidity;
      LOGD("SENSOR", "%d*C %d%%", (int)tempC, (int)umid);
    }
    else {
      tempC = -100;
      umid = 0;
      LOGD("SENSOR", "Temperatura/Umidade não disponível");
      LOGD("SENSOR", "%d*C %d%%", (int)tempC, (int)umid);
      LOGW("SENSOR", "Humidity and temperature read fail");
    }
    *t       = TabHexa[(tempC >> 4) & 0x0f];
    *(t + 1) = TabHexa[(tempC & 0x0f)];
    *u       = TabHexa[(umid >> 4) & 0x0f];
    *(u + 1) = TabHexa[(umid & 0x0f)];
  #else
    // Preenche com 00 (Temp) e 00 (Umid) se desabilitado
    *t = '0'; *(t+1) = '0';
    *u = '0'; *(u+1) = '0';
  #endif
}

//-----------------------------------------------------------------------------------------------------
//      leSenTempPress - le Sensor Temperatura e Pressao
//
void leSenTempPress(char *p) {
  #if SENSOR_BMP_ENABLED
    int32_t pressao;
    uchar b, c;

    if (g_bBMPPresente) {
      LOGD("SENSOR", "Temperature = %.2f *C", bmp.readTemperature());

      pressao = (uint32_t)bmp.readPressure();
      LOGD("SENSOR", "Pressure = %lu Pa", (unsigned long)pressao);

      LOGD("SENSOR", "Approx altitude = %.2f m", bmp.readAltitude(1013.25));

      c = (char) pressao;
      pressao = pressao >> 8;
      b = (char) pressao;
      pressao = pressao >> 8;
      *p     = TabHexa[pressao & 0x0f];
      *(p + 1) = TabHexa[(b >> 4) & 0x0f];
      *(p + 2) = TabHexa[b & 0x0f];
      *(p + 3) = TabHexa[(c >> 4) & 0x0f];
      *(p + 4) = TabHexa[c & 0x0f];
    }
    else {
      LOGW("SENSOR", "Temperatura e Pressao falha!");
      *p     = '0';
      *(p + 1) = '0';
      *(p + 2) = '0';
      *(p + 3) = '0';
      *(p + 4) = '0';
    }  
  #else
    // Preenche com 00000 se desabilitado
    *p = '0'; *(p+1) = '0'; *(p+2) = '0'; *(p+3) = '0'; *(p+4) = '0';
  #endif             
}

//-----------------------------------------------------------------------------------------------------
//      zeraBufferLora - zera Buffer LoRa
//
void zeraBufferLora(char *p, int tam) {
  for (int i = 0; i < tam; i++) {
    *p = '0';
    p++;
  }
}

//-----------------------------------------------------------------------------------------------------
//      bufferLora - Monta Buffer LoRa
//
void bufferLora(char *p, int tam) {
  for (int i = 0; i < tam; i++) {
    if (inputBuffer[i] != ',') {
      *p = inputBuffer[i];
      p++;
    }
  }
}

//-----------------------------------------------------------------------------------------------------
//      convHStrHInt - Converte string Hexa para int
//
int convHStrInt(char *p, char tam) {
  int k, num, r, d;
  r = 0;
  d = tam - 1;
  for (k = 0; k < tam; k++) {
    num = *p - '0';
    if (num > 9) num = num - 7;
    num = num << ( d << 2);
    r = r + num;
    d--;
    p++;
  }
  return r;
}

//-----------------------------------------------------------------------------------------------------
//      mostraSenSPendio - Mostra Sensores SPendio
//
void mostraSenSPendio(char s) {
  #if SENSOR_SPENDIO_ENABLED
    char *p;
    int16_t num;
    int32_t solo;

    p = inputBuffer;
    LOGD("SENSOR", "Sensor %d", s);
    LOGD("SENSOR", "%s", inputBuffer);

    num = convHStrInt(p, 3);                          // x
    LOGD("SENSOR", "%d,", num);

    p += 4;
    num = convHStrInt(p, 3);                          // y
    LOGD("SENSOR", "%d,", num);

    p += 4;
    num = convHStrInt(p, 3);                          // z
    LOGD("SENSOR", "%d,", num);

    p += 4;
    solo = convHStrInt(p, 5);                         // solo
    LOGD("SENSOR", "%d", solo);
  #endif
}

//------------------------------------------------------------------------------
//  rs485_TX - Habilita Transmissão RS485
//
void rs485_TX(void) {
  #if SENSOR_SPENDIO_ENABLED
    digitalWrite(nRE, HIGH);
    digitalWrite(pDE, HIGH);
  #endif
}

//------------------------------------------------------------------------------
//  rs485_RX - Habilita Recepção RS485
//
void rs485_RX(void) {
  #if SENSOR_SPENDIO_ENABLED
    digitalWrite(pDE, LOW);
    digitalWrite(nRE, LOW);
  #endif
}

//-----------------------------------------------------------------------------------------------------
//      leSenSPendio - le Sensor SPendio
//
bool leSenSPendio(char sensor) {
  #if SENSOR_SPENDIO_ENABLED
    int timeout, num;
    rs485_TX();
    //delay(10);
    Serial2.write(sensor);
    Serial2.flush();
    rs485_RX();
    timeout = SPENDIO_TIMEOUT;
    while (timeout) {
      if (Serial2.available() > 0) {
        num = Serial2.readBytesUntil(LF, inputBuffer, sizeof(inputBuffer));
        return true;
        break;
      }
      delay(10);
      timeout--;
    }
    return false;
  #else
    return false;
  #endif
}

//-----------------------------------------------------------------------------------------------------
//      varrSensoresSPendio - Varre Sensores SPendio
//
void varrSensoresSPendio(CPendio_Sensor_Data_Type &dado) {
  #if SENSOR_SPENDIO_ENABLED
    if (leSenSPendio('S')) {                    // Sensor SPendio S
      mostraSenSPendio('S');
      bufferLora(dado.bytes_B, 17);
    }
    else {
      zeraBufferLora(dado.bytes_B, 14);
    }

    if (leSenSPendio('M')) {                    // Sensor SPendio M
      mostraSenSPendio('M');
      bufferLora(dado.bytes_M, 17);
    }
    else {
      zeraBufferLora(dado.bytes_M, 14);
    }

    if (leSenSPendio('T')) {                    // Sensor SPendio T
      mostraSenSPendio('T');
      bufferLora(dado.bytes_T, 17);
    }
    else {
      zeraBufferLora(dado.bytes_T, 14);
    }
  #else
    // Se desabilitado, zera todos os buffers relacionados ao SPendio
    zeraBufferLora(dado.bytes_B, 14);
    zeraBufferLora(dado.bytes_M, 14);
    zeraBufferLora(dado.bytes_T, 14);
  #endif
}

//-----------------------------------------------------------------------------------------------------
//      varrSensores - Varre Sensores
//
void varrSensores(CPendio_Sensor_Data_Type &dado) {
  ligLLED();

  varrSensoresSPendio(dado);                  // Sensores SPendio

  leSenTempUmid(dado.temp, dado.umid);        // Sensores de temperatura e umidade

  leSenTempPress(dado.pressao);               // Sensores de temperatura e pressão

  leSenChuva(dado.pluv);                      // Sensor de chuva

  leSenBateria(dado.bat);                     // Sensor de bateria

  dado.final = 0;                             // Finalizador

  //mostraBufferLora(dado);
  desLLED();
}
