# Referenzprüfung — 15.10.2026

Es wurde kein kommerzielles Plugin disassembliert, kein DSP-Binärcode extrahiert und kein identischer Klang nachgewiesen. Die Programme sind in dieser Entwicklungsumgebung nicht installiert. Die folgenden Herstellerquellen belegen Funktionen und professionelle Nutzung; Testimonials sind Herstellerangaben, keine unabhängigen Qualitätsmessungen.

| Referenz | Nachweis / Quelle | Verwendung in Malle Vocals |
|---|---|---|
| FabFilter Pro-Q | [Tom Elmhirst, Featured Artist](https://www.fabfilter.com/artists/tom-elmhirst), [Dynamic EQ Handbuch](https://www.fabfilter.com/help/pro-q/using/dynamic-eq) | Direkte Frequenz/Gain-Interaktion, Q, dynamische Bänder, Spektrum und Zahlenfelder als Workflow-Referenz |
| oeksound soothe2 | [Profi-Testimonials](https://oeksound.com/testimonials/), [Handbuch](https://oeksound.com/manuals/soothe2/) | Dynamische Resonanzdetektion, Selektivität, zeitliche Regelung, Delta; eigene konservative Filterbank, kein Qualitätsgleichheitsversprechen |
| UAD 1176 / LA-2A | [Hersteller-Anwendung](https://www.uaudio.com/blogs/ua/how-to-use-the-la-2a-and-1176-in-your-mix), [LA-2A Anwenderberichte](https://www.uaudio.com/products/teletronix-la-2a-collection) | Schnelle Spitzenkontrolle vor langsamer Verdichtung; eigene Kompressoralgorithmen, keine Schaltungsemulation |

## Für eine echte Vergleichsmessung

Unter derselben Host-Samplerate Sweep, Impuls, Sinusburst, Pegelstufen und ein autorisiertes trockenes Vocal durch Referenz und eigene Engine rendern. Pegel abgleichen. Frequenzgang, harmonische Verzerrungen, Attack/Release, Sibilanz und Resonanzreduktion vergleichen. Render und genaue Einstellungen/Versionsnummern protokollieren. Keine Aussage „wie Referenz X“ ohne diese Messungen und Hörvergleich.

## Lizenzierte Implementierungsbasis

- JUCE 8.0.12, Revision `29396c22c93392d6738e021b83196283d6e4d850`.
- Signalsmith Stretch `a670068d9aeb64913331d5cc29337b19a457a7df`, MIT: spektrales Pitch-Shifting mit Formantkompensation. [Offizielle Anleitung](https://github.com/Signalsmith-Audio/signalsmith-stretch). Die Wahl ist technisch begründet; eine belegte Gleichwertigkeit zu Auto-Tune/Melodyne wird nicht behauptet.
- Signalsmith Linear 0.6.4, MIT, transitive FFT/STFT-Basis.
- Eigene Biquads, Hüllkurven, YIN-Erkennung, Makros, Delaywege, FDN-Hall, Resonanzbank und Vocoder.

Vor einer öffentlichen kommerziellen Veröffentlichung muss die zum Eigentümer passende JUCE-Lizenz feststehen; es wurde nichts gekauft und keine freie Lizenz für die eigenen Produktquellen festgelegt. [JUCE-Lizenzbedingungen](https://juce.com/legal/juce-8-licence/).
