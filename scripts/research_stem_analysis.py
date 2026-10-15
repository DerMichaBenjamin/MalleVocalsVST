import os,json,pathlib,numpy as np,scipy.signal as sig,scipy.io.wavfile as wav,time,torch
from demucs.pretrained import get_model
from demucs.apply import apply_model
import argparse
parser=argparse.ArgumentParser();parser.add_argument('directory',help='Local directory with manifest.json and authorised preview audio');args=parser.parse_args()
root=pathlib.Path(args.directory)
rows=json.loads((root/'manifest.json').read_text())
torch.set_num_threads(2)
model=get_model('htdemucs').eval()
def measure(x,sr):
 x=x.astype(float);mono=x.mean(axis=1);mid=mono;side=(x[:,0]-x[:,1])/2
 rms=np.sqrt(np.mean(x*x));peak=np.max(np.abs(x));f,t,z=sig.stft(mono,sr,nperseg=2048,noverlap=1536);power=np.abs(z)**2
 total=power.sum()+1e-20
 bands={f'{lo}-{hi}':float(power[(f>=lo)&(f<hi)].sum()/total) for lo,hi in [(80,250),(250,1000),(1000,4000),(4000,8000),(8000,16000)]}
 return {'rms_dbfs':float(20*np.log10(rms+1e-12)),'peak_dbfs':float(20*np.log10(peak+1e-12)),'crest_db':float(20*np.log10((peak+1e-12)/(rms+1e-12))),'spectral_centroid_hz':float((f[:,None]*power).sum()/total),'band_energy_fraction':bands,'side_mid_db':float(10*np.log10((np.mean(side*side)+1e-15)/(np.mean(mid*mid)+1e-15))),'stereo_correlation':float(np.corrcoef(x.T)[0,1])}
cache={};out=[]
for row in rows:
 tid=row['track_id']
 if row['analysis_status']!='audio_accessible':out.append(row);continue
 if tid not in cache:
  sr,x=wav.read(row['audio_file']);x=x.astype(np.float32)/32768 if x.dtype==np.int16 else x.astype(np.float32)/2147483648 if x.dtype==np.int32 else x.astype(np.float32)
  duration=len(x)/sr;start=max(0,(duration-8)/2);clip=x[int(start*sr):int((start+8)*sr)]
  y=torch.from_numpy(clip.T.copy());mean=y.mean();std=y.std();normalized=(y-mean)/(std+1e-8)
  try:
   t=time.time()
   with torch.no_grad(): stems=apply_model(model,normalized[None],device='cpu',shifts=0,split=True,overlap=.25,segment=7.8,progress=False)[0]
   vocal=(stems[model.sources.index('vocals')]*std+mean).T.numpy();stemfile=root/f'{tid}-vocals.wav';wav.write(stemfile,sr,vocal.astype(np.float32))
   data={'preview_duration_s':duration,'analysis_window_preview_s':[round(start,3),round(start+len(clip)/sr,3)],'absolute_song_offset':'unknown - preview offset not supplied by catalog','master':measure(clip,sr),'vocal_estimate':measure(vocal,sr),'separation':'Demucs 4.0.1 htdemucs 955717e8-8726e21a, CPU, shifts=0','separation_note':'Estimated combined vocal stem. Main/backing/crowd not separated; artefacts possible.'}
   print(tid,round(time.time()-t,2),'s',flush=True)
  except Exception as e:data={'error':str(e),'master':measure(clip,sr)};print(tid,'ERROR',str(e),flush=True)
  cache[tid]=data
 clean={k:v for k,v in row.items() if k not in ['audio_file','preview_url']};clean['measurements']=cache[tid];out.append(clean)
 (root/'analysis.json').write_text(json.dumps(out,ensure_ascii=False,indent=2,allow_nan=False))
print('COMPLETE',len(out),len(cache),flush=True)
