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
    vector<float> mSlope;
    vector<float> mAspect;
    vector<Vec3f> ps;
    vector<ColorAf> cs;
};

void cApp::setup(){
    
    setWindowPos( 0, 0 );
    setWindowSize( 1080*3*0.5, 1920*0.5 );
    mExp.setup( 1080*3, 1920, 3000, GL_RGB, uf::getRenderPath(), 0);
    
    CameraPersp cam(1080*3, 1920, 54.4f, 0.1, 10000 );
    cam.lookAt( Vec3f(0,0, 1600), Vec3f(0,0,0) );
    cam.setCenterOfInterestPoint( Vec3f(0,0,0) );
    camUi.setCurrentCam( cam );
    
    mPln.setSeed(123);
    mPln.setOctaves(4);
    
    Surface32f sAspect( loadImage(loadAsset("img/00/halpha3000_aspect_32bit.tif")) );
    Surface32f sSlope( loadImage(loadAsset("img/00/halpha3000_slope1.tif")) );
    
    {
        // make point from intensity
        Surface32f sIntensity( loadImage(loadAsset("img/00/halpha3000-skv3264879915580.tiff")) );
        intensityW = sIntensity.getWidth();
        intensityH = sIntensity.getHeight();
        
        Surface32f::Iter itr = sIntensity.getIter();
        float threashold = 0.15;
        float extrusion = 300;
        while ( itr.line() ) {
            while( itr.pixel() ){
                float gray = itr.r();
                if( threashold < gray ){
                    Vec2i pos = itr.getPos();
                    Vec3f v(pos.x, pos.y, gray*extrusion );
                    Vec3f noise = mPln.dfBm( Vec3f(pos.x*0.1, pos.y*0.1, gray*0.5) );
                    ps.push_back( v + noise );
                    float c = gray + 0.2f;
                    float a = lmap(c, 0.0f, 1.0f, 0.3f, 0.7f);
                    cs.push_back( ColorAf(c, c, c, a) );
                    
                    float * asp = sAspect.getDataRed( pos );
                    float * slp = sSlope.getDataRed( pos );
                    mSlope.push_back(*slp);
                    mAspect.push_back(*asp);
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
    
    gl::VboMesh::VertexIter vitr( mPoints );
    for(int i=0; i<mPoints.getNumVertices(); i++ ){
        float slope = mSlope[i];
        float aspect = mAspect[i];
        
        if( slope!=0 && aspect!=-9999 ){
            Vec2f vel( 0, slope*10.0 );
            vel.rotate( toRadians(aspect) );
            Vec3f &pos = ps[i];
            pos.set(pos.x + vel.x, pos.y+vel.y, pos.z );

            Vec3f noise = mPln.dfBm( pos.x, pos.y, pos.z );
            pos += noise;
            
            vitr.setPosition( pos );
            vitr.setColorRGBA( cs[i] );
            ++vitr;
        }
    }
}

void cApp::draw(){
    
    mExp.begin( camUi.getCamera() );{
        gl::clear( ColorA(0,0,0,1) );
        gl::enableAlphaBlending();
        gl::enableDepthRead();
        gl::enableDepthWrite();
        
        if( !mExp.bSnap && !mExp.bRender ){
            // Guide
            uf::drawCoordinate( 100 );
        }
        
        glPointSize( 1 );
        gl::translate( -intensityW/2, -intensityH/2 );
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
