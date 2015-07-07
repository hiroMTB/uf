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
    cam.lookAt( Vec3f(0,0,10), Vec3f(0,0,0) );
    cam.setCenterOfInterestPoint( Vec3f(0,0,0) );
    camUi.setCurrentCam( cam );
    
    mPln.setSeed(123);
    mPln.setOctaves(4);
    
    vector<Vec3f> vs;
    vector<ColorAf> cs;
    
    float rf = randFloat();
    for( int i=0; i<20; i++ ){
        for( int j=0; j<20; j++ ){
            
            Vec3f v = mPln.dfBm(rf, i*0.04, j*0.04 );
            vs.push_back( v * 10.0f );
            
            ColorAf c( randFloat(), randFloat(), randFloat(), 1 );
            cs.push_back( c );
        }
    }

    mDg.createDot( vs, cs, 0.0 );

    
    //
    //  Write bin file for Realflow particle simulation
    //
    string fileName = uf::getTimeStamp() + "_00001.bin";
    FILE * pFile;
    
    {
        // HEADER
        int     verif           = 0xFABADA;
        char    fluidName[250]  = "myFluidFromC++";
        short   version         = 11;
        float   scale           = 1.0f;
        int     fluidType       = 8;
        float   elapTime        = 0.0f;
        int     frameNum        = 0;
        int     fps             = 30;
        int     nParticles      = vs.size();
        float   radius          = 0.1f;
        float   pressure[3]     = { -1000, 2500, 2000 };
        float   speed[3]        = { 1, 2, 0 };
        float   temperature[3]  = { 300, 300, 299.9 };
        float   emPosition[3]   = { 0, 1, 0 };
        float   emRotation[3]   = { 0, 0, 0 };
        float   emScale[3]      = { 1,1,1 };
        
        pFile = fopen ( fileName.c_str(), "wb");
        fwrite( &verif,         sizeof(int),        1,   pFile );
        fwrite( fluidName,      sizeof(char),     250,   pFile );
        fwrite( &version,       sizeof(short),      1,   pFile );
        fwrite( &scale,         sizeof(float),      1,   pFile );
        fwrite( &fluidType,     sizeof(int),        1,   pFile );
        fwrite( &elapTime,      sizeof(float),      1,   pFile );
        fwrite( &frameNum,      sizeof(int),        1,   pFile );
        fwrite( &fps,           sizeof(int),        1,   pFile );
        fwrite( &nParticles,    sizeof(int),        1,   pFile );
        fwrite( &radius,        sizeof(float),      1,   pFile );
        fwrite( &pressure,      sizeof(float),      3,   pFile );
        fwrite( &speed,         sizeof(float),      3,   pFile );
        fwrite( &temperature,   sizeof(float),      3,   pFile );
        fwrite( &emPosition,    sizeof(float),      3,   pFile );
        fwrite( &emRotation,    sizeof(float),      3,   pFile );
        fwrite( &emScale,       sizeof(float),      3,   pFile );

    }
    
    {
        for( int i=0; i<vs.size(); i++ ){
        //for( int i=0; i<1; i++ ){
            
            float position[3]   = { vs[i].x, vs[i].y, vs[i].z };
            float velocity[3]   = { cs[i].r, cs[i].g, cs[i].b };
            float force[3]      = { 0.0f, 0.0f, 0.0f };
            float vorticity[3]  = { 0.0f, 0.0f, 0.0f };
            float normal[3]     = { 0.0f, 0.0f, 0.0f };
            int   nNeihbors     = 0;
            float texVec[3]     = { 0.0f, 0.0f, 0.0f };
            short infoBit       = 0;
            float elapTime      = 0.0f;
            float isoTime       = 0.0f;
            float viscosity     = 3.0f;
            float density       = 539 + randFloat()*50.0f;
            float pressure      = -1000.0f + randFloat()*10.0f;
            float mass          = 1.0f;
            float temperature   = 300.0f;
            int   pId           = vs.size() - i;
            
            fwrite( position,   sizeof(float),      3,  pFile );
            fwrite( velocity,   sizeof(float),      3,  pFile );
            fwrite( force,      sizeof(float),      3,  pFile );
            fwrite( vorticity,  sizeof(float),      3,  pFile );
            fwrite( normal,     sizeof(float),      3,  pFile );
            fwrite( &nNeihbors, sizeof(int),        1,  pFile );
            fwrite( texVec,     sizeof(float),      3,  pFile );
            fwrite( &infoBit,   sizeof(short),      1,  pFile );
            fwrite( &elapTime,  sizeof(float),      1,  pFile );
            fwrite( &isoTime,   sizeof(float),      1,  pFile );
            fwrite( &viscosity, sizeof(float),      1,  pFile );
            fwrite( &density,   sizeof(float),      1,  pFile );
            fwrite( &pressure,  sizeof(float),      1,  pFile );
            fwrite( &mass,      sizeof(float),      1,  pFile );
            fwrite( &temperature,  sizeof(float),   1,  pFile );
            fwrite( &pId,       sizeof(int),        1,  pFile );
        }
    }
    
    {
        // Footor
        int additioanl_data_per_particle = 0;
        bool RF4_internal_data = 0;
        bool RF5_internal_data = 1;
        int dummy = 0;
        
        fwrite( &additioanl_data_per_particle, sizeof(int), 1, pFile );
        fwrite( &RF4_internal_data, sizeof(bool), 1, pFile );
        fwrite( &RF5_internal_data, sizeof(bool), 1, pFile );
        fwrite( &dummy, sizeof(int), 1, pFile );
    }
    
    fclose(pFile);
    
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
