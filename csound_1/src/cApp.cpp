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
#include "csPerfThread.hpp"
#include "csound.h"

#include "ContourMap.h"
#include "mtUtil.h"
#include "ConsoleColor.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
public:
    void setup();
    void update();
    void draw();
    void shutdown();
    
    Csound * csound;
    CsoundPerformanceThread * perfThread;
    double mPitch;
    double mPitchFirst;
    Perlin mPln;

};

void cApp::setup(){
    
    mPitch = mPitchFirst = 200;
    
    mPln.setSeed(123);
    mPln.setOctaves(3);

    csound = new Csound();
    csound->SetOption( (char*)"-odac" );
    
    string orc = R"dlm(
                        ; orchestra code
    
                        sr=44100
                        ksmps=32
                        nchnls=2
                        0dbfs=1
    
                        instr 1
                        kfreq chnget "pitch"
                        aout vco2 0.01, kfreq
                        outs aout, aout
                        endin
                )dlm";

    string sco = R"dlm(

                        ; score code

                        i1 0 10000
                 )dlm";
    
    {
        cout << orc << endl;
        int result = csound->CompileOrc( orc.c_str() );
        
        if( result ==0 ){
            ccout::b( "Orcestra file compile OK" );
        }else{
            ccout::r( "Orcestra file compile Failed" );
            quit();
        }
    }

    {
        cout << sco << endl;
        int result = csound->ReadScore(sco.c_str());
        if( result ==0 ){
            ccout::b("Score file compile OK");
        }else{
            ccout::r("Score file compile Failed");
        }
    }
    
    {
        
        int result = csound->Start();
        if( result ==0 ){
            ccout::b("Csound start OK");
        }else{
            ccout::r("CSound start Failed");
            quit();
        }
    }
    perfThread = new CsoundPerformanceThread(csound);
    perfThread->Play();
}

void cApp::update(){
    double * pvalue;
    int result = csoundGetChannelPtr( csound->GetCsound(), &pvalue, "pitch", CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL );
    if( result != 0){
        cout << "Cant get chPtr" << endl;
    }else{
        *pvalue = mPitch;
        mPitch += mPln.fBm( getElapsedFrames()*0.05, mPitch*0.05 )*100.0;
    }
}

void cApp::draw(){
    int w = getWindowWidth();
    int h = getWindowHeight();

    double d = mPitch - mPitchFirst;
    
    gl::clear(Colorf(1,1,1));
    gl::pushMatrices();
    gl::translate(w/2, h/2);
    gl::color(0, 0, 1);
    gl::drawSolidEllipse( Vec2f(0, -d*0.2), 5, 5);
    gl::drawLine(Vec2f(0,0), Vec2f(0,-d*0.2));
    gl::popMatrices();
}

void cApp::shutdown(){
    delete perfThread;
    delete csound;
}


CINDER_APP_NATIVE( cApp, RendererGl(0) )
