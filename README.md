# Malle Vocals — 0.2.0 Alpha

**Rohstoff Audio / Micha Benjamin.** Ein tatsächlich implementiertes C++/JUCE-Audioeffektplugin in Entwicklung. Noch keine verkaufsfertige Vollversion und keine bestätigte Klanggleichheit mit kommerziellen Referenzen.

## Bauen

CMake 3.24+, C++17, Git und ein nativer Compiler. Der erste Build lädt JUCE 8.0.12 und die versionierte Signalsmith-Pitch-Engine. Späterer Pluginbetrieb ist vollständig offline.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 3
ctest --test-dir build -C Release --output-on-failure
```

Windows (Visual Studio 2022 mit Desktop C++): im Configure-Befehl zusätzlich `-A x64`. macOS (Xcode Command Line Tools): zusätzlich `'-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0`. Ergebnis: `build/MalleVocals_artefacts/Release/` mit VST3, Standalone und auf Mac AU. Linux benötigt ALSA-, X11-, FreeType-, Fontconfig- und OpenGL-Entwicklungspakete. UI-Test dort unter Xvfb ausführen.

Nur die unabhängige Audioengine bauen:

```sh
cmake -S . -B build-core -DMALLE_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-core --parallel 3
ctest --test-dir build-core --output-on-failure
```

Aktuelle gebaute Testpakete, Architekturprüfung, Screenshots und genaue Prüfgrenzen: [Validierung 0.2.0](docs/VALIDATION_0.2.0.md).

## Download / Installation

[GitHub Actions](https://github.com/DerMichaBenjamin/MalleVocalsVST/actions): erfolgreichen Lauf öffnen, unter **Artifacts** das Windows- oder macOS-Paket laden. Windows: den gesamten `Malle Vocals.vst3`-Ordner nach `C:\Program Files\Common Files\VST3` kopieren. Mac: VST3 nach `~/Library/Audio/Plug-Ins/VST3`, AU nach `~/Library/Audio/Plug-Ins/Components`. Host neu scannen.

Mac-Testbuilds sind nur ad hoc signiert, nicht mit Developer ID notarisiert. Das ist keine fertige Gatekeeper-Freigabe. Der lokale, credentialfreie Einstieg in den regulären Signierungsablauf ist `scripts/sign-macos.sh`; Identität und Notarisierungsprofil kommen aus dem eigenen Schlüsselbund. Sonoma 14.5 muss separat auf einem echten Rechner geprüft werden.

## Bedienen

1. Simple startet automatisch. Sechs Makros plus gemeinsame Varianten geben die grobe Richtung vor; Advanced zeigt den ausgewählten Modulbereich.
2. Input/Output und FX Mix sind getrennt. Für Main zuerst wenig Breite/Raum; für Chor Tuning ausgeschaltet lassen.
3. EQ-Punkte ziehen: Frequenz/Gain, Mausrad: Q. Alle Detailwerte im Bereich „Klang EQ“. Zahlen lassen sich direkt editieren; Doppelklick setzt den Regler zurück.
4. Crowd über „Crowd laden“ oder Drag-and-drop auf Sample/Wellenform laden; vorhören. Bis eigene Samples vorliegen, erklingen ausdrücklich synthetische Testtexturen. Stadium selbst aktivieren und Menge dosieren.
5. Für externe Lead-Steuerung im Host Sidechain anlegen und Ducking auf Extern stellen. Die Statuszeile meldet fehlendes Signal.
6. Throw halten oder `throw.trigger` automatisieren. Loslassen schließt den latenzausgerichteten Send, spätestens endet er am Send-Fenster. Die Fahne läuft weiter. Throw-Mix ist der eigene Return; Gesamt-FX-Mix skaliert zusätzlich alle parallelen Effektreturns. Optionaler Throw-Hall folgt ausschließlich dem Throw-Delay.
7. Eigene Presets speichern/laden; Tonart, Input und Routing werden standardmäßig geschützt. Undo/Redo, Presetsuche und Favoriten sind vorhanden.

8. Advanced: Direktmodule links per Drag-and-drop umordnen; orange Linie markiert gültige Ziele. Parallelwege bleiben getrennte Returns; die Liste macht daraus keine serielle Kette.
9. Die Bank umfasst 55 technische Ausgangspresets und 45 klar als Studien markierte Referenzansätze. Letztere beruhen auf echten Preview-/Vocal-Schätzmessungen; eine getrennte Rollen-Hörabnahme ist noch offen. [Song-Zuordnung und Methode](docs/SONG_REFERENCE_ANALYSIS.md).

Siehe [Funktions- und Teststand](docs/STATUS.md), [Architektur und Automationsvertrag](docs/ARCHITECTURE.md), [Referenzprüfung](docs/REFERENCE_RESEARCH.md) und [Changelog](CHANGELOG.md).
