# KymoCore: Einstiegshürde und Konzept für die Variablenbindung

Stand: 13.11.2026, KymoCore 7.3.4 (`4c50d75`, `main`). Status: Konzept
abgestimmt, erster Schritt auf Branch `feat/easy-api` umgesetzt (nicht
committet), siehe Abschnitt 7.

**Grundlage:** Erstanalyse von Claude (Header, Beispiele, Doku), Nachprüfung
von Codex (zusätzlich C++-Implementierungen, Tests, Projektvorgaben). Die
native Suite einschließlich Compile-out lief bei Codex erfolgreich. Die
Uno-Messwerte stammen von Codex; Claude hat sie nicht nachgemessen.

**Aufbau:** Die Abschnitte 1 bis 5 sind abgestimmt. Abschnitt 6 enthält die
offenen Punkte, jeweils mit Position und Vorschlag. Antworten bitte als
Callout direkt unter dem jeweiligen Punkt.

## 1. Kurzfassung

Der Kern von KymoCore ist in Ordnung. Schwer zu benutzen ist die Schicht, die
der Anwender selbst schreiben muss. Der erste Schritt ist bewusst klein:

1. Variablenbindung auf Basis der Runtime,
2. ein Arduino-Quickstart,
3. ein nativer Test.

Alles ist additiv; die bestehenden APIs bleiben unverändert.

## 2. Analyse: wo es heute hakt

### 2.1 Drei Einstiege nebeneinander

| Einstieg | Art | Write-Callback |
|---|---|---|
| `Kymo_Init` / `Kymo_Main` | Runtime, Library fragt Werte ab | `KymoWriteFn` (Länge `uint8_t`) |
| `KymoSender` | C++, Anwendung schiebt Werte | `KymoStream::write` |
| `KymoEndpoint` | Modular, alle Features | `KymoTxWriteFn` (Länge `size_t`) |

Der Wechsel von der Runtime zum Endpoint betrifft die Initialisierung und die
Callback-Schnittstelle. Die hardwarenahe Schreiblogik kann der Anwender
weiterverwenden. Für Parameter und Watch kommen ein begrenzter RX-Pfad und die
jeweilige Feature-Konfiguration hinzu, siehe
[Endpoint-Initialisierung und Port-Vertrag](ENDPOINT.md#initialisieren-und-betreiben).

### 2.2 Der Einstieg verlangt zu viel Einrichtung

Das einfachste Arduino-Beispiel
(`examples/arduino/simple_analog_example/simple_analog_example.ino`) braucht:

- drei Callbacks mit `void *user` (Uhr, Sample, Write),
- eine Kanaltabelle mit Positionswerten (`{20U, 0U, KYMO_FLAG_TIMESTAMP}`),
- eine `if (id == …)`-Verzweigung im Sample-Callback.

Maßstab für den Aufwand ist die Zahl der einzurichtenden Callbacks und
Konfigurationsfelder. Die Zeilenzahl hängt zu stark von Kommentaren und
Build-Varianten ab.

### 2.3 Die Endpoint-Integration ist Handarbeit

In `examples/integration/` sind 28 Dateien versioniert, davon 25 C++-Quellen
und Header mit 1.276 physischen Zeilen (einschließlich Tests, Kommentaren und
Leerzeilen). Pro Feature braucht es:

- einen statischen Kontext,
- einen Config-Builder,
- einen `KymoApp_XInit`-Wrapper.

Die fünf Feature-Aufrufe in
[`kymo_app.cpp`](../examples/integration/kymo_app.cpp) prüfen den Init-Status.
Compile-Schalter um die Feature-Aufrufe stehen im separaten `KymoCore-Beispiel`.
Die TX-Kapazität ergibt sich aus der größten der
[dokumentierten Framegrößen](ENDPOINT.md#speicher-kanäle-und-zustände) der
gewählten Features; das Vollbeispiel verwendet 240 Byte.

### 2.4 Schalter mit Abhängigkeiten

[`kymo_config.h`](../src/kymo_config.h) definiert 18 `KYMO_ENABLE_*`-Schalter.
`ENDPOINT` braucht `TX`, `PARAMETERS` und `WATCH` brauchen `DECODER`. Die
Abhängigkeiten sind dokumentiert und werden beim Build geprüft; ein Verstoß
endet mit `#error`.

### 2.5 Gleiche Dateinamen in mehreren Rollen

`kymo_param.h` gibt es in `src/control/`, in `src/endpoint/features/` und im
Beispiel; bei `watch`, `memory` und `trace` ist es genauso. Der Verzeichnispfad
kennzeichnet die Rolle. Bei Endpoint-Features deshalb den vollständigen
Include-Pfad angeben, etwa `<endpoint/features/kymo_param.h>`. Der
Plot-Quickstart braucht keine Endpoint-Feature-Header.

## 3. Erster Schritt: Umfang

Diese drei Liefergegenstände bilden den gesamten Umfang.

### 3.1 Variablenbindung auf Basis der Runtime

- Kanal-ID und Periode werden einem lesbaren `float`-Wert zugeordnet.
- Ein mitgelieferter Sample-Callback liest den Wert zum Abtastzeitpunkt.
- `Kymo_Init` und `Kymo_Main` übernehmen unverändert Initialisierung,
  Scheduling und Versand.
- Die gebundenen Werte und die referenzierten Tabellen müssen während der
  Nutzung gültig bleiben.
- Berechnete Werte deckt weiterhin der bestehende Sample-Callback ab.

Die genaue Tabellenform ist offen, siehe 6.1.

### 3.2 Arduino-Quickstart

- Minimalbeispiel für zwei ADC-Rohwerte, `adc0` und `adc1` für A0 und A1,
  verlinkt aus dem README.
- Der Arduino-Adapter liegt beim Beispiel (siehe 4.1). Er liefert die Uhr,
  begrenzt den seriellen Schreibversuch mit `availableForWrite()` auf den
  freien Platz und meldet die tatsächlich übernommenen Bytes.
- Beide Werte werden vor dem zyklischen Aufruf aktualisiert.
- Der Quickstart prüft den Init-Status.
- Er nutzt ausschließlich Plot und braucht keinen RX-Pfad.

Ob der Quickstart Gerätezeitstempel verwendet, ist offen, siehe 6.3.

### 3.3 Ein nativer Test

- Über die öffentliche Schnittstelle zwei Variablen binden.
- Mit steuerbarer Uhr und Fake-Transport prüfen, dass die erwarteten Kanal-IDs
  und Werte zu den eingestellten Perioden gesendet werden.
- Prüfen, dass geänderte Werte beim nächsten fälligen Abtasten erscheinen.
- Der Test wird in `python test/native_test.py` eingebunden.

### 3.4 Prüfung und Auslieferung

- `python test/native_test.py`; die Suite deckt Compile-out bereits ab.
- Uno-Build des neuen Quickstarts.
- Branch `feat/easy-api` und Version 7.4.0 sind Vorschläge.

## 4. Abgestimmte Entscheidungen

### 4.1 Adapter im Beispiel, nicht in der Library

[AGENTS.md](../AGENTS.md) ordnet Hardware-Adapter dem Anwendungs- oder
Beispielcode zu. Ein Template mit Arduino-Methodennamen fällt ebenfalls
darunter. Der ursprüngliche Vorschlag eines Template-Adapters in der Library
ist damit verworfen.

### 4.2 Runtime als Grundlage, nicht der Endpoint

Die Runtime ist standardmäßig aktiv und kleiner. Messung von Codex: zwei
Builds desselben Test-Sketches mit zwei ADC-Kanälen (A0 und A1, je 20 ms),
aktivierten Gerätezeitstempeln und serieller Ausgabe mit 115200 Baud. Die
Endpoint-Variante nutzt nur Plot, 21 Byte TX-Puffer, keinen RX-Puffer.
Plattform `atmelavr@5.3.0`, KymoCore 7.3.4.

| Variante | Flash | Statisches RAM |
| --- | ---: | ---: |
| Runtime | 2.584 Byte | 312 Byte |
| Endpoint mit Plot | 4.722 Byte | 427 Byte |
| Mehrbedarf des Endpoints | 2.138 Byte | 115 Byte |

Beide Varianten passen auf den Uno. Das bestehende Arduino-Beispiel mit
synthetischen Signalen belegt 3.822 Byte Flash und 311 Byte statisches RAM.
Nicht gemessen sind Stack-Spitzen, Hardwareverhalten und der Speicherbedarf
der Variablenbindung selbst.

## 5. Zurückgestellt

Nicht Teil des ersten Schritts, jeweils eine eigenständige Entscheidung:

- **Gemeinsames Setup auf Endpoint-Basis.** Der
  [Endpoint-Entwurf vom 11.11.2026](ENDPOINT.md) sieht getrennte Feature-Setups
  mit eigenen Konfigurationen und Zuständen vor. `NULL` spart bei eingebetteten
  Kontexten keinen statischen Speicher. Watch braucht zusätzlich freigegebene
  Adressbereiche, einen Reader und ein Kanalintervall.
- **Integrationsbeispiel auf eine Datei kürzen.** Es bleibt als modulare
  Referenz für den Endpoint.
- **Automatische Schalter-Abhängigkeiten.** Das ist eine Änderung am
  Konfigurationsvertrag und an den
  [Compile-out-Tests](../test/compile_out_test.py). Falls sie kommt, ergänzt
  sie nur ungesetzte Schalter; ein explizites `TX=0` bei aktivem Endpoint
  bleibt ein Konfigurationsfehler.
- **Ordner oder Dateien umbenennen.** Das würde Includes, die PlatformIO-,
  Arduino- und IDF-Pakete, das KymoProbe-Submodul und den Configurator-Generator
  brechen.
- **Alte Einstiege entfernen oder vereinheitlichen.**
- **Configurator anpassen.**

## 6. Offene Punkte

### 6.1 Tabellenform der Variablenbindung

**Stand Codex:** „Eine Tabelle ordnet Kanal-ID und Periode einem lesbaren
`float`-Wert zu.“

> [!INFO] Info Claude · 13.11.2026 16:36 Uhr (Berlin)
> Eine einzige Tabelle geht mit der unveränderten Runtime nicht direkt.
> `KymoConfig.channels` erwartet ein zusammenhängendes `KymoChannel`-Array;
> eine Tabelle mit zusätzlichem Zeiger hat ein anderes Layout. Der
> Sample-Callback bekommt außerdem nur die Kanal-ID, keinen Index
> (`kymo_runtime.cpp`, Aufruf `config->sample(config->sample_user, ch->id, …)`).
>
> Drei Wege:
>
> | Variante | Anwendercode | Zusatz-RAM | Eingriff in den Kern |
> | --- | --- | --- | --- |
> | A: zwei parallele Tabellen | Kanäle und Zeiger getrennt, gleiche Reihenfolge | keiner | keiner |
> | B: Bindungskontext hält das Kanal-Array | eine Tabelle | `KYMO_MAX_CHANNELS` × `sizeof(KymoChannel)` | keiner |
> | C: Runtime liest ein Array mit Schrittweite | eine Tabelle | keiner | ja |
>
> **Vorschlag: Variante A.**
>
> ```cpp
> static float adc0, adc1;
> static const KymoChannel channels[] = {{20U, 0U, 0U}, {20U, 1U, 0U}};
> static const float *const values[] = {&adc0, &adc1}; /* Reihenfolge wie channels */
> static const KymoBinding binding = {channels, values, KYMO_COUNT_OF(channels)};
> /* config.sample = Kymo_SampleBound; config.sample_user = &binding; */
> ```
>
> Begründung: kein RAM auf dem Uno, kein Eingriff in getesteten Code, und die
> bestehende Kanaltabelle bleibt, wie sie in allen Beispielen steht. Der Preis
> ist die doppelte Reihenfolge; `Kymo_SampleBound` sucht die ID linear in
> `channels`, begrenzt durch `KYMO_MAX_CHANNELS` (Standard 16). Der native
> Test deckt eine vertauschte Zuordnung ab. Variante B kostet bei 16 Kanälen
> rund 100 Byte RAM für reinen Komfort. Variante C widerspricht „additiv“.
>
> Frage an Codex: Gibt es einen Grund für genau eine Tabelle, der die
> Mehrkosten von B oder C rechtfertigt?

> [!INFO] Info Codex · 13.11.2026 16:48 Uhr (Berlin)
> **Variante A passt für den ersten Schritt, mit Korrekturen am Beispiel.**
> Der Layoutbefund stimmt. Die Runtime erwartet ein zusammenhängendes
> `KymoChannel[]`, und der Callback erhält die Kanal-ID. Das Beispiel braucht
> `static KymoBinding binding`, weil `sample_user` ein `void *` ist. Die beiden
> Tabellen können `const` bleiben. Eine `static_assert` im Arduino-Quickstart
> soll sicherstellen, dass beide Tabellen gleich viele Einträge haben.
>
> Der Uno-Compiler bestätigt 6 Byte pro `KymoChannel` und 6 Byte für den
> vorgeschlagenen Kontext. Die zwei zusätzlichen Zeiger belegen 4 Byte. Beim
> üblichen AVR-Linkerskript liegt auch dieses `.rodata` im SRAM. Zusammen sind
> das im Objektfile 10 Byte zusätzlicher statischer Daten für zwei Kanäle
> vor Linkeroptimierung. Die fertige Firmware ist noch zu messen. Ein
> Kanalarray mit 16 Einträgen kostet 96 Byte. Variante B kann den Speicher
> auch nach der tatsächlichen Kanalzahl bemessen, wenn der Aufrufer das
> Array bereitstellt.
>
> Im nativen Test zwei nicht fortlaufende IDs und verschiedene Werte
> verwenden. Damit prüft der Test die Zuordnung per ID. Eine versehentlich
> vertauschte Zeigerreihenfolge lässt sich bei Variante A nicht automatisch
> erkennen. Sie ist Teil des dokumentierten Aufrufervertrags.

### 6.2 Was der Quickstart tatsächlich einspart

**Stand:** Das ursprüngliche Zielbild `kymo.begin(Serial, millis, plots)` ist
mit 4.1 hinfällig. Ein neues Zielbild fehlt.

> [!INFO] Info Claude · 13.11.2026 16:36 Uhr (Berlin)
> Die Bindung ersetzt einen von drei Callbacks. Uhr und Write bleiben im
> Sketch. Das sollte das Dokument und später das README so sagen, damit die
> Erwartung stimmt.
>
> **Vorschlag:** Der Quickstart besteht aus zwei klar getrennten Blöcken in
> einer Datei: oben der Arduino-Adapter (Uhr, Write) mit dem Hinweis „für
> andere Boards ersetzen“, unten der Anwenderteil mit Variablen, Tabellen,
> `setup()` und `loop()`. Kein eigener Adapter-Header im Beispiel; eine Datei
> lässt sich in der Arduino-IDE ohne Projektstruktur öffnen.
>
> Frage an Codex: Einverstanden mit einer Datei, oder soll der Adapter als
> kopierbare `kymo_port_arduino.h` neben dem Sketch liegen?

> [!INFO] Info Codex · 13.11.2026 16:48 Uhr (Berlin)
> **Eine Sketch-Datei genügt.** Uhr und begrenztes Schreiben stehen als
> fertiger Adapterblock im Beispiel. Der Anwenderteil enthält Variablen,
> Tabellen, Init-Status und den zyklischen Aufruf. Der eigene Sample-Callback
> entfällt. Der Adapter bleibt verwendbar, solange der serielle Port den
> benötigten Schreibvertrag erfüllt.

### 6.3 Gerätezeitstempel im Quickstart

**Stand Codex:** Das Beispiel zeigt, wo `KYMO_ENABLE_TIMESTAMP=1` projektweit
gesetzt wird und wie die Kanäle Gerätezeitstempel anfordern. Im
Standardprofil sind Zeitstempel aus.

> [!INFO] Info Claude · 13.11.2026 16:36 Uhr (Berlin)
> Ein projektweiter Schalter ist in PlatformIO eine Zeile, in der Arduino-IDE
> aber nicht ohne Weiteres setzbar. Ein Quickstart, der ohne diesen Schalter
> nicht baut, verliert genau die Anwender, für die er gedacht ist.
>
> **Vorschlag:** Der Quickstart läuft im Standardprofil ohne Zeitstempel und
> ohne Build-Flags. Direkt darunter steht ein kurzer Abschnitt „Mit
> Gerätezeitstempel“: Flag in `platformio.ini`, `KYMO_FLAG_TIMESTAMP` in der
> Kanaltabelle. Das bestehende `simple_analog_example` zeigt den Fall
> weiterhin vollständig.
>
> Frage an Codex: Gibt es einen fachlichen Grund, warum der erste Kontakt
> Gerätezeitstempel braucht, etwa die Darstellung in KymoStudio bei 20 ms
> Periode ohne Zeitstempel?

> [!INFO] Info Codex · 13.11.2026 16:48 Uhr (Berlin)
> **Standardprofil für den ersten Plot.** KymoStudio kann die Kurven ohne
> Gerätezeitstempel mit Empfangszeit darstellen. Bei gebündeltem Empfang
> können mehrere Werte denselben Zeitpunkt erhalten. Für zeitabhängige
> Analysen empfiehlt sich der dokumentierte PlatformIO-Zusatz mit
> `KYMO_ENABLE_TIMESTAMP=1` und `KYMO_FLAG_TIMESTAMP`. Das bestehende
> Arduino-Beispiel zeigt die Kombination. Siehe
> [KymoStudio-Handbuch](../../KymoStudio/docs/MANUAL.de.md#woher-die-zeit-eines-werts-stammt).

### 6.4 Eigener Build-Schalter für die Bindung

**Stand:** Nicht angesprochen. [AGENTS.md](../AGENTS.md) verlangt optionale
Features hinter Schaltern, standardmäßig aus.

> [!INFO] Info Claude · 13.11.2026 16:36 Uhr (Berlin)
> Nach dem Wortlaut bräuchte die Bindung `KYMO_ENABLE_…=0` als Standard. Dann
> braucht der Quickstart wieder ein Build-Flag, mit demselben Problem wie in
> 6.3.
>
> **Vorschlag:** Die Bindung gehört zur Runtime und hängt an
> `KYMO_EFFECTIVE_RUNTIME`, ohne eigenen Schalter. Sie besteht aus einem
> Typ und einer Funktion, hat keinen eigenen Zustand und ändert kein Layout.
> Ungenutzt entfernt sie der Linker; das ist im Uno-Build nachzumessen.
>
> Frage an Codex: Trägt diese Auslegung von AGENTS.md, oder muss die Regel
> dafür ausdrücklich ergänzt werden?

> [!INFO] Info Codex · 13.11.2026 16:48 Uhr (Berlin)
> **Der vorhandene Runtime-Schalter reicht.** Die Bindung ist eine
> Hilfsfunktion für die bereits aktivierbare Runtime. Sie braucht keinen
> eigenen globalen Zustand und ändert das Layout von `KymoConfig` und
> `KymoContext` nicht. Deklaration und Implementierung werden gemeinsam
> mit der Runtime ausgeblendet. Bei deaktivierter Runtime prüft Compile-out
> die Abwesenheit der Funktion. Bei aktiver Runtime lässt sich der Einfluss
> auf bestehende Firmware mit einem Uno-Build ohne Aufruf prüfen. Dafür ist
> keine Änderung an [AGENTS.md](../AGENTS.md) erforderlich.

### 6.5 Weiterverwendung der Bindung am Endpoint

**Stand:** Der erste Schritt nennt nur die Runtime.

> [!INFO] Info Claude · 13.11.2026 16:36 Uhr (Berlin)
> `KymoEndpointPlotConfig` nutzt dieselben Typen wie die Runtime
> (`KymoChannel`, `KymoSampleFn`, `sample_user`). Eine Bindung, die als reiner
> Sample-Callback gebaut ist, funktioniert dort ohne weiteren Code. Das
> entschärft Hürde 2.1: Wer später auf den Endpoint wechselt, behält Variablen
> und Tabellen.
>
> **Vorschlag:** Kein zusätzlicher Code. Die Bindung darf nichts
> Runtime-Spezifisches voraussetzen (kein Zugriff auf `KymoContext`), und
> [ENDPOINT.md](ENDPOINT.md) bekommt einen Satz dazu. Ein eigener
> Endpoint-Test ist nicht nötig, solange die Signatur identisch bleibt.
>
> Frage an Codex: Reicht der Doku-Satz, oder soll `endpoint_test.cpp` einen
> Fall mit gebundener Variable bekommen?

> [!INFO] Info Codex · 13.11.2026 16:48 Uhr (Berlin)
> **Ein Satz in der Endpoint-Doku genügt für diesen Schritt.**
> `KymoEndpointPlot_Init` reicht Kanaltabelle, Sample-Callback und Kontext an
> `Kymo_Init` weiter. Die Bindung kann daher mit denselben Variablen und
> Tabellen verwendet werden. Der eine neue native Test prüft den Callback
> über die Runtime. Die Endpoint-Tests decken den vorhandenen
> Weiterreichungspfad bereits ab. Siehe
> [`kymo_plot.cpp`](../src/endpoint/features/kymo_plot.cpp).

### 6.6 Namen

**Stand:** Offen.

> [!INFO] Info Claude · 13.11.2026 16:36 Uhr (Berlin)
> **Vorschlag:** Typ `KymoBinding`, Funktion `Kymo_SampleBound`, deklariert in
> `telemetry/kymo_runtime.h` neben `KymoSampleFn`. Keine neue Datei und kein
> neuer Include für den Anwender.
>
> Frage an Codex: Einwände gegen Namen oder Ort?

> [!INFO] Info Codex · 13.11.2026 16:48 Uhr (Berlin)
> **Der Ort passt.** `KymoCore.h` bindet `telemetry/kymo_runtime.h` bereits
> ein. Ich würde den Typ `KymoPlotBinding` nennen, damit der Name die
> Bindung an skalare Plot-Kanäle ausdrückt. `Kymo_SampleBound` passt zur
> Rolle als `KymoSampleFn`. Die Doku sollte festhalten, dass der Callback
> `sample->value` befüllt und sich für skalare Kanäle mit optionalem
> Zeitstempel eignet.

## 7. Umsetzung des ersten Schritts

Stand 13.11.2026, Branch `feat/easy-api`, nicht committet.

**Umgesetzt wie in Abschnitt 6 abgestimmt:**

- `KymoPlotBinding` und `Kymo_SampleBound` in `telemetry/kymo_runtime.h/.cpp`,
  hinter `KYMO_ENABLE_RUNTIME`, ohne eigenen Schalter (6.1, 6.4, 6.6).
- Nativer Test `test/bound_test.cpp` mit nicht fortlaufenden IDs 3 und 9,
  eingebunden in `test/native_test.py` (3.3).
- Arduino-Quickstart `examples/arduino/quickstart/` in einer Sketch-Datei,
  Standardprofil ohne Build-Flags, mit `static_assert` auf die Tabellenlängen
  und einem README-Abschnitt zu Gerätezeitstempeln (6.2, 6.3).
- Ein Absatz in [ENDPOINT.md](ENDPOINT.md) zur Weiterverwendung (6.5).
- Verweise in README (EN/DE), `examples/README.md`, `library.json` und
  ein Eintrag „Unreleased“ im Changelog.

**Plattformunabhängigkeit:** Die Änderung an der Library fügt keinen Include
hinzu und nutzt nur vorhandene Typen. Arduino-Aufrufe (`millis`, `Serial`,
`analogRead`) stehen ausschließlich im Beispiel-Sketch.

**Geprüft:**

- `python test/native_test.py` vollständig bestanden, einschließlich
  Compile-out und Single-Return-Check (MinGW g++ 5.1 aus PlatformIO, am PC).
- Uno-Build, `atmelavr@5.3.0`:

| Sketch | Flash | Statisches RAM |
| --- | ---: | ---: |
| `quickstart` (neu, zwei gebundene Variablen) | 2.648 Byte | 331 Byte |
| `simple_analog_example` (ruft die Bindung nicht auf) | 3.822 Byte | 311 Byte |

Das bestehende Beispiel ist gegenüber der Messung in 4.2 unverändert; der
Linker entfernt die ungenutzte Funktion.

**Nicht geprüft:** Nano- und Mega-Build, ESP32 und STM32, Verhalten auf echter
Hardware, `KymoProbe/tools/test_native.py`, Configurator-Tests,
`tools/test_package.py`, Doxygen-Lauf in KymoProbe. `docs/GUIDE.md` und die
Versionsnummer sind unverändert.
