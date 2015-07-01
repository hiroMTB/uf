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
    
    vector< BSpline3f > bspls;
};


void cApp::setup(){
    
    setWindowPos( 0, 0 );
    setWindowSize( 1080*3*0.5, 1920*0.5 );
    mExp.setup( 1080*3, 1920, 3000, GL_RGB, uf::getRenderPath(), 0);
    
    CameraPersp cam(1080*3, 1920, 54.4f, 0.1, 10000 );
    cam.lookAt( Vec3f(0,0, 800), Vec3f(0,0,0) );
    cam.setCenterOfInterestPoint( Vec3f(0,0,0) );
    camUi.setCurrentCam( cam );
    
    mPln.setSeed(123);
    mPln.setOctaves(4);
    
//    {
//        // make VectorMap
//        Surface32f sAspect( loadImage(loadAsset("img/03/RCW36_conbine_log_aspect.tif")) );
//        Surface32f sSlope( loadImage(loadAsset("img/03/RCW36_conbine_log_slope2.tif")) );
//
//        int w = sAspect.getWidth();
//        int h = sAspect.getHeight();
//        
//        mVecMap.assign(w, vector<Vec2f>(h) );
//
//        for( int i=0; i<sAspect.getWidth(); i++) {
//            for( int j=0; j<sAspect.getHeight(); j++ ) {
//                
//                Vec2i pos(i, j);
//                float aspect = *sAspect.getDataRed( pos );
//                float slope = *sSlope.getDataRed( pos );
//                if( slope!=0 && aspect!=-9999 ){
//
//                    Vec2f vel( 0, slope*0.00001 );
//                    vel.rotate( toRadians(aspect) );
//                    mVecMap[i][j] = vel;
//                }else{
//                    mVecMap[i][j] = Vec2f::zero();
//                }
//                
//                mVelocity.push_back( Vec3f(mVecMap[i][j].x, mVecMap[i][j].y, 0) );
//            }
//        }
//    }
    
    {
        // make point from intensity
        Surface32f sIntensity( loadImage(loadAsset("img/05/Prom_gather_erupt_medium 316.png")) );
        intensityW = sIntensity.getWidth();
        intensityH = sIntensity.getHeight();
        
        Surface32f::Iter itr = sIntensity.getIter();
        float threashold = 0.4;
        float extrusion = 100;
        while ( itr.line() ) {
            while( itr.pixel() ){
                float r = itr.r();
                float g = itr.g();
                float b = itr.b();
                
                if( threashold<r && r<0.99 ){
                    Vec2i pos = itr.getPos();
                    Vec3f v(pos.x, pos.y, 0 );
                    
                    Vec3f center( intensityW, 0, 0 );
                    float rad = intensityW*0.75;
                    Vec3f dir = v - center;
                    Vec3f prep = dir;
                    prep.rotateZ( 90.0f );
                    prep.normalize();
                    
                    float angle = dir.length()/rad * 90.0f;
                    v.rotate( prep, toRadians(angle) );

                    //v += Vec3f(-intensityW/2, -intensityH/2, 0);
                    //v.rotateZ( 45.0f );
//                    v.rotateY( toRadians(-2.0f) );
                    
                    //v += Vec3f(intensityW-200, 0, 0);
//                    
//                    Vec3f noise = mPln.dfBm( Vec3f(pos.x, pos.y, r) ) * 1.11;
//                    v += noise;
                    ps.push_back( v  );

                    float a = lmap(r, 0.0f, 1.0f, 0.3f, 0.7f);
                    cs.push_back( ColorAf(r, g, b, a) );
                    
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
    
    {
        // Spline
        int num = 100;
        
        Vec3f center( mExp.mFbo.getWidth(), mExp.mFbo.getHeight()/2-300, -4000);
       
        for( int i=0; i<num; i++ ){
            
            int id1 = randInt( 0, ps.size() );
            Vec3f start = ps[id1];
            
            for( int m=0; m<30; m++ ){
                
                int id2 = randInt( 0, ps.size() );
                Vec3f end = ps[id2];
                
                float dist = start.distance( end );
                if( 3<dist && dist<500+m/3 ){
                    Vec3f dir = end - start;
                    Vec3f mid = start + dir * 0.5;
                    
                    Vec3f prep = mid - center;
                    prep.normalize();
                    
                    
                    vector< Vec3f > ctrls;
                    ctrls.push_back( start );
                    int numCtrl = randInt( 5, 9 );
                    float rate = 0.0;
                    
                    for( int j=1; j<numCtrl-1; j++ ){
                        rate = (float)j/numCtrl;
                        Vec3f cp = start + dir*rate + prep*randFloat(0.001, 1.0)*300.0f;
                        ctrls.push_back( cp );
                    }
                    
                    ctrls.push_back( end );
                    BSpline3f b( ctrls, 3, false, true );
                    bspls.push_back( b );
                }
            }
        }
    }
    
}

void cApp::update(){

}

void cApp::draw(){
    
    mExp.begin( camUi.getCamera() );{
        gl::clear( ColorA(0,0,0,1) );
        gl::enableAlphaBlending();
        gl::enableDepthRead();
        gl::enableDepthWrite();
        
        glPointSize( 1 );
//        gl::translate( -intensityW/2, -intensityH/2, );
        
        if( !mExp.bSnap && !mExp.bRender ){
            uf::drawCoordinate( 100 );
        }
        
        
//        float resolution = 20.0f;
//        for( int i=0; i<bspls.size(); i++ ){
//            
//            int colorIndex = i*33 % cs.size();
//            ColorAf & c = cs[colorIndex];
//            gl::color( c.r, c.g, c.b, c.a*0.9 );
//            
//            glBegin(GL_LINES);
//            for( int j=0; j<resolution-1; j++ ){
//                float rate1 = (float)j/resolution;
//                float rate2 = (float)(j+1)/resolution;
//                Vec3f v1 = bspls[i].getPosition( rate1 );
//                Vec3f v2 = bspls[i].getPosition( rate2 );
//                glVertex3f( v1.x, v1.y, v1.z );
//                glVertex3f( v2.x, v2.y, v2.z );
//            }
//            glEnd();
        
            
//            glColor4f( 0, 0, 1, 0.6 );
//            glPointSize( 2 );
//            glBegin( GL_POINTS );
//            for( int j=0; j<bspls[i].getNumControlPoints(); j++ ){
//                Vec3f v1 = bspls[i].getControlPoint( j );
//                glVertex3f( v1.x, v1.y, v1.z );
//            }
//            glEnd();
            
//            glColor4f( 0, 0, 1, 1 );
//            glPointSize( 2 );
//            glBegin( GL_POINTS );
//            Vec3f st = bspls[i].getControlPoint( 0 );
//            glVertex3f( st.x, st.y, st.z );
//            glEnd();
//        }
        
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
