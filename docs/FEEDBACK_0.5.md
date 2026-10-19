# 0.5.0 Alpha: Klang, Wärme, Breite und Mischung

Die neue Abstimmung gilt direkt, ohne Versions-/Legacy-Schalter. Die sechs
Simple-Regler bleiben normale Klangregler derselben Engine wie Advanced.
Pegelgleich ist für Druck/Klang/Wärme/Breite standardmäßig aktiv. Alte Teststände
werden mit der neuen Verarbeitung geladen und können anders klingen.
Vier neue Klangparameter plus Crowd-Pitch ergänzen den Zustand; die ehemaligen
Versuchs-Umschalter quality.on/macro.v2.on wurden entfernt. State-Schema 6.
Simple/Advanced teilen Parameter und Audioverarbeitung; Ansichtswechsel laden nichts.

- **Klang:** Intensität verstärkt das gewählte Profil (Körper, Mitten, Präsenz,
  Höhen); `Dunkel / Hell` ergänzt breite Shelves bei 250 Hz und 4,5 kHz:
  -100% = +6/-8 dB, 0 = neutral, +100% = -6/+8 dB. Dies sind Offsets zu den
  Advanced-Basiswerten. Der Regler ist in beiden Ansichten erreichbar.
- **Wärme:** Von unverändertem Signal bei neutraler Basis zu deutlich dichterer
  Sättigung; stärkerer Drive/Mix und zurückgenommener oberer Frequenzbereich.
  Soft/Tube/Hard bleiben in Advanced wählbar. Kein Nachweis aliasingfreier
  Verzerrung: die Stufe ist noch nicht oversampled.
- **Pegel:** Der Vergleich erfasst jetzt auch den zusätzlichen Parallelweg und
  den De-Esser. Letzterer liegt bei der Standardreihenfolge zwischen den
  Kompressoren und EQ; sein Pegelverlust wurde in 0.4 nicht ausgeglichen.
  In 0.5 wird er bei aktivem Klang-Pegelgleich mit erfasst. Makeup, Saturation-
  Output und Sektionspegel bleiben bewusste Pegeländerungen. Der Vergleich
  braucht etwa 1–2 Sekunden Signal; er ist kein Limiter, kein LUFS-Meter und
  garantiert keine identische subjektive Lautheit jeder Stimme.
- **Breite:** Größere Zeitabstände, langsamere unabhängige Phasen pro Stimme
  und angepasste Summierung erzeugen deutlichere L/R-Unterschiede. Mono bleibt
  mono. Zusätzlich wird jetzt Mono-Input → Stereo-Output als Buslayout
  unterstützt; der externe Lead wird vor überlappenden Bus-Schreibzugriffen
  geschützt. Ob Cubase dieses Layout auf einer konkreten Spur anbietet, muss
  im Host geprüft werden. Bei Mono-Ausgang steht ein Hinweis im Status.
- **Hall/Delay-Ducking:** direkt in der Simple-Karte, Details im Hall-Panel.
  0% = keine Absenkung, 100% = vollständig unterdrückter FX-Return, solange
  die erkannte Vocal-Hüllkurve die Schwelle erreicht. Attack/Release bestimmen
  Ein- und Rückblenden. Eigene Vocals oder externer Lead; Dry-Ducking bleibt
  unabhängig in Advanced. Keine Send-Unterbrechung: Tails bleiben im Speicher.
- **Throw:** Delay-Zeit-/Raster-/Tempoänderungen werden in 0.5 über 30 ms
  zwischen alten/neuen Abgriffen geblendet, anstatt abrupt umzuschalten.
  Send-Kanten bleiben geglättet; Loslassen schließt ausschließlich den Send.
  Throw-Hall, Filter und eigener Return bleiben getrennt vom normalen Delay.
- **Stadium:** eigener Pitch -12…+12 Halbtöne, in beiden Ansichten; neue
  Low-/High-Cuts, hörbarerer Hintergrundpegel und begrenzte Carrier-Spitzen.
  Pitch verändert die Wiedergaberate der Crowd-Textur einschließlich ihrer
  Länge/Formanten; kein formanterhaltendes Vocal-Tuning und keine Erzeugung
  echter Harmonien. Diese Funktion bleibt auch im Aufnahme-Modus aktiv.

## Nachgewiesene lokale Prüfungen

GCC 13 Release: DSP, MeasuredDSP, CrowdBankDSP, FeedbackRegression und
CharacterRegression bestanden (24,16 s). Letztere misst an synthetischem Material:

| Messung | Ergebnis |
| --- | --- |
| Dunkel/Hell, Änderung Hochton/Tiefton-Verhältnis | 23,19 dB |
| Dunkel/Hell, RMS-Differenz nach Pegelvergleich | -0,002 dB |
| Wärme, Energie außerhalb des ursprünglichen Sinus | etwa 4,14% |
| Wärme, RMS-Differenz | +0,0004 dB |
| Breite, Seitenenergie/Gesamtenergie bei Dual-Mono | -12,58 dB |
| Druck inkl. Parallelweg an/aus, eingeschwungene RMS-Differenz | -0,057 dB |
| Ducking 100%, intern und extern, stationäres Testsignal | FX unterdrückt, Dry erhalten |
| Crowd Pitch 0/+12 | Carrier-Spektrallinie 110/220 Hz nachgewiesen |

Das ist keine subjektive Hörbewertung. Reale trockene Einzelstimmen,
Cubase-Abnahme, verschiedene ASIO-Systeme und Hörvergleiche der Throw-/Crowd-
Qualität bleiben offen. Native CI-Ergebnisse werden nach dem Build ergänzt.
