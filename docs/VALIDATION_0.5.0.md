# Prüfung 0.5.0 Alpha

Native Build-Quelle: 849ca06f31ebda9a4b0a3a9d2ca55a8287dd28e5,
Branch feature/character-0.5. [Erfolgreicher CI-Run 39327352799](https://github.com/DerMichaBenjamin/MalleVocalsVST/actions/runs/39327352799).

## Native Builds und Prüfungen

| Plattform | Gebaut | Tatsächlich geprüft |
| --- | --- | --- |
| Windows x64 | VST3, Standalone | sechs CTest-Suiten bestanden, 30,63 s; PE-Architektur AMD64 beider Binaries und ZIP-CRC geprüft |
| macOS Universal | VST3, AU, Standalone | sechs CTest-Suiten auf arm64 bestanden, 24,29 s; arm64+x86_64-Slices aller drei Binaries geprüft; ad-hoc Codesign in CI verifiziert |

Die sechs Suiten sind DSP, PluginStateAndUI, MeasuredDSP, CrowdBankDSP,
FeedbackRegression und CharacterRegression. Der native UI-Test prüft unter
anderem den Mono-zu-Stereo-Bus mit externem Lead (auch größere Callbacks),
Undo/Redo der neuen Werte, vollständigen State-Recall, neutralen Ansichtswechsel
und das sichere Schließen des verschachtelten Hall-Panels. Die echten
UI-Screenshots im nativen Paket wurden visuell kontrolliert; Version 0.5.0,
Dunkel/Hell, FX-Ducking und Crowd Pitch sind sichtbar.

Die Intel-Slices wurden nicht ausgeführt. Keine Developer-ID-Signatur oder
Notarisierung; Windows unsigniert. Kein 0.5-pluginval- oder DAW-Test behauptet.

Native Pakete (unverändert aus genau diesem CI-Run):
- Windows: 12.610.789 Bytes, SHA256 `0765d8e543213a16ab2db0c96aa0dc5b9357d1d67b132da8829818920fe0cbce`.
- Mac: 30.642.978 Bytes, SHA256 `8e3a1e89faf95adb0e1286701a083f77a81dcc6acff71e02dc6876450c0c52e5`.

Neue Verarbeitung gilt direkt ohne Legacy-/Versionsschalter; alter Teststand
wird klanglich aktualisiert. Die sechs Simple-Regler bleiben vorhanden.

## Lokal tatsächlich ausgeführt

- GCC 13, Release, unabhängige DSP-Engine; fünf CTest-Suiten bestanden (25,39 s).
- CharacterRegression: EQ-Farbe, erzeugte Obertöne, Stereo-Seitenenergie,
  Druck-Bypass inklusive Parallelweg, internes/externes Ducking 0/100%,
  Carrier-Pitch 0/+12, Throw-Tails nach Send-Ende und Zeitwechsel.
- 45 zusätzliche Offline-Durchläufe mit den fünf gelieferten Crowd-Ausschnitten
  als Eingang, drei Factory-Presets und Eingangs-RMS -24/-18/-12 dBFS;
  48 kHz Stereo, 256 Samples, Aufnahme-Modus, erste zwei Sekunden verworfen.
  Output-RMS minus Input-RMS: -0,088…+0,908 dB. Kleinster Mono-Fold-Wert
  -1,227 dB. Bei -18 dBFS Eingangs-RMS höchster Ausgangspeak -3,935 dBFS.
  Die -12-dBFS-Varianten enthalten bereits übersteuerte Eingangsspitzen;
  kein Limiter/Clipping-Schutz wird behauptet.
- Stadium-Layer separat mit allen fünf importierten Carriern und automatischem
  Slotwechsel über 25 s, Envelope-Modus bei -5 st und Vocoder bei +5 st,
  Stadium Mix 60%, Output -6 dB. Spitzen -13,29/-12,85 dBFS; Mono-Fold
  -0,47/-0,83 dB. Alle 47 Durchläufe endlich und ohne C++-new-Allokationen
  innerhalb des beobachteten Prozesspfads.

Crowds sind kein Ersatz für trockene Einzelstimmen. Die Zahlen belegen weder
eine professionelle Klangabnahme noch für jede Stimme gleichen Lautheitseindruck.
Stadium und Raum sind zusätzliche Effektwege und werden absichtlich nicht
gegen ihre zugemischte Effektmenge pegelnormalisiert. Für Druck/Klang/Wärme/
Breite bleiben bewusste Makeup-/Output-/Trim-Änderungen erhalten.

## Offen

Cubase und andere DAWs, ASIO-Monitoring, tatsächliche Installationen auf
Nutzerrechnern, macOS exakt 14.5, Signierung/Notarisierung für öffentliche
Verteilung, Hörabnahme an Solo-Vocals und Publikumsklängen. Die bekannten
Grenzen aus STATUS.md bleiben bestehen.

## Private Benutzerkopie

Das Paket mit den fünf vorbereiteten Benutzer-Crowds wurde vollständig
erzeugt, per CRC/Manifest geprüft, gespeichert und erneut heruntergeladen.
Auch die gespeicherte Kopie besteht vollständige Extraktion und Hashprüfung:
52.281.756 Bytes, SHA256
`7a1582d75833a0a84a17abade00b029c2b154753376e73667bbf74a13116f69c`.
Das Paket enthält die unveränderten nativen ZIPs aus dem oben genannten Run.
Kommerzielle Weitergaberechte der Benutzer-Crowds sind weiterhin nicht belegt.
