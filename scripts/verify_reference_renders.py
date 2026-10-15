"""Objective offline checks on locally available estimated vocal stems, no audio distributed.
Usage: python scripts/verify_reference_renders.py STEM_DIRECTORY [build-core/malle_render]
No subjective listening or isolated-role validation is claimed.
"""
from pathlib import Path
import sys,json,subprocess,tempfile,concurrent.futures
import numpy as np
from scipy.io import wavfile
root=Path(__file__).resolve().parents[1]
stems=Path(sys.argv[1]);exe=Path(sys.argv[2] if len(sys.argv)>2 else root/'build-core/malle_render').resolve()
presets=json.loads((root/'assets/references/presets.json').read_text())

def run(job):
    i,track,level=job
    with tempfile.TemporaryDirectory(prefix='malle-render-') as tmp:
        dst=Path(tmp)/'render.wav'
        proc=subprocess.run([str(exe),str(stems/f'{track}-vocals.wav'),str(55+i),str(dst),str(level)],capture_output=True,text=True,check=True)
        latency=int(proc.stdout.strip());sr,y=wavfile.read(dst);y=y.astype(float)
    if y.ndim==1:y=np.column_stack([y,y])
    duration=8*sr
    active=y[latency:latency+duration]
    rms=np.sqrt(np.mean(active**2));mono=active.mean(axis=1)
    # Equal-RMS spectrum permits numerical comparisons independent of output trim.
    spec=np.abs(np.fft.rfft(mono/(rms+1e-12)))**2;freq=np.fft.rfftfreq(len(mono),1/sr)
    bands=[float(spec[(freq>=lo)&(freq<hi)].sum()/(spec.sum()+1e-20)) for lo,hi in [(80,250),(250,1000),(1000,4000),(4000,8000),(8000,16000)]]
    return {'preset':presets[i]['name'],'source_track_id':track,'calibrated_input_rms_dbfs':-24+level,'finite':bool(np.isfinite(y).all()),'peak_dbfs':float(20*np.log10(np.abs(y).max()+1e-12)),'output_rms_dbfs':float(20*np.log10(rms+1e-12)),'mono_fold_rms_relative_db':float(20*np.log10(np.sqrt(np.mean(mono**2))/(rms+1e-12)+1e-12)),'tail_last_250ms_rms_dbfs':float(20*np.log10(np.sqrt(np.mean(y[-sr//4:]**2))+1e-12)),'equal_rms_spectral_energy_fractions':bands,'latency_samples':latency}

jobs=[(i,p['reference_track_ids'][0],0) for i,p in enumerate(presets)]
# Different actual recorded voices / levels, same Main study.
for i in [0,15,18,21,42]:
    for level in [-12,6]:jobs.append((i,presets[i]['reference_track_ids'][0],level))
# All 15 Main studies process the SAME reference to expose names-only duplicates.
common=presets[0]['reference_track_ids'][0]
for i in range(0,45,3):jobs.append((i,common,0))
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
    results=list(pool.map(run,jobs))
assert all(r['finite'] for r in results)
report={'method':'Objective offline rendering only. Demucs combined-vocal estimates are processed references, not dry isolated singers. No human listening/role approval. RMS normalisation is not a perceptual LUFS match. Last 250 ms is measured, not required silent because reverb may continue.','renders':results,'summary':{'count':len(results),'finite':all(r['finite'] for r in results),'max_peak_dbfs':max(r['peak_dbfs'] for r in results),'peak_above_0dbfs_count':sum(r['peak_dbfs']>0 for r in results),'max_mono_fold_loss_db':min(r['mono_fold_rms_relative_db'] for r in results)}}
v=np.array([r['equal_rms_spectral_energy_fractions'] for r in results[-15:]])
report['summary']['same_source_main_studies_distinct_spectral_vectors']=len(set(tuple(x) for x in v))
report['summary']['minimum_pairwise_equal_rms_band_vector_distance']=min(float(np.linalg.norm(a-b)) for i,a in enumerate(v) for b in v[i+1:])
(root/'assets/references/render_verification.json').write_text(json.dumps(report,indent=2,ensure_ascii=False,allow_nan=False)+'\n')
print(json.dumps(report['summary']))
