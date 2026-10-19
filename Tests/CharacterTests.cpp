#include "dsp/Engine.h"
#include <iostream>
#include <stdexcept>
using namespace malle;
void require(bool b,const char* message){if(!b)throw std::runtime_error(message);}
Values neutral(){auto p=defaults();for(size_t i=0;i<p.size();++i)if(parameters()[i].id.find(".on")!=std::string::npos)p[i]=0;p[index("monitor.on")]=1;return p;}
struct Result {double energy=0,side=0;std::array<double,3> re{},im{};};
Result render(Values base,bool tone=false,int channels=2){
 Engine e;e.prepare(48000,256,channels);e.setParameters(resolve(base));
 Result r;std::array<float,256> l{},right{};float* ptr[]{l.data(),right.data()};
 for(int pos=0;pos<48000*6;pos+=256){
  for(int i=0;i<256;++i){float t=float(pos+i)/48000;float env=.15f+.85f*std::pow(.5f+.5f*std::sin(2*malle::pi*2*t),2.f);
   l[i]=right[i]=tone?.3f*std::sin(2*malle::pi*220*t):env*(.12f*std::sin(2*malle::pi*220*t)+.06f*std::sin(2*malle::pi*2800*t)+.03f*std::sin(2*malle::pi*11000*t));}
  e.process(ptr,channels,256,nullptr,0,{});
  if(pos>=48000*4)for(int i=0;i<256;++i){require(std::isfinite(l[i])&&std::isfinite(right[i]),"finite output");float mono=channels==1?l[i]:(l[i]+right[i])*.5f;
   r.energy+=(l[i]*l[i]+(channels==2?right[i]*right[i]:l[i]*l[i]))*.5;
   if(channels==2)r.side+=std::pow((l[i]-right[i])*.5,2);
   for(int j=0;j<3;++j){double ph=2*malle::pi*std::array<float,3>{220,2800,11000}[j]*(pos+i)/48000.;r.re[j]+=mono*std::cos(ph);r.im[j]+=mono*std::sin(ph);}
  }
 }
 return r;
}
double magnitude(const Result& r,int i){return std::hypot(r.re[i],r.im[i]);}
int main(){try{
 auto p=neutral();p[index("klang.on")]=1;p[index("eq.on")]=1;p[index("klang.match.on")]=1;
 p[index("klang.colour")]=-1;auto dark=render(p);p[index("klang.colour")]=1;auto bright=render(p);
 double tilt=20*std::log10((magnitude(bright,2)/magnitude(bright,0))/(magnitude(dark,2)/magnitude(dark,0)));
 std::cout<<"Colour endpoint high/low contrast="<<tilt<<" dB, RMS delta="<<10*std::log10(bright.energy/dark.energy)<<" dB\n";
 require(tilt>18,"audible dark/bright spectral contrast");require(std::abs(10*std::log10(bright.energy/dark.energy))<1,"colour gain matched");
 p=neutral();p[index("waerme.on")]=1;p[index("sat.on")]=1;p[index("waerme.match.on")]=1;
 auto dry=render(p,true);p[index("waerme.amount")]=1;auto warm=render(p,true);
 // Difference from a sine includes harmonics; energy is independently matched.
 double pureFraction=std::pow(magnitude(warm,0)/magnitude(dry,0),2)*dry.energy/warm.energy;
 std::cout<<"Warmth harmonic residual="<<1-pureFraction<<", RMS delta="<<10*std::log10(warm.energy/dry.energy)<<" dB\n";
 require(1-pureFraction>.015,"warmth generates measurable harmonics");require(std::abs(10*std::log10(warm.energy/dry.energy))<1,"warmth gain matched");
 p=neutral();p[index("breite.on")]=1;p[index("double.on")]=1;p[index("breite.match.on")]=1;p[index("breite.amount")]=1;
 auto wide=render(p);double sideDb=10*std::log10(wide.side/wide.energy);
 std::cout<<"Dual-mono widened side/total="<<sideDb<<" dB\n";require(sideDb>-20,"dual mono produces actual side energy");
 p=presets()[0].values;p[index("monitor.on")]=1;for(auto id:{"room.on","short.on","long.on","reverb.on","throw.on","stadium.on","double.on"})p[index(id)]=0;
 p[index("parallel.on")]=1;p[index("parallel.threshold")]=-10;p[index("parallel.mix")]=1;
 p[index("druck.on")]=0;auto uncompressed=render(p);p[index("druck.on")]=1;auto compressed=render(p);
 double pressureDelta=10*std::log10(compressed.energy/uncompressed.energy);
 std::cout<<"Pressure incl. parallel on/off RMS="<<pressureDelta<<" dB\n";require(std::abs(pressureDelta)<1.5,"pressure bypass gain agreement");
 // Normalised ducking acts on wet return only, including external lead mode.
 for(bool external:{false,true}){
  std::array<double,2> energy{};
  for(int amount=0;amount<2;++amount){Engine e;e.prepare(48000,256,1);auto v=neutral();v[index("short.on")]=1;v[index("short.send")]=1;v[index("short.return")]=1;v[index("short.sync")]=0;v[index("short.time")]=100;v[index("duck.on")]=1;v[index("duck.source")]=external?1.f:0.f;v[index("duck.amount")]=float(amount);e.setParameters(v);
   std::array<float,256>a{},sc{};float* ptr[]{a.data()};const float* side[]{sc.data()};
   for(int pos=0;pos<96000;pos+=256){for(int i=0;i<256;++i)a[i]=sc[i]=.1f*std::sin(2*malle::pi*220*(pos+i)/48000);e.process(ptr,1,256,external?side:nullptr,external?1:0,{});if(pos>48000)for(int i=0;i<256;++i){float d=a[i]-.1f*std::sin(2*malle::pi*220*(pos+i)/48000);energy[amount]+=d*d;}}
  }
  std::cout<<"Ducking external="<<external<<" wet ratio="<<energy[1]/energy[0]<<"\n";require(energy[0]>1e-6&&energy[1]<energy[0]*.001,"100 percent ducks wet while retaining dry");
 }
 // The pitch parameter must alter the imported carrier, not the dry voice.
 Sample carrier;carrier.sampleRate=48000;carrier.left.resize(48000);for(int i=0;i<48000;++i)carrier.left[i]=.1f*std::sin(2*malle::pi*110*i/48000);
 for(float pitch:{0.f,12.f}){Engine e;e.prepare(48000,256,1);e.setSample(&carrier);auto v=neutral();v[index("stadium.on")]=1;v[index("stadium.mix")]=1;v[index("stadium.pitch")]=pitch;v[index("stadium.size")]=0;v[index("stadium.vocoder")]=0;v[index("stadium.rough")]=0;e.setParameters(v);
  std::array<float,256>a{};float* ptr[]{a.data()};double targetRe=0,targetIm=0,wrongRe=0,wrongIm=0;
  for(int pos=0;pos<96000;pos+=256){a.fill(.1f);e.process(ptr,1,256,nullptr,0,{});if(pos>48000)for(int i=0;i<256;++i){double t=(pos+i)/48000.;float y=a[i]-.1f;double f=pitch==0?110:220;targetRe+=y*std::cos(2*malle::pi*f*t);targetIm+=y*std::sin(2*malle::pi*f*t);wrongRe+=y*std::cos(2*malle::pi*(pitch==0?220:110)*t);wrongIm+=y*std::sin(2*malle::pi*(pitch==0?220:110)*t);}}
  double ratio=std::hypot(targetRe,targetIm)/std::max(1e-9,std::hypot(wrongRe,wrongIm));std::cout<<"Crowd pitch="<<pitch<<" target/other="<<ratio<<"\n";require(ratio>8,"stadium pitch moves carrier octave");
 }
 // Captured throw survives send release, and a grid change is crossfaded.
 {Engine e;e.prepare(48000,128,2);auto v=neutral();v[index("throw.on")]=1;v[index("throw.trigger")]=1;v[index("throw.send")]=1;v[index("throw.return")]=.5f;v[index("throw.feedback")]=.55f;v[index("throw.type")]=2;v[index("throw.sync")]=0;v[index("throw.time")]=125;v[index("throw.verb.on")]=1;
  std::array<float,128>l{},r{};float* ptr[]{l.data(),r.data()};double tail=0;float jump=0,previous=0;
  for(int pos=0;pos<96000;pos+=128){v[index("throw.trigger")]=pos<12032?1.f:0.f;if(pos>=36096)v[index("throw.time")]=250;e.setParameters(v);
   for(int i=0;i<128;++i)l[i]=r[i]=pos+i<12000?.1f*std::sin(2*malle::pi*220*(pos+i)/48000):0;
   e.process(ptr,2,128,nullptr,0,{});
   for(int i=0;i<128;++i){require(std::isfinite(l[i])&&std::isfinite(r[i]),"finite throw automation");if(pos>=24000)tail+=l[i]*l[i]+r[i]*r[i];if(pos>=35968&&pos<40000)jump=std::max(jump,std::abs(l[i]-previous));previous=l[i];}
  }
  std::cout<<"Throw after release tail energy="<<tail<<", grid-change jump="<<jump<<"\n";require(tail>.001,"throw tail survives release");require(jump<.08f,"throw grid change bounded");
 }
 std::cout<<"PASS: character, width, gain match, duck range, crowd pitch, throw tails and time change\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
