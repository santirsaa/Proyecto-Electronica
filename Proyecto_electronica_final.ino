#include "DHT.h"
#define DHTPIN D1        
#define DHTTYPE DHT11
#define vent D2       // Cable azul (PWM a 25 kHz) del cooler Intel
#define led D5       // Luz calefactora / LED indicador
#define bomba D4     // //  Pin IN1 del L298N hacia la bomba

DHT dht(DHTPIN, DHTTYPE);


float tempMax = 24.0;    // Sobre 24°C: enciende ventilador
float tempMin = 23.0;    // Bajo 23°C: enciende luz
float humMin  = 50.0;    // Bajo 50%: activa bomba de riego

// --- Memoria circular de las últimas 10 mediciones con marca de tiempo ---
const int MAX_REGISTROS = 10;
struct Medicion {
  float temp;
  float hum;
  unsigned long tiempoSeg;
};

Medicion historial[MAX_REGISTROS];
int indiceActual = 0;
int totalRegistros = 0;

// Temporización de muestreo en segundo plano (cada 2 segundos)
unsigned long ultimoMuestreo = 0;
const unsigned long intervaloMuestreo = 2000;


void consola();
void ventilador();
void grafica();
void variables();
void estado();
String entrada();

void setup() {
  Serial.begin(115200);

  // Configuración del ventilador Intel (PWM a 25 kHz)
  pinMode(vent, OUTPUT);
  analogWriteFreq(25000);
  analogWrite(vent, 0);

  // Configuración de la luz calefactora
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);

  // Configuración de la bomba (L298N)
  pinMode(bomba, OUTPUT);
  digitalWrite(bomba, LOW);

  dht.begin();

  Serial.println("\n[SISTEMA DE INVERNADERO INICIADO]");
  Serial.println("Comandos disponibles en la consola:");
  Serial.println("  HISTORIAL   -> Entrega lista y GRÁFICAS de Temp y Humedad vs Tiempo");
  Serial.println("  PARAMETROS  -> Muestra y permite modificar los umbrales");
  Serial.println("  ESTADO      -> Muestra la medición y estado en vivo");
  Serial.println("--------------------------------------------------------------------\n");
}

void loop() {
  consola();

  if (millis() - ultimoMuestreo >= intervaloMuestreo) {
    ultimoMuestreo = millis();
    ventilador();
  }
}

void ventilador() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) return;

  historial[indiceActual].temp = t;
  historial[indiceActual].hum = h;
  historial[indiceActual].tiempoSeg = millis() / 1000;
  indiceActual = (indiceActual + 1) % MAX_REGISTROS;
  if (totalRegistros < MAX_REGISTROS) totalRegistros++;

  // Control de Temperatura
  if (t > tempMax) {
    analogWrite(vent, 1023);
    digitalWrite(led, LOW);
  } else if (t < tempMin) {
    analogWrite(vent, 0);
    digitalWrite(led, HIGH);
  } else {
    analogWrite(vent, 0);
    digitalWrite(led, LOW);
  }

  // Control de Humedad (Bomba)
  if (h < humMin) {
    digitalWrite(bomba, HIGH);
  } else {
    digitalWrite(bomba, LOW);
  }
}

void grafica() {
  if (totalRegistros == 0) {
    Serial.println("\n[!] Aún no hay lecturas almacenadas en memoria.\n");
    return;
  }

  int inicio = (totalRegistros < MAX_REGISTROS) ? 0 : indiceActual;
  unsigned long tiempoActual = millis() / 1000;

  Serial.println("\n==================================================================");
  Serial.println("           HISTORIAL Y GRÁFICAS EN FUNCIÓN DEL TIEMPO             ");
  Serial.println("==================================================================");

  Serial.println("N°  | Tiempo Relativo | Temperatura | Humedad");
  Serial.println("----+-----------------+-------------+--------");
  for (int i = 0; i < totalRegistros; i++) {
    int pos = (inicio + i) % MAX_REGISTROS;
    long segundosAtras = tiempoActual - historial[pos].tiempoSeg;

    Serial.print(" "); Serial.print(i + 1);
    if (i + 1 < 10) Serial.print("  | hace "); else Serial.print(" | hace ");
    if (segundosAtras < 10) Serial.print(" ");
    Serial.print(segundosAtras); Serial.print(" seg     |    ");
    Serial.print(historial[pos].temp, 1); Serial.print(" °C   |  ");
    Serial.print(historial[pos].hum, 1); Serial.println(" %");
  }

  Serial.println("\n------------------------------------------------------------------");
  Serial.println(" GRÁFICA 1: TEMPERATURA (°C) vs TIEMPO");
  Serial.println(" Escala: [15°C] . . . . [20°C] . . . . [25°C] . . . . [30°C] . . . . [35°C]");
  Serial.println("------------------------------------------------------------------");

  for (int i = 0; i < totalRegistros; i++) {
    int pos = (inicio + i) % MAX_REGISTROS;
    long segAtras = tiempoActual - historial[pos].tiempoSeg;

    Serial.print("-");
    if (segAtras < 10) Serial.print(" ");
    Serial.print(segAtras);
    Serial.print("s | ");

    int columnas = map((int)(historial[pos].temp * 10), 150, 350, 0, 25);
    if (columnas < 0) columnas = 0;
    if (columnas > 25) columnas = 25;

    for (int c = 0; c < columnas; c++) Serial.print("-");
    Serial.print("*");
    for (int c = columnas + 1; c <= 25; c++) Serial.print(" ");
    Serial.print(" ("); Serial.print(historial[pos].temp, 1); Serial.println(" °C)");
  }

  Serial.println("\n------------------------------------------------------------------");
  Serial.println(" GRÁFICA 2: HUMEDAD RELATIVA (%) vs TIEMPO");
  Serial.println(" Escala: [0%] . . . . . [25%] . . . . . [50%] . . . . . [75%] . . . . . [100%]");
  Serial.println("------------------------------------------------------------------");

  for (int i = 0; i < totalRegistros; i++) {
    int pos = (inicio + i) % MAX_REGISTROS;
    long segAtras = tiempoActual - historial[pos].tiempoSeg;

    Serial.print("-");
    if (segAtras < 10) Serial.print(" ");
    Serial.print(segAtras);
    Serial.print("s | ");

    int columnas = map((int)historial[pos].hum, 0, 100, 0, 25);
    if (columnas < 0) columnas = 0;
    if (columnas > 25) columnas = 25;

    for (int c = 0; c < columnas; c++) Serial.print("=");
    Serial.print("#");
    for (int c = columnas + 1; c <= 25; c++) Serial.print(" ");
    Serial.print(" ("); Serial.print(historial[pos].hum, 1); Serial.println(" %)");
  }
  Serial.println("==================================================================\n");
}

String entrada() {
  while (!Serial.available()) {
    delay(10);
  }
  String entrada = Serial.readStringUntil('\n');
  entrada.trim();
  return entrada;
}

void variables() {
  Serial.println("\n=============================================");
  Serial.println("          CONFIGURACIÓN DE PARÁMETROS        ");
  Serial.println("=============================================");
  Serial.print("1. Temp Máxima (Ventilador): "); Serial.print(tempMax); Serial.println(" °C");
  Serial.print("2. Temp Mínima (Luz):        "); Serial.print(tempMin); Serial.println(" °C");
  Serial.print("3. Humedad Mínima (Bomba):   "); Serial.print(humMin); Serial.println(" %");
  Serial.println("---------------------------------------------");
  Serial.print("¿Deseás modificar algún parámetro? (S/N): ");

  String rta = entrada();
  Serial.println(rta);

  if (rta.equalsIgnoreCase("S")) {
    Serial.print("Nueva Temp Máxima (Enter para conservar "); Serial.print(tempMax); Serial.print(" °C): ");
    String inMax = entrada();
    Serial.println(inMax);
    if (inMax.length() > 0) tempMax = inMax.toFloat();

    Serial.print("Nueva Temp Mínima (Enter para conservar "); Serial.print(tempMin); Serial.print(" °C): ");
    String inMin = entrada();
    Serial.println(inMin);
    if (inMin.length() > 0) tempMin = inMin.toFloat();

    Serial.print("Nueva Humedad Mínima (Enter para conservar "); Serial.print(humMin); Serial.print(" %): ");
    String inHum = entrada();
    Serial.println(inHum);
    if (inHum.length() > 0) humMin = inHum.toFloat();

    Serial.println("\n[OK] Parámetros actualizados:");
    Serial.print("T_MAX: "); Serial.print(tempMax);
    Serial.print(" °C | T_MIN: "); Serial.print(tempMin);
    Serial.print(" °C | HUM_MIN: "); Serial.print(humMin);
    Serial.println(" %\n");
  } else {
    Serial.println("Parámetros sin modificaciones.\n");
  }
}

void estado() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  Serial.println("\n--- ESTADO ACTUAL BAJO DEMANDA ---");
  if (isnan(h) || isnan(t)) {
    Serial.println("Error leyendo el sensor DHT11.");
  } else {
    Serial.print("Temperatura:  "); Serial.print(t, 1); Serial.println(" °C");
    Serial.print("Humedad:      "); Serial.print(h, 1); Serial.println(" %");
    Serial.print("Ventilador:   "); Serial.println(t > tempMax ? "ENCENDIDO" : "APAGADO");
    Serial.print("Luz:          "); Serial.println(t < tempMin ? "ENCENDIDA" : "APAGADA");
    Serial.print("Bomba Riego:  "); Serial.println(h < humMin ? "ENCENDIDA" : "APAGADA");
  }
  Serial.println("----------------------------------\n");
}

void consola() {
  if (Serial.available() > 0) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    if (comando.length() == 0) return;

    if (comando.equalsIgnoreCase("HISTORIAL")) {
      grafica();
    } else if (comando.equalsIgnoreCase("PARAMETROS")) {
      variables();
    } else if (comando.equalsIgnoreCase("ESTADO")) {
      estado();
    } else {
      Serial.println("[!] Comando no reconocido. Opciones: HISTORIAL, PARAMETROS, ESTADO");
    }
  }
}
