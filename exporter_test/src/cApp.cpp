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
#include "ufUtil.h"
#include "ConsoleColor.h"
#include "Exporter.h"

#include <iostream>
#include <fstream>

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
public:
    void setup();
    void update();
    void draw();
    void keyDown( KeyEvent event );
    void resize();
    
    Exporter mExp;
};

void cApp::setup(){
    mExp.setup( 1080*3, 1920, 1, GL_RGB, uf::getRenderPath(), 0);
    setWindowPos( 0, 0 );
    setWindowSize( 1080*3*0.5, 1920*0.5 );
}


void cApp::update(){
}

void cApp::draw(){


    mExp.begin();{
        
        gl::translate( mExp.mFbo.getWidth()/2, mExp.mFbo.getHeight()/2);
        gl::rotate( Vec3f(-1, -20, 30) );
        gl::clear( ColorA(1,1,1,1) );
        // test cube
        gl::lineWidth(3);
        gl::color(1,0,0.2,1);
        
        gl::drawCube( Vec3f(0,0,0), Vec3f(250,250,250) );
        
        // Guide
        uf::drawCoordinate(500);
        
    }mExp.end();
    
    {
        gl::clear( ColorA(0.5, 0.5, 0.5, 1) );
        gl::color( Color::white() );
        mExp.draw();
    }
}

void cApp::keyDown( KeyEvent event ) {
    char key = event.getChar();
    switch (key) {
        case 'S':
            mExp.snapShot();
            break;
    }
}


void cApp::resize(){
//    CameraPersp cam = camUi.getCamera();
//    cam.setAspectRatio( getWindowAspectRatio() );
//    camUi.setCurrentCam( cam );
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
