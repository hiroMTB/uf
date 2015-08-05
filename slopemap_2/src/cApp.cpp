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
    
    vector< vector<Vec2f> > mVecMap;
    vector<Vec3f> mVelocity;
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
    
    
    {
        // make VectorMap
        Surface32f sAspect( loadImage(loadAsset("img/00/halpha3000_aspect_32bit.tif")) );
        Surface32f sSlope( loadImage(loadAsset("img/00/halpha3000_slope1.tif")) );

        int w = sAspect.getWidth();
        int h = sAspect.getHeight();
        
        mVecMap.assign(w, vector<Vec2f>(h) );

        for( int i=0; i<sAspect.getWidth(); i++) {
            for( int j=0; j<sAspect.getHeight(); j++ ) {
                
                Vec2i pos(i, j);
                float aspect = *sAspect.getDataRed( pos );
                float slope = *sSlope.getDataRed( pos );
                if( slope!=0 && aspect!=-9999 ){

                    Vec2f vel( 0, slope*10.0 );
                    vel.rotate( toRadians(aspect) );
                    mVecMap[i][j] = vel;
                }else{
                    mVecMap[i][j] = Vec2f::zero();
                }
                
                mVelocity.push_back( Vec3f(mVecMap[i][j].x, mVecMap[i][j].y, 0) );
            }
        }
    }
    
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
                    Vec3f noise = mPln.dfBm( Vec3f(pos.x, pos.y, gray) ) * 2.0;
                    ps.push_back( v + noise );
                    float c = gray + 0.2f;
                    float a = lmap(c, 0.0f, 1.0f, 0.3f, 0.7f);
                    cs.push_back( ColorAf(c, c, c, a) );
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
    
    
    mExp.startRender();
    bStart = true;
    
}

void cApp::update(){
    if( !bStart )
        return;
    
    gl::VboMesh::VertexIter vitr( mPoints );
    for(int i=0; i<mPoints.getNumVertices(); i++ ){
        
        Vec3f &pos = ps[i];
        int x = pos.x;
        int y = pos.y;

        x = cinder::math<int>::clamp( x, 0, intensityW-1 );
        y = cinder::math<int>::clamp( y, 0, intensityH-1 );
        
        Vec3f vel( mVecMap[x][y].x, mVecMap[x][y].y, 0);
        Vec3f noise = mPln.dfBm(x, y, cs[i].a ) * 0.2;
        mVelocity[i] = mVelocity[i] + (vel + noise);
        
        vitr.setPosition( pos + mVelocity[i] );
        vitr.setColorRGBA( cs[i] );
        ++vitr;
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
