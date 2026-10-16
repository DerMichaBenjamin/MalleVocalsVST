# Feedback-Iteration 0.4.0 Alpha

Stand 16.10.2026. Dies ist eine gezielte technische Überarbeitung, noch keine Endabnahme des vollständigen Auftrags.

## Ursache und Umsetzung

| Rückmeldung | Änderung | Grenze |
|---|---|---|
| Fenster passt nicht | Default höchstens 1040 × 813, zusätzlich Bildschirmbegrenzung; skalierbare Arbeitsfläche 1280 × 1000; letzte Maße gespeichert | Sehr kleine Displays unter 800 × 625 brauchen weiterhin Platz; Darstellung bei 80% ist kleiner als bei 100% |
| Enorme Verzögerung | Aufnahme-Modus überspringt beide spektralen Pitch-Nodes und alle zugehörigen Ausrichtungsverzögerungen; 0 Samples an den Host | Tune und kreative Pitch/Formant-Bearbeitung in diesem Modus aus. Interface-/Hostpuffer bleiben. Mix-Modus weiterhin ca. 240 ms |
| Regler machen hauptsächlich leiser | Energievergleich für Druck, Klang, Wärme und Breite; sechs eigene Sektionpegel; klarer dosierte technische Startpresets | Langsamer RMS-Ausgleich, keine LUFS-Normierung und kein Limiter. Eingangsenergie >−60 dBFS, Korrektur begrenzt auf −12/+18 dB je bearbeitetem Node; Pause hält Schätzung. Die ersten ca. 1–2 s können sich einpegeln |
| Simple-Raum zu eingeschränkt | Zusammenhängendes natives Panel mit vier Wegen, On, Send, Hall-Dauer/Pre-Delay, Room/Plate/Hall-Charakter und Delay-Zeit/Raster/Sync/Feedback | Filter/Return/Breite bleiben Advanced. Room/Plate/Hall sind eigene FDN-Abstimmungen, keine physikalischen oder kommerziellen Emulationen |
| Hall wirkt dünn, Throw unsauber | Vier serielle Allpass-Diffusoren vor der achtlinigen FDN; neue Decay-Abstimmung; Throw-Send mit 3-ms-Zeitkonstante; Stereo-Zeitabweichung auf ±0,3% statt ±3,5% | Algorithmen technisch verbessert; subjektive Schönheit/Profiqualität nicht nachgewiesen. Mono/Ping-Pong bleibt rastergenau, Stereo bewusst minimal verteilt |
| Crowd fehlt im Download | Private Testpakete enthalten dieselbe vorbereitete Fünfer-Bank und optionale lokale Installationsskripte; neue Instanzen laden installierte Bank automatisch | Nur eigene Benutzerkopie; keine dokumentierte kommerzielle Sample-Weitergabelizenz, daher keine öffentlichen Audio-Assets |

Pegelvergleich arbeitet stereoverknüpft auf quadratischer Energie vor/nach aktiven Klang-Nodes; deaktivierte Nodes sowie Listen-Modi werden nicht automatisch angehoben. Druck betrifft FET, Level und Multiband, Klang betrifft Input-EQ, EQ, Resonanz und Air, Wärme betrifft Sättigung. Der Sektionpegel wird genau einmal nach dem letzten zugehörigen Direktmodul der aktuellen Reihenfolge angewandt. Breite gleicht den zusätzlichen Doubler-Weg relativ zum Signal einschließlich Parallel-Kompression aus. Hall und Pumpen haben Pegelregler, aber keinen automatischen Gegeneingriff gegen den gewählten Effekt. Der Output bleibt separat.

Simple/Advanced ändern keine Audioeinstellungen. Der Aufnahme/Mix-Modus ist eine ausdrückliche DSP-Umschaltung und wird bei Factory- oder Userpresetwechseln beibehalten, bei DAW-Projektrecall vollständig gespeichert. Modus möglichst bei gestopptem Transport wechseln: Der Host muss seine Latenzkompensation aktualisieren; Sample-Kontinuität über einen Wechsel zwischen 0 und ca. 240 ms ist nicht möglich. Pitch-/Ausrichtungshistorien werden beim Wechsel aus vorallokiertem Speicher geleert, FX-Tails bleiben erhalten.

Schema 4 ergänzt IDs ausschließlich am Ende. Fehlende neue Parameter beim Laden alter Projekte und Userpresets bekommen Legacy-Defaults: ursprünglicher Hall, kein Pegelvergleich und Mix-Latenz. Die vorhandenen Werte/Makrozuordnungen alter Projekte werden nicht neu abgestimmt. Factory-Presets wurden bewusst aktualisiert; bereits gespeicherte Projektwerte bleiben davon unabhängig. Die Advanced-Regler zeigen Basiswerte, Makros wirken weiter als gespeicherte Offsets.

## Recherche: Befund, Ableitung und Nachweisgrenzen

- [Xtreme Sound – Über uns](https://www.xtreme-sound.de/ueber-uns/) und [Gerig – Xtreme Sound](https://gerig.de/de/xtreme-sound/) belegen die direkte Genre-/Produzentenreferenz, liefern aber **keine** beweisbare Compressor-/EQ-Geheimkette. Daraus werden keine erfundenen Originalwerte abgeleitet.
- [Tom Lord-Alge, Inside Track, SOS](https://www.soundonsound.com/techniques/inside-track-tom-lord-alge): direktes Mix-Engineer-Interview beschreibt Pitch-/Zeit-Dopplung und konkrete Vocal-Werkzeuge im professionellen Einsatz. Das stützt die technische Richtung Unisono-Verbreiterung, nicht eine Behauptung über identische Malle-Vocals.
- [ValhallaDelay – Controls](https://valhalladsp.com/2019/04/16/valhalladelay-the-controls/) und [PitchDuck](https://valhalladsp.com/2020/11/23/valhalladelay-1-8-2-update-pitch-duck-mode/) dokumentieren voneinander getrennte Timing-/Feedback-/Filter-/Ducking-Aufgaben. Daraus folgt für uns: feste rhythmische Wiederholungen, gefilterte eigene Returns und Ducking statt pauschal weniger Effektlautstärke.
- [Signalsmith – Let's Write A Reverb](https://signalsmith-audio.co.uk/writing/2021/lets-write-a-reverb/) trennt Echobildung/Diffusion und langen FDN-Tail. Unsere konkrete vierstufige serielle Allpass-Vorstufe ist eine eigene vereinfachte Umsetzung des allgemeinen Diffusionsziels, keine Kopie des dort gezeigten Mehrkanal-Diffusors.
- [Signalsmith Stretch – offizielle Dokumentation](https://signalsmith-audio.co.uk/code/stretch/) erklärt Input-/Output-Latenz, Offline-/Realtime-Verwendung und MIT-Lizenz. Im vorhandenen Code sind 120-ms-Fenster pro Pitch-Prozessor konfiguriert. Zwei Stufen verursachen die gemessene ca. 240-ms-Gesamtverzögerung. Das ist eine Architekturursache, kein DAW-Bufferfehler.
- [Signalsmith Compose](https://compose.signalsmith-audio.co.uk/) nennt eine proprietäre Low-Latency-Pitch-Engine; keine Lizenz gekauft oder integriert. Ein künftig niedrig latentes Tuning benötigt eine separate Evaluation. Aktuell wird es nicht durch eine bloße Abspielgeschwindigkeitsänderung ersetzt.

Die schon gemessenen 42 Referenzpreviews und 45 Studienpresets bleiben dokumentiert in SONG_REFERENCE_ANALYSIS.md. Diese Runde ergänzt Dokumentations-/Codeanalyse und neue technische Tests; sie behauptet keine neue subjektive Höranalyse. Zielableitung: stabile vordere Stimme, deutliche Mitten, kontrollierte Zischlaute, dosierte harmonische Verdichtung, separater breiter Hintergrund und rhythmische Effektantwort in Pausen. Stimmperformance, echte Doubles/Harmonien, Arrangement und Mastering bleiben entscheidend; ein Regler erzeugt keine vollständige eingesungene Partyproduktion.

Reverse Engineering: verwendet wurden eigener bestehender Quellcode, offizielle technische Dokumentation und die bereits MIT-lizenzierte Pitch-Bibliothek. Keine kommerziellen Plugin-Binaries dekompiliert, keine Markenalgorithmen übernommen. Professioneller Einsatz einzelner Vorbilder beweist keine gleiche Qualität unserer Umsetzung.

## Prüfungen

Die neue FeedbackRegression prüft Aufnahme-Impulse bei 44,1/48/96 kHz, Mono/Stereo und Block 1/64/257/1024, absichtlichen Pitch-Bypass, wiederhergestellte Mix-Latenz, stationären Kompressor-Pegelvergleich und FDN-Dichte/Decay. DSP prüft zusätzlich Allokationen bei Aufnahme/Mix-Umschaltungen. PluginStateAndUI prüft kompakten Start, sichtbaren Simple-Raumzugang, identischen View-State, Legacy-Migration, neue Parameter und Offline-Recall. Ergebnisse und native Builds werden nach dem tatsächlichen Lauf ergänzt.

Keine Cubase-/Hardware-Monitoring-/DAW-Hostprüfung und keine subjektive Klangabnahme in dieser Umgebung. Echtes niedrig latentes Tuning, pro Doublerstimme unabhängige Formanten, umfassende Detail-Automationsglättung, Aliasing-/Stereolink-Verbesserungen und die musikalische Abnahme bleiben offen.
