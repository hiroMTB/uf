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
#include "DataGroup.h"
#include "RfExporterBin.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
public:
    void setup();
    void update();
    void draw();
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void keyDown( KeyEvent event );
    void resize();
    
    MayaCamUI camUi;
    Perlin mPln;
    
    DataGroup mDg;
};

void cApp::setup(){
    setWindowPos( 0, 0 );
    setWindowSize( 1920, 1080 );
    
    CameraPersp cam( 1920, 1080, 54.4f, 1, 100000 );
    cam.lookAt( Vec3f(0,0,100), Vec3f(0,0,0) );
    cam.setCenterOfInterestPoint( Vec3f(0,0,0) );
    camUi.setCurrentCam( cam );
    
    mPln.setSeed(123);
    mPln.setOctaves(4);
    
    vector<Vec3f> vs;
    vector<ColorAf> cs;
    vector<float> pos;
    vector<float> vel;
    
    float rf = randFloat();
    for( int i=0; i<100; i++ ){
        for( int j=0; j<100; j++ ){
            
            Vec3f v = mPln.dfBm(rf, rf+i*0.04, rf+j*0.04 );
            v *= 5.0f;
            vs.push_back( v );
            
            ColorAf c( randFloat(), randFloat(), randFloat(), 1 );
            cs.push_back( c );
            
            pos.push_back( v.x );
            pos.push_back( v.y );
            pos.push_back( v.z );
            
            vel.push_back( c.r-0.5 );
            vel.push_back( c.g-0.5 );
            vel.push_back( -c.r );
        }
    }

    mDg.createDot( vs, cs, 0.0 );

    RfExporterBin rfB;
    rfB.write( /*uf::getTimeStamp() +*/ "myParticle_00000.bin", pos, vel );
}

void cApp::update(){
}

void cApp::draw(){
    
    gl::clear( ColorA(1,1,1,1) );

    gl::setMatrices( camUi.getCamera() );
    uf::drawCoordinate( 10 );
    
    // data
    glPointSize( 3 );
    gl::draw( mDg.mDot );
}

void cApp::keyDown( KeyEvent event ) {
    char key = event.getChar();
    switch (key) {
        case 'E':
            
            break;
    }
}

void cApp::mouseDown( MouseEvent event ){
    camUi.mouseDown( event.getPos() );
}

void cApp::mouseDrag( MouseEvent event ){
    camUi.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void cApp::resize(){
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
