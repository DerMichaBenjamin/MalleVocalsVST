# Ehrlicher Funktionsstand — 0.1.0 Alpha

Diese Version ist eine erste native Entwicklungsfassung. Der vollständige Auftrag ist noch nicht abgeschlossen.

## Implementiert

- Native Audioverarbeitung mit stabilen IDs; Mono/Stereo und separater externer Sidechain-Bus.
- Alle in ARCHITECTURE beschriebenen Hauptprozessoren; grafischer EQ, dynamische Resonanzdetektoren, getrennte Presence/Air-Dosierung.
- Echtzeit-Pitch-Shifting mit Formantkompensation und monophoner YIN-Erkennung, Tonart/Skalen/Custom-Noten, Speed/Humanize, kreative Pitch/Formant-Presets.
- Unabhängiger Parallel-Kompressor, zeit-/pitchmodulierter Doubler.
- Sieben eigene synthetische Testcarrier; Import, Vorhören, Wellenform/Drop, eingebetteter Sample-Recall und 16-Band-Cross-Synthesis.
- Separate Delay-/Raum-/Hall-/Throwwege; internes/externes Ducking; tempoabhängiges Pumpen und echte externe Kompression.
- Simple/Advanced, Makros, 55 globale Ausgangspresets und 20 gemeinsame Varianten, Userpresets, Locks, Suche, Favoriten, Undo/Redo.

## Noch nicht als fertig freigegeben

- Design-Mockups und endgültige Crowd-Aufnahmen stehen aus.
- Presets sind technische Startpunkte. Keine 55 professionell gehörten/abgenommenen Klangabstimmungen.
- Tuning-Qualität an realem Sprech-/Gesangsmaterial, Stimmwechseln, tiefen Stimmen, Vibrato und Atemgeräuschen benötigt Hörtests. Spektrale Formantkompensation ist keine perfekte monophone Formantrekonstruktion.
- Der Doubler besitzt noch keine echte individuelle Formantverschiebung pro Stimme. Der gemeinsame Pitch/Formant-Prozessor ist getrennt davon vorhanden. Dafür wurde kein Dummy-Regler eingebaut.
- Resonanzbank und Air sind eigene erste Algorithmen; nicht gleichwertig zu soothe/Fresh Air nachgewiesen. Noch kein vollständiges Aliasing-/Rausch-/Phasen-Messprogramm, kein Stereo-Link für die Resonanzbank.
- Stadium übernimmt grobe Artikulation. Natürliche Wörter/Melodie eines Stadionchors sind nicht nachgewiesen. Sample-Loop-Crossfade und feinere Carrier-Pitchführung fehlen noch.
- Detailparameter/Filterwechsel sind noch nicht umfassend gegen schnelle Automation geglättet. Throw-Automation ist blockgenau, nicht unabhängig von der Host-Puffergröße samplegenau.
- GUI ist funktional und vorläufig; noch kein finales plastisches Design, kein auditierter vollständiger Tastatur-/Screenreader-Workflow.
- Undo/Redo über JUCE-State ist implementiert; Routing bezieht sich auf Pluginparameter. Host-eigene Bus-/Spurzuordnungen kann ein Plugin nicht im eigenen Undo-Verlauf zurücknehmen.
- Signierungs-/Notarisierungsskript vorbereitet; Developer-ID-Signierung und Notarisierung nicht erfolgt. macOS-CI benutzt ad-hoc Signierung.

## Kompatibilität

| Umgebung | Stand |
|---|---|
| Linux x86_64 / GCC 13 | Native DSP, VST3 und Standalone lokal gebaut; zusätzliche automatisierte Prüfungen im Tests-Verzeichnis |
| Windows x64 | GitHub-CI eingerichtet; aktuelles Ergebnis siehe erfolgreichen Workflow-Lauf |
| macOS arm64 + x86_64 | Universal-CI eingerichtet; aktuelles Ergebnis siehe Workflow-Lauf |
| Sonoma 14.5 | Als Ziel berücksichtigt, aber nicht auf dieser exakten Betriebssystemversion getestet |
| Cubase | Primärer Zielhost, hier nicht installiert, kein echter Hosttest |
| REAPER, Live, FL Studio, Studio One, Bitwig | Nicht hier im jeweiligen Host getestet |
| Logic Pro AU | Nicht hier im Host getestet |

## Reproduzierbare Prüfungen

`malle_tests`: 55 Presets/20 Varianten, Wertebereiche, 44.1/48/96 kHz × Mono/Stereo × 1/64/257/1024 Samples, latenzkorrigierter Bypass, finite/beschränkte Presetausgaben, monophone 220-Hz-Erkennung, kein C++-Heap im konfigurierten DSP-Prozessdurchlauf, maßgeblicher Detail-Bypass trotz Makro.

`malle_integration`: Tonart-/Input-Locks, Preset Undo/Redo, kompletter Parameter-Recall, identischer State beim Ansichtswechsel, PNG-Screenshots, eingebetteter Sample-Recall nach Löschen der Originaldatei, Offline-Ausgabe nach Recall.

Tests sind keine menschlichen Hörtests und ersetzen keine DAW-/Gatekeeper-Prüfung. Prüfprotokolle und Workflows zeigen, welche Tests tatsächlich bestanden haben.

## Lokal ausgeführte Messungen (15.10.2026)

`malle_analysis` hat Oktav-Pitch-Shift 220→440 Hz, Hard-Tune 224→220 Hz, lokale Resonanzabsenkung, Air-Hochtonanhebung, externe Sidechain-Kompression sowie das latenzausgerichtete Throw-Send-Fenster inklusive weiterlaufender Fahne bestanden. `malle_integration` bestand lokal unter einem virtuellen X-Display. Das bestätigt State-/Audiologik und gerenderte UI; noch keinen Cubase-Test und keinen menschlichen Hörtest.

Gemessene Werte des lokalen Analyselaufs: Hard-Tune 224 Hz → 221,14 Hz (Ziel 220 Hz; Toleranz 2 Hz), Resonanztest 1355,59 Hz → −2,04 dB, Air bei 10 kHz → +2,88 dB. Das sind synthetische Messsignale, keine Aussage über die Wahrnehmungsqualität realer Stimmen.
