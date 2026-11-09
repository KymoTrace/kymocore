# KymoCore-Konfigurator

Erstellt die Build-Konfiguration für **KymoCore 7** (Konfigurations-API 2): welche
Features einkompiliert werden, welche Kanäle gesendet werden und welche Funktionen der
Anwendung KymoCore mit der Hardware verbinden. Der Konfigurator ist ein optionales Entwicklungswerkzeug. Python-Engine und CLI benötigen nur die Standardbibliothek; PySide6 wird ausschließlich für die Widgets-Oberfläche gebraucht. Firmware-Builds verwenden feste erzeugte C/C++-Snapshots und starten weder Python noch Qt.

## Installation und Start

Mit Python 3.12 im KymoCore-Repository:

```powershell
python -m venv .venv-configurator
.venv-configurator\Scripts\python -m pip install -e .\tools\configurator
.venv-configurator\Scripts\python -m kymo_configurator new kymo.project.json --name KymoProbe
.venv-configurator\Scripts\python -m kymo_configurator gui kymo.project.json
```

## Zwei Arbeitsweisen

- **Geführte Einrichtung** (`gui --mode guided`): fragt zuerst nach dem Ziel (Werte über der Zeit
  sind immer dabei; X/Y- bzw. X/Y/Z-Punkte und Execution-Tracing lassen sich dazu ankreuzen) und führt dann Schritt für Schritt durch
  Kanäle, Callbacks, optional Tracing und Ausgabe. Fortgeschrittene Optionen behalten
  sichere Defaults; ein Schritt mit offenen Problemen lässt sich nicht vorwärts verlassen.
- **Profi-Modus** (`gui --mode professional`): alle Einstellungen auf einmal, nach Themen
  gegliedert, mit Live-Status je Thema (✓/⚠/✖), Problemen direkt am Feld, einer
  Prüfseite mit den effektiven Makros und einer Vorschau der erzeugten Dateien.

Ohne `--mode` bietet eine Startseite beide an. Die Oberfläche gibt es auf Englisch und Deutsch: `--lang en|de` oder die Sprachauswahl in der Werkzeugleiste (wird gemerkt); ohne Auswahl gilt die Systemsprache. Auch die Prüfmeldungen erscheinen in der Oberfläche in der gewählten Sprache; CLI- und `--json`-Ausgabe bleiben englisch. Beide Modi bearbeiten dasselbe Projekt,
ein Wechsel ist jederzeit möglich. Jede Einstellung zeigt eine verständliche Erklärung und
das betroffene C-Makro, Konfigurationsfeld oder die Funktionssignatur (in der geführten Einrichtung als Tooltip, damit Einsteiger nicht von Makronamen abgelenkt werden).

Für die GUI zusätzlich `pip install -e "tools/configurator[gui]"` ausführen. Unterstützt wird PySide6 `>=6.8.3,<6.10`; getestet wurde hier Python 3.12.9 mit PySide6 6.9.3.

## CLI

```text
python -m kymo_configurator validate kymo.project.json --profile development
python -m kymo_configurator generate kymo.project.json --profile development --out generated/kymo/development
python -m kymo_configurator check kymo.project.json --profile development --out generated/kymo/development
python -m kymo_configurator.build_check compile_commands.json
python -m kymo_configurator migrate old.project.json  --output kymo.project.json
python -m kymo_configurator init-port kymo.project.json --out src/kymo_port
python -m kymo_configurator gui kymo.project.json
```

Diagnosen haben stabile Codes und Feldpfade; `validate --json` liefert JSON. Validierung und `check` schreiben keine Dateien. `migrate` hebt Projekte von Schema 1 (KymoCore 6.x) auf Schema 2 (KymoCore 7.0) und schreibt standardmäßig eine neue Datei, damit die Eingabe erhalten bleibt; die GUI aktualisiert ältere Projekte beim Öffnen. Die Generierung legt einen unveränderlichen Stand unter `generations/<sha256>/` an und schaltet `active_generation.json` atomar um. `--force` ist nötig, wenn eine derzeit ausgewählte generatorverwaltete Datei manuell verändert wurde.

`build_check` liest die Argumentlisten aus `compile_commands.json` und prüft effektive Feature-Makros sowie die tatsächlich eingebundene generierte Konfiguration je Übersetzungseinheit. Der Befehl startet oder verändert den Build nicht.

`init-port` erzeugt Callback-Vorlagen in einem neuen Verzeichnis und überschreibt vorhandene Pfade nicht. Der Quelltext lässt den Runtime-Build absichtlich mit einem Fehler abbrechen, bis die TODOs durch echte Zielplattformimplementierungen ersetzt sind. Bei aktivem Trace-Profil entstehen zusätzlich Stubs für Trace-Uhr, Session, Write und Busy. Es werden keine Uhr-, Sensor- oder Transportfunktionen erfunden.

## Dateien und Grenzen

Jeder Snapshot enthält `kymo_user_config.h`, `kymo_generated_config.h/.cpp`, Trace-ID-/Dictionary-Dateien, ein Hashmanifest und Exporte für GCC, CMake und PlatformIO. PlatformIO verwendet `kymo_build_export.py` (globale Includes/Definitionen vor dem Build) und `kymo_build_sources.py` (generierte Quelldatei). Die C++-Datei nutzt die KymoCore-7-API: Kanaltabelle und `KymoConfig` mit `channels`, `channel_count`, `clock_ms`, `sample`, `write`, optional `busy`/`service` und konfigurierbaren Benutzerzeigern; `KYMO_MAX_CHANNELS` wird passend zur Kanaltabelle gesetzt. Callback-Implementierungen bleiben in handgepflegtem Portcode.

Der Benutzerheader muss projektweit über `KYMO_USER_CONFIG_HEADER` eingebunden werden, damit Anwendung und Core dieselben Werte erhalten. Für CMake wird `kymo_build_export.cmake` eingebunden und nach Definition beider Targets mit `kymo_configure_targets(application_target core_target)` aufgerufen. GCC-Variablen können in Make eingebunden werden; PlatformIO verwendet die generierten Pre-/Post-Skripte für den Snapshot-Include-Pfad und die generierte CPP-Datei.

Ein aktiviertes Trace-Profil erzeugt kanonische Bodies in `kymo_trace_metadata.json` und `kymo_trace_dictionary.json` sowie `kymo_trace_metadata.h` mit C-kompatiblen Bytearrays. Diese Arrays können an `KymoMux_QueueAdmin` übergeben werden; die API ergänzt CRC32. Der Dictionary-SHA-256 steht in Metadaten und Header. Ringgrößen und Core-Anzahl werden als Compile-Time-Einstellungen in `kymo_user_config.h` gesetzt; die Admin-Staging-Grenze wird passend zu den generierten Records gewählt (bis zum Core-Limit von 8192 Bytes). Scheduler-, ISR- und Core-State-Vollständigkeit bleiben `false`, solange das Projekt nicht explizit die entsprechenden `trace.port.*_complete`-Angaben setzt und die Portfähigkeiten auf Projektebene als verifiziert markiert. Portnamen erzeugen keine Fähigkeiten- oder RAM-Zusagen.

## Erweitern

Zusätzliche KymoCore-Features kommen als ein Katalog-Abschnitt
plus optionalem Widget, Validator und Renderer hinzu; beide GUI-Modi übernehmen sie
automatisch. Siehe [EXTENDING.md](EXTENDING.md). Das JSON-Format beschreibt
`schema/kymo-project-v2.schema.json`.
