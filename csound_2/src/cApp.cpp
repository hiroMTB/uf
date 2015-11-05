#include "cinder/app/AppNative.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Texture.h"
#include "cinder/Camera.h"
#include "cinder/MayaCamUI.h"
#include "cinder/Perlin.h"
#include "cinder/params/Params.h"
#include "CinderOpenCv.h"

#include "csound.hpp"
#include "csound.h"

#include "ContourMap.h"
#include "mtUtil.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
public:
    void setup();
    
    Csound * csound;
    MYFLT mAmp;
    MYFLT mFreq;
    
    Perlin mPln;
    Perlin mPln2;

};

void cApp::setup(){
    
    mAmp = 0.5;
    mFreq = 200;
    
    mPln.setSeed(123);
    mPln.setOctaves(4);

    mPln2.setSeed(555);
    mPln2.setOctaves(32);

    std::string orc =
    "sr=192000\n\
    ksmps=129\n\
    nchnls=2\n\
    0dbfs=1\n\
    \n\
    instr 1\n\
    kAmp chnget \"amp\" \n\
    kFreq chnget \"freq\" \n\
    aout oscil kAmp, kFreq\n\
    outs aout, aout\n\
    endin";
    
    string fileName = "-o " + mt::getTimeStamp() + ".wav";
    csound = new Csound();
    csound->SetOption( const_cast<char*>(fileName.c_str()) );   // file name
    csound->SetOption("-W");                // Wav
    csound->SetOption("-f");                // 32float
    csound->CompileOrc(orc.c_str());

    std::string sco = "i1 0 30";
    csound->ReadScore(sco.c_str());
    csound->Start();
    
    int i = 0;
    while ( csoundPerformKsmps(csound->GetCsound() ) == 0) {

        i++;
        
        float tempAmp = 1;
        
        if( mPln2.fBm(i*0.1) > 0.01 ){
            tempAmp = 0.5;
        }
        
        {
            MYFLT * pAmp;
            int result = csoundGetChannelPtr( csound->GetCsound(), &pAmp, "amp", CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL );
            *pAmp = mAmp*tempAmp;
            mAmp += (mPln.fBm( i*0.05, (float)mAmp*1.0)-0.5) * 0.1;
            if(mAmp<0)
                mAmp = 0.6;
        }
    
        {
            MYFLT * pFreq;
            int result = csoundGetChannelPtr( csound->GetCsound(), &pFreq, "freq", CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL );
            *pFreq = mFreq;
            mFreq += (mPln2.fBm(i*0.01, mFreq*0.1 + randFloat()*0.05)-0.5) * 2.0;
            mFreq *= mAmp;
        }
        
    }
    
    csoundDestroy( csound->GetCsound() );
    quit();
}


CINDER_APP_NATIVE( cApp, RendererGl(0) )
