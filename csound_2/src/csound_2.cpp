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
#include "ufUtil.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
public:
    void setup();
    
    Csound * csound;
    MYFLT mPitch;
    Perlin mPln;

};

void cApp::setup(){
    
    mPitch = 70;
    
    mPln.setSeed(123);
    mPln.setOctaves(3);

    std::string orc =
    "sr=44100\n\
    ksmps=32\n\
    nchnls=2\n\
    0dbfs=1\n\
    \n\
    instr 1\n\
    kfreq chnget \"pitch\" \n\
    aout vco2 0.1, kfreq\n\
    outs aout, aout\n\
    endin";
    
    csound = new Csound();
    csound->SetOption("-o csound_2.wav");   // file name
    csound->SetOption("-W");                // Wav
    csound->SetOption("-f");                // 32float
    csound->CompileOrc(orc.c_str());

    std::string sco = "i1 0 30";
    csound->ReadScore(sco.c_str());
    csound->Start();
    
    int i = 0;
    while ( csoundPerformKsmps(csound->GetCsound() ) == 0) {
        MYFLT * pvalue;
        int result = csoundGetChannelPtr( csound->GetCsound(), &pvalue, "pitch", CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL );
        if( result != 0){
            cout << "Cant get chPtr" << endl;
        }else{
            *pvalue = mPitch;
            mPitch += mPln.fBm( ++i*0.1, mPitch*0.05 )*100.0;
        }
    }
    
    csoundDestroy( csound->GetCsound() );
    quit();
}


CINDER_APP_NATIVE( cApp, RendererGl(0) )
