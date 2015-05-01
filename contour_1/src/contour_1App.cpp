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
#include "Exporter.h"

#include "cinder/params/Params.h"

#include "MyCamera.h"

//#define RENDER

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
public:
    void setup();
    void keyDown( KeyEvent event );
    void update();
    void draw();
    void drawContour();
  	void resize();
    void addContour( double t );
    
    int mWin_w = 1920;
    int mWin_h = 1080;

    Exporter mExp;
    
    gl::Texture mTex;
    
    cv::Mat input;
    cv::Mat gray;
    cv::Mat thresh;
    typedef vector<cv::Point> Contour;
    typedef vector<Contour> ContourVector;
    typedef vector<ContourVector> ContourContainer;
    ContourContainer mCtc;

    params::InterfaceGlRef mParams;
    Quatf mObjOrientation;
    
    MyCamera mCam;
};

void cApp::setup(){
    
    setWindowPos(0, 0);
    setWindowSize(mWin_w, mWin_h);
    mExp.setup( mWin_w, mWin_h, 100, GL_RGBA16F_ARB, uf::getRenderPath(), 0);
    
    // load image
    ci::Surface32f sur( loadImage((loadAsset("vela_scana_spire250_signal.tiff"))) );
    mTex = gl::Texture( sur );
    
    // make contour
    input = toOcv( sur );
    
    // convert to mono image
    cv::cvtColor( input, gray, CV_RGB2GRAY );

    // add blur
    cv::blur( gray, gray, cv::Size( 2, 2 ));

    addContour(0.09);
    addContour(0.1);
    addContour(0.2);
    addContour(0.3);
    addContour(0.4);
    addContour(0.5);

    
    // Camera
    mCam.setFov( 54 );
   	mCam.lookAt( Vec3f( 0, 0, 7000 ), Vec3f::zero() );
    mCam.setNearClip(1);
    mCam.setFarClip(100000);
    mCam.setup();
    
    // Interface
    mParams = params::InterfaceGl::create("params", toPixels( Vec2i(250, 250) ) );
    mParams->setOptions ( "", "position=`210 10` valueswidth=150" );
    mParams->addParam("rotation", &mObjOrientation);
    
#ifdef RENDER
    mExp.startRender();
#endif
}

void cApp::addContour( double t ){
    ContourVector vec;
    vector<cv::Point> points;
    cv::threshold( gray, thresh, t, 1.0, CV_THRESH_BINARY );
    thresh.convertTo(thresh, CV_32SC1);
    
    if( thresh.type() == CV_32SC1 ){
        cv::findContours( thresh, vec, CV_RETR_CCOMP, CV_CHAIN_APPROX_NONE, cv::Point(0, 0));
        mCtc.push_back( vec );
    }else{
        cout << "error : wrong image format " << endl;
    }
}

void cApp::update(){
}

void cApp::draw(){
    gl::clear();
    gl::enableAlphaBlending();
    mExp.begin(); {
        gl::clear();
        gl::setMatrices( mCam );
        gl::pushMatrices();
        gl::rotate( mObjOrientation );

        uf::drawCoordinate();
        drawContour();
        gl::popMatrices();
    } mExp.end();
    
    gl::color( Colorf::white() );
    mExp.draw();
    
    
    mParams->draw();
    mCam.drawParam();
}

void cApp::drawContour(){
    
    // retrieve contour set
    for( int i=0; i<mCtc.size(); i++ ){
        ContourVector & cv = mCtc[i];
        gl::pushMatrices();
        gl::translate( Vec3f(-4000.0, 0, 0) );
        
        bool scanFinish = false;
        Vec2f lastPoint(0,0);
        int nVertex = 0;
        float scanSpeed = 20;

        glColor4f(1, i*0.05, 0, 0.5+i*0.1);

        // retrieve 1 contour line
        for( int j=0; j<cv.size(); j++ ){
            //glBegin( GL_LINE_LOOP );
            glBegin( GL_POINTS );
            glPointSize(1);
            glLineWidth(1);
            Contour & ct = cv[j];

            // retrieve each point
            for( int k=0; k<ct.size(); k++ ) {
                cv::Point & p = ct[k];
                if(++nVertex < getElapsedFrames()*scanSpeed ){
                    gl::vertex( fromOcv( p ) );
                    lastPoint = fromOcv( p );
                    scanFinish = (j==cv.size()-1) &&( k==ct.size()-1);
                }
            }
            glEnd();
        }

        // scanline
        if( !scanFinish ){
            glLineWidth(1);
            glBegin(GL_LINES);
            gl::vertex(lastPoint);

            lastPoint.y = -10000;
            gl::vertex( lastPoint );
            
            //gl::vertex(Vec3f(lastPoint.x, lastPoint.y, 5000));
            glEnd();
        }
        gl::popMatrices();

    }
}

void cApp::keyDown( KeyEvent event ) {
    char key = event.getChar();
    switch (key) {
        case 'S':
            mExp.startRender();
            break;
        case 'T':
            mExp.stopRender();
            break;
            
    }
}

void cApp::resize(){
    mCam.setAspectRatio( getWindowAspectRatio() );
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
