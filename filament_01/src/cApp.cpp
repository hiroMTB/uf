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

#include "mtUtil.h"
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
    Surface32f colorMap;
    
    vector< vector<Vec2f> > mVecMap;
    vector<Vec3f> mVelocity;
    vector<Vec3f> ps;
    vector<ColorAf> cs;
    
    vector< BSpline3f > bspls;
};

void cApp::setup(){
    
    setWindowPos( 0, 0 );
    setWindowSize( 1080*3*0.5, 1920*0.5 );
    mExp.setup( 1080*3, 1920, 3000, GL_RGB, mt::getRenderPath(), 0);
    
    CameraPersp cam( 1080*3, 1920, 25/*39.6f*/, 1, 100000 );
    cam.lookAt( Vec3f(0,0, 3300), Vec3f(0,0,0), Vec3f(1,0,0) );
    cam.setCenterOfInterestPoint( Vec3f(0,0,0) );
    cam.setLensShift( 0, 1 );
    
    camUi.setCurrentCam( cam );
    
    mPln.setSeed(123);
    mPln.setOctaves(4);

    fs::path assetPath = mt::getAssetPath();
    colorMap = Surface32f( loadImage(assetPath/"img/Vela_colorMap_x2.tif") ); // 1920*900
    
    {
        // make point from intensity
        Surface32f sIntensity( loadImage( assetPath/"img/04/THY_Bfort 031_blk_x3.png" ) );
        intensityW = sIntensity.getWidth();
        intensityH = sIntensity.getHeight();
        
        Surface32f::Iter itr = sIntensity.getIter();
        float threashold = 0.4;
        float extrusion = 1000;
        
        while ( itr.line() ) {
            while( itr.pixel() ){
                float gray = itr.r();
                
                if( threashold < gray ){
                    Vec2i pos = itr.getPos();
                    Vec3f v( pos.x, pos.y, gray*extrusion );
                    Vec3f noise = mPln.dfBm( Vec3f(pos.x, pos.y, gray) ) * 5.0;
                    ps.push_back( v + noise );

                    pos *= 1.9f;
                    pos.x += 1000;
                    
                    noise *= 0.15;
                    float r = *colorMap.getDataRed( pos ) + noise.x*0.5;
                    float g = *colorMap.getDataGreen( pos )+ noise.y*0.5;
                    float b = *colorMap.getDataBlue( pos ) + noise.z*0.5;
                    float a = lmap(gray, 0.0f, 1.0f, 0.01f, 0.5f);
                    cs.push_back( ColorAf(r, g, b, a) );
                }
            }
        }
        
        mPoints = gl::VboMesh( ps.size(), 0, mt::getVboLayout(), GL_POINTS );
        gl::VboMesh::VertexIter vitr( mPoints );
        for(int i=0; i<ps.size(); i++ ){
            vitr.setPosition( ps[i] );
            vitr.setColorRGBA( cs[i] );
            ++vitr;
        }
    }

    
    {
        // Spline
        int num = 10000;
        Ray center( Vec3f(intensityW/2, intensityH/2, 0), Vec3f(0,1,0) );

        Vec3f center_top( intensityW/2, 0, 0);
        Vec3f center_bottom( intensityW/2, intensityH, 0);
        
        for( int i=0; i<num; i++ ){
            
            int id1 = randInt( 0, ps.size() );
            Vec3f start = ps[id1];
            
            for( int m=0; m<30000; m++ ){
                
                int id2 = randInt( 0, ps.size() );
                Vec3f end = ps[id2];
                
                float dist = start.distance( end );
                float min = 10;
                float max = 30;
                float radius = 3.0;
                int numCtrl = 5;
                
                if( abs(randFloat())>0.99993f ){
                    min = 100;
                    max = 200;
                    radius += randFloat(400, 500);
                }
                    
                if( min<dist && dist<max ){
                    Vec3f dir = end - start;
                    Vec3f mid = start + dir * 0.5;

                    Vec3f prep = -mt::dirToLine( mid, center_top, center_bottom );
                    prep.normalize();
                    
                    vector< Vec3f > ctrls;
                    ctrls.push_back( start );
                    
                    float rate = 0.0f;
                    for( int j=1; j<numCtrl-1; j++ ){
                        rate = (float)j/numCtrl;
                        Vec3f cp = start + dir*rate + prep*radius;
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
    if( !bStart )
        return;
    
}

void cApp::draw(){
    
    mExp.begin( camUi.getCamera() );{
        gl::clear( ColorA(0,0,0,1) );
        gl::enableAlphaBlending();
        gl::enableDepthRead();
        gl::enableDepthWrite();
        
        if( !mExp.bSnap && !mExp.bRender ){
            // Guide
            mt::drawCoordinate( 100 );
        }
        
        gl::translate( -intensityW/2, -intensityH/2 );
        
        
        for( int i=0; i<bspls.size(); i++ ){
            
            int colorIndex = i*33 % cs.size();
            ColorAf & c = cs[colorIndex];
            gl::color( c.r, c.g, c.b, c.a*0.9 );
            
            float resolution = bspls[i].getNumControlPoints() * 10.0f;

            glBegin(GL_LINES);
            for( int j=0; j<resolution-1; j++ ){
                float rate1 = (float)j/resolution;
                float rate2 = (float)(j+1)/resolution;
                Vec3f v1 = bspls[i].getPosition( rate1 );
                Vec3f v2 = bspls[i].getPosition( rate2 );
                glVertex3f( v1.x, v1.y, v1.z );
                glVertex3f( v2.x, v2.y, v2.z );
            }
            glEnd();
            
            
//            glColor4f( 0.9, 0.5, 0.01, 0.8 );
//            glPointSize( 1 );
//            glBegin( GL_POINTS );
//            for( int j=0; j<bspls[i].getNumControlPoints(); j++ ){
//                Vec3f v1 = bspls[i].getControlPoint( j );
//                glVertex3f( v1.x, v1.y, v1.z );
//            }
//            glEnd();

            glColor4f( 0.3, 0.3, 0.9, 0.9 );
            glPointSize( 3 );
            glBegin( GL_POINTS );
            Vec3f st = bspls[i].getControlPoint( 0 );
            glVertex3f( st.x, st.y, st.z );
            glEnd();
        }
        
        
        glPointSize( 1 );
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
