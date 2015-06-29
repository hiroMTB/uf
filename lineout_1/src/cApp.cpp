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

class Grain{
    
public:
    Vec3f pos;
    Vec3f initPos;
    Vec3f speed;
    ColorAf col;
    Vec2i imgIndex;
    
};

class cApp : public AppNative {
    
public:
    void setup();
    void update();
    void draw();
    void draw_spline();
    void draw_prep();
    
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void keyDown( KeyEvent event );
    void resize();
    void loadSimulationData( string fileName );
    
    int intensityW;
    int intensityH;
    bool bStart = false;
    bool bOrtho;
    
    MayaCamUI camUi;
    CameraOrtho ortho;
    Perlin mPln;
    gl::VboMesh mPoints;
    
    vector< vector<Vec2f> > mVecMap;
    vector<Grain> mGrain;

    Exporter mExp;
};

void cApp::setup(){
    
    setWindowPos( 0, 0 );
    setWindowSize( 1080*3*0.7, 1920*0.7 );
    mExp.setup( 1080*3, 1920, 3000, GL_RGB, uf::getRenderPath(), 0);
    
    CameraPersp persp(1080*3, 1920, 54.4f, 1, 100000 );
    persp.lookAt( Vec3f(0,0, -2000), Vec3f(0,0,0), Vec3f(0,1,0) );
    persp.setCenterOfInterestPoint( Vec3f(0,0,0) );
    camUi.setCurrentCam( persp );

    ortho = CameraOrtho(0, 1080*3, 1920, 0, -10000, 10000);
    
    mPln.setSeed(123);
    mPln.setOctaves(4);
    
    
    {
        // make VectorMap
        Surface32f sAspect( loadImage(loadAsset("img/07/hamosaic_aspect_32bit.tif")) );
        Surface32f sSlope( loadImage(loadAsset("img/07/hamosaic_slope1.tif")) );

        int w = sAspect.getWidth();
        int h = sAspect.getHeight();
        
        mVecMap.assign(w, vector<Vec2f>(h) );

        for( int i=0; i<sAspect.getWidth(); i++) {
            for( int j=0; j<sAspect.getHeight(); j++ ) {
                
                Vec2i pos(i, j);
                float aspect = *sAspect.getDataRed( pos );
                float slope = *sSlope.getDataRed( pos );
                if( slope!=0 && aspect!=-9999 ){

                    Vec2f vel( 0, slope );
                    vel.rotate( toRadians(aspect) );
                    mVecMap[i][j] = vel * 0.0005;
                }else{
                    mVecMap[i][j] = Vec2f::zero();
                }
            }
        }
    }
    
    {
        // make point from intensity
        Surface32f sIntensity( loadImage(loadAsset("img/07/hamosaic.gif")) );
        intensityW = sIntensity.getWidth();
        intensityH = sIntensity.getHeight();
        
        Surface32f::Iter itr = sIntensity.getIter();
        float threashold = 0.1;
        float extrusion = 0;
        while ( itr.line() ) {
            while( itr.pixel() ){
                float gray = itr.r();
                if( threashold < gray ){
                    const Vec2i & pos = itr.getPos();
                    Vec3f v(pos.x, pos.y, gray*gray*gray*extrusion );
                  
                    if( pos.x < intensityW/4.0){
                        
                        v.x +=  intensityW* 3.0/8.0;
                        v.z -= 1080/2;
                        
                    }else if( pos.x < intensityW/4.0*2.0){

                        v.x += intensityW * 1.0/8.0;
                        v.z -= 1080/2;

                    }else if( pos.x < intensityW/4.0*3.0 ){
                        
                        v.x -= intensityW * 1.0/8.0;
                        v.z += 1080/2;
                        
                    }else{
                        v.x -= intensityW * 3.0/8.0;
                        v.z += 1080/2;
                    }
        
                    v.x -= intensityW/2;
                    v.rotateY( toRadians(90.0f) );

                    float c = gray + 0.05f;
                    float a = lmap( c, 0.0f, 1.0f, 0.3f, 0.7f);

                    Grain g;
                    g.pos = v;
                    g.initPos = v;
                    g.col = ColorAf(c, c, c, a);
                    g.imgIndex = pos;
                    mGrain.push_back( g );

                }
            }
        }
        
        
        mPoints = gl::VboMesh( mGrain.size(), 0, uf::getVboLayout(), GL_POINTS );
        gl::VboMesh::VertexIter vitr( mPoints );
        for(int i=0; i<mGrain.size(); i++ ){
            vitr.setPosition( mGrain[i].pos );
            vitr.setColorRGBA( mGrain[i].col );

            ++vitr;
        }
    }
    
    
}

void cApp::update(){
    if( !bStart )
        return;
    
    gl::VboMesh::VertexIter vitr( mPoints );
    for(int i=0; i<mGrain.size(); i++ ){
        
        const Vec2i & imgIndex = mGrain[i].imgIndex;
        Vec3f vel( mVecMap[imgIndex.x][imgIndex.y].x, mVecMap[imgIndex.x][imgIndex.y].y, 0);
        float len = vel.length();
        len *= len * 0.5;
        
        Vec3f & pos = mGrain[i].pos;
        
        if( 0<=imgIndex.x && imgIndex.x<intensityW/4){
            pos.x += -len;
        }else if( intensityW/4<=imgIndex.x && imgIndex.x<intensityW/4*2 ){
            pos.x += +len;
        }else if( intensityW/4*2<=imgIndex.x && imgIndex.x<intensityW/4*3 ){
            pos.x += -len;
        }else if( intensityW/4*3<=imgIndex.x && imgIndex.x<intensityW/4*4 ){
            pos.x += +len;
        }
        
        vitr.setPosition( pos );
        vitr.setColorRGBA( mGrain[i].col );
        ++vitr;
    }
}

void cApp::draw(){
    {
        if( bOrtho ){
            mExp.begin( ortho );
            gl::enableDepthRead();
            gl::enableDepthWrite();
            gl::translate( mExp.mFbo.getWidth()/2, 0, 0);
            
        }else{
            mExp.begin( camUi.getCamera() );
            gl::enableDepthRead();
            gl::enableDepthWrite();
        }
     
        gl::clear( ColorA(0,0,0,1) );
        gl::enableAlphaBlending();
        
        if( !mExp.bSnap && !mExp.bRender ){
            // Guide
            glLineWidth( 4 );
            uf::drawCoordinate( 300 );
        }
        
        float s = 1920.0f/intensityH;
        glPushMatrix();
        gl::scale(1, s, s);

        glLineWidth( 1 );
        draw_prep();
        draw_spline();
        
        glPointSize( 2 );
        gl::draw( mPoints );
        glPopMatrix();

        glPushMatrix();
        glLineWidth( 1 );
        gl::translate( -mExp.mFbo.getWidth()/2, 0, 0);
        gl::color( ColorAf(1,0,0,0.7) );
        glBegin(GL_LINES);
        glVertex3f(1080, 0, 0);
        glVertex3f(1080, 1920, 0);
        glVertex3f(1080*2, 0, 0);
        glVertex3f(1080*2, 1920, 0);
        glEnd();
        glPopMatrix();
    }mExp.end();
 
    gl::clear( ColorA(1,1,1,1) );
    gl::color( Color::white() );
    mExp.draw();
    
}

void cApp::draw_spline(){
    if( !bStart ) return;
    
    int numSpline = 500000;
    
    vector<BSpline3f> bsps;
    vector<ColorAf> cs;
    
    for( int i=0; i<numSpline; i++){
        
        int gid = randInt(0, mGrain.size() );
        const Vec3f & start = mGrain[gid].pos;
        const Vec3f & end = mGrain[gid].initPos;
        const Vec3f d = end - start;
        float len = d.length();
        if( 30 < len ){

            int multi = randInt(3,30);
            for( int m=0; m<multi; m++ ){
            
                vector<Vec3f> ctrls;

                ColorAf col = mGrain[gid].col;
                col.a *= 1.0f/multi * 1.5;
                col.a = lmap( col.a, 0.0f, 1.0f, 0.1f, 0.8f );
                
                cs.push_back( col );
                
                ctrls.push_back( start );
                
                int numCtrl = randInt(3, 8);
                
                float startRate = randFloat(0.01, 0.25);
                for( int j=0; j<numCtrl-1; j++ ){
                    float rate = startRate + (float)j/numCtrl * (1.0f - startRate);
                    
                    Vec3f ctrl = start + d*rate + Vec3f( 0,randFloat(-1,1)*40.0*j, 0);
                    ctrls.push_back( ctrl );
                }
                
                ctrls.push_back( end + Vec3f(0,randFloat(-1,1)*len/1.618f,0) );
                
                BSpline3f bsp( ctrls, 3, false, true );
                bsps.push_back( bsp );
            }
        }
    }
    
    int resolution = 100;

    
    for( int i=0; i<bsps.size(); i++ ){

        gl::color( cs[i] );

        glBegin(GL_LINES);
        for( int j=0; j<resolution-1; j++ ){
            float rate1 = (float)j/resolution;
            float rate2 = (float)(j+1)/resolution;

            Vec3f v1 = bsps[i].getPosition( rate1 );
            Vec3f v2 = bsps[i].getPosition( rate2 );
            
            glVertex3f( v1.x, v1.y, v1.z );
            glVertex3f( v2.x, v2.y, v2.z );
        }
        glEnd();
    }

}

void cApp::draw_prep(){
    
    gl::color( 0.5, 0.5, 0.5, 0.8 );
    
    for( int i=0; i<1000; i++ ){

        int gid = randInt(0, mGrain.size() );
        const Vec3f & v1 = mGrain[gid].pos;
        const Vec3f & v2 = mGrain[gid].initPos;
        
        glBegin( GL_LINES );
        glVertex3f( v1.x, v1.y, v1.z );
        glVertex3f( v2.x, v2.y, v2.z );
        glEnd();
    }
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

        case 'o':
            bOrtho = !bOrtho;
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
