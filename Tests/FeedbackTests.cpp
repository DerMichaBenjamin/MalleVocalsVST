#include "dsp/Engine.h"
#include <iostream>
#include <stdexcept>
using namespace malle;
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
Values clean(){auto p=defaults();for(size_t i=0;i<p.size();++i)if(parameters()[i].id.find(".on")!=std::string::npos)p[i]=0;return p;}
int main(){try{
  for(double rate:{44100.,48000.,96000.})for(int block:{1,64,257,1024})for(int channels:{1,2}){
    Engine e;e.prepare(rate,block,channels);auto p=clean();p[index("monitor.on")]=1;e.setParameters(p);check(e.latency()==0,"recording reports zero latency");
    std::vector<float> a(size_t(block),0),b(size_t(block),0);a[0]=b[0]=.25f;float* ptr[]={a.data(),b.data()};
    e.process(ptr,channels,block,nullptr,0,{});check(std::abs(a[0]-.25f)<1e-6f,"recording impulse no hidden delay");
    p[index("tune.on")]=1;p[index("creative.on")]=1;p[index("pitch")]=12;e.setParameters(p);a.assign(size_t(block),.1f);b=a;
    e.process(ptr,channels,block,nullptr,0,{});check(std::abs(a[0]-.1f)<1e-6f,"recording explicitly bypasses pitch");
    p[index("monitor.on")]=0;e.setParameters(p);check(e.latency()>0,"mix latency restored");
  }
  // A deliberately compressed 220 Hz signal: matching restores sustained RMS,
  // while leaving the user's final output control and wet returns independent.
  Engine e;e.prepare(48000,256,1);auto p=clean();p[index("monitor.on")]=1;p[index("fet.on")]=1;
  p[index("fet.threshold")]=-30;p[index("fet.ratio")]=8;p[index("druck.match.on")]=1;e.setParameters(p);
  std::array<float,256>a{};float* ptr[]={a.data()};double in=0,out=0;
  for(int pos=0;pos<48000*5;pos+=256){
    for(int i=0;i<256;++i){a[i]=.2f*std::sin(2*malle::pi*220*(pos+i)/48000.f);if(pos>48000*3)in+=a[i]*a[i];}
    e.process(ptr,1,256,nullptr,0,{});if(pos>48000*3)for(float x:a)out+=x*x;
  }
  float delta=10*std::log10(out/in);std::cout<<"Matched compressor RMS difference: "<<delta<<" dB\n";check(std::abs(delta)<1,"gain comparison within 1 dB");
  // Modern diffuser actually raises initial echo density, has bounded finite tails,
  // and uses a decay control independent from the maximum-tail truncation.
  auto response=[](bool modern,float decay){Reverb r;r.prepare(48000);r.configure(.48f,decay,6500,.8f,modern);std::array<double,2> energy{};int nonzero=0;
    for(int i=0;i<48000*4;++i){float x=r.tick(i==0?1.f:0.f);check(std::isfinite(x),"finite reverb response");if(i>2000 && i<12000 && std::abs(x)>1e-6f)++nonzero;if(i>24000 && i<48000)energy[0]+=x*x;if(i>48000*2)energy[1]+=x*x;}
    std::cout<<"Reverb modern="<<modern<<" decay="<<decay<<" occupied="<<nonzero<<" late energy="<<energy[1]<<"\n";return std::pair<int,double>{nonzero,energy[1]};};
  auto old=response(false,1.6f),dense=response(true,1.6f),longer=response(true,3.f);
  check(dense.first>old.first,"diffuser raises early density");check(longer.second>dense.second*2,"longer decay makes a longer tail");
  std::cout<<"PASS recording rates/buffers/layouts; gain comparison; modern reverb\n";
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
