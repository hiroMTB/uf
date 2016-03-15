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
    void prepareSettings( Settings * s);
    void setup();
    void update();
    void draw();
    void oscCallback( const osc::Message * mes );
    void keyDown( KeyEvent key );
    void shutdown();
    
    int  frame      = 0;
    int  vFrameRate = 25;
    float seconds   = 0;
    
    bool bNeedSeek = false;
    qtime::MovieSurfaceRef	mov;
    Surface	sur;
    osc::Listener listener;

//#define CS_1
#ifdef CS_1
    string windowName = "cs1";
    const fs::path path = "4444/cs_1.mov";
    const int port = 12345;
#else
    string windowName = "cs2";
    const fs::path path = "4444/cs_2.mov";
    const int port = 12345;
#endif
    
};

void cApp::prepareSettings( Settings * s ){

    s->setWindowPos(300,0);
    s->setWindowSize(1920, 1080);
    s->setFullScreen(false);
    s->setTitle(windowName);
    s->setFrameRate(60);
    //s->setAlwaysOnTop();

    gl::enableVerticalSync();
}

void cApp::setup(){
    
    // Mov
    cout << "Quicktime Version : " << qtime::getQuickTimeVersionString() << endl;
    mov = qtime::MovieSurface::create( loadAsset(path) );
    while( mov->checkPlayable() == false ){
        cinder::sleep(10);
    }
    mov->setVolume(0);
    //mov->seekToStart();
    //mov->play();
    //mov->stop();

    // Osc
    listener.setup(port);
    listener.registerMessageReceived( this, &cApp::oscCallback );
}

void cApp::oscCallback(const osc::Message * mes){
    if(!mov) return;
    
    seconds = mes->getArgAsFloat(0);
    mov->seekToTime(seconds);

    int newFrame = round(seconds * (float)vFrameRate);

    frame = newFrame;
}

void cApp::update(){
    if(mov){        
        sur = mov->getSurface();
    }
}

void cApp::draw(){
    
    mt::setMatricesWindow(1920, 1080, false);
    gl::clear(ColorA(1,0,0));
    gl::color(1, 1, 1);
    if(sur) gl::draw( sur );
    
    if(!getWindow()->isFullScreen()){
        int x = 20;
        int y = 20;
        int yp = 20;
        gl::color(1, 1, 1);
        gl::drawString( "SLAVE", Vec2i(x,y) ); y+=yp;
        gl::drawString( "osc port : " + to_string(port), Vec2i(x,y) ); y+=yp;
        gl::drawString( "osc frame    : " + to_string(frame), Vec2i(x,y) ); y+=yp;
        gl::drawString( "osc seconds  : " + to_string(seconds), Vec2i(x,y) ); y+=yp*2;

        gl::drawString( "mov frame    : " + to_string((int)mov->getCurrentTime()/25.0), Vec2i(x,y) ); y+=yp;
        gl::drawString( "mov seconds  : " + to_string(mov->getCurrentTime()), Vec2i(x,y) ); y+=yp;
        gl::drawString( "fps          : " + to_string(getAverageFps()), Vec2i(x,y) ); y+=yp;
        gl::drawString( "file         : " + path.string(), Vec2i(x,y) ); y+=yp;
    }
}

void cApp::keyDown( KeyEvent key){

    int k = key.getCode();
    switch (k) {
        case KeyEvent::KEY_ESCAPE:
        {
            bool f = getWindow()->isFullScreen();
            setFullScreen(!f);
            f ? hideCursor() : showCursor();
            break;
        }
    }
    
    char c = key.getChar();
    switch (c) {
        case 'q':
            quit();
            break;
    }
}

void cApp::shutdown(){
    listener.shutdown();
    mov->stop();
    mov->reset();
    sur.reset();
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
