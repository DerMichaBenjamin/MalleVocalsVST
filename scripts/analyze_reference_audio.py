"""Development research only. Provide a local manifest pointing to authorised audio.
Uses scipy WAV/STFT measurements and optionally a locally installed Demucs model.
The exact executed analysis procedure and versions are documented in docs/SONG_REFERENCE_ANALYSIS.md.
No song audio or model is part of the plugin and nothing is uploaded by this script.
"""
from pathlib import Path
import argparse, json, numpy as np
from scipy.io import wavfile
from scipy.signal import stft
p=argparse.ArgumentParser();p.add_argument('wav');args=p.parse_args()
sr,x=wavfile.read(args.wav)
x=x.astype(float)/(32768 if x.dtype==np.int16 else 2147483648 if x.dtype==np.int32 else 1)
if x.ndim==1:x=np.column_stack([x,x])
f,t,z=stft(x.mean(axis=1),sr,nperseg=2048,noverlap=1536);power=abs(z)**2
rms=float(np.sqrt(np.mean(x*x)));peak=float(np.max(abs(x)))
print(json.dumps({'duration_s':len(x)/sr,'rms_dbfs':20*np.log10(rms+1e-12),'crest_db':20*np.log10((peak+1e-12)/(rms+1e-12)),'centroid_hz':float((f[:,None]*power).sum()/(power.sum()+1e-20))},indent=2))
