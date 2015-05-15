#include "cinder/app/AppNative.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Texture.h"
#include "cinder/Camera.h"
#include "cinder/MayaCamUI.h"
#include "CinderOpenCv.h"
#include "ufUtil.h"
#include "cinder/Perlin.h"
#include "cinder/params/Params.h"
#include "ContourMap.h"

#include "csound.hpp"
#include "csPerfThread.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
public:
    void setup();
    void keyDown( KeyEvent event );
    void update();
    void draw();
  	void resize();
    void exit();
    
    int mWin_w = 1920;
    int mWin_h = 1080;

    Perlin mPln;
    
    Csound * csound;
    CsoundPerformanceThread * perfThread;
};

void cApp::setup(){
    
    mPln.setSeed(123);
    mPln.setOctaves(3);

    std::string orc = "sr=44100\n\
    ksmps=32\n\
    nchnls=2\n\
    0dbfs=1\n\
    \n\
    instr 1\n\
    aout vco2 0.5, 440\n\
    outs aout, aout\n\
    endin";
    
    std::string sco = "i1 0 1";
    csound = new Csound();
    csound->SetOption("-odac");
    csound->CompileOrc(orc.c_str());
    csound->ReadScore(sco.c_str());
    csound->Start();
    perfThread = new CsoundPerformanceThread(csound);
    perfThread->Play();
    
    //while(perfThread->GetStatus() == 0);
}

void cApp::update(){
}

void cApp::draw(){
    gl::clear();
}

void cApp::exit(){
    delete csound, perfThread;
}

void cApp::keyDown( KeyEvent event ) {
    char key = event.getChar();
    switch (key) {
    }
}

void cApp::resize(){
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
