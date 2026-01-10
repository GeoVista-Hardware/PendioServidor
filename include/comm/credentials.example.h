#ifndef _CREDENTIALS_H
#define _CREDENTIALS_H

/*

--------------------------------------------------------------------------------
    ARQUIVO DE CREDENCIAIS - NÃO ENVIAR PARA O GIT - EXEMPLO
--------------------------------------------------------------------------------

  Este é um arquivo TEMPLATE. Copie para "credentials.h" e substitua
  pelos valores reais do dispositivo.

  ⚠️  O arquivo credentials.h deve estar em .gitignore (nunca fazer commit!)
  
*/

// ============================================================================
// CREDENCIAIS LORAWAN
// ============================================================================
// Valores únicos para cada dispositivo (cadastrados no Kore)

const char APPEUI[] = "Seu AppEUI de 16 caracteres aqui";
const char APPKEY[] = "Sua AppKey de 32 caracteres aqui";

// ============================================================================
// CREDENCIAIS WI-FI (Modo Protótipo - PROTOTYPE_MODE_WIFI)
// ============================================================================
// Nome (SSID) e senha da rede Wi-Fi

const char WIFI_SSID[] = "Nome da sua rede Wi-Fi";
const char WIFI_PASSWORD[] = "Senha da sua rede Wi-Fi";

// ============================================================================
// CREDENCIAIS FIREBASE (Modo Protótipo - PROTOTYPE_MODE_WIFI)
// ============================================================================
// Obtidas no Console do Firebase (Project Settings -> Service Accounts)

const char FIREBASE_API_KEY[] = "Sua API Key do Firebase aqui";
const char FIREBASE_DB_URL[] = "seu-projeto.firebaseio.com"; 

// Identificador único do dispositivo (simula o DevEUI no LoRaWAN)
const char DEVICE_ID[] = "ESP32_PENDIO_IDENTIFICADOR";

#endif /* _CREDENTIALS_H */