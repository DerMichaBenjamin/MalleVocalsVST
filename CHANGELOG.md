# 0.5.0 Alpha

Staerkere Klang-/Waerme-/Breite-Makros mit Legacy-Schalter; Dunkel/Hell-Shelves; Pegelausgleich fuer De-Esser und Parallelweg; Simple-FX-Ducking bis 100%; Crowd Pitch/Filter; Delay-Zeitueberblendung und Mono-zu-Stereo-Buslayout. Details und Testgrenzen: docs/FEEDBACK_0.5.md.

# 0.4.0 Alpha — 16.10.2026

- Expliziter Aufnahme-Modus ohne Pitch-Prozessoren oder zusätzliche Plugin-Latenz; Mix-Modus und Legacy-Recall behalten die Pitch-Verarbeitung.
- Kleineres, bildschirmabhängiges Startfenster mit skalierter Oberfläche und gespeicherten Maßen.
- Zuschaltbarer, begrenzter RMS-Pegelvergleich für Druck/Klang/Wärme/Breite; sechs unabhängige Sektionpegel.
- Simple-Raum-Panel: vier getrennte Wege, Send, Hall-Typ/Dauer/Pre-Delay, Delay-Zeit/Sync/Rhythmus/Feedback.
- Neue opt-in Raum-Engine mit Allpass-Diffusion und bereinigter Decay-Berechnung; Throw mit weicher Send-Hüllkurve und engerem Stereo-Timing.
- Deutlichere technische Ausgangspresets; neue Instanzen mit hörbar dosierter Startkette. Keine subjektive Klangabnahme behauptet.
- State-Schema 4, append-only IDs, Legacy-DSP bei fehlenden neuen Parametern, Undo/Redo und Moduserhalt bei Factory-Presetwechseln.
- Private Crowd-Bank optional lokal installierbar; neue Instanzen laden sie automatisch, DAW-Recall bleibt eingebettet. Keine privaten Samples im öffentlichen Repository.

# 0.3.0 Alpha — 16.10.2026

- Sieben echte importierbare Crowd-Speicherplätze mit unabhängigen Samplepegeln, Grundtönen, Loops und Fades. Slot 1 behält bisherige Parameter-IDs.
- Audioseitige Platzwahl über DAW-Automation ohne Dateizugriff oder Decodieren im Prozesspfad.
- Crowd-Bank laden/speichern und Drag-and-drop, Platz leeren, Bank-/Sample-Undo/Redo und eingebetteter vollständiger Projekt-/Preset-Recall (Schema 3).
- Optionale überlappende Loop-Crossfades; 10-ms-Überblendung beim Wechsel verschiedener Carrier.
- Crowd-Upload wiedergefunden und fünf Originaldateien geprüft; separat ladbare Benutzerbank aus dokumentierten Ausschnitten. Weitergaberechte nicht im Archiv belegt, daher keine Audioverteilung im öffentlichen Quellcode/CI.
- Neue DSP-Tests für Loopgrenzen, Platzwechsel und tatsächlich wirksame Carrier-Automation; Integrationstests für Bank-Recall, Undo/Redo und alte Sample-Presets.

# 0.2.0 Alpha — 15.10.2026

- Native Modular-Graphite-Oberfläche mit sechs Simple-Sektionen, gemeinsamem Advanced-Zustand und durchgehendem Throw-Bereich.
- Reorderbare elf Direktmodule, klare Parallel-/Steuerungswege, State-Schema 2 und Legacy-Migration; Routing Undo/Redo.
- Eigene kreative Pitch-Stufe zusätzlich zum Tuning, latenzstabil ausgerichtete Parallelwege.
- Throw Mono/Stereo/Ping Pong, gerade/punktiert/triolisch, eigener Hall, Throw-Mix und separater Tail-Kill.
- De-Esser S / SCH-SH / Breitband mit Split-/Wideband-Bearbeitung.
- Gemeinsamer Fünfer-Baukasten je Vocal-Rolle, alte Varianten-IDs erhalten; Preset-CUSTOM-Markierung.
- 42 echte Preview-Dateien gemessen, 45 ausdrücklich experimentelle Studienpresets zusätzlich zu 55 technischen Ausgangspresets. Keine separat gehörten Rollenrekonstruktionen behauptet.
- Zusätzliche Routing-/De-Esser-/State-Prüfungen; Windows-Signierungsskript vorbereitet.
- Bekannte Lücken, reale Prüfungen und nicht erfolgte DAW-/Hörtests: docs/STATUS.md.

# Changelog

## 0.1.0 — Entwicklungsversion, 15.10.2026

- Neues JUCE/CMake-Projekt mit Windows x64 VST3 und macOS Universal VST3/AU CI.
- Unabhängige DSP-Engine: Inputfilter, spektrales Pitch/Formant-Processing + YIN, serielle Kompression, De-Esser, 4 dynamische EQ-Bänder, 4-Band-Kompression, dynamische Resonanzbank, Air und Saturation.
- Parallelkompression, modulierter Doubler, sieben synthetische Stadium-Testcarrier und 16-Band-Vocoder.
- Getrennte Raum-/Delay-/FDN-Hall-/Throwwege, Sidechain, Ducking und tempoabhängiges Volume-Shaping bzw. externe Kompression.
- Stabile Parameter-IDs, versionierter State, eingebettete Imports, gemeinsame Makrovarianten, 55 globale Startpresets, Locks, Undo/Redo, Userpresets, Suche und Favoriten.
- Vorläufige skalierbare Simple-/Advanced-Oberfläche, EQ-Interaktion, Spektrum, Reduktionsanzeige, Crowd-Drop/Wellenform/Vorhören.
- Technische Tests und Screenshot-Testwerkzeug. Keine Freigabe als fertiges Produkt; Details in STATUS.md.
