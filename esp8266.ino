#include <Wire.h>
#include <VL53L0X.h>
#include <ESP8266WiFi.h>
#include "ThingSpeak.h"
#include "Constantes.h"

WiFiClient client;
bool attemptingToConnect = false;
unsigned int attemptingToConnectTime = 0;

VL53L0X sensor1;
VL53L0X sensor2;

#define XSHUT1 D3
#define XSHUT2 D4

int estado = 0;
int contador = 0;

// TIEMPO
unsigned long previousMillis = 0;  // Almacena el último tiempo
unsigned long previousMillis2 = 0; // Controlar Millis de IR()
unsigned long currentMillis;
unsigned int secondsCounter = 0; // Contador de segundos
int delayAttemptingToConnectWifi = 10;

bool estado_IR1 = false;
bool estado_IR2 = false;
const unsigned int velocidadLecturaIR = 10; // En Milisegundos
unsigned int IRInterval = 0;

void stopExecution(String error)
{
    Serial.println("Error: " + error);
    Serial.println("Deteniendo ejecución...");
    while (true)
    {
        // Bucle infinito para detener la ejecución
    }
}

void setup()
{
    Serial.begin(115200);

    delay(100);
    WiFi.mode(WIFI_STA);
    ThingSpeak.begin(client);

    Wire.begin();

    // Inicializa el primer sensor
    pinMode(XSHUT1, OUTPUT);
    pinMode(XSHUT2, OUTPUT);

    digitalWrite(XSHUT1, LOW); // Mantén el segundo sensor apagado
    digitalWrite(XSHUT2, LOW); // Mantén el primer sensor apagado

    delay(10);

    digitalWrite(XSHUT1, HIGH); // Enciende el primer sensor
    delay(10);
    sensor1.init();
    sensor1.setAddress(0x30); // Cambia la dIRección del primer sensor

    if (debug)
    {
        Serial.println("FIRst VL53L0X booted");
    }
    sensor1.startContinuous();

    digitalWrite(XSHUT2, HIGH); // Enciende el segundo sensor
    delay(10);
    sensor2.init();
    sensor2.setAddress(0x31); // Cambia la dIRección del segundo sensor

    if (debug)
    {
        Serial.println("Second VL53L0X booted");
    }
    sensor2.startContinuous();

    // verificaciones
    if (umbral >= 200 || umbral <= 5)
    {
        String error = "rango fuera de umbral.";
        stopExecution(error);
    }
}

void timeCounting()
{
    currentMillis = millis();

    // Verifica si ha pasado 10 milisegundos
    if (currentMillis - previousMillis2 >= velocidadLecturaIR)
    {
        previousMillis2 = currentMillis;
        IRInterval++;
    }

    if (currentMillis - previousMillis >= 1000)
    {
        previousMillis = currentMillis; // Actualiza el tiempo previo

        // Incrementa el contador de segundos
        secondsCounter++;

        if (attemptingToConnect)
        {
            attemptingToConnectTime++;
        }
    }
}

void debugConsole(int estado_IR1, int estado_IR2, int estado)
{
    // Limpiar consola
    for (int i = 0; i < 20; i++)
    {
        Serial.println("");
    }

    // ImprimIR datos
    Serial.println("Contador: " + String(contador));
    Serial.println("IR 1: " + String(estado_IR1));
    Serial.println("IR 2: " + String(estado_IR2));
    Serial.println("Tiempo: " + String(secondsCounter) + "s");
    Serial.println("Estado: " + String(estado));
}

void IR()
{
    if (IRInterval >= 1)
    {
        int estado_IR1 = ((sensor1.readRangeContinuousMillimeters() <= (umbral * 10)) ? 1 : 0);
        int estado_IR2 = ((sensor2.readRangeContinuousMillimeters() <= (umbral * 10)) ? 1 : 0);

        if (debug)
        {
            debugConsole(estado_IR1, estado_IR2, estado);
        }

        switch (estado)
        {
        case (0):
            if (estado_IR1 == 1 && estado_IR2 == 0)
            {
                estado = 1;
            }
            else if (estado_IR1 == 0 && estado_IR2 == 1)
            {
                estado = -1;
            }
            break;
        case (1):
            if (estado_IR1 == 1 && estado_IR2 == 1)
            {
                estado = 2;
            }
            else if (estado_IR1 == 0 && estado_IR2 == 0)
            {
                estado = 0;
            }
            break;
        case (2):
            if (estado_IR1 == 0 && estado_IR2 == 1)
            {
                estado = 3;
            }
            else if (estado_IR1 == 1 && estado_IR2 == 0)
            {
                estado = 1;
            }
            break;
        case (3):
            if (estado_IR1 == 0 && estado_IR2 == 0)
            {
                estado = 0;
                contador++;
            }
            else if (estado_IR1 == 1 && estado_IR2 == 1)
            {
                estado = 2;
            }
            break;
        case (-1):
            if (estado_IR1 == 1 && estado_IR2 == 1)
            {
                estado = -2;
            }
            else if (estado_IR1 == 0 && estado_IR2 == 0)
            {
                estado = 0;
            }
            break;
        case (-2):
            if (estado_IR1 == 1 && estado_IR2 == 0)
            {
                estado = -3;
            }
            else if (estado_IR1 == 0 && estado_IR2 == 1)
            {
                estado = -1;
            }
            break;
        case (-3):
            if (estado_IR1 == 0 && estado_IR2 == 0)
            {
                contador--;
                estado = 0;
            }
            else if (estado_IR1 == 1 && estado_IR2 == 1)
            {
                estado = -2;
            }
            break;
        }
        IRInterval = 0;
    }
}

void WiFiCheck()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        if (debug)
        {
            Serial.print("Intentando conectarse a WiFi...");
        }
        attemptingToConnect = true;
        attemptingToConnectTime = 0;
        while (WiFi.status() != WL_CONNECTED)
        {
            if (attemptingToConnectTime >= delayAttemptingToConnectWifi)
            {
                WiFi.begin(ssid, pass);
                attemptingToConnectTime = 0;
            }
            IR();
            timeCounting();
        }
        attemptingToConnect = false;
    }
}

void uploadData()
{
    if (secondsCounter >= intervalo)
    {
        ThingSpeak.setField(1, contador);
        int httpCode = ThingSpeak.writeFields(myChannel, writeAPIKey);
        if ((httpCode == 200) && debug)
        {
            Serial.println("Channel write successful.");
        }
        else if (httpCode != 200)
        {
            String error = "No se pudo conectar con el canal: " + String(myChannel) + ". httpCode: " + String(httpCode);
            stopExecution(error);
        }
        secondsCounter = 0;
    }
}

void loop()
{
    timeCounting();
    WiFiCheck();
    IR();
    uploadData();
}