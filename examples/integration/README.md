# Modulare Integration mit Endpoint

Dieses Beispiel verbindet **Plot, Parameter, Watch, Memory und Trace** mit dem
optionalen KymoCore-Endpoint. Jedes Feature hat eine eigene Datei mit seinem
Namen und enthält nur seine Konfiguration, Bindings und Initialisierung.
Die gemeinsame Basis stellt Port, Puffer und Endpoint bereit. Danach genügt
`KymoApp_Main()` für Empfang, Verarbeitung und Versand.

## Einstieg und Dateien

Beginne mit [kymo_app.cpp](kymo_app.cpp): Dort stehen ausschließlich die
Initialisierung der Basis und die ausgewählten Feature-Initialisierungen.
Alle Rückgabewerte werden geprüft. Die Configs werden nach Namen befüllt;
referenzierte Tabellen, Puffer und Callback-Kontexte bleiben dauerhaft gültig.

| Datei | Zuständigkeit |
|---|---|
| [kymo_init.h](kymo_init.h) / [CPP](kymo_init.cpp) | Gemeinsames Init, Main und optionaler Stop; statischer Endpoint und Puffer |
| [features/kymo_plot.h](features/kymo_plot.h) / [CPP](features/kymo_plot.cpp) | Messkanäle, Perioden und Sample-Callback |
| [features/kymo_param.h](features/kymo_param.h) / [CPP](features/kymo_param.cpp) | Parameter, Grenzen und Zielvariablen |
| [features/kymo_watch.h](features/kymo_watch.h) / [CPP](features/kymo_watch.cpp) | Lesbare Adressen, Reader und reservierte Watch-Kanäle |
| [features/kymo_memory.h](features/kymo_memory.h) / [CPP](features/kymo_memory.cpp) | Speicherwerte, Namen und Abtastraten |
| [features/kymo_trace.h](features/kymo_trace.h) / [CPP](features/kymo_trace.cpp) | Recorder, Metadaten, Dictionary und Instrumentierung |
| [demo_values.h](demo_values.h) / [CPP](demo_values.cpp) | Gemeinsam verwendete Anwendungsvariable `exampleGain` |
| [kymo_port.h](kymo_port.h) | Plattformunabhängiger Port-Vertrag |
| [kymo_port_native.cpp](kymo_port_native.cpp) | Simulierte Uhr, RX/TX, Teilübertragungen und Pufferfreigabe |
| [main_native.cpp](main_native.cpp) | Dateien öffnen, Simulationsdauer und Abschlussfrist |
| [kymo_user_config.h](kymo_user_config.h) | Einheitliche Build-Schalter und Speichergrenzen |
| [kymo_trace_records.h](kymo_trace_records.h) | Statische Trace-Metadaten und Symbolnamen |
| [run.py](run.py) / [CMakeLists.txt](CMakeLists.txt) | Native Buildwege und Prüfungen |

Die Projektdateien kapseln statische Instanzen und Anwendungswerte. Die eigentliche
Koordination liegt in [der Library](../../src/endpoint/kymo_endpoint.h), die
Feature-Adapter in `src/endpoint/features/`. Keiner der Feature-Header bindet
die anderen Feature-Fassaden ein. Es gibt keine Dateien mit `_setup` im Namen.
Details zur Library-Schnittstelle stehen im [Endpoint-Leitfaden](../../docs/ENDPOINT.md).

## Starten

Vom **KymoCore-Verzeichnis** mit Python 3.10+ und `g++` auf PATH:

```sh
python examples/integration/run.py
python examples/integration/run.py --check
```

Mit benachbartem KymoStudio-Checkout prüft zusätzlich
`python examples/integration/run.py --check --app ../KymoStudio` genau diese
Captures mit dem echten Streamdecoder und der Trace-Session-Verarbeitung.

Ein anderer Compilerpfad kann mit `--cxx /pfad/zu/g++` angegeben werden.
Python verwendet ausschließlich die Standardbibliothek; Konfigurator und Qt
werden nicht benötigt. Der eigentliche C++-Build benötigt Python nicht.

Die Ausgabe liegt standardmäßig unter `.pio/integration/partial-dma/`:

- `integration` beziehungsweise `integration.exe`: ausführbares Beispiel.
- `requests.bin`: simulierte Host-Anfragen.
- `capture.bin`: unveränderter binärer KymoCore-Ausgabestrom.

Exitcode **0** bedeutet einen erfolgreichen Lauf einschließlich Trace-Abschluss,
**1** einen Start-, Datei- oder Abschlussfehler und **2** einen erkannten
Transportvertragsfehler. Keine Textausgabe wird in den Binärstrom gemischt.
Der Python-Runner prüft anschließend den Inhalt, nicht nur den Exitcode.

Alternativ per CMake, ebenfalls vom Library-Verzeichnis:

```sh
cmake -S examples/integration -B .pio/integration-cmake
cmake --build .pio/integration-cmake
```

Danach `kymo_integration` mit zwei **verschiedenen** Dateipfaden aufrufen:

```sh
.pio/integration-cmake/kymo_integration .pio/integration/partial-dma/requests.bin .pio/integration-cmake/capture.bin
```

Unter Windows heißt das Programm `kymo_integration.exe`; bei Visual Studio liegt
es zusätzlich im gewählten Konfigurationsordner, z. B. `Debug`.
Die Eingabedatei darf leer sein. Die Ausgabedatei wird überschrieben.
`run.py` erzeugt die oben verwendete Anfragedatei.

Beide Buildwege setzen `KYMO_USER_CONFIG_HEADER` für **jede** Übersetzungseinheit,
auch für die Library. Ein `#define` nur in einer Anwendungsdatei wäre dafür unzureichend.
Das Beispiel benötigt alle fünf aktivierten Module; es ist kein Test aller
beliebigen Feature-Kombinationen.

## Beispieldaten

| Feature | Konfiguration |
|---|---|
| Plot | Kanal 0, Sägezahn alle 50 ms, skaliert mit `exampleGain` |
| Parameter | ID 100, Gain im Bereich 0..10 |
| Watch | Slot 0, Kanal 150, virtuelle Adresse `0x20000000`, Float32 |
| Memory | Kanal 220, synthetisch 2048 freie von 4096 Bytes |
| Trace | Session 1, Kontext 1, Marker 1, Uhr mit 1000 Hz |

Kanäle, Raten und Grenzen stehen als benannte Konstanten beim jeweiligen Feature.
Der Endpoint prüft Überschneidungen. Watch reserviert hier ausschließlich Kanal
150 und lehnt andere Kanäle mit `OUT_OF_RANGE` ab. Die Adresse ist eine feste
Abbildung im Reader, kein gekürzter Hostzeiger. Ein Board-Port benötigt seine
wirklichen Speicherbereiche und einen passenden Build-ID.

## Ablauf

1. Der native Einstieg öffnet die Dateien und ruft `exampleAppInit()` auf.
2. Die Anwendung initialisiert Basis und ausgewählte Features jeweils einmal.
3. Der Einstieg ruft zyklisch `KymoApp_Main()` auf. Der Endpoint bedient dabei
   selbst den Port-Service, liest höchstens das RX-Budget und bearbeitet TX.
4. Bei freiem Sender erhält eine Quelle ihren Turn. Trace-Metadaten und
   Dictionary werden vor den Trace-Ereignissen übertragen.
5. Zum Abschluss wird `KymoApp_RequestStop()` aufgerufen; danach läuft derselbe
   Main-Aufruf bis `KYMO_IDLE`. Neue RX-Anfragen und Messungen werden gestoppt;
   gepufferte Daten und Trace einschließlich Session-End werden ausgeleitet.

`KYMO_BUSY` ist normaler Rückstau; `KYMO_OK` bedeutet Arbeit und `KYMO_IDLE`
aktuell keine Arbeit beziehungsweise abgeschlossenen Stop. Ein ungültiger
Messwert kann `KYMO_BAD_SAMPLE` liefern, ohne andere Features zu stoppen.
Initialisierungs- und Transportfehler müssen behandelt werden.

Der Endpoint verwendet den vorhandenen `KymoTx`: Ein zusätzlicher, statischer
240-Byte-Puffer enthält jeweils eine vollständige Frame-Kopie. Pro Main gibt es
höchstens einen physischen Schreibversuch. `busy` schützt ausgeliehene Bytes auch
nach Annahme des letzten Bytes; fremde Quellen können keine Bytes dazwischen
senden. `write` darf Teilmengen oder null annehmen, aber niemals mehr als angeboten.
Unter Rückstau entstehen spätere Messungen und gegebenenfalls Trace-Verluste;
es gibt keine Zusage verlustfreier Aufzeichnung.

Der native Port simuliert einen Treiber auf dem Rechner. Datei-I/O ist kein
Echtzeit-Treiber. Die Abschlussfrist im Test gibt einen DMA-Puffer nicht frei.
Es gibt keinen automatischen Neustart oder Reconnect. Trace-Session, Metadaten
und Dictionary sind feste Demo-Werte. Änderungen an Symbolen erfordern einen
angepassten Dictionary-Hash; Hardware benötigt passende Session- und Taktangaben.

## Auf das eigene Projekt übertragen

1. `kymo_init`, den Port-Vertrag und die benötigten Feature-Dateien übernehmen.
2. Die jeweilige Feature-Config und ihre Reader an die eigenen Werte anpassen.
3. Nicht benötigte Feature-Init-Aufrufe und Dateien entfernen und die passenden
   Build-Schalter deaktivieren. Den gemeinsamen TX-Puffer für die größte
   verbleibende Frame-Art dimensionieren.
4. Einen nichtblockierenden UART-/USB-Port implementieren; `busy` muss bei DMA
   bis zur tatsächlichen Freigabe gesetzt bleiben.
5. Einmal initialisieren, anschließend `KymoApp_Main()` aus einer Superloop oder
   Task aufrufen. Port-Service nicht zusätzlich aufrufen.
6. Benötigte Trace-Makros aus dem Trace-Header in die Anwendung einbetten.
   Parallele Hooks erfordern externe Synchronisierung mit dem Endpoint sowie
   geeignete Recorder-Locks; vor Stop müssen alle Produzenten ruhen.

Kein Feature verlangt Arduino-, UART- oder RTOS-Aufrufe in seinen Bindings.
Die native Simulation bleibt vom Anwendungsablauf getrennt. Reale Treiber und
RTOS-/ISR-Anbindung müssen auf dem Zielsystem geprüft werden.

## Automatische Prüfung

`--check` prüft kopierendes Senden, verzögerte Freigabe, Ein-Byte-Teilübertragungen,
Rückstau, Null-Annahmen, Betrieb ohne Anfragen und einen fehlerhaften Treiber.
Geprüft werden vollständige Frames, CRC8/CRC16/CRC32, Fortschritt der Quellen,
Trace-Reihenfolge, Dictionary-Hash und Session-Abschluss. Mit `--app` werden die
Captures zusätzlich vom echten KymoStudio-Decoder verarbeitet.

Vergleich mit dem bisherigen Beispiel (`968920e`): Frame- und Ereigniszahlen
bleiben in allen vier gültigen Captures gleich. Die Captures sind nicht bytegleich:
Die jetzt im Anwendungsablauf vor Main gesetzten Trace-Marker liegen vor dem
Port-Service und damit einen simulierten Tick früher. Session-End entsteht beim
Trace-Turn der gemeinsamen Abschlussrotation statt in einer eigenen Drain-
Schleife. Nur diese Trace-Zeitwerte und die zugehörigen Prüfsummen ändern sich;
DATA, Control, Metadaten, Dictionary und Ereignisinhalte bleiben gleich.

Der [Sendertest](tests/tx_test.cpp) prüft den KymoTx-Vertrag separat.
Der [Anwendungstest](tests/app_test.cpp) verwendet einen zweiten Port ohne
Dateifunktionen und ohne nativen Programmeinstieg. Die native Library-Suite
prüft zusätzlich Feature-Kombinationen, C99-Aufrufer, Header-Isolation,
Compile-out, Kanalüberschneidungen und Fehlerbehandlung.

English: selected feature modules initialize independently; the shared Endpoint
coordinates RX and TX through one Main call. The supplied file port simulates
partial writes and delayed DMA ownership. Run `python examples/integration/run.py
--check` from the library root. See the [library API guide](../../docs/ENDPOINT.md).
