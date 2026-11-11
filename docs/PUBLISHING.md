# KymoCore veröffentlichen

[Dokumentationsübersicht](README.md) · [Projektübersicht](../README.de.md)

Alle Befehle werden aus dem Repository-Wurzelverzeichnis ausgeführt,
sofern kein anderes Arbeitsverzeichnis genannt ist.

## Separate development checkout

Library development takes place in `Plotter/KymoCore`, on the official `main`
branch. `Plotter/KymoProbe/lib/KymoCore` is a consumer checkout pinned to a
published release tag. Do not develop the library in that consumer checkout.
Run `python tools/test_package.py` here to validate the exported archive and
build real Uno/ESP32 consumers before publishing. Then update the Probe pin.

## Paket und Voraussetzungen

Veröffentlichungseinheit ist dieses Repository **kymocore**. In der Firmware
[kymoprobe](https://github.com/KymoTrace/kymoprobe) ist es als Git-Submodul `lib/KymoCore` eingebunden.
`library.json` liegt im Paketwurzelverzeichnis. Quellen, Lizenz, README,
Protokoll, Bilder und portables Beispiel werden explizit exportiert.
Die größeren Hardware- und Transportbeispiele liegen im KymoProbe-Repository.

PlatformIO benötigt ein gültiges Manifest. `headers` und `examples` sind
zusätzliche optionale Metadaten. Lizenzdatei, Changelog, Bilder und ein
Installationsbeispiel vervollständigen das nutzbare Paket; nicht alles davon
ist eine technische Pflicht des Registry-Validators.

Die Kurzbeschreibung im Manifest darf maximal 255 Zeichen haben. Die
ausführliche Registry-Dokumentation steht in README.md. PNG-Bilder liegen im
Archiv unter `docs/images` und sind relativ verlinkt; sie benötigen weder
Mermaid-Unterstützung noch einen externen Bildhost. Die tatsächliche Darstellung
im Registry-Frontend nach Veröffentlichung zusätzlich prüfen.

## Lokale Prüfung

Im KymoCore-Repository (Python 3.10+, GCC/G++ und PlatformIO auf PATH):

```sh
python test/native_test.py
python tools/test_package.py
python -m pytest -q tools/configurator/tests
```

Zusätzlich im Firmware-Repository kymoprobe (Submodul initialisiert):

```sh
python tools/test_native.py --app ../KymoStudio
pio run -e nodemcu-32s
pio run -d examples/arduino/multi_sensor_example -e uno
pio run -d examples/esp32/wifi_mqtt_example -e esp32dev
pio run -d examples/stm32/uart_example -e nucleo_f401re
```

`test_package.py` validiert das Manifest mit `pio pkg pack`, kontrolliert
Archivinhalt und lokale Dokumentationslinks, kompiliert und startet das
portable Beispiel und baut den vollständigen README-Sketch auf Uno und ESP32.
Diese Verbraucher installieren ausschließlich das Archiv, keine Repo-Symlinks.
Der schnelle Modus `--skip-firmware` lässt nur die beiden Boardbuilds aus.
Buildartefakte und getrennte Verbraucher liegen unter `.pio/package/`.

Ergebnis ist `KymoCore-7.3.2.tar.gz` mit separater SHA256-Datei. Das Archiv
enthält den aktuellen Arbeitsstand einschließlich neuer, noch uncommitteter
Dateien. Nach Änderungen neu packen und prüfen. CI prüft das Paket ebenfalls;
ein lokal erfolgreicher Lauf beweist keinen erfolgreichen Remote-CI-Lauf.

Die Dokumentationsbilder können mit Pillow neu erzeugt werden:
`python tools/render_library_diagrams.py`. Pillow ist nur ein Dokuwerkzeug,
keine Abhängigkeit der Library oder des Paketchecks.

## Vor dem Upload noch festlegen beziehungsweise prüfen

1. PlatformIO-Account oder Organisation und Schreibberechtigung bestimmen.
   GitHub-Owner und PlatformIO-Owner müssen nicht identisch sein.
2. Mit `pio pkg show codename666/KymoCore@7.3.2` prüfen, ob die Version schon
   existiert. Netzwerk- oder Authentifizierungsfehler beweisen keine freie Version.
   Ein einmal veröffentlichtes Name-/Versionspaar lässt sich nicht erneut
   verwenden, auch nicht nach dem Löschen. Bei Bedarf beide Manifeste,
   Installationsbeispiele und Changelog auf eine neue Version setzen.
3. Lizenz ist `MIT` (LICENSE), im Manifest ebenso. Vor Veröffentlichung sicherstellen, dass alle Quellen eine
   SPDX-Zeile tragen und für jeden fremden Beitrag die Rechteeinräumung aus
   [Beitragsregeln](CONTRIBUTING.de.md) vorliegt.
4. Änderungen reviewen, gezielt committen und in diesem Repository pushen.
   Einen Release-Tag `vX.Y.Z` anlegen; die Git-Installation über `lib_deps`
   verwendet ihn. Danach in kymoprobe das Submodul `lib/KymoCore` auf den Tag
   setzen und committen.
5. Den kompatiblen KymoStudio-Stand öffentlich bereitstellen beziehungsweise
   dessen NO_CRC-Unterstützung prüfen. Der lokale Integrationstest verwendet
   `../KymoStudio`; er prüft nicht automatisch den öffentlich erreichbaren Stand.
6. Der bestätigte PlatformIO-Namespace ist `codename666`. Die
   Installationsbeispiele müssen zum Release passen. Erneut packen und testen;
   Prüfergebnisse und unterstützte Boards dokumentieren.

## Upload als eigener Schritt

Diese Vorbereitung veröffentlicht nichts. Nach Freigabe am eigenen Account
anmelden und das geprüfte Archiv veröffentlichen:

```sh
pio account login
pio pkg publish .pio/package/KymoCore-7.3.2.tar.gz --owner codename666 --type library
```

Danach die Registry-Seite und Bilder öffnen, `pio pkg show` prüfen und in einem
frischen Projekt `codename666/KymoCore @ 7.3.2` installieren. Das validiert die
Registry-Auslieferung zusätzlich zum bereits geprüften lokalen Archiv.

Hardwaretests sind gesondert nötig: Flashen, Live-Übertragung, DMA/IRQ, USB und
Broker-Verbindungen sind keine Folgen eines erfolgreichen Compile-Tests.

## Arduino Library Manager

Der Library Manager liest `library.properties` und indiziert jeden Git-Tag,
dessen `library.properties`-Version zum Tag passt. Ein eigener Upload entfällt.

- Prüfung: `arduino-lint --library-manager update --compliance strict` mit
  absolutem Pfad zum Repository. Der Workflow
  `.github/workflows/registries.yml` führt dieselbe Prüfung aus.
- Den registrierten Namen `KymoCore` beibehalten.
- Neue Versionen: Version in `library.properties` erhöhen und Tag `vX.Y.Z`
  pushen. Der Index aktualisiert sich innerhalb etwa eines Tages.

## ESP Component Registry

`idf_component.yml` und das Wurzel-`CMakeLists.txt` machen das Repository zu einer
ESP-IDF-Komponente (`codename-666/kymocore`, ab ESP-IDF 5.0). Das Beispiel
`examples/esp_idf/uart_telemetry` baut gegen den lokalen Stand; der Workflow
`registries.yml` baut es mit ESP-IDF 5.4 und packt das Komponentenarchiv.
Lokal funktioniert das nur, wenn der Repository-Ordner `kymocore` heißt: ESP-IDF
benennt eine Komponente aus `override_path` nach ihrem Ordner, in `KymoCore`
schlägt der Build mit „unknown name“ fehl. Für den Test daher
`git clone https://github.com/KymoTrace/kymocore.git` verwenden.

Einmalige Einrichtung:

1. Auf [components.espressif.com](https://components.espressif.com) mit GitHub
   anmelden und den Namespace prüfen. Weicht er von `codename-666` ab, ihn in
   `.github/workflows/publish-idf.yml`, im Beispiel-`idf_component.yml` und in
   dieser Datei anpassen.
2. Unter **Permissions** die Komponente `kymocore` anlegen und als Trusted
   Uploader dieses Repository mit dem Workflow `publish-idf.yml` eintragen (OIDC,
   kein Token-Secret nötig).
3. Im GitHub-Repository die Variable `ESP_REGISTRY_UPLOAD` auf `true` setzen.
4. Ersten Upload über **Actions → Publish to the ESP Component Registry → Run
   workflow** starten. Danach lädt jeder Tag `vX.Y.Z` hoch; der Workflow prüft,
   dass Tag und `idf_component.yml` übereinstimmen.

Wie bei PlatformIO ist jede Version unveränderlich. Die Versionen in
`library.json`, `library.properties` und `idf_component.yml` müssen gleich sein
(`test_package.py` prüft das).

## Offizielle Referenzen

- [Library-Struktur und Veröffentlichung](https://docs.platformio.org/en/latest/librarymanager/creating.html)
- [Manifestfelder und Validierung](https://docs.platformio.org/en/latest/manifests/library-json/index.html)
- [Kurzbeschreibung bis 255 Zeichen](https://docs.platformio.org/en/latest/manifests/library-json/fields/description.html)
- [Paket packen](https://docs.platformio.org/en/latest/core/userguide/pkg/cmd_pack.html)
- [Veröffentlichen und unveränderliche Versionen](https://docs.platformio.org/en/latest/core/userguide/pkg/cmd_publish.html)
- [Arduino Library Manager: Anmeldung](https://github.com/arduino/library-registry/blob/main/FAQ.md)
- [Arduino-Library-Spezifikation](https://arduino.github.io/arduino-cli/latest/library-specification/)
- [ESP Component Registry: Komponente veröffentlichen](https://docs.espressif.com/projects/idf-component-manager/en/latest/guides/packaging_components.html)
- [KymoStudio](https://kymotrace.de/#download)
