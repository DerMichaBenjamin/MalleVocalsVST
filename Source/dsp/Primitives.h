#pragma once
#include <cmath>
#include <algorithm>
#include <vector>
#include <array>
#include <string>
namespace malle {
constexpr float pi=3.14159265358979323846f;
inline float gain(float db){return std::pow(10.f,db*.05f);} inline float db(float a){return 20*std::log10(std::max(a,1e-9f));}
inline float coef(float ms,float sr){return std::exp(-1.f/(std::max(.01f,ms)*.001f*sr));}
struct Biquad {
 float b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
 float tick(float x){float y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;return y;}
 void reset(){z1=z2=0;}
 void set(int type,float hz,float q,float g,float sr){hz=std::clamp(hz,10.f,sr*.46f);q=std::max(.2f,q);float w=2*pi*hz/sr,c=std::cos(w),s=std::sin(w),al=s/(2*q),A=std::sqrt(gain(g)),a0=1+al;float B0=1,B1=0,B2=0,A1=-2*c,A2=1-al;
 if(type==0){B0=(1-c)/2;B1=1-c;B2=B0;}
 else if(type==1){B0=(1+c)/2;B1=-1-c;B2=B0;}
 else if(type==2){B0=al;B1=0;B2=-al;}
 else if(type==3){B0=1+al*A;B1=-2*c;B2=1-al*A;a0=1+al/A;A2=1-al/A;}
 else {float t=2*std::sqrt(A)*al;if(type==4){B0=A*((A+1)-(A-1)*c+t);B1=2*A*((A-1)-(A+1)*c);B2=A*((A+1)-(A-1)*c-t);a0=(A+1)+(A-1)*c+t;A1=-2*((A-1)+(A+1)*c);A2=(A+1)+(A-1)*c-t;}else{B0=A*((A+1)+(A-1)*c+t);B1=-2*A*((A-1)+(A+1)*c);B2=A*((A+1)+(A-1)*c-t);a0=(A+1)-(A-1)*c+t;A1=2*((A-1)-(A+1)*c);A2=(A+1)-(A-1)*c-t;}}
 b0=B0/a0;b1=B1/a0;b2=B2/a0;a1=A1/a0;a2=A2/a0;}
};
struct Envelope{float e=0;float tick(float x,float a,float r){x=std::abs(x);e=(x>e?a:r)*(e-x)+x;return e;}};
struct Compressor {Envelope detector;float reduction=0;float tickGain(float x,float threshold,float ratio,float attack,float release,float knee,float sr){float level=db(detector.tick(x,coef(attack,sr),coef(release,sr))),over=level-threshold,red=0;float slope=1-1/ratio;if(knee>0&&over>-knee/2&&over<knee/2)red=slope*(over+knee/2)*(over+knee/2)/(2*knee);else if(over>0)red=slope*over;reduction=red;return gain(-red);}};
struct Delay {std::vector<float> data;int pos=0;void prepare(int n){data.assign(n,0);pos=0;}float read(float samples)const{float p=float(pos)-std::clamp(samples,1.f,float(data.size()-2));if(p<0)p+=float(data.size());int i=int(p);float f=p-i;return data[size_t(i)]*(1-f)+data[size_t((i+1)%int(data.size()))]*f;}void push(float x){data[size_t(pos)]=x;if(++pos==int(data.size()))pos=0;}};
struct Sample {std::vector<float> left,right;double sampleRate=48000;std::string name="Synthetic test crowd (original)";};
}
