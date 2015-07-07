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
    void loadSimulationData( string fileName );
    
    int intensityW;
    int intensityH;
    bool bStart = false;
    
    MayaCamUI camUi;
    Perlin mPln;
    
    Exporter mExp;
    gl::VboMesh mPoints;
    
    vector<Vec3f> ps;
    vector<ColorAf> cs;
};


void cApp::setup(){
    
    setWindowPos( 0, 0 );
    setWindowSize( 1080*3*0.5, 1920*0.5 );
    mExp.setup( 1080*3, 1920, 3000, GL_RGB, uf::getRenderPath(), 0);
    
    CameraPersp cam(1080*3, 1920, 60, 1, 10000 );
    cam.lookAt( Vec3f(0,0, 400), Vec3f(0,0,0), Vec3f(1,0,0));
    cam.setCenterOfInterestPoint( Vec3f(0,0,0) );
    cam.setLensShift( 0.6, 0 );
    
    camUi.setCurrentCam( cam );
    
    mPln.setSeed(123);
    mPln.setOctaves(4);

    
    {
        // make point from intensity
        Surface32f sIntensity( loadImage(loadAsset("img/08/chandra-vela_2.0-8.0_flux_loglog_x3_rot.tif")) );
        intensityW = sIntensity.getWidth();
        intensityH = sIntensity.getHeight();
        
        float threashold = 0.08;

        int step = 120;
        float dAngle = 360.0f/step;
        for( int a=0; a<step; a++ ){
            
            float angle = dAngle * a;

            Surface32f::Iter itr = sIntensity.getIter();
            while ( itr.line() ) {
                while( itr.pixel() ){
                    float gray = itr.r();
                    
                    if( threashold<gray && gray<0.85 ){
                        Vec2i pos = itr.getPos();
                        Vec3f v(pos.x-intensityW/2, pos.y-intensityH/2, 0 );
                        Vec3f noise = mPln.dfBm( Vec3f(pos.x, pos.y, gray) );
                        v.rotate( Vec3f(0,1,0),  toRadians(angle) );
                        ps.push_back( v + noise );
                        float c = gray;
                        float a = lmap(c, 0.0f, 1.0f, 0.0001f, 0.3f);
                        cs.push_back( ColorAf(c, c, c, a) );
                    }
                }
            }
        }
        
        mPoints = gl::VboMesh( ps.size(), 0, uf::getVboLayout(), GL_POINTS );
        gl::VboMesh::VertexIter vitr( mPoints );
        for(int i=0; i<ps.size(); i++ ){
            vitr.setPosition( ps[i] );
            vitr.setColorRGBA( cs[i] );

            ++vitr;
        }
    }
    
    
}

void cApp::update(){
    if( !bStart )
        return;
    
}

void cApp::draw(){
    
    mExp.begin( camUi.getCamera() );{
        gl::clear( ColorA(0,0,0,1) );
        gl::enableAlphaBlending();
        gl::enableDepthRead();
        gl::enableDepthWrite();
        
        glPointSize( 1 );
        
        if( !mExp.bSnap && !mExp.bRender ){
            uf::drawCoordinate( 100 );
            gl::drawStrokedCube( Vec3f(0,0,0), Vec3f(intensityW,intensityW,intensityW) );
        }
        uf::drawCoordinate( 100 );
        
        gl::draw( mPoints );
        
    }mExp.end();
    
    gl::clear( ColorA(1,1,1,1) );
    gl::color( Color::white() );
    mExp.draw();
}

void cApp::keyDown( KeyEvent event ) {
    char key = event.getChar();
    switch (key) {
        case 'S':
            mExp.snapShot();
            break;
        
        case 'R':
            mExp.startRender();
            break;

         case 'T':
            mExp.stopRender();
            break;
            
        case ' ':
            bStart = !bStart;
            break;
    }
}

void cApp::mouseDown( MouseEvent event ){
    camUi.mouseDown( event.getPos() );
}

void cApp::mouseDrag( MouseEvent event ){
    camUi.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void cApp::resize(){}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
