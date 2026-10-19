"""Package built native bundles without losing Mac permissions (ditto ZIP)."""
from pathlib import Path
import hashlib, platform, shutil, subprocess, zipfile
root=Path(__file__).resolve().parent.parent
system=platform.system()
name=f'MalleVocals-0.5.0-{system}'
stage=root/'packages'/name
if stage.exists():shutil.rmtree(stage)
stage.mkdir(parents=True)
art=root/'build'/'MalleVocals_artefacts'/'Release'
for fmt in ('VST3','AU','Standalone'):
    if (art/fmt).exists():shutil.copytree(art/fmt,stage/fmt,symlinks=True)
for file in ('README.md','CHANGELOG.md'):
    shutil.copy2(root/file,stage/file)
shutil.copytree(root/'docs',stage/'docs')
if (root/'build'/'screenshots').exists():shutil.copytree(root/'build'/'screenshots',stage/'screenshots')
archive=stage.parent/(stage.name+'.zip')
if archive.exists():archive.unlink()
if system=='Darwin':subprocess.run(['ditto','-c','-k','--sequesterRsrc','--keepParent',str(stage),str(archive)],check=True)
else:
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
        for f in stage.rglob('*'):
            if f.is_file():z.write(f,f.relative_to(stage.parent))
archive.with_suffix('.sha256').write_text(hashlib.sha256(archive.read_bytes()).hexdigest()+'  '+archive.name+'\n')
print(archive)
