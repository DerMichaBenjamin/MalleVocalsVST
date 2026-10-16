# Malle Vocals 0.4.0 Alpha — technische Abnahme der Feedback-Iteration

**Kein fertiges/klanglich freigegebenes Gesamtprodukt.** Implementierung und Recherche: [FEEDBACK_0.4.md](FEEDBACK_0.4.md). Stand: 16.10.2026.

Gebauter nativer Quellcode: `33267523cb0a86bfdd7902137811722ee127ef1a`.
[Finaler CI-Lauf](https://github.com/DerMichaBenjamin/MalleVocalsVST/actions/runs/38923565359).

## Tatsächliche lokale Prüfungen

GCC 13, Linux x86_64, CMake Release mit `MALLE_BUILD_PLUGIN=OFF`: alle vier Core-Suites bestanden (16,12 s). Native Linux-VST3/Standalone und pluginval wurden für 0.4 nicht erneut gebaut/ausgeführt.

- DSP: 100 Presets, 20 kompatible Varianten, 24 Kombinationen aus 44,1/48/96 kHz × Mono/Stereo × Puffer 1/64/257/1024, Latenz-/Bypassprüfung, Wertebereiche, keine gezählten C++-Heap-Allokationen bei Parameterauflösung, DSP-Verarbeitung und Betriebsmoduswechseln.
- MeasuredDSP: Pitch-Oktave/Hard-Tune im Mix-Modus, Resonanzdetektoren, Air, externer Sidechain, Routingreihenfolge und Throw-Fenster/Ping-Pong/Tail-Kill. Die bisherigen Throw-Spezialtests verwenden Legacy-Parameter; neue Send-Glättung und neue Hallstufe brauchen weiter musikalische Hörabnahme.
- CrowdBankDSP: sieben Sampleplätze, Loop-/Wechselübergänge und wirksame Carrier-Auswahl.
- FeedbackRegression: Aufnahme-Impulse ohne versteckte Sampleverzögerung bei sämtlichen oben genannten Kombinationen; Tune/Pitch ausdrücklich umgangen; Mix-Latenz wiederhergestellt. Stationärer starker FET mit Pegelvergleich: **−0,0026 dB** RMS-Abweichung; manuelles +6-dB-Makeup: **+5,999 dB**, manuelles −6-dB-Saturation-Output: **−5,995 dB**. Neue Hall-Antwort bleibt endlich, hat mehr frühe Samples oberhalb der festgelegten absoluten Schwelle und längere späte Energie bei erhöhtem Decay. Dies ist kein Nachweis identischer oder überlegener wahrgenommener Hallqualität.

Logdateien: [Core](validation/ctest-core-0.4.log), [DSP-Messwerte](validation/feedback-0.4-measurements.log).

## Tatsächliches Audio aus der gelieferten Crowd-Bank

Fünf vorbereitete eigene Benutzer-Ausschnitte (7,5 s plus viermal 10 s), Stereo/48 kHz; je drei technische globale Presets (Main 0, Chor 14, Backing/Harmony 32) und Ziel-RMS −24/−18/−12 dBFS: **45 Renderdurchläufe**. Aufnahme-Modus, 256 Samples, Tempo 128 BPM. Auswertung lässt die ersten zwei Sekunden zum Einschwingen aus. Keine subjektive Höranalyse und kein trockenes Solo-Vocal-Material.

- Alle Ausgaben endlich; 0 gezählte C++-Heap-Allokationen in den gemessenen Prozessabschnitten.
- Gemessene mittlere Ausgangsenergie relativ zum Eingang: **−0,071 bis +0,616 dB**. Damit kein systematischer Pegelverlust bei diesen geprüften Ketten; keine Garantie identischer LUFS oder identischer wahrgenommener Lautheit beliebiger Stimmen.
- Mono-Fold-Verlust maximal **1,045 dB** in dieser Auswahl; keine allgemeine Mono-Freigabe für sämtliche Einstellungen.
- Bei Ziel-RMS −18 dBFS höchste Ausgangsspitze **−4,35 dBFS**.
- Bei Ziel-RMS −12 dBFS hatten bereits alle fünf Eingänge Spitzen **über 0 dBFS**; auch die Ausgänge überschreiten dort 0 dBFS. Kein Limiter eingebaut/behauptet. Genügend Input-/Output-Headroom verwenden.

[Messlog](validation/feedback-input-0.4.log), [ausgeführter Probe-Quellcode](validation/feedback_input_probe.cpp). Die dazugehörigen Samples bleiben privat; keine Nutzer-Audiodateien im öffentlichen Repository.

## Native Builds und Integration

Finaler Lauf **38923565359: beide Plattformen erfolgreich, alle fünf CTest-Suites bestanden**.

| Plattform | Gebaut | Tatsächlich ausgeführt |
|---|---|---|
| Windows x64 | VST3 und Standalone, PE AMD64 `0x8664` geprüft | Fünf Suites auf Windows Server 2025/MSVC; zusammen 13,98 s |
| macOS Universal | VST3, AU, Standalone; x86_64 und arm64 in allen drei Mach-O-Binaries geprüft | Fünf Suites auf Sonoma **14.8.9 / arm64**, zusammen 20,38 s; ad-hoc Codesign-Verifikation erfolgreich |

Die Intel-Slices sind gebaut/geprüft, nicht auf einem Intel-Mac ausgeführt. Deployment-Ziel ist macOS 11.0; der Build ist damit auch für Sonoma **14.5** vorgesehen. **14.5 wurde nicht separat ausgeführt.** Kein AU-Host/auval und kein VST3-Host/pluginval für 0.4 gelaufen. PluginStateAndUI arbeitet direkt am nativen Prozessor/Editor und prüft keine vollständige DAW-Hostintegration.

PluginStateAndUI bestätigt zusätzlich: alle 100 Factory-Presets, Locks, globale/Bank-/Routing-Undo/Redo, kompakter Start, identischer Simple/Advanced-State, state-neutrales Öffnen des eingebetteten Simple-FX-Panels, sichere Parameterablösung beim Schließen eines Editors mit offenem Panel, Schema-2-Legacy-Projektrecall, Moduserhalt bei Userpresets, eingebetteten Bank-/Sample-Recall ohne Originaldatei und exakten Offline-Recall-Output.

Nach Download: beide äußeren und inneren ZIP-CRC-Prüfungen bestanden; SHA256 mit den CI-Summen identisch. [Binär-/Paketmanifest](validation/build-0.4.0.json).

| Öffentliches natives Testpaket, ohne private Crowd-Audiodaten | SHA256 |
|---|---|
| [Windows](https://github.com/DerMichaBenjamin/MalleVocalsVST/actions/runs/38923565359/artifacts/12056225821) | `b0ad6142b5701b28b6bfd50230b57f1581f966c07b4aadd0ac0fcb729f83c44e` |
| [macOS](https://github.com/DerMichaBenjamin/MalleVocalsVST/actions/runs/38923565359/artifacts/12054874619) | `ec18c532fdfbda220cc68b5fea8966b617465f3408b9ab12a9b2f18b78bb8359` |

Artifacts: 30 Tage Aufbewahrung; GitHub-Anmeldung kann erforderlich sein. Das separat bereitgestellte private Benutzerpaket kombiniert diese **unveränderten** nativen ZIPs mit der bereits vorbereiteten Fünfer-Crowd-Bank und optionalen Benutzer-Installationsskripten. Die Skripte werden nicht automatisch ausgeführt; die automatische Bank-Erkennung nach einer Installation ist in dieser Umgebung nicht in einer DAW abgenommen. Manuelles Bank-Laden bleibt verfügbar.

Echte native Editor-Snapshots aus diesem Mac-Lauf: [Simple](screenshots/0.4-simple.png), [Advanced](screenshots/0.4-advanced.png), [Hall/Delay-Panel](screenshots/0.4-hall-delay.png), [Stadium](screenshots/0.4-stadium.png). Visuell geprüft; keine Cubase-Screenshots.

## Ehrliche Grenzen

- Keine Cubase-, REAPER-, Ableton-, FL-Studio-, Studio-One-, Bitwig- oder Logic-Abnahme; keine Hardware-Roundtrip-Messung.
- Aufnahme meldet 0 Samples zusätzliche Plugin-Latenz, mit deaktiviertem Tuning und kreativer Pitch-/Formant-Stufe. Interface- und DAW-Puffer sowie andere Plugins bleiben relevant. Mix behält etwa 240 ms. Betriebsmodus bei gestopptem Transport wechseln; der Host muss die geänderte Latenzkompensation übernehmen.
- Kein Nachweis des typisch gewünschten Party-Vocal-Klangs durch menschliche Hörtests. Vorhandene Referenzstudien sind Annäherungen; Quellen und Grenzen bleiben offen dokumentiert.
- Noch keine pro Doublerstimme eigenen Formanten, keine abschließende Aliasing-/Stereo-Link-/Automationsprüfung und kein live-taugliches Tuning.
- Mac nur ad-hoc signiert, nicht Developer-ID-signiert/notarisiert. Windows-Testbinaries unsigniert. Kommerzielle Weitergaberechte der gelieferten Crowd-Dateien nicht belegt.
