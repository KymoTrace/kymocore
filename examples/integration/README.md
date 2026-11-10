# Vollständiges Integrationsbeispiel / Complete integration example

Dieses Beispiel verbindet **Runtime, Parameter, Watch, Memory und Trace** über
die vorhandenen C-kompatiblen KymoCore-Schnittstellen. Die Modulverknüpfung nutzt
C++11 und statischen Speicher, ohne Exceptions oder RTTI. Es gibt keine neue Endpoint-Schnittstelle
und keine Änderungen an der Library.

Der mitgelieferte **native Port ist eine Simulation auf dem Rechner**: Er liest
Anfragen aus einer Datei, schreibt den binären Datenstrom in eine zweite Datei
und simuliert Uhr, Rückstau, Teilübertragungen und verzögerte Pufferfreigabe.
Damit lässt sich die Zusammenschaltung ohne Board prüfen. Datei-I/O selbst ist
kein Echtzeit-Treiber. Messwerte und Speichertelemetrie sind synthetisch.

## Dateien und Zuständigkeiten

| Datei | Hier wird angepasst |
|---|---|
| [main_native.cpp](main_native.cpp) | Dateipfade, 5000 Simulationszyklen und Abschlussfrist auf dem Rechner |
| [kymo_app.cpp](kymo_app.cpp) / [Header](kymo_app.h) | Explizite Modulinitialisierung, Empfang, Quellenreihenfolge und Trace-Abschluss |
| [kymo_tx.cpp](kymo_tx.cpp) / [Header](kymo_tx.h) | Ein Sendepuffer, Teilübertragungen, Rückstau und DMA-Pufferfreigabe |
| [kymo_bindings.cpp](kymo_bindings.cpp) / [Header](kymo_bindings.h) | Kanäle, Parameter, Messfunktionen, Watch-Bereiche, Memory- und Trace-Bindings |
| [kymo_port_native.cpp](kymo_port_native.cpp) / [Port-Vertrag](kymo_port.h) | Uhr, Lesen, Schreiben, Pufferfreigabe und Plattformstart |
| [kymo_port_native.h](kymo_port_native.h) | Dateistart/-ende ausschließlich für das native Startprogramm |
| [kymo_user_config.h](kymo_user_config.h) | Projektweite Feature-Schalter und Speichergrenzen |
| [kymo_trace_records.h](kymo_trace_records.h) | Trace-Metadaten und Symbolnamen für dieses feste Beispiel |
| [run.py](run.py) | Bauen, Host-Anfragen erzeugen und tatsächlichen Datenstrom prüfen |
| [CMakeLists.txt](CMakeLists.txt) | Alternativer Build ohne Python im Buildprozess |

Die Bindings besitzen langlebige Tabellen. `kymo_app.cpp` kopiert deren
Konfigurationen und ergänzt die Transportverbindung. Keine Bindings-Datei
enthält UART-, Arduino- oder RTOS-Aufrufe.

Fachwerte stehen als benannte Konstanten bei ihrer Zuständigkeit: Kanäle,
Grenzen und Messperioden in den Bindings, Empfangsbudget und Quellenzustände
in der Anwendung, Simulationsfristen im nativen Einstieg. Größen werden aus
Tabellen oder vorhandenen Kapazitätskonstanten abgeleitet. `run.py` verwendet
unabhängige benannte Protokollwerte und Wire-Strukturen statt fester Byte-Offsets.
Trace-IDs müssen weiterhin zum kanonischen JSON-Dictionary passen; die Tests
prüfen diesen Zusammenhang. Die Zahlen in diesen JSON-Daten sind Protokolldaten.

`kymo_app` ist die feste Anwendung dieses Beispiels, keine neue konfigurierbare
Library-Fassade. Sie kennt weder Dateien noch Simulationsfristen. `kymo_tx`
kennt keine Parameter, Watch-Slots oder Trace-Scheduler, sondern nur Frames
und den Port-Vertrag. Beide bleiben ausschließlich im Beispielverzeichnis.

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

## Was das Beispiel zeigt

| Funktion | Bindung / beobachtbares Ergebnis |
|---|---|
| Runtime | Kanal **0**, Sägezahn alle 50 ms, skaliert mit `gain` |
| Parameter | ID **100**, `gain` im Bereich 0..10, Beschreibung und Rücklesewert |
| Watch | Slot 0, Kanal **150**, virtuelle Adresse `0x20000000`, Float32 |
| Memory | Kanal **220**, 2048 freie Bytes als ausdrücklich synthetischer Wert |
| Trace | Kontext 1, Marker 1 (`main cycle`), Uhr mit 1000 Hz |

IDs sind bewusst getrennt. Ein reales Projekt muss auch die vom Host wählbaren
Watch-Kanäle gegen seine eigene Kanalbelegung abstimmen. Die Watch-Adresse ist
eine feste Abbildung im Reader und **kein gekürzter 64-Bit-Hostzeiger**.
Der Watch-Build-ID ist unbekannt; für echte ELF-basierte Variablenwahl muss der
Port die passenden Adressen, zulässigen Bereiche und den echten Build-ID liefern.

Der Runner schreibt `gain=2.5`, fordert die Parameterbeschreibung und
Watch-Informationen an und startet die Live-Variable. Zusätzlich sendet er
einen beschädigten CRC-Frame sowie einen Parameterwert außerhalb des Bereichs.
Beide dürfen den gültigen Wert nicht überschreiben. Der Port liefert die
Anfragen in Fragmenten von höchstens drei Bytes an beide vorhandenen Feed-APIs.

## Ablauf und gemeinsamer Versand

1. `main_native.cpp` öffnet Dateien und ruft einmal `exampleAppInit()` auf.
2. Pro Zyklus führt der Host `examplePortService()` und `exampleAppCycle()` aus.
3. Der Anwendungszyklus gibt höchstens 16 Empfangsbytes an Parameter und Watch
   und lässt `exampleTxService()` höchstens einen Schreibversuch ausführen.
4. Nur bei `KYMO_IDLE` bekommt eine Quelle ihren nächsten Turn. Ein vollständiger
   Frame wird mit `exampleTxSubmit()` übergeben und vor Rückkehr kopiert.

Der Sender meldet `KYMO_BUSY`, solange ein Frame wartet oder der Port seinen
Puffer noch besitzt. `exampleAppCycle()` behandelt diesen Rückstau als normalen
Zyklus und liefert `KYMO_OK`; echte Fehler werden gespeichert und zurückgegeben.
`exampleAppDrain()` beendet die Produktion und liefert bis zum vollständigen
Abschluss `KYMO_BUSY`, danach `KYMO_IDLE`. Ein erneutes Init oder ein Cycle nach
Beginn des Abschlusses wird abgelehnt und setzt keine Puffer zurück.

Die acht Quellen rotieren: Runtime, Parameterrücklesewert, Parameterbeschreibung,
Watch-Antwort, Watch-Messwert, Memory-Beschreibung, Memory-Messwert und Trace-Mux.
Eine leere Quelle kostet nur einen Turn. Der Mux verwaltet seine Trace- und
Admin-Frames weiterhin selbst. DATA und Control werden daneben als ihre
üblichen Frames gesendet; es ist ein gemischter Datenstrom, keine neue Verpackung.

**Bewusster Einfachheitskompromiss:** Die Anwendung hat einen zusätzlichen
**240-Byte-Sendepuffer**. Runtime und Mux kopieren jeweils einen vollständigen
Frame hinein und dürfen ihren eigenen Speicher danach sofort wiederverwenden.
Control-Antworten und einzelne Messpunkte werden zuerst in kleinen temporären
Stack-Puffern kodiert und anschließend ebenfalls hineinkopiert. Gegenüber der
zusammenhängenden Variante kostet die gekapselte Submit-Schnittstelle für diese
Quellen eine zusätzliche Kopie und temporären Stack-Speicher; der permanente
Sendepuffer bleibt bei 240 Bytes.
Die Port-Prüfung benötigt auf dem Host weitere 240 Bytes zum Erkennen vorzeitiger
Pufferänderungen; dieser Prüf-Speicher ist für einen echten Treiber nicht nötig.

Nur der Port darf den Anwendungspuffer ausleihen. `busy` schützt den Puffer auch
dann noch, wenn das letzte Byte bereits angenommen wurde. Bei Teilübertragungen
bleibt die Quelle bis zum vollständigen Frame exklusiv. Ein akzeptierter Wert
größer als die angebotene Länge stoppt den Versand dauerhaft. Eine neue Quelle
kann deshalb weder einen Frame überschreiben noch ihre Bytes dazwischen senden.
Unter Rückstau werden fällige Messungen später erfasst; es gibt kein Nachholen
aller verpassten Samples und keine Zusage verlustfreier Trace-Aufzeichnung.

Das native Startprogramm beendet nach 5000 simulierten Zyklen die Produktion.
Mit wiederholtem Port-Service und `exampleAppDrain()` werden bereits gepufferte Daten und
Trace werden ausgeleitet, dann `KymoMux_QueueSessionEnd` gesendet. Die zusätzliche
Abschlussfrist ist eine Host-Testgrenze, keine Freigabe für einen noch von DMA
verwendeten Puffer. Unbegrenzte Live-Verbindungen und Wiederverbindung sind
nicht Teil dieses endlichen Replay-Beispiels.

Trace-Session 1 und der beschreibende Build-ID sind feste Demo-Werte. Metadaten
und Dictionary werden einmal übertragen und zusammen geprüft. Bei Änderungen
an Namen/IDs muss der SHA-256 in den Metadaten mitgeändert werden. Auf Hardware
sind außerdem Session-ID und Taktangaben an die tatsächliche Laufzeit anzupassen.

## Auf ein Board übertragen

1. `kymo_user_config.h` projektweit für Anwendung und Library einbinden.
2. In `kymo_bindings.cpp` die synthetischen Reader durch Anwendungswerte ersetzen.
3. Den nativen Port durch UART/USB ersetzen: nichtblockierendes Lesen/Schreiben,
   echte Uhr und korrekte `busy`-Meldung bei DMA. Dateistart/-ende durch
   plattformspezifische Initialisierung und geordneten Verbindungsabbau ersetzen.
4. `main_native.cpp` durch den Board-Einstieg ersetzen: `exampleAppInit()` einmal,
   danach `examplePortService()` und `exampleAppCycle()` aus genau einer
   Superloop oder Task aufrufen und Statuswerte behandeln. Zum geordneten Ende
   nur noch Port-Service und `exampleAppDrain()` ausführen, bis `KYMO_IDLE`
   erreicht ist. `kymo_app.cpp`, `kymo_tx.cpp` und die Bindings bleiben unabhängig
   vom nativen Startprogramm. RTOS-/ISR-Aufrufe brauchen zusätzliche geprüfte
   Synchronisierung; dieses Beispiel verwendet keine parallelen Hooks.

Portierungen werden separat auf dem Zielboard geprüft. Das native Beispiel
bestätigt keine reale UART-, USB-, DMA- oder RTOS-Implementierung.

## Automatische Prüfung

`--check` prüft kopierendes Senden, verzögerte Freigabe, Ein-Byte-Teilübertragungen,
anfänglichen Rückstau, Null-Annahmen, Betrieb ohne Anfragen und einen fehlerhaften
Treiber. Der Decoder akzeptiert weder übersprungene Bytes noch unvollständige
Frames. Geprüft werden CRC8/CRC16/CRC32, Trace-Reihenfolge, Dictionary-Hash,
Session-Abschluss und Fortschritt aller aktiv angefragten Quellen.
Ein zusätzlicher [Sendertest](tests/tx_test.cpp) kontrolliert die Submit-Grenzen,
Kopiersemantik, Null-Annahmen, verzögerte Freigabe des letzten Bytes und den
dauerhaften Stopp nach einem Treiberfehler direkt mit einem kontrollierten Port.
Der [Anwendungstest](tests/app_test.cpp) baut den Anwendungsablauf mit einem
zweiten Port ohne Dateifunktionen und ohne nativen Programmeinstieg. Er prüft
auch Initialisierung, Abschluss und die Ablehnung eines erneuten Starts.

Die vollständige Prüfung läuft auch über `python test/native_test.py`.
Der Pakettest baut und startet das Beispiel ausschließlich aus dem exportierten
Archiv. Kleine Einzelmodul-Beispiele bleiben unter [examples](../README.md).

English: this is a complete, portable composition of the existing Runtime,
Parameter, Watch, Memory and Trace APIs. Port, bindings and application loop are
separate. Run `python examples/integration/run.py --check` from the library root.
The supplied port simulates file-backed RX/TX, short writes and delayed DMA
ownership; it is not a hardware driver. The application deliberately uses one
extra 240-byte frame copy and does not introduce an Endpoint implementation.
