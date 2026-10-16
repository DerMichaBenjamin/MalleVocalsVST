#include "dsp/Engine.h"
#include <iostream>
#include <stdexcept>
using namespace malle;
void require(bool b,const char* what){if(!b)throw std::runtime_error(what);}
int main(){try {
 Sample a,b; a.sampleRate=b.sampleRate=48000;
 a.left.assign(4800,1);b.left.assign(4800,-1);
 CrowdPlayer p;float maxJump=0,previous=0;
 for(int i=0;i<24000;++i){auto y=p.next(i<12000?&a:&b,48000,1,true,20,true,false);
   maxJump=std::max(maxJump,std::abs(y[0]-previous));previous=y[0];
   require(std::isfinite(y[0])&&std::abs(y[0])<=1.00001,"bounded mono sample rendering");
 }
 require(maxJump<.01,"slot crossfade limits jumps");
 Sample ramp;a.left.clear();ramp.sampleRate=48000;ramp.left.resize(4800);
 for(int i=0;i<4800;++i)ramp.left[size_t(i)]=1-2*i/4799.f;
 p.reset();float seamJump=0;previous=0;
 for(int i=0;i<19200;++i){auto y=p.next(&ramp,48000,1,true,20,true,false);if(i>4800)seamJump=std::max(seamJump,std::abs(y[0]-previous));previous=y[0];}
 require(seamJump<.01,"loop boundary crossfade limits seam jump");
 Sample positive,negative;positive.left.assign(4800,.25f);negative.left.assign(4800,-.25f);
 Engine e;e.prepare(48000,64,1);e.setSample(0,&positive);e.setSample(1,&negative);
 auto params=defaults();for(size_t i=0;i<params.size();++i)if(parameters()[i].id.find(".on")!=std::string::npos)params[i]=0;
 params[index("stadium.on")]=1;params[index("stadium.mix")]=1;params[index("stadium.vocoder")]=0;
 params[index("stadium.rough")]=0;params[index("stadium.loopSmooth")]=1;
 std::array<float,64> block{};float* ptr[]{block.data()};double sumA=0,sumB=0;
 for(int t=0;t<48000;t+=64) {
   params[index("stadium.slot")]=t<24000?0.f:1.f;e.setParameters(params);
   for(int i=0;i<64;++i)block[size_t(i)]=.1f*std::sin(2*malle::pi*220*(t+i)/48000);
   e.process(ptr,1,64,nullptr,0,{});
   if(t>18000&&t<23000)for(float v:block)sumA+=v;
   if(t>42000)for(float v:block)sumB+=v;
 }
 require(sumA>10&&sumB<-10,"automated slot changes select actual immutable carrier");
 std::cout<<"PASS: slot crossfade max jump "<<maxJump<<", loop seam "<<seamJump<<"\n";
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
