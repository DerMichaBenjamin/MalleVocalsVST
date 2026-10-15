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
