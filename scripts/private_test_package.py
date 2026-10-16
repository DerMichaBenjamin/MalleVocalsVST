"""Combine verified native ZIPs and an explicitly supplied user bank, privately.
Never run this in public CI or commit the resulting audio/package.
"""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile

parser = argparse.ArgumentParser()
parser.add_argument('--windows', type=Path, required=True)
parser.add_argument('--macos', type=Path, required=True)
parser.add_argument('--bank', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
for item in (args.windows, args.macos, args.bank):
    if not item.is_file():
        parser.error(f'Missing input: {item}')
for item in (args.windows, args.macos):
    with zipfile.ZipFile(item) as archive:
        if archive.testzip():
            parser.error(f'Corrupt archive: {item}')
args.output.parent.mkdir(parents=True, exist_ok=True)
readme = '''Malle Vocals 0.4.0 Alpha / Rohstoff Audio / Micha Benjamin

1. Passende native ZIP aus Builds entpacken und VST3 installieren.
   Windows: VST3-Bundle nach C:\\Program Files\\Common Files\\VST3.
   Mac: VST3 nach ~/Library/Audio/Plug-Ins/VST3,
        AU nach ~/Library/Audio/Plug-Ins/Components.
   Die Mac-ZIP separat entpacken: sie bewahrt Bundle-Rechte und Metadaten.
2. Crowd-Bank: Install_Crowd_Windows.cmd bzw. Install_Crowd_macOS.command
   optional aus diesem entpackten Ordner starten. Nur eine Benutzerdatei
   wird kopiert, keine System-/Sicherheits- oder DAW-Einstellungen geaendert.
   Alternativ: Im Plugin "Sample / Bank laden" -> Crowd/Micha.mallecrowd.
3. Neue Instanzen laden die installierte Bank automatisch. Stadium einschalten,
   Platz 1-5 waehlen; zwei weitere Plaetze sind frei. Projekte enthalten die
   geladenen Samples anschliessend selbst, auch ohne die Originaldatei.
4. Oben Betrieb = Aufnahme: 0 Samples zusaetzliche Plugin-Latenz;
   Tune und kreative Pitch/Formant-Stufe sind in diesem Modus aus.
   Interface-Puffer und andere Plugins koennen weiter verzoegern.
   Mix-Modus aktiviert Pitch samt bisheriger ca. 240-ms-Latenz wieder.
   Modus bei gestopptem Transport wechseln. Alte Projekte laden Legacy/Mix.
5. Pegelgleich fuer Druck/Klang/Waerme/Breite braucht ca. 1-2 s Einschwingen;
   Sektionspegel und Output bleiben separat. Auto-Ausgleich ist kein Limiter.
   Manuelle Compressor-Makeup-/Saturation-Output-Werte bleiben wirksam.
6. "Hall / Delay einstellen" oeffnet die normalen vier Wege direkt aus Simple.
   "Party-Klang 0.4" aktiviert die neue Abstimmung samt Dauer/Typ bei Legacy.
   Throw ist ein eigener Weg; Loslassen schliesst nur den Send, nicht den Tail.

Beide Plattformen sind Entwicklungs-/Testversionen, keine Verkaufsfreigabe.
Mac ad-hoc signiert, nicht Developer-ID-signiert/notarisiert; Windows unsigniert.
Tatsaechliche technische Pruefungen und verbleibende Klang-/Host-Tests stehen
in der Dokumentation der nativen ZIPs. Kein Cubase- oder Hardware-Hoertest
wird durch einen erfolgreichen Build ersetzt.

Diese Benutzerkopie enthaelt fuenf vorbereitete Ausschnitte deiner gelieferten
Crowd-Dateien (einmal 7,5 s, viermal 10 s). Weder deren Grundton noch kommerzielle
Weitergaberechte sind belegt. Nicht als lizenzierte Factory-Samplebank verbreiten.
'''
windows = r'''@echo off
setlocal
set "malleBankDir=%APPDATA%\Rohstoff Audio\Malle Vocals"
if not exist "%malleBankDir%" mkdir "%malleBankDir%"
if exist "%malleBankDir%\Micha.mallecrowd" copy /Y "%malleBankDir%\Micha.mallecrowd" "%malleBankDir%\Micha.previous.mallecrowd" >nul
copy /Y "%~dp0Crowd\Micha.mallecrowd" "%malleBankDir%\Micha.mallecrowd"
if errorlevel 1 exit /B 1
echo Crowd-Bank installiert. Neue Plugin-Instanz oeffnen.
pause
'''.replace('\n', '\r\n')
macos = '''#!/bin/bash
set -eu
malle_package_dir="$(cd -- "$(dirname -- "$0")" && pwd)"
malle_bank_dir="$HOME/Library/Application Support/Rohstoff Audio/Malle Vocals"
mkdir -p "$malle_bank_dir"
if [ -f "$malle_bank_dir/Micha.mallecrowd" ]; then
  cp "$malle_bank_dir/Micha.mallecrowd" "$malle_bank_dir/Micha.previous.mallecrowd"
fi
cp "$malle_package_dir/Crowd/Micha.mallecrowd" "$malle_bank_dir/Micha.mallecrowd"
printf 'Crowd-Bank installiert. Neue Plugin-Instanz oeffnen.\\n'
'''
manifest = {str(item.name): hashlib.sha256(item.read_bytes()).hexdigest()
            for item in (args.windows, args.macos, args.bank)}
with zipfile.ZipFile(args.output, 'w', zipfile.ZIP_DEFLATED) as archive:
    for item in (args.windows, args.macos):
        archive.write(item, 'Builds/' + item.name)
    archive.write(args.bank, 'Crowd/Micha.mallecrowd')
    provenance=args.bank.parent/'CrowdBank_Quellen_und_Messungen.json'
    if provenance.is_file():archive.write(provenance,'Crowd/'+provenance.name)
    archive.writestr('BITTE_ZUERST_LESEN.txt', readme)
    archive.writestr('SHA256.json', json.dumps(manifest, indent=2))
    archive.writestr('Install_Crowd_Windows.cmd', windows)
    info = zipfile.ZipInfo('Install_Crowd_macOS.command')
    info.create_system = 3
    info.external_attr = (0o100755 << 16)
    info.compress_type = zipfile.ZIP_DEFLATED
    archive.writestr(info, macos)
print(args.output)
print(hashlib.sha256(args.output.read_bytes()).hexdigest())
