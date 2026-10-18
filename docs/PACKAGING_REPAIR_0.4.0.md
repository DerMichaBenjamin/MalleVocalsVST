# ZIP-Reparatur vom 18.10.2026

Der gemeldete 7-Zip-Fehler ist reproduzierbar. Die abgerufene private ZIP-Datei
hat 46.509.558 Bytes (SHA256
`22bb3e2a86c1712fb5d9745ebc5e747bb047bbacae5d62bb15695a232966a27e`).
Sie endet im Crowd-Datenstrom; Central Directory und End-of-Central-Directory
fehlen. Auch die zuvor gespeicherte private 0.3-Bank-ZIP enthält unvollständige
Bankdaten. Die genaue Ursache beim damaligen Erstellen/Übertragen ist nicht
nachgewiesen. Ein erfolgreicher damaliger lokaler Test beweist nicht die
Integrität der ausgelieferten Kopie.

Das private Testpaket wird aus den unveränderten nativen CI-Artefakten des
Runs 38923565359 und einer neu aus den fünf ursprünglichen Benutzer-WAVs
erstellten Bank aufgebaut. Original-Quell-ZIP vollständig per CRC geprüft.
Bank: 48 kHz, Stereo, PCM16; 7,5 s bzw. vier mittige 10-s-Ausschnitte,
50-ms-Randfades, Pegelmetadaten auf etwa -20 dBFS RMS; Grundton unbekannt.
Quellen, Ausschnittpositionen und Sample-Prüfsummen liegen privat im Paket.
Diese neu erzeugte Bank wurde als XML und durch Rückdekodieren aller fünf
WAV-Daten geprüft. Das ist kein erneuter DAW-Import- oder Hörtest.

Der Packager schreibt jetzt zuerst eine temporäre Datei, schließt und
synchronisiert sie, prüft sämtliche CRCs und die Payload-Prüfsummen und
ersetzt erst danach atomar den endgültigen Dateinamen. Fehlende/ungültige
Bank-XML wird vor dem Schreiben abgewiesen. Die fertige ZIP wird zusätzlich
mit `unzip -t` und vollständigem Extrahieren geprüft.

Die Plugin-Binaries bleiben 0.4.0 Alpha. Dies ist eine Reparatur der
Auslieferung, kein neuer DSP-Build. Die technischen und Host-Testgrenzen
aus VALIDATION_0.4.0.md bleiben bestehen.

Die ersetzte Auslieferung wurde nach dem Speichern erneut abgerufen:
48.389.892 Bytes, SHA256
`9726ec1c1b56e30753e370b3a92daec9af60e9cb2721411df066cbc30013335e`.
Dateigröße und Hash stimmen mit der lokal fertiggestellten ZIP überein;
auch diese abgerufene Kopie besteht CRC-, Payload-Hash- und vollständige
Extraktionsprüfung. Die beiden enthaltenen nativen ZIPs bestehen `unzip -t`.
Die konkrete Windows-7-Zip-Version des Nutzers wurde hier nicht ausgeführt.
