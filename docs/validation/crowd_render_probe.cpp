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
int main(int argc,char**argv){try{
 if(argc!=6)return 2;std::array<Sample,5> bank;for(int i=0;i<5;++i)bank[i]=load(argv[i+1]);
 for(int mode=0;mode<2;++mode){
 Engine e;e.prepare(48000,256,2);for(int i=0;i<5;++i)e.setSample(i,&bank[i]);
 auto p=defaults();for(size_t i=0;i<p.size();++i)if(parameters()[i].id.find(".on")!=std::string::npos)p[i]=0;
 p[index("stadium.on")]=1;p[index("stadium.mix")]=.6;p[index("stadium.loopSmooth")]=1;p[index("stadium.vocoder")]=float(mode);p[index("output")]=-6;
 double energy=0;for(float x:bank[0].left)energy+=x*x;float cal=.0630957f/std::sqrt(energy/bank[0].left.size());
 std::array<float,256> l{},r{};float* out[]{l.data(),r.data()};double sum=0,mono=0;float peak=0;int count=0;
 e.setParameters(p);allocations=0;tracing=true;
 for(int t=0;t<48000*25;t+=256){p[index("stadium.slot")]=float(std::min(4,t/(48000*5)));e.setParameters(p);for(int j=0;j<256;++j){size_t at=size_t(t+j)%bank[0].left.size();l[j]=bank[0].left[at]*cal;r[j]=bank[0].right[at]*cal;}e.process(out,2,256,nullptr,0,{});for(int j=0;j<256;++j){if(!std::isfinite(l[j])||!std::isfinite(r[j]))throw std::runtime_error("nonfinite");peak=std::max(peak,std::max(std::abs(l[j]),std::abs(r[j])));sum+=(l[j]*l[j]+r[j]*r[j])*.5;mono+=std::pow((l[j]+r[j])*.5,2);++count;}}
 tracing=false;if(allocations)throw std::runtime_error("allocation during processing");
 if(peak>=1)throw std::runtime_error("peak exceeds unity");
 std::cout<<"mode="<<mode<<" frames="<<count<<" peak_dbfs="<<db(peak)<<" rms_dbfs="<<db(float(std::sqrt(sum/count)))<<" mono_fold_db="<<10*std::log10(mono/sum)<<" finite=true allocations="<<allocations<<"\n";
 }
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}return 0;}
