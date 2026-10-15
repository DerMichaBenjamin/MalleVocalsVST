# Architektur und Automationsvertrag

`Source/dsp` kennt JUCE und die UI nicht. `Parameters` definiert stabile IDs, Defaultwerte, 20 gemeinsame Varianten und 55 globale Ausgangspresets. Der JUCE-Adapter enthält Buses, APVTS, State und Import; der Editor bindet sich an dieselben Parameter.

Signalfluss: Input Gain → Low/High-Cut + Korrektur → Pitch/Formant → FET-orientierte Spitzenkompression → weichere Leveling-Stufe → Splitband-De-Esser → 4-Band parametrischer/dynamischer EQ → 4-Band Kompressor → Resonanzbank → Presence/Air → Sättigung → parallele Verdichtung + Doubler + Stadium → Dry/FX Ducking und Shaping → Mix/Output. Parallelkompression erhält den Signalabgriff vor der seriellen Kompression. FET/Leveling sind eigene Hüllkurvenkompressoren, keine emulierten Originalschaltungen.

## Makros

Advanced zeigt Basiswerte. Der effektive DSP-Wert ist Basis + dokumentierter Makrobeitrag, auf den Parameterbereich begrenzt. Makros stehen separat in den IDs `druck/klang/waerme/breite/raum/bewegung.amount`; Varianten in `.variant`. Die Auflösung erfolgt aus dem vollständigen aktuellen Basiszustand, niemals akkumulativ. Detailänderungen werden nicht zurückgesetzt. Modul-Bypasses sind maßgeblich; ein Makro schaltet ein manuell deaktiviertes Modul nicht wieder ein. Die für Makros nötigen Wege starten aktiv mit neutralem Wet-Anteil.

Die Makroformeln stehen in `resolveInto`. Sechs Sektionen teilen dieselbe Variantentabelle. Es gibt keine 120 separaten Sektion-Presets. Die Varianten sind zunächst Abstimmungsprofile; finales Voicing erfordert Vocal-Hörtests. Alle Basiswerte und Makroparameter werden gespeichert und können unabhängig automatisiert werden. Die Ansicht ist eine reine UI-Eigenschaft.

## Timing

Die Pitch-Engine läuft dauerhaft; Tuning-Bypass und globaler Bypass bleiben deshalb latenzstabil. Die Summe aus Signalsmith-Eingangs-/Ausgangslatenz wird an den Host gemeldet. Unbearbeiteter Mix und Sidechain werden passend verzögert. Die Latenz wird nur in `prepareToPlay` gesetzt.

JUCE stellt Parameteränderungen in diesem Adapter blockweise bereit. Throw startet an einer steigenden `throw.trigger`-Flanke, verzögert um interne Latenz, optionalen Startversatz und nächste quantisierte Beatgrenze. `throw.window` bestimmt das Send-Fenster. Ein gehaltenes Trigger-Signal retriggert nicht. Automation muss zum erneuten Triggern erst auf 0 zurückgehen. Nach Schließen des Sends läuft der Delay-Ring weiter. Auf Samplepositionen innerhalb eines großen Hostblocks kann diese Version noch nicht getrennt reagieren: zeitliche Genauigkeit ist durch die Host-Puffergröße begrenzt.

Pumpen folgt bei vorhandenem PPQ jedem Block neu und berücksichtigt Sprünge/Loops/Tempoänderungen. Ohne PPQ läuft ein interner Zähler. Externes Ducking benutzt ausschließlich den gemeldeten externen Bus; fehlendes Signal wird angezeigt und nicht heimlich intern ersetzt.

## Resonanzen und Air

32 logarithmisch verteilte Bandpassdetektoren. Die Energie eines Bandes wird mit seinen Nachbarn verglichen; nur lokale Prominenz oberhalb einer Schwelle erzeugt zeitabhängige Reduktion. Keine festen Notches als „automatisch“ etikettiert. Konservative subtraktive Rückführung, maximaler Stellwert, frequenzabhängige Gewichtung, Attack/Release, Delta. Die Anzeige zeigt Detektor-Stellwerte, nicht einen gemessenen exakten Ausgangs-Frequenzgang. Stereokanäle sind bei dieser Bank noch unabhängig. Air ist eine geschützte Hochtonanhebung, keine identische Fresh-Air-Schaltung und kein Rauschunterdrücker.

## Samples / Echtzeit

Audioimport und WAV-Kodierung erfolgen außerhalb `processBlock`; Projektstate enthält WAV-Daten in Base64. Decode bei Restore läuft außerhalb des DSP-Prozesspfads. Der Audio-Thread liest einen atomar ausgetauschten Zeiger auf unveränderliche Sampledaten. Alte Sampleobjekte bleiben bis zum Instanzende gültig, damit kein Sample im Audiothread zerstört wird; identische Imports werden wiederverwendet. Importbegrenzung: 30 s, 192 kHz, 128 MB gehaltener Audiodaten pro Instanz. Die Undo-Historie kann zusätzliche kodierte Daten halten.

DSP-Puffer werden beim Prepare angelegt; `resolveInto`, Parameterkonfiguration und `process` werden auf C++-Heap-Allokationen geprüft. Pegel, Tune-Crossfade und Hüllkurven werden geglättet. Filter-/Detailautomation benötigt noch einen gezielten Extrem-Automationstest; nicht alle Koeffizienten werden sampleweise interpoliert.

## Stadium

Sieben eigene synthetische Testtexturen. Import ersetzt die Testtextur. Envelope-Following plus 16-Band-Vocoder übertragen Energie und grobe Artikulation, nicht automatisch die Wörter/Melodie eines echten Chors. Sample-Grundton bewirkt Carrier-Transposition, keine polyphone Melodieanalyse. Samplefades wirken an den Dateikanten; Loop-Crossfade und natürlichere Artikulation bleiben Verfeinerungsbedarf.
