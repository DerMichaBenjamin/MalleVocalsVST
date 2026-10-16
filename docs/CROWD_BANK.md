# Crowd-Bank 0.3.0 Alpha

Sieben unabhängig belegbare Plätze. Platz 1 verwendet die bisherigen IDs `stadium.root/loop/fade/level`; Plätze 2–7 ergänzen `stadium.1.*` bis `stadium.6.*`. Die neue automatisierbare ID `stadium.slot` wählt 0–6 (in der Oberfläche 1–7). Jeder Platz speichert Audio, Namen, Pegel, Grundton, Loop und Fade. Simple/Advanced bedienen dieselben Daten. Advanced zeigt die Details des aktuell gewählten Platzes.

„Sample / Bank laden“ oder eine Datei auf die Oberfläche/Wellenform ziehen. WAV/AIFF/FLAC belegen den gewählten Platz; `.mallecrowd` ersetzt die ganze Bank. Bank speichern/Platz leeren sind direkt erreichbar. Neue Sampledaten und alle Plätze werden in DAW-State und Userpresets eingebettet. Banks laden/leeren und Parameteränderungen unterstützen Undo/Redo. Alte 0.2-Samples werden aus `sample.data/name` in Platz 1 gelesen; bisherige IDs bleiben erhalten. Schema 3 liest weiterhin ältere Projekte.

Die sieben Plätze sind eine Texturauswahl, kein Summierer von sieben gleichzeitig laufenden Crowd-Tracks. Nicht belegte Plätze benutzen die klar bezeichnete synthetische Testtextur. Der frühere Parameter `stadium.texture` wählt diese Testtextur und erscheint in Advanced nur bei leerem Platz. Eine neue Bank muss Stadium nicht automatisch einschalten und verändert nicht die übrige Vocal-Kette.

## Echtzeit und Loops

Alle Audiodateien werden vor dem Prozesspfad dekodiert. Der Audiothread liest pro Block atomare Zeiger auf unveränderliche Daten. Automatisierte Platzwechsel benötigen keine Dateiöffnung, Base64-Decodierung oder Speicherallokation. Die vorherige Textur wird über 10 ms in die neue überblendet. Ein optionaler Loop-Crossfade überlappt Dateiende und Dateianfang; seine Länge folgt dem Fade des Platzes, begrenzt auf ein Viertel der Datei. Die effektive Schleife ist dadurch um die Überlappung kürzer. Ohne Loop-Crossfade bleibt das frühere Rand-Fade-Verhalten erhalten. Altprojekte starten mit deaktiviertem Crossfade.

Maximal 30 Sekunden und 192 kHz pro Platz, höchstens 128 MB behaltene dekodierte Daten pro Instanz. Alte Sampleobjekte bleiben für sichere Echtzeitreferenzen/Undo bis zum Instanzende gültig; bei vielen unterschiedlichen Imports kann das Limit erreicht werden. Der Audio-Prozesspfad enthält keinen Cache-Reclamation-Lock. Ein direkter langer Import meldet die Grenze; die vorbereitete Benutzerbank enthält dokumentierte Ausschnitte.

## Wiedergefundener Upload

`Crowd Samples.zip` vom 15.10.2026 enthält fünf WAV-Dateien:

| Original | Dauer | Format |
|---|---:|---|
| M&K_wow_stadium_crowd_128.wav | 7,5 s | 44,1 kHz, Stereo, 16 Bit |
| MobPanic_BW.15965.wav | 123,237 s | 96 kHz, Stereo, 24 Bit |
| MobPanic_BW.15968.wav | 136,485 s | 96 kHz, Stereo, 24 Bit |
| StadiumCrowd_BW.15982.wav | 215,653 s | 96 kHz, Stereo, 24 Bit |
| StadiumCrowd_BW.15983.wav | 174,656 s | 96 kHz, Stereo, 24 Bit |

Die privat bereitgestellte Benutzerbank enthält den ersten Clip vollständig sowie jeweils zehn Sekunden aus der Mitte der vier langen Dateien, auf 48 kHz Stereo PCM16 konvertiert. Die Originaldateien bleiben unverändert. Startpunkte, technische Pegelmessungen und Prüfsummen liegen der Bank bei. Die Bank enthält fünf echte Texturen, zwei Plätze bleiben frei. Kein künstliches Vervielfachen zu sieben angeblich verschiedenen Originalaufnahmen.

Im Archiv liegt kein Nachweis über kommerzielle Weitergaberechte. Die Audiodaten werden deshalb separat als Benutzerbank bereitgestellt und nicht in das öffentliche Repository oder öffentliche CI-Builds eingebettet. Vor einer mitgelieferten Factory-Samplebank müssen Herkunft/Lizenz und das Recht zur isolierten Weitergabe in einem kommerziellen Plugin geklärt und dokumentiert sein.

Die Untersuchung ist eine technische Dateiprüfung und DSP-Messung. Keine subjektive Höranalyse oder natürliche Übernahme von Wörtern/Melodie behauptet. Die Stadium-Engine unterstützt weiterhin Envelope-Following und 16-Band-Cross-Synthesis.

## Objektive Prüfung der tatsächlichen Dateien

Ein separater Linux-Probe-Lauf las die fünf vorbereiteten PCM16-Clips, steuerte die Engine mit dem ersten Original-Crowdclip (auf −24 dBFS RMS kalibriert) und wechselte alle fünf Sekunden automatisiert den Carrier. Je Modus wurden 25 Sekunden gerendert, bei 48 kHz / Stereo / 256 Samples; Stadium-Mix 0,6, Output −6 dB, Carrierpegel 0 dB. Bankdatei-Pegelkalibrierungen sind davon getrennt dokumentiert. Es wurden keine tatsächlichen Main-Vocals oder isolierten Leads dafür behauptet.

Envelope-Modus: Peak −13,20 dBFS, RMS −28,23 dBFS, Mono-Fold −0,34 dB. Cross-Synthesis: Peak −8,73 dBFS, RMS −16,72 dBFS, Mono-Fold −0,69 dB. Beide Läufe: ausschließlich endliche Ausgaben und null gezählte C++-Heap-Allokationen während Parameterkonfiguration und Audio-Prozessschleife. Die Modi sind bei diesen Einstellungen nicht lautheitsgleich; die Messung ist kein gehörtes A/B-Urteil.

Prüfquelle: `docs/validation/crowd_render_probe.cpp`; Ergebnisse: `docs/validation/crowd-0.3.0-measurements.log`. Kompilierbar nach Core-Build mit `c++ -std=c++17 -O2 -I Source -I build-core/_deps/stretch-src -I build-core/_deps/signalsmith-linear-src/include docs/validation/crowd_render_probe.cpp build-core/libmalle_dsp.a -o crowd_probe`; anschließend die fünf vorbereiteten PCM16-48-kHz-Stereodateien als fünf Argumente übergeben. Keine Audiodatei wird dafür öffentlich mitverteilt.

## Presets / Locks

Factory-Vocal-Presets erhalten die importierte Bank, die gewählte Sampleposition und ihre Pegel-/Grundton-/Loop-/Fade-Einstellungen. Eigene vollständige `.mallevocals`-Presets speichern diese Daten mit. Routing-Lock schützt Direktmodulreihenfolge, Parallelabgriff und Source-/Target-Zuordnungen; reine Ducking-Mengen bleiben Preset-Klangparameter.
