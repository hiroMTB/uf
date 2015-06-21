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
#include "ConsoleColor.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
public:
    void setup();
    void update();
    void draw();
    
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

    csound = new Csound();
    //csound->SetOption( (char*)"-odac" );
    
    string fileName = "-o" + uf::getTimeStamp() + ".wav";
    csound->SetOption( const_cast<char*>(fileName.c_str()) );   // file name
    csound->SetOption("-W");                // Wav
    csound->SetOption("-f");                // 32float
    
    std::string orc = loadString( loadAsset("partikkle_2_r1.orc") );
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
    
    
    std::string sco =   R"dlm(
        ; score code
        ;i1	st	dur     speed	grate	gsize	cent	posrnd	cntrnd	pan	dist
        i1	0	10      1.5     1000    1      1200     10000	1400     1	1
        i2  0   10
    )dlm";
    
    {
        cout << sco << endl;
        int result = csound->ReadScore(sco.c_str());
        if( result ==0 ){
            ccout::b("Score file compile OK");
        }else{
            ccout::r("Score file compile Failed");
        }
    }
    
    csound->ReadScore(sco.c_str());
    csound->Start();
    
    int i = 0;
    while ( csoundPerformKsmps(csound->GetCsound() ) == 0) {
         MYFLT * cpp1, *cpp2;
        int result1 = csoundGetChannelPtr( csound->GetCsound(), &cpp1, "cpp1", CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL );
        int result2 = csoundGetChannelPtr( csound->GetCsound(), &cpp2, "cpp2", CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL );
        
        *cpp1 = (mPln.fBm( i*0.05, *cpp2*1.0)-0.5)*0.001;
        *cpp2 = (mPln2.fBm(i*0.01, *cpp1*0.1 + randFloat()*0.05)-0.5);
        i++;
    }
    
    csoundDestroy( csound->GetCsound() );
    quit();
}

void cApp::update(){
}

void cApp::draw(){
    gl::clear();
}


CINDER_APP_NATIVE( cApp, RendererGl(0) )
