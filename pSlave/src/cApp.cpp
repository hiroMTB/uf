#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/System.h"
#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/qtime/QuickTime.h"
#include "cinder/Text.h"
#include "cinder/Utilities.h"
#include "cinder/ImageIo.h"
#include "OscListener.h"

#include "mtUtil.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
public:
    void setup();
    void update();
    void draw();
    void oscCallback( const osc::Message * mes );
    void keyDown( KeyEvent key );

    bool bFull = false;
    int frame = 0;
    float seconds;
    
    osc::Listener listener;
    int vFrameRate = 25;
    
    qtime::MovieSurfaceRef	mov;
    Surface	sur;
    
//#define CS_1
#ifdef CS_1
    const int port = 12345;
    const fs::path path = "4444/cs_1.mov";
    string windowName = "cs1";
#else
    const int port = 12345;
    const fs::path path = "4444/cs_2.mov";
    string windowName = "cs2";
#endif
    
};


void cApp::setup(){
    
    cout << qtime::getQuickTimeVersionString() << endl;
    
//    setFrameRate(25)
    gl::enableVerticalSync();
    setWindowPos(0, 0);
    setWindowSize(1920, 1080);
    setFullScreen(bFull);
    
    getWindow()->setTitle(windowName);
    listener.setup(port);
    listener.registerMessageReceived( this, &cApp::oscCallback );

    mov = qtime::MovieSurface::create( loadAsset(path) );

    while( mov->checkPlayable() == false ){
        cinder::sleep(10);
    }
    
    mov->setVolume(0);
    //mov->seekToStart();
    //mov->play();
    //mov->stop();
}

void cApp::oscCallback(const osc::Message * mes){
    if(!mov) return;
    
    seconds = mes->getArgAsFloat(0);
    int newFrame = round(seconds * (float)vFrameRate);
    long diff = newFrame - frame;
    
    if( abs(diff) > 25){
        mov->seekToTime(seconds);
        frame = seconds*25.0f;
        cout << "seekToTime" << endl;
    }else if( diff>0){
        for( int i=0; i<diff; i++){
            mov->stepForward();
        }
        frame = newFrame;
    }else if (diff<0){
        for( int i=0; i<diff; i++){
            mov->stepBackward();
        }
        frame = newFrame;
    }

//    if( abs(diff) > 25){
//        mov->seekToTime(seconds);
//        frame = seconds*25.0f;
//    }else if( diff>0){
//        mov->stepCount(diff);
//        frame = newFrame;
//    }

}

void cApp::update(){
    if(mov){
        if(mov->checkNewFrame()){
            sur = mov->getSurface();
        }
    }
}

void cApp::draw(){
    
    mt::setMatricesWindow(1920, 1080, false);
    gl::clear(ColorA(1,0,0));
    gl::color(1, 1, 1);
    if(mov)gl::draw( sur );
    
    if(!bFull){
        int y = 10;
        int yp = 20;
        gl::color(1, 1, 1);
        gl::drawString( "SLAVE", Vec2i(10,y) ); y+=yp;
        gl::drawString( "osc port : " + to_string(port), Vec2i(10,y) ); y+=yp;
        gl::drawString( "frame    : " + to_string(frame), Vec2i(10,y) ); y+=yp;
        gl::drawString( "seconds  : " + to_string(seconds), Vec2i(10,y) ); y+=yp;
        gl::drawString( "seconds(mov): " + to_string(mov->getCurrentTime()), Vec2i(10,y) ); y+=yp;
        gl::drawString( "fps      : " + to_string(getAverageFps()), Vec2i(10,y) ); y+=yp;
        gl::drawString( "file     : " + path.string(), Vec2i(10,y) ); y+=yp;
    }
}

void cApp::keyDown( KeyEvent key){

    int k = key.getCode();
    switch (k) {
        case KeyEvent::KEY_ESCAPE:
            bFull = !bFull;
            setFullScreen(bFull);
            bFull ? hideCursor() : showCursor();
            break;
        default:
            break;
    }
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
