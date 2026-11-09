<p align="center">
  <a href="README.md"><img alt="English" src="https://img.shields.io/badge/%F0%9F%8C%90-English-A78BFA"></a>
  <a href="README.de.md"><img alt="Deutsch" src="https://img.shields.io/badge/%F0%9F%8C%90-Deutsch-15123A"></a>
</p>

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/images/kymotrace-logo-dark.svg">
    <img src="docs/images/kymotrace-logo-light.svg" alt="Kymotrace – Embedded Telemetry" width="480">
  </picture>
</p>

<h3 align="center">KymoCore · Messwerte aus dem Mikrocontroller, live auf dem Bildschirm</h3>

<p align="center">
  Portable C++11-Telemetrie-Library für Arduino, ESP32 und STM32 – das Herzstück von <b>Kymotrace</b>.
</p>

<p align="center">
  <img alt="Version 7.2.1" src="https://img.shields.io/badge/version-7.2.1-7C5CFF">
  <img alt="Lizenz MIT" src="https://img.shields.io/badge/Lizenz-MIT-15123A">
  <img alt="C++11 mit C-API" src="https://img.shields.io/badge/C%2B%2B11-C--API-A78BFA">
  <img alt="Plattformen" src="https://img.shields.io/badge/Arduino%20%C2%B7%20ESP32%20%C2%B7%20STM32-PlatformIO-FDE047">
</p>

---

## Worum geht es?

Der optionale [Kymo-Konfigurator](tools/configurator/README.de.md) bietet eine
geführte Schritt-für-Schritt-Einrichtung und einen Profi-Modus mit allen Einstellungen,
dazu eine Qt-freie CLI für versionierte Projektprofile. Erzeugte
Konfigurationsstände lassen sich ohne Python-Generator und ohne Qt kompilieren.
Compilerdefinitionen haben Vorrang vor dem Projektheader und den Bibliotheksvorgaben.
`KYMO_ENABLED=0` schaltet die Instrumentierung aus; anwendungseigene Telemetrieressourcen
müssen mit den effektiven Feature-Makros geschützt werden. Die Fassade liegt in
`KymoCore.h` (oder `trace/kymo_instrumentation.h`). Ungeschützte direkte API-Aufrufe werden im Off-Build
bewusst als Übersetzungsfehler erkannt.

Wer Sensoren auswertet, einen Regler einstellt oder einen Motor ansteuert, will
**sehen, was im Mikrocontroller gerade passiert**: sofort und nicht erst nach
dem Auslesen einer Logdatei. `Serial.print` stößt dabei schnell an Grenzen. Text
kostet Rechenzeit und Bandbreite, blockiert die Hauptschleife und muss auf dem
PC mühsam zerlegt werden.

**KymoCore** löst genau das. Die Library liest deine Messwerte im
eingestellten Takt, verpackt sie in kompakte Binärframes von **9 bis 21 Bytes**
und gibt sie an eine beliebige Schnittstelle weiter, ohne den Hauptzyklus
aufzuhalten. Auf dem PC zeigt [KymoStudio](https://kymotrace.de/#download)
die Signale live als Kurve, als XY-Bahn oder in 3D.

<p align="center">
  <img src="docs/images/kymostudio-workbench.png" alt="KymoStudio zeigt Messwerte live als Zeitverlauf und XY-Diagramm" width="900">
  <br><sub>Live-Daten in KymoStudio: drei Signale im Zeitverlauf, zwei als XY-Bahn.</sub>
</p>

## Was KymoCore kann

| | |
|---|---|
| **Viele Kanäle, eigener Takt** | Bis zu 256 Kanäle, jeder mit eigenem Abtastintervall. Ein Round-Robin-Scheduler entscheidet, wer dran ist. |
| **Mehr als eine Zahl** | Y, XY oder XYZ pro Messpunkt, optional mit Zeitstempel und CRC-Prüfsumme. |
| **Kompakt auf der Leitung** | 9–21 Bytes pro Messpunkt, float32, Little Endian. Ein feststehendes, dokumentiertes [Protokoll](docs/PROTOCOL.md). |
| **Blockiert nie** | Einmal `Kymo_Init()`, dann zyklisch `Kymo_Main()`. Läuft ohne RTOS, ohne Heap, ohne Exceptions und ohne RTTI. |
| **Jede Schnittstelle** | UART, USB-CDC, WLAN/TCP, UDP, MQTT, CAN …: Du lieferst einen `write`-Callback, KymoCore liefert die Bytes. |
| **Langsame Leitungen inklusive** | Teilübertragungen, Rückstau und ausgeliehene DMA-Puffer werden sauber behandelt. |
| **Nur was du brauchst** | X, Z, Zeitstempel, CRC, Decoder und C++-API lassen sich zur Compile-Zeit einzeln zuschalten. Standard ist minimal. |
| **C und C++** | C++11-Kern mit C-kompatibler API, auch aus C99-Projekten nutzbar. |

## Wofür man es einsetzt

- **Regelungstechnik:** Soll-, Ist- und Stellgröße eines PID-Reglers live vergleichen und die Parameter einstellen.
- **Sensorentwicklung:** Rauschen, Drift und Sprungantwort von Sensoren sichtbar machen.
- **Motoren und Leistungselektronik:** Ströme, Drehzahlen und Positionen mit Zeitstempel aufzeichnen.
- **Prüfstände und Dauerläufe:** viele Kanäle parallel überwachen; mit der optionalen CRC werden Übertragungsfehler erkannt.
- **Robotik und Bewegung:** Bahnen und Trajektorien direkt als XY- oder XYZ-Kurve darstellen.
- **Lehre, Studium und Maker-Projekte:** Messdaten verständlich machen, statt Zahlenkolonnen im seriellen Monitor zu lesen.

## So funktioniert es

![Vom Messwert zum Diagramm](docs/images/architecture.png)

1. **Deine Anwendung** stellt Uhr und Messfunktion bereit und legt fest, welche Kanäle wie oft gemessen werden.
2. **KymoCore** entscheidet, welcher Kanal fällig ist, ruft deine Messfunktion auf und kodiert den Frame.
3. **Dein Transport** (UART, USB, TCP, UDP, MQTT …) nimmt die Bytes entgegen, auch stückweise.
4. **KymoStudio** setzt den Datenstrom wieder zusammen, prüft ihn und zeichnet die Kurven.

Die Library enthält bewusst **keine Hardwaretreiber**. Sie bleibt dadurch auf
jeder Plattform gleich, vom Arduino Uno bis zum STM32 mit DMA.

## Schnellstart

In der `platformio.ini` deines Projekts eintragen:

```ini
lib_deps =
    codename666/KymoCore @ 7.2.1
```

### Vollständiges Beispiel für Arduino und ESP32

Folgender Inhalt für `src/main.cpp` sendet auf Kanal 0 alle 20 ms einen
synthetischen Sägezahn zwischen 0 und knapp 1. Er verwendet ausschließlich
die Standardeinstellungen. Die serielle Schnittstelle bleibt für Binärdaten
reserviert; Diagnoseausgaben würden denselben Datenstrom verändern.

```cpp
#include <Arduino.h>
#include <KymoCore.h>

static KymoContext context = {};
static KymoStatus lastStatus = KYMO_BAD_CONFIG;
/* period_ms, id, flags: channel 0, every 20 ms, scalar value only. */
static const KymoChannel channels[] = {{20U, 0U, 0U}};

/*******************************************************************************
 * readClockMs
 ******************************************************************************/
static uint32_t readClockMs(void *user)
{
    uint32_t result = static_cast<uint32_t>(millis());
    (void)user;
    return result;
}

/*******************************************************************************
 * readSample
 ******************************************************************************/
static uint8_t readSample(void *user, uint8_t id, KymoSample *out)
{
    uint8_t result = 0U;
    (void)user;
    (void)id;
    if (out != nullptr) {
        out->value = static_cast<float>(millis() % 1000UL) / 1000.0f;
        result = 1U;
    }
    return result;
}

/*******************************************************************************
 * writeBytes
 ******************************************************************************/
static uint8_t writeBytes(void *user, const uint8_t *bytes, uint8_t length)
{
    uint8_t result = 0U;
    int available = Serial.availableForWrite();
    (void)user;
    if ((bytes != nullptr) && (available > 0)) {
        if (available < length) {
            length = static_cast<uint8_t>(available);
        }
        result = static_cast<uint8_t>(Serial.write(bytes, length));
    }
    return result;
}

/*******************************************************************************
 * setup
 ******************************************************************************/
void setup()
{
    KymoConfig config = {};   /* copied by Kymo_Init, may be local */
    Serial.begin(115200);
    config.channels = channels;                     /* what */
    config.channel_count = KYMO_COUNT_OF(channels);
    config.clock_ms = readClockMs;                  /* when */
    config.sample = readSample;                     /* which values */
    config.write = writeBytes;                      /* where to */
    lastStatus = Kymo_Init(&context, &config);
}

/*******************************************************************************
 * loop
 ******************************************************************************/
void loop()
{
    if ((lastStatus != KYMO_BAD_CONFIG) && (lastStatus != KYMO_IO_ERROR)) {
        lastStatus = Kymo_Main(&context);
    }
}
```

`int available` folgt dem Rückgabetyp der Arduino-API; vor der Verengung wird
der Wertebereich geprüft. Serial kopiert die angenommenen Bytes; deshalb ist
hier kein Busy-Callback nötig. Die tatsächliche Ausführungszeit hängt vom
Serial-Treiber ab. Den Messcallback später durch den Sensorzugriff ersetzen.

Mit `pio run` bauen, anschließend mit `pio run -t upload` auf das angeschlossene
Board übertragen. KymoStudio mit 115200 Baud und 8N1 verbinden und Kanal 0
einem Diagramm zuordnen. Einen geöffneten seriellen Monitor vorher schließen.
Ein vollständig SDK-freies, ausführbares Beispiel liegt in
[examples/basic](examples/basic/README.md).

## Unterstützte Plattformen

KymoCore ist plattformunabhängig. Die größeren Hardwarebeispiele in
[KymoProbe](https://github.com/KymoTrace/kymoprobe) decken diese Ziele ab:

| Familie | Boards | Transport im Beispiel |
|---|---|---|
| Arduino AVR | Uno, Nano, Mega | UART |
| ESP32 | ESP32, ESP32-S3, ESP32-C3 | UART, WLAN/MQTT |
| STM32 | Nucleo F401RE, F411RE, Blue Pill F103C8 | UART, USB-CDC |
| PC (nativ) | GCC/Clang | Testprogramme |

## Das Protokoll auf einen Blick

![Aufbau eines Frames](docs/images/protocol.png)

Die vollständige Spezifikation mit Beispielframes steht in [PROTOCOL.md](docs/PROTOCOL.md).

## Teil von Kymotrace

| Projekt | Rolle |
|---|---|
| **[KymoCore](https://github.com/KymoTrace/kymocore)** | diese Library: Messwerte erfassen und senden |
| [KymoProbe](https://github.com/KymoTrace/kymoprobe) | Firmware und Hardwarebeispiele für ESP32, Arduino und STM32 |
| [KymoStudio](https://kymotrace.de/#download) | Desktop-App: empfangen, darstellen, analysieren, exportieren |

## Dokumentation

[Alle Dokumente im Überblick](docs/README.md)

- **[Anleitung und API-Referenz](docs/GUIDE.de.md):** Konfiguration, Scheduling, Transporte und Dimensionierung
- **[Protokoll](docs/PROTOCOL.md):** Byte-Aufbau, CRC, Beispielframes
- **[Portables Beispiel](examples/basic/README.md):** ohne Hardware auf dem PC ausführbar
- **[Kleine Referenzbeispiele](examples/README.md):** portables Init/Main und Arduino-Signalbeispiele
- **[Änderungen](CHANGELOG.md)** · **[Veröffentlichung](docs/PUBLISHING.md)** · **[Mitmachen](docs/CONTRIBUTING.de.md)**

## Lizenz

Copyright (c) 2026 Christof Seidel. KymoCore steht unter der
[MIT-Lizenz](LICENSE): frei für jede Nutzung, auch in kommerzieller Firmware und
in Geräten, ohne den eigenen Quellcode offenzulegen. Jede Version behält die
Lizenz, unter der sie veröffentlicht wurde: 6.2.0 bis 7.1.0 unter GPLv3 oder
kommerzieller Lizenz, bis 6.1.0 unter MIT.
