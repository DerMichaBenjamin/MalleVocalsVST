#include "dsp/Engine.h"
#include <fstream>
#include <cstring>
#include <iostream>
#include <cstdint>
#include <new>
#include <cstdlib>
static bool tracing=false;static size_t allocations=0;
void* operator new(std::size_t n){if(tracing)++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void*p)noexcept{std::free(p);}
void operator delete[](void*p)noexcept{std::free(p);}
void operator delete(void*p,std::size_t)noexcept{std::free(p);}
void operator delete[](void*p,std::size_t)noexcept{std::free(p);}
using namespace malle;
uint32_t u32(std::ifstream& f){uint32_t v=0;f.read((char*)&v,4);return v;}
Sample load(const char* path){
 std::ifstream f(path,std::ios::binary);char tag[4];f.read(tag,4);if(std::memcmp(tag,"RIFF",4))throw std::runtime_error("RIFF");u32(f);f.read(tag,4);int rate=0,ch=0,bits=0,format=0;std::vector<char> bytes;
 while(f){f.read(tag,4);auto size=u32(f);if(!f)break;if(!std::memcmp(tag,"fmt ",4)){std::vector<char> a(size);f.read(a.data(),size);auto get=[&](int off,int n){uint32_t v=0;std::memcpy(&v,a.data()+off,n);return v;};format=get(0,2);ch=get(2,2);rate=get(4,4);bits=get(14,2);}else if(!std::memcmp(tag,"data",4)){bytes.resize(size);f.read(bytes.data(),size);}else f.seekg(size,std::ios::cur);if(size%2)f.seekg(1,std::ios::cur);}
 if(format!=1||bits!=16||ch!=2||rate!=48000)throw std::runtime_error("PCM16 stereo 48k required");
 Sample s;s.sampleRate=rate;size_t n=bytes.size()/4;s.left.resize(n);s.right.resize(n);
 for(size_t i=0;i<n;++i){int16_t l,r;std::memcpy(&l,bytes.data()+4*i,2);std::memcpy(&r,bytes.data()+4*i+2,2);s.left[i]=l/32768.f;s.right[i]=r/32768.f;}return s;
}
int main(int argc,char** argv){try{
 if(argc!=6)return 2;
 for(int clip=1;clip<6;++clip){auto s=load(argv[clip]);double sourceEnergy=0;for(size_t i=0;i<s.left.size();++i)sourceEnergy+=(s.left[i]*s.left[i]+s.right[i]*s.right[i])*.5;float originalRms=std::sqrt(sourceEnergy/s.left.size());
 for(int preset:{0,14,32})for(float targetDb:{-24.f,-18.f,-12.f}){
  Engine e;e.prepare(48000,256,2);auto p=presets()[size_t(preset)].values;p[index("monitor.on")]=1;e.setParameters(resolve(p));
  std::array<float,256>l{},r{};float* channels[]={l.data(),r.data()};float cal=gain(targetDb)/originalRms;double sumIn=0,sumOut=0,sumMono=0;float peakIn=0,peakOut=0;long count=0;size_t bad=0;
  allocations=0;tracing=true;
  for(size_t pos=0;pos<s.left.size();pos+=256){int n=int(std::min(size_t(256),s.left.size()-pos));for(int i=0;i<n;++i){l[i]=s.left[pos+i]*cal;r[i]=s.right[pos+i]*cal;if(pos>96000){sumIn+=(l[i]*l[i]+r[i]*r[i])*.5;peakIn=std::max(peakIn,std::max(std::abs(l[i]),std::abs(r[i])));}}
   e.process(channels,2,n,nullptr,0,{128,double(pos)*128/(48000*60),true,true});
   if(pos>96000)for(int i=0;i<n;++i){sumOut+=(l[i]*l[i]+r[i]*r[i])*.5;sumMono+=std::pow((l[i]+r[i])*.5,2);peakOut=std::max(peakOut,std::max(std::abs(l[i]),std::abs(r[i])));if(!std::isfinite(l[i])||!std::isfinite(r[i]))++bad;++count;}
  }
  tracing=false;
  if(bad||allocations)throw std::runtime_error("nonfinite or heap allocation");
  std::cout<<"clip="<<clip<<" preset="<<preset<<" target_rms="<<targetDb<<" measured_in_rms="<<db(float(std::sqrt(sumIn/count)))<<" measured_out_rms="<<db(float(std::sqrt(sumOut/count)))<<" peak_in="<<db(peakIn)<<" peak_out="<<db(peakOut)<<" mono_fold="<<10*std::log10(sumMono/sumOut)<<" finite=true allocations="<<allocations<<"\n";
 }
 }
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}return 0;}
