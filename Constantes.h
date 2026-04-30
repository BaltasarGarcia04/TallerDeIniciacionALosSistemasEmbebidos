
#ifndef CONSTANTES_H
#define CONSTANTES_H
// Constantes.h

// debug
const bool debug = true;

// Rango máximo de detección de IR (5cm > umbral 200cm)
const int umbral = 50;

// ThingSpeak
// ID Canal contador 
const unsigned long myChannel = 2586942;
// API key Canal contador
const char* writeAPIKey = "VLD1JSPEVQKZEON7";
// Frecuencia de subida de datos (Mayor o igual a 20 segundos)
const long intervalo = 60;

// Wifi
// SSID
const char ssid[] = "";
// Contraseña
const char pass[] = "";


#endif