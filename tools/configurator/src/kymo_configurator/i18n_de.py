# SPDX-License-Identifier: MIT
"""German user interface texts, keyed by the English original.

Keep ``{placeholders}`` identical; tests/test_i18n.py checks that every visible
text is translated. Names of other UI elements quoted in a text use their German
label (for example 'Weiter', 'Build-Profil').
"""

DE: dict[str, str] = {
    # --- window, toolbar, files -------------------------------------------------
    "KymoCore Configurator": "KymoCore-Konfigurator",
    "Project": "Projekt",
    "&File": "&Datei",
    "&New": "&Neu",
    "&Open…": "Ö&ffnen…",
    "&Save": "&Speichern",
    "Save &as…": "Speichern &unter…",
    "&Quit": "&Beenden",
    "&View": "&Ansicht",
    "Step by step, only what matters for your goal.": "Schritt für Schritt, nur was für dein Ziel wichtig ist.",
    "All settings at once.": "Alle Einstellungen auf einmal.",
    "Build profile:": "Build-Profil:",
    "A profile is a named set of settings for one firmware build, e.g. 'development' with all "
    "measurements and 'production' with KymoCore switched off. All steps edit the profile selected here.":
        "Ein Profil ist ein benannter Satz Einstellungen für einen Firmware-Build, z. B. 'development' mit "
        "allen Messwerten und 'production' mit abgeschaltetem KymoCore. Alle Schritte bearbeiten das hier "
        "ausgewählte Profil.",
    "New profile…": "Neues Profil…",
    "New profile": "Neues Profil",
    "Adds a profile that starts as a copy of the selected one.":
        "Legt ein Profil an, das als Kopie des ausgewählten beginnt.",
    "The new profile starts as a copy of '{name}'.\nAfterwards you can change it without affecting the "
    "original.\n\nName (e.g. production):":
        "Das neue Profil beginnt als Kopie von '{name}'.\nDanach kannst du es ändern, ohne das Original "
        "zu beeinflussen.\n\nName (z. B. production):",
    "A profile named '{name}' already exists.": "Ein Profil mit dem Namen '{name}' gibt es schon.",
    "unsaved project": "ungespeichertes Projekt",
    "ready to generate": "bereit zum Erzeugen",
    "{count} problem(s) to fix": "{count} Problem(e) zu beheben",
    "{count} warning(s)": "{count} Warnung(en)",
    "Profile '{name}': {state}": "Profil '{name}': {state}",
    "Unsaved changes": "Ungespeicherte Änderungen",
    "Save changes before continuing?": "Änderungen vor dem Fortfahren speichern?",
    "Open Kymo project": "Kymo-Projekt öffnen",
    "Kymo project (*.json)": "Kymo-Projekt (*.json)",
    "Open failed": "Öffnen fehlgeschlagen",
    "Save Kymo project": "Kymo-Projekt speichern",
    "Save failed": "Speichern fehlgeschlagen",
    "This project was written for KymoCore 6.x and has been upgraded to the KymoCore 7.0 format. "
    "Save to keep the upgrade.":
        "Dieses Projekt wurde für KymoCore 6.x geschrieben und auf das Format von KymoCore 7.0 "
        "umgestellt. Speichere es, um die Umstellung zu behalten.",

    # --- start page ---------------------------------------------------------------
    "Creates the build configuration for KymoCore 7 (configuration API {version}): which features are "
    "compiled in, which channels are sent and which of your functions connect KymoCore to the hardware.":
        "Erstellt die Build-Konfiguration für KymoCore 7 (Konfigurations-API {version}): welche Funktionen "
        "einkompiliert werden, welche Kanäle gesendet werden und welche deiner Funktionen KymoCore mit der "
        "Hardware verbinden.",
    "Guided setup": "Geführte Einrichtung",
    "Step by step: KymoCore asks what you want to achieve and only shows what matters for it. Advanced "
    "options keep safe defaults. Recommended for the first project.":
        "Schritt für Schritt: KymoCore fragt, was du erreichen willst, und zeigt nur, was dafür wichtig "
        "ist. Fortgeschrittene Optionen behalten sichere Voreinstellungen. Empfohlen für das erste Projekt.",
    "Start guided setup": "Geführte Einrichtung starten",
    "Professional mode": "Profi-Modus",
    "All settings at once, grouped by topic, with live status, diagnostics and a preview of the "
    "generated files. For users who know KymoCore.":
        "Alle Einstellungen auf einmal, nach Themen gegliedert, mit Live-Status, Prüfungen und einer "
        "Vorschau der erzeugten Dateien. Für alle, die KymoCore kennen.",
    "Open professional mode": "Profi-Modus öffnen",
    "Open existing project…": "Vorhandenes Projekt öffnen…",

    # --- guided and professional frame ----------------------------------------------
    "You can switch to the professional mode at any time; nothing is lost.":
        "Du kannst jederzeit in den Profi-Modus wechseln; dabei geht nichts verloren.",
    "Open in professional mode": "Im Profi-Modus öffnen",
    "← Back": "← Zurück",
    "Next →": "Weiter →",
    "Step {n} of {total}": "Schritt {n} von {total}",
    "Please fix this first: ": "Bitte zuerst beheben: ",
    "{count} rarely needed setting(s) are hidden here and keep safe defaults. The professional mode "
    "shows them.":
        "{count} selten benötigte Einstellung(en) sind hier ausgeblendet und behalten sichere "
        "Voreinstellungen. Der Profi-Modus zeigt sie.",
    "Configuration": "Konfiguration",
    "✓ ready   ⚠ warning   ✖ needs attention": "✓ bereit   ⚠ Warnung   ✖ braucht Aufmerksamkeit",
    "Technical detail: {target}": "Technisches Detail: {target}",
    "Affected C macro, configuration field or function signature":
        "Betroffenes C-Makro, Konfigurationsfeld oder Funktionssignatur",
    "optional": "optional",

    # --- step 1: goal and profile -----------------------------------------------------
    "Goal & profile": "Ziel & Profil",
    "Name the project and choose what KymoCore should do in the selected build profile. A profile is "
    "one named build variant (for example development and production).":
        "Benenne das Projekt und wähle, was KymoCore im ausgewählten Build-Profil tun soll. Ein Profil ist "
        "eine benannte Build-Variante (zum Beispiel development und production).",
    "What do you want to achieve?": "Was möchtest du erreichen?",
    "KymoCore sends measurements from your microcontroller to KymoStudio on your PC, where they are "
    "drawn as live curves. This wizard asks a few questions and then writes the matching configuration "
    "files for your firmware. Nothing is final: every answer can be changed later.":
        "KymoCore schickt Messwerte von deinem Mikrocontroller an KymoStudio auf deinem PC, wo sie als "
        "Live-Kurven gezeichnet werden. Dieser Assistent stellt ein paar Fragen und schreibt dann die "
        "passenden Konfigurationsdateien für deine Firmware. Nichts ist endgültig: Jede Antwort lässt "
        "sich später ändern.",
    "Project": "Projekt",
    "Project name": "Projektname",
    "Used for generated metadata and as the trace device name.":
        "Wird für die erzeugten Metadaten und als Gerätename beim Tracing verwendet.",
    "Any name you like, e.g. 'Plant monitor'. KymoStudio shows it so you recognise your device.":
        "Ein beliebiger Name, z. B. 'Pflanzenwächter'. KymoStudio zeigt ihn an, damit du dein Gerät "
        "wiedererkennst.",
    "This profile": "Dieses Profil",
    "A profile is a named set of settings for one firmware build, e.g. 'development' with all "
    "measurements and 'production' with KymoCore switched off. Most projects need only one. The "
    "'Build profile' selector at the top switches between profiles, 'New profile…' adds one.":
        "Ein Profil ist ein benannter Satz Einstellungen für einen Firmware-Build, z. B. 'development' mit "
        "allen Messwerten und 'production' mit abgeschaltetem KymoCore. Die meisten Projekte brauchen nur "
        "eines. Die Auswahl 'Build-Profil' oben wechselt zwischen Profilen, 'Neues Profil…' legt eines an.",
    "Use KymoCore in this profile": "KymoCore in diesem Profil verwenden",
    "Off compiles the whole library out (for example in production). All other settings are kept and "
    "become active again when switched back on.":
        "Aus kompiliert die ganze Bibliothek heraus (zum Beispiel in production). Alle anderen "
        "Einstellungen bleiben erhalten und gelten wieder, sobald du einschaltest.",
    "Leave this ticked. Untick it only for a build in which KymoCore should disappear completely, e.g. "
    "the finished product. Your settings are kept.":
        "Lass das angehakt. Entferne den Haken nur für einen Build, in dem KymoCore komplett verschwinden "
        "soll, z. B. im fertigen Produkt. Deine Einstellungen bleiben erhalten.",
    "What do you want to do?": "Was möchtest du tun?",
    "Choosing a goal sets the matching features in one step. You can change each feature afterwards; "
    "the profile then shows as custom.":
        "Ein Ziel setzt die passenden Funktionen in einem Schritt. Du kannst jede Funktion danach ändern; "
        "das Profil gilt dann als eigene Auswahl.",
    "Tick everything you want to use; the options combine freely. Hover over an option for examples. "
    "This sets the matching features for you, and the next steps show what was chosen.":
        "Hake alles an, was du nutzen willst; die Optionen lassen sich frei kombinieren. Fahr mit der Maus "
        "über eine Option, um Beispiele zu sehen. Das setzt die passenden Funktionen für dich, und die "
        "nächsten Schritte zeigen, was gewählt wurde.",
    "Custom selection": "Eigene Auswahl",
    "Values over time": "Werte über der Zeit",
    "Draw single measurements such as temperature, current or speed as curves over time":
        "Einzelne Messwerte wie Temperatur, Strom oder Drehzahl als Kurven über der Zeit zeichnen",
    "X/Y or X/Y/Z points": "X/Y- oder X/Y/Z-Punkte",
    "Draw a value over something other than time, e.g. a plotter head position as X/Y, a 3D position "
    "as X/Y/Z or a value over an angle":
        "Einen Wert über etwas anderem als der Zeit zeichnen, z. B. die Position eines Plotterkopfs als "
        "X/Y, eine 3D-Position als X/Y/Z oder einen Wert über einem Winkel",
    "Execution tracing": "Ablauf-Tracing",
    "Record which functions, interrupts and flows run when (timing analysis)":
        "Aufzeichnen, welche Funktionen, Interrupts und Abläufe wann laufen (Zeitanalyse)",
    "Always included.": "Immer dabei.",
    "Each sample carries one number, the Y value. KymoStudio draws it over time.\nExample: temperature, "
    "motor current, battery voltage.\nAvailable in every project, also together with the options below.":
        "Jeder Messwert enthält eine Zahl, den Y-Wert. KymoStudio zeichnet ihn über der Zeit.\nBeispiel: "
        "Temperatur, Motorstrom, Akkuspannung.\nIn jedem Projekt verfügbar, auch zusammen mit den Optionen "
        "darunter.",
    "Curves over time keep working; each channel decides in step 3 which kind it sends.":
        "Kurven über der Zeit funktionieren weiter; jeder Kanal entscheidet in Schritt 3, was er sendet.",
    "A sample may carry its own X (and Z) next to the Y value, plus the time it was measured.\n"
    "KymoStudio then draws Y over X, or a point in 3D, instead of Y over time.\nExample: pen position of "
    "a plotter, robot arm position, torque over angle.\nCompiles in X, Z and timestamps "
    "(KYMO_ENABLE_X/_Z/_TIMESTAMP).":
        "Ein Messwert kann neben dem Y-Wert ein eigenes X (und Z) sowie seinen Messzeitpunkt enthalten.\n"
        "KymoStudio zeichnet dann Y über X oder einen Punkt in 3D statt Y über der Zeit.\nBeispiel: "
        "Stiftposition eines Plotters, Position eines Roboterarms, Drehmoment über dem Winkel.\nKompiliert "
        "X, Z und Zeitstempel ein (KYMO_ENABLE_X/_Z/_TIMESTAMP).",
    "Works with or without X/Y points.": "Funktioniert mit und ohne X/Y-Punkte.",
    "Records when your functions, interrupts and data flows run and shows them on a timeline in "
    "KymoStudio.\nExample: find out why a control loop is sometimes late.\nCompiles in the trace "
    "recorder (KYMO_ENABLE_TRACE).":
        "Zeichnet auf, wann deine Funktionen, Interrupts und Datenflüsse laufen, und zeigt sie in "
        "KymoStudio auf einer Zeitachse.\nBeispiel: herausfinden, warum ein Regelkreis manchmal zu spät "
        "kommt.\nKompiliert den Trace-Rekorder ein (KYMO_ENABLE_TRACE).",
    "Enable KymoCore for this profile first.": "Schalte zuerst KymoCore für dieses Profil ein.",

    # --- step 2: telemetry features ---------------------------------------------------
    "Telemetry features": "Telemetrie-Funktionen",
    "Which parts of KymoCore are compiled in and which fields a data frame may carry. Disabled parts "
    "cost no code.":
        "Welche Teile von KymoCore einkompiliert werden und welche Felder ein Datenframe enthalten darf. "
        "Abgeschaltete Teile kosten keinen Code.",
    "Which extras do you need?": "Welche Extras brauchst du?",
    "Here you choose which building blocks of KymoCore end up on your microcontroller. Your answer in "
    "step 1 has already ticked what you need. Not sure? Leave everything as it is and click Next. "
    "Unticked parts use no memory.":
        "Hier wählst du, welche Bausteine von KymoCore auf deinem Mikrocontroller landen. Deine Antwort in "
        "Schritt 1 hat schon angehakt, was du brauchst. Unsicher? Lass alles, wie es ist, und klick auf "
        "Weiter. Nicht angehakte Teile belegen keinen Speicher.",
    "Programming interface": "Programmierschnittstelle",
    "How your program hands its values to KymoCore.": "Wie dein Programm seine Werte an KymoCore übergibt.",
    "Cyclic runtime (Kymo_Init / Kymo_Main)": "Zyklische Laufzeit (Kymo_Init / Kymo_Main)",
    "KymoCore samples your channels on schedule; recommended for most projects.":
        "KymoCore liest deine Kanäle nach Zeitplan; empfohlen für die meisten Projekte.",
    "Leave ticked unless you know otherwise. KymoCore then reads your measurements by itself at a fixed "
    "rhythm (e.g. every 20 ms) and sends them. Your program only calls Kymo_Init() once and Kymo_Main() "
    "in its main loop.":
        "Lass das angehakt, wenn du es nicht besser weißt. KymoCore liest deine Messwerte dann selbst in "
        "einem festen Takt (z. B. alle 20 ms) und sendet sie. Dein Programm ruft nur einmal Kymo_Init() "
        "und in seiner Hauptschleife Kymo_Main() auf.",
    "C++ push sender (KymoSender)": "C++-Sender (KymoSender)",
    "Send single values whenever your code decides, in addition to or instead of the cyclic runtime.":
        "Einzelne Werte senden, wann immer dein Code es entscheidet, zusätzlich zur oder statt der "
        "zyklischen Laufzeit.",
    "Tick this only if you want to send a value at a moment you choose, e.g. right after a sensor "
    "reading. Needs C++ (Arduino is C++). Not needed for normal cyclic sending.":
        "Hake das nur an, wenn du einen Wert zu einem selbst gewählten Zeitpunkt senden willst, z. B. "
        "direkt nach dem Lesen eines Sensors. Braucht C++ (Arduino ist C++). Für das normale zyklische "
        "Senden nicht nötig.",
    "Data per sample": "Daten pro Messwert",
    "Enabling a field allows it; each channel then chooses whether it uses it.":
        "Ein eingeschaltetes Feld ist erlaubt; jeder Kanal entscheidet dann, ob er es nutzt.",
    "By default every measurement is one number, drawn over time. Tick an extra field only if some of "
    "your signals need it; each channel can then decide in the next step whether it uses it.":
        "Normalerweise ist jeder Messwert eine Zahl, die über der Zeit gezeichnet wird. Hake ein "
        "zusätzliches Feld nur an, wenn einige deiner Signale es brauchen; jeder Kanal kann dann im "
        "nächsten Schritt entscheiden, ob er es nutzt.",
    "X coordinate": "X-Koordinate",
    "Allows channels to send an X value, e.g. position or angle.":
        "Erlaubt Kanälen, einen X-Wert zu senden, z. B. Position oder Winkel.",
    "Tick if a signal is a point instead of a single number, e.g. the X/Y position of a plotter head, "
    "or a value drawn over an angle instead of over time.":
        "Hake das an, wenn ein Signal ein Punkt statt einer einzelnen Zahl ist, z. B. die X/Y-Position "
        "eines Plotterkopfs oder ein Wert, der über einem Winkel statt über der Zeit gezeichnet wird.",
    "Z coordinate": "Z-Koordinate",
    "Allows channels to send a Z value for 3D data.": "Erlaubt Kanälen, einen Z-Wert für 3D-Daten zu senden.",
    "Tick for 3D points (X, Y and Z), e.g. a position in space.":
        "Hake das für 3D-Punkte (X, Y und Z) an, z. B. eine Position im Raum.",
    "Timestamp": "Zeitstempel",
    "Adds the time since start to each sample (4 bytes).":
        "Fügt jedem Messwert die Zeit seit dem Start hinzu (4 Bytes).",
    "Tick if KymoStudio must know exactly when each value was measured, e.g. for precise timing. "
    "Without it, KymoStudio uses the arrival time, which is fine for most live curves. Costs 4 extra "
    "bytes per value.":
        "Hake das an, wenn KymoStudio genau wissen muss, wann jeder Wert gemessen wurde, z. B. für "
        "präzise Zeitmessung. Ohne Zeitstempel nimmt KymoStudio die Ankunftszeit, was für die meisten "
        "Live-Kurven reicht. Kostet 4 zusätzliche Bytes pro Wert.",
    "Integrity and decoding": "Datensicherheit und Dekodierung",
    "CRC checksum on sent frames": "CRC-Prüfsumme auf gesendeten Frames",
    "Detects transmission errors on noisy links. Older receivers require it.":
        "Erkennt Übertragungsfehler auf gestörten Verbindungen. Ältere Empfänger setzen sie voraus.",
    "Tick if the connection can corrupt data (long cables, radio, motors nearby). Broken frames are then "
    "dropped instead of drawn wrong. Over a short USB cable it is usually not needed.":
        "Hake das an, wenn die Verbindung Daten verfälschen kann (lange Kabel, Funk, Motoren in der Nähe). "
        "Kaputte Frames werden dann verworfen statt falsch gezeichnet. Über ein kurzes USB-Kabel ist das "
        "meist nicht nötig.",
    "Frame decoder on the MCU": "Frame-Decoder auf dem Mikrocontroller",
    "Parameters from KymoStudio (back channel)": "Parameter aus KymoStudio (Rückkanal)",
    "Lets KymoStudio change registered variables at run time; needs the frame decoder.":
        "KymoStudio darf angemeldete Variablen zur Laufzeit ändern; braucht den Frame-Decoder.",
    "Tick if you want to tune values such as controller gains or a setpoint from KymoStudio while measuring. You list them in your code with KYMO_PARAM; the firmware checks every value. Also tick the frame decoder.":
        "Ankreuzen, wenn du Werte wie Reglerverstärkungen oder einen Sollwert während der Messung aus "
        "KymoStudio verstellen willst. Du trägst sie im Code mit KYMO_PARAM ein; die Firmware prüft jeden "
        "Wert. Kreuze auch den Frame-Decoder an.",
    "The parameter back channel needs the frame decoder": "Der Parameter-Rückkanal braucht den Frame-Decoder",
    "Live variables from the ELF file": "Live-Variablen aus der ELF-Datei",
    "Lets KymoStudio sample global variables by address inside the memory ranges you allow; needs the frame decoder.":
        "KymoStudio darf globale Variablen per Adresse in den von dir freigegebenen Speicherbereichen "
        "abtasten lassen; braucht den Frame-Decoder.",
    "Tick if you want to pick variables in KymoStudio from your firmware's ELF file and watch them as curves without adding code for each one. Your firmware lists which memory may be read; nothing is ever written. Also tick the frame decoder.":
        "Ankreuzen, wenn du in KymoStudio Variablen aus der ELF-Datei deiner Firmware auswählen und als "
        "Kurven sehen willst, ohne für jede einzelne Code zu schreiben. Deine Firmware legt fest, welcher "
        "Speicher gelesen werden darf; geschrieben wird nie. Kreuze auch den Frame-Decoder an.",
    "Live variables need the frame decoder": "Live-Variablen brauchen den Frame-Decoder",
    "Enable feature decoder as well.": "Aktiviere zusätzlich die Funktion decoder.",
    "Only needed when the microcontroller itself must read Kymo frames.":
        "Nur nötig, wenn der Mikrocontroller selbst Kymo-Frames lesen muss.",
    "Init/Main runtime": "Init/Main-Laufzeit",
    "no runtime": "keine Laufzeit",
    "scalar values only": "nur Einzelwerte",

    # --- step 3: channels -------------------------------------------------------------
    "Channels": "Kanäle",
    "Each channel is one signal in KymoStudio. It has an ID (0..255), a sampling period and the "
    "optional fields it sends. KYMO_MAX_CHANNELS is sized automatically.":
        "Jeder Kanal ist ein Signal in KymoStudio. Er hat eine ID (0..255), eine Abtastperiode und die "
        "optionalen Felder, die er sendet. KYMO_MAX_CHANNELS wird automatisch bemessen.",
    "What do you want to measure?": "Was möchtest du messen?",
    "Add one row per signal. The period is how often it is sampled (20 ms = 50 times per second). The "
    "estimate below shows the resulting traffic.":
        "Leg eine Zeile pro Signal an. Die Periode gibt an, wie oft es gelesen wird (20 ms = 50-mal pro "
        "Sekunde). Die Schätzung darunter zeigt die entstehende Datenmenge.",
    "Name": "Name",
    "Period (ms)": "Periode (ms)",
    "Rate": "Rate",
    "Time": "Zeit",
    "Bytes/frame": "Bytes/Frame",
    "Channel number 0..255; KymoStudio shows each ID as its own signal.":
        "Kanalnummer 0..255; KymoStudio zeigt jede ID als eigenes Signal.",
    "Readable name shown in KymoStudio.": "Lesbarer Name, der in KymoStudio angezeigt wird.",
    "How often the value is read: 20 ms = 50 times per second. 0 = on every Kymo_Main call.":
        "Wie oft der Wert gelesen wird: 20 ms = 50-mal pro Sekunde. 0 = bei jedem Aufruf von Kymo_Main.",
    "Resulting values per second.": "Ergebnis: Werte pro Sekunde.",
    "Send an X value as well (point instead of value over time).":
        "Zusätzlich einen X-Wert senden (Punkt statt Wert über der Zeit).",
    "Send a Z value as well (3D point).": "Zusätzlich einen Z-Wert senden (3D-Punkt).",
    "Send the measuring time with each value.": "Den Messzeitpunkt mit jedem Wert senden.",
    "Bytes per value on the link.": "Bytes pro Wert auf der Verbindung.",
    "Add channel": "Kanal hinzufügen",
    "Duplicate": "Duplizieren",
    "Remove": "Entfernen",
    "every cycle": "jeder Zyklus",
    "Channel {id}": "Kanal {id}",
    "Enable the {key} feature under Telemetry features first.":
        "Schalte zuerst die Funktion {key} unter Telemetrie-Funktionen ein.",
    "Estimate: {summary}. Channels with period 0 depend on how often Kymo_Main runs.":
        "Schätzung: {summary}. Kanäle mit Periode 0 hängen davon ab, wie oft Kymo_Main läuft.",
    "Link load: {load}% of {budget} bytes/s.": "Verbindungsauslastung: {load} % von {budget} Bytes/s.",
    "{count} channel(s), ~{rate} frames/s, ~{traffic} bytes/s":
        "{count} Kanal/Kanäle, ~{rate} Frames/s, ~{traffic} Bytes/s",
    "invalid channel list": "ungültige Kanalliste",

    # --- step 4: callbacks and transport ----------------------------------------------
    "Callbacks & transport": "Callbacks & Übertragung",
    "KymoCore never touches hardware itself. You name small functions in your own code; the generated "
    "configuration calls them. Their implementations stay in your project.":
        "KymoCore greift nie selbst auf die Hardware zu. Du nennst kleine Funktionen in deinem eigenen "
        "Code; die erzeugte Konfiguration ruft sie auf. Ihre Implementierung bleibt in deinem Projekt.",
    "How does KymoCore reach your hardware?": "Wie erreicht KymoCore deine Hardware?",
    "KymoCore runs on any microcontroller because it never talks to the hardware itself. Instead it "
    "calls a few functions from your own program: one for the time, one that reads a measurement and "
    "one that sends bytes. Here you only enter their names.":
        "KymoCore läuft auf jedem Mikrocontroller, weil es nie selbst mit der Hardware spricht. Stattdessen "
        "ruft es ein paar Funktionen aus deinem eigenen Programm auf: eine für die Zeit, eine, die einen "
        "Messwert liest, und eine, die Bytes sendet. Hier trägst du nur ihre Namen ein.",
    "Required functions": "Benötigte Funktionen",
    "KymoCore does not know your hardware. You write a few small functions and enter their names here; "
    "the generated files call them. Not written yet? Use the grey suggested names: the 'init-port' "
    "command creates a starter file containing exactly these functions.":
        "KymoCore kennt deine Hardware nicht. Du schreibst ein paar kleine Funktionen und trägst hier ihre "
        "Namen ein; die erzeugten Dateien rufen sie auf. Noch nicht geschrieben? Nimm die grau "
        "vorgeschlagenen Namen: Der Befehl 'init-port' erzeugt eine Startdatei mit genau diesen Funktionen.",
    "Not used: telemetry goes through the shared trace link.":
        "Nicht verwendet: Die Telemetrie läuft über die gemeinsame Trace-Verbindung.",
    "Needs the telemetry runtime.": "Braucht die Telemetrie-Laufzeit.",
    "Transport behaviour": "Übertragungsverhalten",
    "How the bytes leave your microcontroller.": "Wie die Bytes deinen Mikrocontroller verlassen.",
    "Driver keeps the bytes after write() returns (DMA/USB)":
        "Treiber nutzt die Bytes noch nach dem Ende von write() (DMA/USB)",
    "Then a busy function is required so KymoCore does not overwrite the frame while it is still being "
    "sent.":
        "Dann ist eine Belegt-Funktion nötig, damit KymoCore den Frame nicht überschreibt, während er "
        "noch gesendet wird.",
    "Tick if your driver sends in the background (DMA, many USB stacks) and still uses the bytes after "
    "your send function returned. Then the 'Busy check' function below is needed too. For a plain "
    "Serial.write() untick it.":
        "Hake das an, wenn dein Treiber im Hintergrund sendet (DMA, viele USB-Stacks) und die Bytes noch "
        "nutzt, nachdem deine Sendefunktion zurückgekehrt ist. Dann wird auch die Funktion 'Belegt-Prüfung' "
        "darunter gebraucht. Für ein einfaches Serial.write() nimm den Haken weg.",
    "C++ push sender": "C++-Sender",
    "KymoStream object": "KymoStream-Objekt",
    "Enable the C++ push sender.": "Schalte den C++-Sender ein.",
    "Millisecond clock": "Millisekunden-Uhr",
    "Needs the C++ sender and timestamps.": "Braucht den C++-Sender und Zeitstempel.",
    "Send timestamps by default": "Standardmäßig Zeitstempel senden",
    "Initial value of KymoSender::setTimestampEnabled.": "Startwert von KymoSender::setTimestampEnabled.",
    "all required callbacks named": "alle benötigten Callbacks benannt",
    "missing: ": "fehlt: ",

    # --- execution tracing --------------------------------------------------------------
    "Records function scopes, markers, interrupts and flows with timestamps and sends them to KymoStudio "
    "for timing analysis.":
        "Zeichnet Funktionsbereiche, Marker, Interrupts und Abläufe mit Zeitstempeln auf und sendet sie "
        "zur Zeitanalyse an KymoStudio.",
    "Configure execution tracing": "Ablauf-Tracing einrichten",
    "Tracing records when your functions and interrupts run, so KymoStudio can show a timeline. It needs "
    "a fast clock and a connection to the PC. The lists below give your functions (symbols) and tasks "
    "(contexts) readable names in KymoStudio.":
        "Tracing zeichnet auf, wann deine Funktionen und Interrupts laufen, damit KymoStudio eine Zeitachse "
        "zeigen kann. Es braucht eine schnelle Uhr und eine Verbindung zum PC. Die Listen darunter geben "
        "deinen Funktionen (Symbole) und Tasks (Kontexte) lesbare Namen in KymoStudio.",
    "What is recorded": "Was aufgezeichnet wird",
    "Trace recording": "Trace-Aufzeichnung",
    "Compiles the recorder, the KYMO_TRACE_* macros and the transport mux.":
        "Kompiliert den Rekorder, die KYMO_TRACE_*-Makros und den Übertragungs-Mux ein.",
    "Leave ticked to record when your functions run. You mark the interesting places in your code with "
    "KYMO_TRACE_* macros.":
        "Lass das angehakt, um aufzuzeichnen, wann deine Funktionen laufen. Die interessanten Stellen in "
        "deinem Code markierst du mit KYMO_TRACE_*-Makros.",
    "Interrupts": "Interrupts",
    "Records interrupt entry and exit.": "Zeichnet Eintritt und Austritt von Interrupts auf.",
    "Tick to see when interrupt handlers run and how long they take.":
        "Hake das an, um zu sehen, wann Interrupt-Handler laufen und wie lange sie dauern.",
    "Task switches": "Taskwechsel",
    "Records RTOS task switches. Needs interrupt tracing and a verified port.":
        "Zeichnet RTOS-Taskwechsel auf. Braucht Interrupt-Tracing und einen geprüften Port.",
    "Only for RTOS projects (e.g. FreeRTOS): shows which task runs when. Leave unticked if you do not use "
    "an RTOS.":
        "Nur für RTOS-Projekte (z. B. FreeRTOS): zeigt, welcher Task wann läuft. Ohne RTOS nicht anhaken.",
    "Flows": "Abläufe",
    "Correlates send/receive pairs across tasks.": "Verknüpft Sende-/Empfangspaare über Tasks hinweg.",
    "Draws arrows from where data is sent to where it is received, e.g. between an interrupt and a task.":
        "Zeichnet Pfeile vom Ort, an dem Daten gesendet werden, zum Ort, an dem sie ankommen, z. B. "
        "zwischen einem Interrupt und einem Task.",
    "Enable trace recording first.": "Schalte zuerst die Trace-Aufzeichnung ein.",
    "Target": "Zielsystem",
    "Target port": "Ziel-Port",
    "Describes the platform integration. Changing it resets capability verification.":
        "Beschreibt die Plattformanbindung. Eine Änderung setzt die Prüfung der Fähigkeiten zurück.",
    "Your hardware family. Choose 'Generic' if yours is not listed.":
        "Deine Hardware-Familie. Wähle 'Generisch', wenn deine nicht aufgeführt ist.",
    "Generic": "Generisch",
    "ESP32 + FreeRTOS": "ESP32 + FreeRTOS",
    "STM32 bare metal": "STM32 ohne Betriebssystem",
    "CPU cores": "CPU-Kerne",
    "Number of cores that record events.": "Anzahl der Kerne, die Ereignisse aufzeichnen.",
    "1 for most microcontrollers; 2 for dual-core chips such as the ESP32.":
        "1 für die meisten Mikrocontroller; 2 für Dual-Core-Chips wie den ESP32.",
    "Trace clock (Hz)": "Trace-Uhr (Hz)",
    "Tick rate of your trace clock, e.g. 1000000 for microseconds.":
        "Taktrate deiner Trace-Uhr, z. B. 1000000 für Mikrosekunden.",
    "How many times per second your trace clock counts. 1000000 means it counts microseconds, e.g. "
    "micros() on Arduino.":
        "Wie oft pro Sekunde deine Trace-Uhr zählt. 1000000 bedeutet, sie zählt Mikrosekunden, z. B. "
        "micros() auf Arduino.",
    "Clock counter width": "Breite des Uhrenzählers",
    "Width of the hardware counter.": "Breite des Hardware-Zählers.",
    "32 bit": "32 Bit",
    "64 bit": "64 Bit",
    "Clock domain ID": "ID der Uhrendomäne",
    "Identifies the clock in KymoStudio.": "Kennzeichnet die Uhr in KymoStudio.",
    "Clock synchronized across cores": "Uhr über alle Kerne synchron",
    "Set when all cores read the same counter.": "Setzen, wenn alle Kerne denselben Zähler lesen.",
    "Buffers": "Puffer",
    "Events per core": "Ereignisse pro Kern",
    "Ring size per core; 32 bytes RAM per event.": "Ringgröße pro Kern; 32 Bytes RAM pro Ereignis.",
    "How many events are buffered before they are sent. More events need more RAM (32 bytes each) but "
    "lose less during bursts.":
        "Wie viele Ereignisse gepuffert werden, bevor sie gesendet werden. Mehr Ereignisse brauchen mehr "
        "RAM (je 32 Bytes), verlieren aber bei Spitzen weniger.",
    "Tasks / contexts": "Tasks / Kontexte",
    "How many tasks or ISRs are tracked.": "Wie viele Tasks oder ISRs verfolgt werden.",
    "Nesting depth": "Verschachtelungstiefe",
    "Deepest nesting of traced scopes.": "Tiefste Verschachtelung aufgezeichneter Bereiche.",
    "Interrupt nesting": "Interrupt-Verschachtelung",
    "Deepest interrupt nesting.": "Tiefste Verschachtelung von Interrupts.",
    "Link": "Verbindung",
    "Link usage": "Nutzung der Verbindung",
    "Shared: telemetry frames and trace events use the same link through the mux.":
        "Gemeinsam: Telemetrie-Frames und Trace-Ereignisse nutzen über den Mux dieselbe Verbindung.",
    "Choose 'Shared' if measurements and trace events go over the same cable, e.g. one USB serial port.":
        "Wähle 'Gemeinsam mit Telemetrie', wenn Messwerte und Trace-Ereignisse über dasselbe Kabel laufen, "
        "z. B. eine USB-Seriell-Verbindung.",
    "Trace only on this link": "Nur Tracing auf dieser Verbindung",
    "Shared with telemetry": "Gemeinsam mit Telemetrie",
    "Largest frame (bytes)": "Größter Frame (Bytes)",
    "Maximum bytes your link sends at once (64..240).": "Maximale Bytes, die deine Verbindung auf einmal sendet (64..240).",
    "The default of 128 works for serial and USB.": "Die Voreinstellung 128 passt für Seriell und USB.",
    "Events per service call": "Ereignisse pro Dienstaufruf",
    "Upper bound of events sent per cycle.": "Obergrenze der pro Zyklus gesendeten Ereignisse.",
    "Bytes per service call": "Bytes pro Dienstaufruf",
    "Upper bound of bytes sent per cycle.": "Obergrenze der pro Zyklus gesendeten Bytes.",
    "Health interval (ms)": "Statusintervall (ms)",
    "How often a health record is sent.": "Wie oft ein Statusdatensatz gesendet wird.",
    "Driver keeps the bytes after write() returns": "Treiber nutzt die Bytes noch nach dem Ende von write()",
    "Then the transport busy function is required.": "Dann ist die Belegt-Funktion der Übertragung nötig.",
    "Tick only if your driver sends in the background (DMA, USB) and then fill in 'Link busy' below.":
        "Nur anhaken, wenn dein Treiber im Hintergrund sendet (DMA, USB), und dann darunter "
        "'Verbindung belegt' ausfüllen.",
    "Trace functions": "Trace-Funktionen",
    "Trace clock": "Trace-Uhr",
    "Returns the free-running tick counter.": "Liefert den frei laufenden Taktzähler.",
    "Name of your function that returns the trace clock count.":
        "Name deiner Funktion, die den Zählerstand der Trace-Uhr liefert.",
    "Session ID": "Sitzungs-ID",
    "Returns a new random ID per boot.": "Liefert bei jedem Start eine neue Zufalls-ID.",
    "Name of a function that returns a random number, new on every start, so KymoStudio can tell "
    "recordings apart.":
        "Name einer Funktion, die bei jedem Start eine neue Zufallszahl liefert, damit KymoStudio "
        "Aufzeichnungen unterscheiden kann.",
    "Send trace bytes": "Trace-Bytes senden",
    "Writes mux frames to the link.": "Schreibt Mux-Frames auf die Verbindung.",
    "Name of the function that sends the trace bytes to the PC.":
        "Name der Funktion, die die Trace-Bytes an den PC sendet.",
    "Link busy": "Verbindung belegt",
    "Returns nonzero while the driver owns the frame.": "Liefert ungleich 0, solange der Treiber den Frame benutzt.",
    "Only needed with background sending: returns 1 while still sending.":
        "Nur bei Senden im Hintergrund nötig: liefert 1, solange noch gesendet wird.",
    "Empty means core 0.": "Leer bedeutet Kern 0.",
    "Empty means unknown; used as interrupted context.": "Leer bedeutet unbekannt; dient als unterbrochener Kontext.",
    "Needed when several tasks or ISRs record.": "Nötig, wenn mehrere Tasks oder ISRs aufzeichnen.",
    "Must be set together with enter.": "Muss zusammen mit dem Betreten gesetzt werden.",
    "Symbol dictionary": "Symbolverzeichnis",
    "Functions, scopes and markers you instrument with KYMO_TRACE_*; the ID is the symbol argument.":
        "Funktionen, Bereiche und Marker, die du mit KYMO_TRACE_* markierst; die ID ist das Symbol-Argument.",
    "Context dictionary": "Kontextverzeichnis",
    "Tasks and interrupts that record events; the ID is the context argument.":
        "Tasks und Interrupts, die Ereignisse aufzeichnen; die ID ist das Kontext-Argument.",
    "Kind": "Art",
    "Add symbol": "Symbol hinzufügen",
    "Add context": "Kontext hinzufügen",
    "off": "aus",
    "recording": "zeichnet auf",

    # --- build and output ---------------------------------------------------------------
    "Build & output": "Build & Ausgabe",
    "Where the generated, immutable configuration snapshot is written and which build system helper "
    "files are produced.":
        "Wohin der erzeugte, unveränderliche Konfigurationsstand geschrieben wird und welche Hilfsdateien "
        "für das Build-System entstehen.",
    "Where should the configuration go?": "Wohin soll die Konfiguration?",
    "The generator writes a snapshot that your build includes. Nothing in your own source files is "
    "overwritten.":
        "Der Generator schreibt einen Konfigurationsstand, den dein Build einbindet. In deinen eigenen "
        "Quelldateien wird nichts überschrieben.",
    "Output": "Ausgabe",
    "Build system": "Build-System",
    "Produces matching include helpers (Makefile, CMake or PlatformIO scripts).":
        "Erzeugt passende Einbinde-Hilfen (Makefile, CMake- oder PlatformIO-Skripte).",
    "The tool that builds your firmware. It decides which helper files are created so your build finds "
    "the configuration. Using VS Code with PlatformIO? Choose PlatformIO.":
        "Das Werkzeug, das deine Firmware baut. Davon hängt ab, welche Hilfsdateien entstehen, damit dein "
        "Build die Konfiguration findet. Du nutzt VS Code mit PlatformIO? Dann wähle PlatformIO.",
    "Plain compiler / Makefile": "Einfacher Compiler / Makefile",
    "CMake": "CMake",
    "PlatformIO": "PlatformIO",
    "Output folder": "Ausgabeordner",
    "Relative to the project file; one subfolder per profile.":
        "Relativ zur Projektdatei; ein Unterordner pro Profil.",
    "Where the generated files are stored, next to the project file. The default is fine for most "
    "projects.":
        "Wo die erzeugten Dateien neben der Projektdatei abgelegt werden. Die Voreinstellung passt für die "
        "meisten Projekte.",
    "Project include folder": "Include-Ordner des Projekts",
    "Your project's include folder (informational).": "Include-Ordner deines Projekts (nur zur Information).",
    "Project source folder": "Quellordner des Projekts",
    "Your project's source folder (informational).": "Quellordner deines Projekts (nur zur Information).",
    "Budgets (optional)": "Budgets (optional)",
    "RAM budget (bytes)": "RAM-Budget (Bytes)",
    "Shown in the review for comparison.": "Wird in der Prüfung zum Vergleich angezeigt.",
    "Link budget (bytes/s)": "Verbindungsbudget (Bytes/s)",
    "Used to estimate the link load in the review, e.g. 11520 for 115200 baud.":
        "Dient in der Prüfung zur Schätzung der Verbindungsauslastung, z. B. 11520 für 115200 Baud.",

    # --- review and generate -----------------------------------------------------------
    "Review & generate": "Prüfen & erzeugen",
    "Effective settings, open problems and the files that will be generated.":
        "Wirksame Einstellungen, offene Probleme und die Dateien, die erzeugt werden.",
    "Review and generate": "Prüfen und erzeugen",
    "Check the summary. 'Generate' writes the configuration files that your firmware build includes. "
    "You can come back and generate again at any time.":
        "Prüf die Zusammenfassung. 'Konfiguration erzeugen' schreibt die Konfigurationsdateien, die dein "
        "Firmware-Build einbindet. Du kannst jederzeit zurückkommen und neu erzeugen.",
    "Profile '{name}': {title}, {state}": "Profil '{name}': {title}, {state}",
    "enabled": "eingeschaltet",
    "compiled out (KYMO_ENABLED=0)": "herauskompiliert (KYMO_ENABLED=0)",
    "Generated settings": "Erzeugte Einstellungen",
    "✓ compiled in, – left out": "✓ einkompiliert, – weggelassen",
    "Open problems": "Offene Probleme",
    "✓ No problems found. The configuration can be generated.":
        "✓ Keine Probleme gefunden. Die Konfiguration kann erzeugt werden.",
    "Preview generated files": "Vorschau der erzeugten Dateien",
    "Generate configuration": "Konfiguration erzeugen",
    "The generated files and their changes appear here.": "Hier erscheinen die erzeugten Dateien und ihre Änderungen.",
    "Resolve the open problems first.": "Behebe zuerst die offenen Probleme.",
    "Generation failed": "Erzeugen fehlgeschlagen",
    "Generated snapshot:\n{snapshot}\n\nActive selector: {selector}\n\nAdd the snapshot folder to your "
    "include path and define\nKYMO_USER_CONFIG_HEADER=\"kymo_user_config.h\" for all files, or use the "
    "generated build helper for your build system.":
        "Erzeugter Stand:\n{snapshot}\n\nAktive Auswahl: {selector}\n\nNimm den Ordner des Stands in deinen "
        "Include-Pfad auf und definiere\nKYMO_USER_CONFIG_HEADER=\"kymo_user_config.h\" für alle Dateien, "
        "oder nutze die erzeugte Build-Hilfe für dein Build-System.",

    # --- function fields: names, Required/Optional, code examples -------------------
    "Required": "Pflicht",
    "Optional": "Optional",
    "In your code, for example:": "In deinem Code, zum Beispiel:",
    "Example of the function in your own code; the name follows your entry.":
        "Beispiel für die Funktion in deinem eigenen Code; der Name folgt deiner Eingabe.",
    "Time function": "Zeitfunktion",
    "Enter the name of your function that returns the milliseconds since start.":
        "Trag den Namen deiner Funktion ein, die die Millisekunden seit dem Start liefert.",
    "Measurement function": "Messfunktion",
    "Enter the name of your function that reads a measurement. KymoCore passes the channel ID; your "
    "function writes the value into out->value and returns 1 (or 0 to skip this time).":
        "Trag den Namen deiner Funktion ein, die einen Messwert liest. KymoCore übergibt die Kanal-ID; "
        "deine Funktion schreibt den Wert nach out->value und liefert 1 (oder 0, um diesmal auszulassen).",
    "Send function": "Sendefunktion",
    "Enter the name of your function that sends bytes to the PC and returns how many it accepted.":
        "Trag den Namen deiner Funktion ein, die Bytes an den PC sendet und liefert, wie viele sie "
        "angenommen hat.",
    "Busy check": "Belegt-Prüfung",
    "Enter the name of your function that returns 1 while the driver is still sending the last data, "
    "otherwise 0. Only needed when the box above is ticked.":
        "Trag den Namen deiner Funktion ein, die 1 liefert, solange der Treiber die letzten Daten noch "
        "sendet, sonst 0. Nur nötig, wenn das Kästchen darüber angehakt ist.",
    "Service function": "Dienstfunktion",
    "Name of a function KymoCore calls on every cycle, e.g. to keep a WiFi connection alive. Leave empty "
    "if you do not need it.":
        "Name einer Funktion, die KymoCore in jedem Zyklus aufruft, z. B. um eine WLAN-Verbindung am Leben "
        "zu halten. Lass das Feld leer, wenn du sie nicht brauchst.",
    "Extra data for your functions": "Zusatzdaten für deine Funktionen",
    "Optional, usually left empty. Every function above receives a parameter void *user. If a function "
    "needs to know which object to work with, e.g. which of two serial ports, enter that object's name "
    "here and KymoCore passes a pointer to it as user. If your functions simply use globals such as "
    "Serial, leave everything empty.":
        "Optional, meist leer. Jede Funktion oben bekommt einen Parameter void *user. Muss eine Funktion "
        "wissen, mit welchem Objekt sie arbeiten soll, z. B. mit welcher von zwei seriellen Schnittstellen, "
        "trag hier den Namen dieses Objekts ein; KymoCore übergibt dann einen Zeiger darauf als user. "
        "Nutzen deine Funktionen einfach globale Objekte wie Serial, bleibt alles leer.",
    "Object for the time function": "Objekt für die Zeitfunktion",
    "Name of a global object that KymoCore passes to your time function as user.":
        "Name eines globalen Objekts, das KymoCore deiner Zeitfunktion als user übergibt.",
    "Object for the measurement function": "Objekt für die Messfunktion",
    "Name of a global object that KymoCore passes to your measurement function as user.":
        "Name eines globalen Objekts, das KymoCore deiner Messfunktion als user übergibt.",
    "Object for the send functions": "Objekt für die Sendefunktionen",
    "Name of a global object that KymoCore passes to your send, busy and service functions as user.":
        "Name eines globalen Objekts, das KymoCore deiner Sende-, Belegt- und Dienstfunktion als user "
        "übergibt.",
    "Name of your object derived from KymoStream that the C++ sender writes to.":
        "Name deines von KymoStream abgeleiteten Objekts, in das der C++-Sender schreibt.",
    "Name of your function without arguments that returns milliseconds; used for C++ timestamps.":
        "Name deiner Funktion ohne Argumente, die Millisekunden liefert; für C++-Zeitstempel.",
    "Current core": "Aktueller Kern",
    "Current task": "Aktueller Task",
    "Critical section enter": "Kritischen Abschnitt betreten",
    "Critical section exit": "Kritischen Abschnitt verlassen",

    # --- validation messages (templates from validation.py; CLI output stays English) ---
    "Project root must be an object": "Die Projektdatei muss ein Objekt enthalten",
    "A busy callback is required when write borrows the transmit buffer":
        "Eine Belegt-Funktion ist nötig, wenn write den Sendepuffer weiter benutzt",
    "A busy callback is required while the trace writer borrows its input buffer":
        "Eine Belegt-Funktion ist nötig, solange die Trace-Sendefunktion ihren Puffer weiter benutzt",
    "A no-argument millisecond clock callback is required for timestamped C++ sender":
        "Für den C++-Sender mit Zeitstempeln ist eine Millisekunden-Uhr ohne Argumente nötig",
    "A valid C identifier is required for {key}":
        "Für {key} ist ein gültiger C-Funktionsname nötig",
    "A valid hand-owned KymoStream object binding is required when C++ sender is active":
        "Wenn der C++-Sender aktiv ist, wird der Name eines eigenen KymoStream-Objekts gebraucht",
    "Active runtime requires 1..256 channels": "Die aktive Laufzeit braucht 1..256 Kanäle",
    "An active trace profile requires a valid {key} callback binding":
        "Ein aktives Trace-Profil braucht einen gültigen Funktionsnamen für {key}",
    "At least one named profile is required": "Mindestens ein benanntes Profil ist nötig",
    "Binding must be a valid C/C++ identifier": "Der Name muss ein gültiger C/C++-Bezeichner sein",
    "Binding option must be boolean": "Die Option muss ja oder nein (boolean) sein",
    "Build system must be gcc, cmake, or platformio": "Das Build-System muss gcc, cmake oder platformio sein",
    "C++ timestamp default cannot be enabled when the DATA timestamp feature is disabled":
        "C++-Zeitstempel können nicht standardmäßig an sein, wenn die Funktion Zeitstempel aus ist",
    "Channel ID must be an integer from 0 to 255": "Die Kanal-ID muss eine ganze Zahl von 0 bis 255 sein",
    "Channel ID {cid} is duplicated": "Die Kanal-ID {cid} ist doppelt vergeben",
    "Channel flag must be boolean": "Die Kanaloption muss ja oder nein (boolean) sein",
    "Channel must be an object": "Ein Kanal muss ein Objekt sein",
    "Channel name must not be empty": "Der Kanalname darf nicht leer sein",
    "Channel requests {key}, but the profile does not enable that feature":
        "Der Kanal möchte {key} senden, aber das Profil hat diese Funktion nicht eingeschaltet",
    "Clock callback must be an identifier": "Der Name der Uhr-Funktion muss ein gültiger Bezeichner sein",
    "Clock domain ID must be 0..65535": "Die ID der Uhrendomäne muss 0..65535 sein",
    "Clock frequency must be a positive integer": "Die Taktfrequenz muss eine positive ganze Zahl sein",
    "Enable feature {key} or clear this channel flag.":
        "Schalte die Funktion {key} ein oder nimm den Haken bei diesem Kanal weg.",
    "Enable trace.isr or disable scheduler tracing.":
        "Schalte das Interrupt-Tracing ein oder das Taskwechsel-Tracing aus.",
    "Entry must be an object": "Ein Eintrag muss ein Objekt sein",
    "Feature value must be boolean": "Der Wert einer Funktion muss ja oder nein (boolean) sein",
    "ID must be from 1 to {maximum}": "Die ID muss zwischen 1 und {maximum} liegen",
    "ID {ident} is duplicated": "Die ID {ident} ist doppelt vergeben",
    "Kind must be one of: {choices}": "Die Art muss eine dieser sein: {choices}",
    "KymoCore 7.0 requires configuration API version {api}":
        "KymoCore 7.0 braucht die Konfigurations-API-Version {api}",
    "Name must be a valid identifier": "Der Name muss ein gültiger Bezeichner sein",
    "Name must be at most 128 UTF-8 bytes": "Der Name darf höchstens 128 UTF-8-Bytes lang sein",
    "Name {name!r} is duplicated": "Der Name {name!r} ist doppelt vergeben",
    "Only reset rule new_session is supported": "Nur die Rücksetzregel new_session wird unterstützt",
    "Period must be an integer from 0 to INT32_MAX": "Die Periode muss eine ganze Zahl von 0 bis INT32_MAX sein",
    "Preset must be one of: {choices}": "Die Voreinstellung muss eine dieser sein: {choices}",
    "Profile must be an object": "Ein Profil muss ein Objekt sein",
    "Profile names may contain only letters, digits, underscore and hyphen (1..64 characters)":
        "Profilnamen dürfen nur Buchstaben, Ziffern, Unterstrich und Bindestrich enthalten (1..64 Zeichen)",
    "Profile {selected!r} does not exist": "Das Profil {selected!r} gibt es nicht",
    "Project name must not be empty": "Der Projektname darf nicht leer sein",
    "Registry must be an array": "Das Verzeichnis muss eine Liste sein",
    "Resource budgets must be positive integers or null":
        "Budgets müssen positive ganze Zahlen oder leer sein",
    "Run: kymo-configurator migrate <project> --to-schema {schema}":
        "Führe aus: kymo-configurator migrate <project> --to-schema {schema}",
    "Scheduler tracing is unavailable: this port has no verified scheduler/ISR capability":
        "Taskwechsel-Tracing ist nicht verfügbar: Dieser Port hat keine geprüfte Scheduler-/ISR-Fähigkeit",
    "Scheduler tracing requires ISR tracing": "Taskwechsel-Tracing braucht Interrupt-Tracing",
    "Schema version 1 targets KymoCore 6.x": "Schemaversion 1 gehört zu KymoCore 6.x",
    "Select a port implementation that has passed capability tests.":
        "Wähle eine Port-Implementierung, die die Fähigkeitstests bestanden hat.",
    "Session callback must be an identifier": "Der Name der Sitzungs-Funktion muss ein gültiger Bezeichner sein",
    "Trace binding must be a valid identifier": "Der Name der Trace-Funktion muss ein gültiger Bezeichner sein",
    "Trace capability claims require port.capabilities_verified=true":
        "Angaben zu Trace-Fähigkeiten setzen port.capabilities_verified=true voraus",
    "Trace option must be boolean": "Die Trace-Option muss ja oder nein (boolean) sein",
    "Transport mode must be dedicated_trace or shared_v2":
        "Der Übertragungsmodus muss dedicated_trace oder shared_v2 sein",
    "Unknown binding field {key!r}": "Unbekanntes Funktionsfeld {key!r}",
    "Unknown build field {key!r}": "Unbekanntes Build-Feld {key!r}",
    "Unknown channel field {key!r}": "Unbekanntes Kanalfeld {key!r}",
    "Unknown feature {key!r}": "Unbekannte Funktion {key!r}",
    "Unknown port field {key!r}": "Unbekanntes Port-Feld {key!r}",
    "Unknown port kind": "Unbekannte Port-Art",
    "Unknown profile field {key!r}": "Unbekanntes Profilfeld {key!r}",
    "Unknown registry field {key!r}": "Unbekanntes Verzeichnisfeld {key!r}",
    "Unknown resource field {key!r}": "Unbekanntes Budgetfeld {key!r}",
    "Unknown top-level field {key!r} in schema version {schema}":
        "Unbekanntes Feld {key!r} auf oberster Ebene in Schemaversion {schema}",
    "Unknown trace field {key!r}": "Unbekanntes Trace-Feld {key!r}",
    "Unknown trace port kind": "Unbekannte Trace-Port-Art",
    "Unknown trace {section} field {key!r}": "Unbekanntes Feld {key!r} in trace.{section}",
    "Unknown {section} field {key!r}": "Unbekanntes Feld {key!r} in {section}",
    "Unsupported schema version {version}; supported version is {schema}":
        "Schemaversion {version} wird nicht unterstützt; unterstützt wird Version {schema}",
    "Use a non-empty relative path without parent-directory segments":
        "Verwende einen nicht leeren, relativen Pfad ohne '..'",
    "bindings must be an object": "bindings muss ein Objekt sein",
    "build must be an object": "build muss ein Objekt sein",
    "channels must be an array": "channels muss eine Liste sein",
    "core_count must be 1 or 2": "core_count muss 1 oder 2 sein",
    "counter_bits must be 32 or 64": "counter_bits muss 32 oder 64 sein",
    "critical_enter and critical_exit must be supplied together":
        "critical_enter und critical_exit müssen zusammen angegeben werden",
    "enabled must be boolean": "enabled muss ja oder nein (boolean) sein",
    "features must be an object": "features muss ein Objekt sein",
    "port must be an object": "port muss ein Objekt sein",
    "resources must be an object": "resources muss ein Objekt sein",
    "schema_version must be an integer": "schema_version muss eine ganze Zahl sein",
    "smp_synchronized must be boolean": "smp_synchronized muss ja oder nein (boolean) sein",
    "trace must be an object": "trace muss ein Objekt sein",
    "trace.{section} must be an object": "trace.{section} muss ein Objekt sein",
    "write_borrows_buffer must be boolean": "write_borrows_buffer muss ja oder nein (boolean) sein",
    "{key} must be boolean": "{key} muss ja oder nein (boolean) sein",
    "{key} must be in {low}..{high}": "{key} muss im Bereich {low}..{high} liegen",
    "{key} must be in {low}..{high} and a power of two":
        "{key} muss im Bereich {low}..{high} liegen und eine Zweierpotenz sein",
    "{key} must be {low}..{high}": "{key} muss {low}..{high} sein",
    "{section} must be an object": "{section} muss ein Objekt sein",
}
