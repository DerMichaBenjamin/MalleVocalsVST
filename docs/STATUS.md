# Ehrlicher Funktionsstand — 0.4.0 Alpha

Diese Version ist eine erste native Entwicklungsfassung. Der vollständige Auftrag ist noch nicht abgeschlossen.

0.4 ergänzt einen Aufnahme-Modus mit 0 Samples Plugin-Latenz (Pitch/Tuning aus), Pegelvergleich und Sektionpegel, ein kompakteres skalierbares Fenster sowie Simple-Hall-/Delay-Details. Neue Raum-Engine und Presetabstimmungen sind technisch überprüfbare Änderungen, keine professionelle Hörabnahme. [Umsetzung und Grenzen](FEEDBACK_0.4.md).

## Implementiert

- Native Audioverarbeitung mit stabilen IDs; Mono/Stereo und separater externer Sidechain-Bus.
- Alle in ARCHITECTURE beschriebenen Hauptprozessoren; grafischer EQ, dynamische Resonanzdetektoren, getrennte Presence/Air-Dosierung.
- Echtzeit-Pitch-Shifting mit Formantkompensation und monophoner YIN-Erkennung, Tonart/Skalen/Custom-Noten, Speed/Humanize, kreative Pitch/Formant-Presets.
- Unabhängiger Parallel-Kompressor, zeit-/pitchmodulierter Doubler.
- Sieben eigene synthetische Testcarrier; Import, Vorhören, Wellenform/Drop, eingebetteter Sample-Recall und 16-Band-Cross-Synthesis.
- Separate Delay-/Raum-/Hall-/Throwwege; internes/externes Ducking; tempoabhängiges Pumpen und echte externe Kompression.
- Simple/Advanced, Makros, 55 globale Ausgangspresets, 45 experimentelle Referenzstudien und ein gemeinsamer Fünfer-Baukasten pro Rolle (zusätzlich fünf erhaltene Legacy-IDs), Userpresets, Locks, Suche, Favoriten, Undo/Redo.

- Tatsächlich reorderbare Direktkette mit sicherem Fade-Übergang, Undo/Redo und versioniertem Routing-Recall.
- De-Esser-Startabstimmungen S / SCH-SH / Breitband; Mono/Stereo/Ping-Pong-Throw mit eigener Hallstufe, Tail-Kill und unabhängigem Return.
- Tatsächliche Messung von 42 Preview-Dateien; genaue Grenzen und Zuordnung in SONG_REFERENCE_ANALYSIS.md. Keine subjektive Höranalyse behauptet.

## Noch nicht als fertig freigegeben

- Modular-Graphite-Designrichtung ist in der nativen Oberfläche umgesetzt; finales visuelles Feintuning bleibt offen. Die Crowd-ZIP wurde am 16.10. wiedergefunden. Fünf WAVs sind technisch geprüft und als separate Benutzerbank vorbereitet; öffentliche Factory-Audioverteilung erfordert noch dokumentierte Weitergaberechte.
- Presets sind technische Startpunkte. Keine 55 professionell gehörten/abgenommenen Klangabstimmungen.
- Tuning-Qualität an realem Sprech-/Gesangsmaterial, Stimmwechseln, tiefen Stimmen, Vibrato und Atemgeräuschen benötigt Hörtests. Spektrale Formantkompensation ist keine perfekte monophone Formantrekonstruktion.
- Der Doubler besitzt noch keine echte individuelle Formantverschiebung pro Stimme. Der gemeinsame Pitch/Formant-Prozessor ist getrennt davon vorhanden. Dafür wurde kein Dummy-Regler eingebaut.
- Resonanzbank und Air sind eigene erste Algorithmen; nicht gleichwertig zu soothe/Fresh Air nachgewiesen. Noch kein vollständiges Aliasing-/Rausch-/Phasen-Messprogramm, kein Stereo-Link für die Resonanzbank.
- Stadium übernimmt grobe Artikulation. Natürliche Wörter/Melodie eines Stadionchors sind nicht nachgewiesen. Loop-Crossfade und überblendete Samplewechsel sind jetzt implementiert; feinere Carrier-Pitchführung bleibt offen.
- Detailparameter/Filterwechsel sind noch nicht umfassend gegen schnelle Automation geglättet. Throw-Automation ist blockgenau, nicht unabhängig von der Host-Puffergröße samplegenau.
- Graphite-GUI mit plastischen Reglern ist eine Entwicklungsfassung; kein auditierter vollständiger Tastatur-/Screenreader-Workflow.
- Undo/Redo über JUCE-State ist implementiert; Routing bezieht sich auf Pluginparameter. Host-eigene Bus-/Spurzuordnungen kann ein Plugin nicht im eigenen Undo-Verlauf zurücknehmen.
- Signierungs-/Notarisierungsskript vorbereitet; Developer-ID-Signierung und Notarisierung nicht erfolgt. macOS-CI benutzt ad-hoc Signierung.

## Kompatibilität

| Umgebung | Stand |
|---|---|
| Linux x86_64 / GCC 13 | 0.3-Core lokal gebaut/geprüft; native VST3-/Standalone-/pluginval-Prüfung zuletzt für 0.2.0 |
| Windows x64 | GitHub-CI eingerichtet; aktuelles Ergebnis siehe erfolgreichen Workflow-Lauf |
| macOS arm64 + x86_64 | Universal-CI eingerichtet; aktuelles Ergebnis siehe Workflow-Lauf |
| Sonoma 14.5 | Als Ziel berücksichtigt, aber nicht auf dieser exakten Betriebssystemversion getestet |
| Cubase | Primärer Zielhost, hier nicht installiert, kein echter Hosttest |
| REAPER, Live, FL Studio, Studio One, Bitwig | Nicht hier im jeweiligen Host getestet |
| Logic Pro AU | Nicht hier im Host getestet |

## Reproduzierbare Prüfungen

`malle_tests`: 100 Presets/20 kompatible Varianten-IDs, Wertebereiche, 44.1/48/96 kHz × Mono/Stereo × 1/64/257/1024 Samples, latenzkorrigierter Bypass, finite/beschränkte Presetausgaben, monophone 220-Hz-Erkennung, kein C++-Heap im konfigurierten DSP-Prozessdurchlauf, maßgeblicher Detail-Bypass trotz Makro.

`malle_integration`: Tonart-/Input-Locks, Preset Undo/Redo, kompletter Parameter-Recall, identischer State beim Ansichtswechsel, PNG-Screenshots, eingebetteter Sample-Recall nach Löschen der Originaldatei, Offline-Ausgabe nach Recall.

Tests sind keine menschlichen Hörtests und ersetzen keine DAW-/Gatekeeper-Prüfung. Prüfprotokolle und Workflows zeigen, welche Tests tatsächlich bestanden haben.

## Lokal ausgeführte Messungen (15.10.2026)

`malle_analysis` hat Oktav-Pitch-Shift 220→440 Hz, Hard-Tune 224→220 Hz, lokale Resonanzabsenkung, Air-Hochtonanhebung, externe Sidechain-Kompression sowie das latenzausgerichtete Throw-Send-Fenster inklusive weiterlaufender Fahne bestanden. `malle_integration` bestand lokal unter einem virtuellen X-Display. Das bestätigt State-/Audiologik und gerenderte UI; noch keinen Cubase-Test und keinen menschlichen Hörtest.

Gemessene Werte des lokalen Analyselaufs: Hard-Tune 224 Hz → 221,14 Hz (Ziel 220 Hz; Toleranz 2 Hz), Resonanztest 1355,59 Hz → −2,04 dB, Air bei 10 kHz → +2,88 dB. Das sind synthetische Messsignale, keine Aussage über die Wahrnehmungsqualität realer Stimmen.

## Referenzbank 0.2

45 neue Studienpresets sind echte unterschiedliche Engine-Einstellungen, aber noch keine nachgehörten Künstlerrekonstruktionen. Die gemeinsamen Varianten sind funktionale Engineering-Profile. Nicht belegt sind die getrennten Original-Rollen, genaue Originaleffektzeiten, Strophen/Refrainunterschiede und die musikalische Eignung nach menschlicher Hörabnahme. Mix-/Stem-Messwerte ersetzen diese Prüfungen nicht.

## Objektive Referenz-Renderprüfung (0.2.0)

70 Offline-Render aus den tatsächlich erzeugten Vocal-Schätzdateien: alle 45 Studien, fünf verschiedene Stimmen mit zusätzlichen Eingangspegeln −36/−18 dBFS RMS sowie alle 15 Main-Studien auf derselben Referenz. Alle Ausgaben endlich; höchster Peak −13,32 dBFS, keine Ausgabe über 0 dBFS; größter Mono-Fold-RMS-Verlust 2,33 dB. Alle 15 Main-Studien lieferten unterschiedliche, RMS-normalisierte Spektralmerkmalsvektoren auf identischem Eingang. Das belegt numerische Unterschiede, noch keine wahrnehmbare/musikalisch gute Differenz.

Die Vergleichsspektren sind auf RMS angeglichen; dies ist keine gehörte oder wahrnehmungsbasierte LUFS-Abnahme. Die Eingangsaudios sind bereits produzierte, artefaktbehaftete Vocal-Schätzungen und keine trockenen Sängeraufnahmen. Ergebnisse: `assets/references/render_verification.json`; ausführbares Verfahren: `scripts/verify_reference_renders.py`. Keine urheberrechtlich geschützten Audiodateien im Repository oder Pluginpaket.

## Bestätigte Build-Abnahme 0.2.0

Windows x64 und macOS Universal sind im Workflow 38673300202 erfolgreich gebaut und haben alle drei CTest-Suites bestanden. Mac-CI: Sonoma 14.8.9, ad-hoc Codesign geprüft. Lokal bestand zusätzlich pluginval 1.0.4 Strictness 5 einschließlich GUI. Details, echte Screenshots und direkte Paketlinks in VALIDATION_0.2.0.md. Keine hier erfolgten DAW- oder menschlichen Hörtests. Die zwei ständig latenzstabil laufenden Pitch-Stufen verursachen im gemessenen 44,1-kHz-Render 10.584 Samples (240 ms) Latenz; primär Mixing, kein als verzögerungsarm abgenommenes Live-Monitoring.

## Ergänzung 0.3.0

Sieben echte Crowd-Importslots, eigene Parameter je Platz, atomare audioseitige Slotwahl, Bank-Import/-Export, Clear/Undo und Schema-3-Recall. Details, Altprojektverhalten und Sampleherkunft: [CROWD_BANK.md](CROWD_BANK.md). Neue Build-/Testresultate werden separat ausgewiesen; die älteren Prüfprotokolle gelten für 0.2.0.

## Bestätigte Build-Abnahme 0.3.0

Workflow 38911281676: Windows x64 und Mac Universal erfolgreich; alle vier CTest-Suites bestanden. Mac-CI Sonoma 14.8.9, Codesign nur ad hoc. Nach Download wurden ZIP/SHA256 und PE-/Mach-O-Architekturen geprüft. Neue native Screenshots und genaue Grenzen: [VALIDATION_0.3.0.md](VALIDATION_0.3.0.md).
