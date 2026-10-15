# Architektur und Automationsvertrag

`Source/dsp` kennt JUCE und die UI nicht. `Parameters` definiert stabile IDs, Defaultwerte, 15 aktive gemeinsame Rollenvarianten plus fünf kompatible Legacy-IDs, 55 globale Ausgangspresets und 45 experimentelle Referenzstudien. Der JUCE-Adapter enthält Buses, APVTS, State und Import; der Editor bindet sich an dieselben Parameter.

Standard-Direktkette: Eingangs-EQ → monophones Tuning → FET → Leveling → De-Esser → Klang-EQ → Multiband → Resonanzen → Air → Sättigung → kreative Pitch/Formant-Stufe. Die elf Direktmodule sind als vollständige Permutation in `route.0` bis `route.10` gespeichert. Ungültige Permutationen werden nicht übernommen. Ein 10-ms-Aus-/Einblenden begrenzt Übergangsartefakte beim Umordnen. Es entstehen keine Routing-Schleifen.

Parallele Verdichtung nimmt wahlweise den latenzkompensierten Input oder Post-Direct ab. Doubler/Stadium und die fünf FX-Wege erhalten den Vocalbus und führen getrennt zurück. Listenpositionen sind keine seriellen Verbindungen. Ducking/Pumpen sind Steuerungen mit eigener Quelle und Ziel. Output bildet den Abschluss. FET/Leveling sind eigene Hüllkurvenkompressoren, keine emulierten Originalschaltungen.

## Makros

Advanced zeigt Basiswerte. Der effektive DSP-Wert ist Basis + dokumentierter Makrobeitrag, auf den Parameterbereich begrenzt. Makros stehen separat in den IDs `druck/klang/waerme/breite/raum/bewegung.amount`; Varianten in `.variant`. Die Auflösung erfolgt aus dem vollständigen aktuellen Basiszustand, niemals akkumulativ. Detailänderungen werden nicht zurückgesetzt. Modul-Bypasses sind maßgeblich; ein Makro schaltet ein manuell deaktiviertes Modul nicht wieder ein. Die für Makros nötigen Wege starten aktiv mit neutralem Wet-Anteil.

Die Makroformeln stehen in `resolveInto`. Sechs Sektionen teilen dieselbe Variantentabelle. Es gibt keine 120 separaten Sektion-Presets. Die Varianten sind zunächst Abstimmungsprofile; finales Voicing erfordert Vocal-Hörtests. Alle Basiswerte und Makroparameter werden gespeichert und können unabhängig automatisiert werden. Die Ansicht ist eine reine UI-Eigenschaft.

## Timing

Zwei getrennte Signalsmith-Instanzen für Tune und kreative Bearbeitung laufen dauerhaft; Tuning-Bypass und globaler Bypass bleiben deshalb latenzstabil. Die Summe beider Signalsmith-Eingangs-/Ausgangslatenzen wird an den Host gemeldet. Unbearbeiteter Mix und Sidechain werden passend verzögert. Die Latenz wird nur in `prepareToPlay` gesetzt.

JUCE stellt Parameteränderungen in diesem Adapter blockweise bereit. Throw startet an einer steigenden `throw.trigger`-Flanke, verzögert um interne Latenz, optionalen Startversatz und nächste quantisierte Beatgrenze. `throw.window` begrenzt das Send-Fenster. Die fallende Triggerflanke schließt es nach derselben Latenz-/Offset-/Quantisierungsausrichtung wie die steigende Flanke früher. Ein gehaltenes Trigger-Signal retriggert nicht. Automation muss zum erneuten Triggern erst auf 0 zurückgehen. Nach Schließen des Sends läuft der Delay-Ring weiter. `throw.kill` löscht nur die eigene Throw-Fahne; normale Delays laufen unabhängig. Throw-Hall wird vom Delay-Ausgang gespeist. Alle internen parallelen FX sind vollständig wet: `throw.return` ist der Throw-Return zum trockenen Signal, `fx.mix` skaliert sämtliche FX-Returns einschließlich Throw und dessen Hall einmal gemeinsam. Auf Samplepositionen innerhalb eines großen Hostblocks kann diese Version noch nicht getrennt reagieren: zeitliche Genauigkeit ist durch die Host-Puffergröße begrenzt.

Pumpen folgt bei vorhandenem PPQ jedem Block neu und berücksichtigt Sprünge/Loops/Tempoänderungen. Ohne PPQ läuft ein interner Zähler. Externes Ducking benutzt ausschließlich den gemeldeten externen Bus; fehlendes Signal wird angezeigt und nicht heimlich intern ersetzt.

## Resonanzen und Air

32 logarithmisch verteilte Bandpassdetektoren. Die Energie eines Bandes wird mit seinen Nachbarn verglichen; nur lokale Prominenz oberhalb einer Schwelle erzeugt zeitabhängige Reduktion. Keine festen Notches als „automatisch“ etikettiert. Konservative subtraktive Rückführung, maximaler Stellwert, frequenzabhängige Gewichtung, Attack/Release, Delta. Die Anzeige zeigt Detektor-Stellwerte, nicht einen gemessenen exakten Ausgangs-Frequenzgang. Stereokanäle sind bei dieser Bank noch unabhängig. Air ist eine geschützte Hochtonanhebung, keine identische Fresh-Air-Schaltung und kein Rauschunterdrücker.

## Samples / Echtzeit

Audioimport und WAV-Kodierung erfolgen außerhalb `processBlock`; Projektstate enthält WAV-Daten in Base64. Decode bei Restore läuft außerhalb des DSP-Prozesspfads. Der Audio-Thread liest einen atomar ausgetauschten Zeiger auf unveränderliche Sampledaten. Alte Sampleobjekte bleiben bis zum Instanzende gültig, damit kein Sample im Audiothread zerstört wird; identische Imports werden wiederverwendet. Importbegrenzung: 30 s, 192 kHz, 128 MB gehaltener Audiodaten pro Instanz. Die Undo-Historie kann zusätzliche kodierte Daten halten.

DSP-Puffer werden beim Prepare angelegt; `resolveInto`, Parameterkonfiguration und `process` werden auf C++-Heap-Allokationen geprüft. Pegel, Tune-Crossfade und Hüllkurven werden geglättet. Filter-/Detailautomation benötigt noch einen gezielten Extrem-Automationstest; nicht alle Koeffizienten werden sampleweise interpoliert.

## Stadium

Sieben eigene synthetische Testtexturen. Import ersetzt die Testtextur. Envelope-Following plus 16-Band-Vocoder übertragen Energie und grobe Artikulation, nicht automatisch die Wörter/Melodie eines echten Chors. Sample-Grundton bewirkt Carrier-Transposition, keine polyphone Melodieanalyse. Samplefades wirken an den Dateikanten; Loop-Crossfade und natürlichere Artikulation bleiben Verfeinerungsbedarf.

## State / gemeinsame Presetdaten

Schema 2 speichert sämtliche Parameter einschließlich Reihenfolge, Makro-Basiswerte und Intensitäten, Sampledaten, Locks und Preset-Baseline. Schema-1-Projekte erhalten die damalige Reihenfolge und den kompatiblen Creative-Bypass. Keine alten IDs werden entfernt; eine bitidentische alte DSP-Ausgabe wird wegen Engine-Weiterentwicklung nicht behauptet.

`profiles()` ist die einzige Variantenquelle für beide Ansichten. `ReferencePresets.inc` wird aus Messdaten mit `scripts/generate_reference_presets.py` reproduziert. Manuelle Detailwerte bleiben Basiswerte, Makrobeiträge werden bei jeder Auflösung neu berechnet. Globaler CUSTOM-Status wird über den Vergleich zur gespeicherten Preset-Baseline ermittelt. Momentane Audiospeicher (Delay-Fahnen) werden nicht im State gespeichert oder durch Undo rekonstruiert.
