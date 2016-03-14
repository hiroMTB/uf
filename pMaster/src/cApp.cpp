#include "cinder/app/AppBasic.h"
#include "cinder/Surface.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/System.h"
#include "OscSender.h"

#include "cinder/audio/GenNode.h"
#include "cinder/audio/GainNode.h"
#include "cinder/audio/ChannelRouterNode.h"
#include "cinder/audio/MonitorNode.h"
#include "cinder/audio/SamplePlayerNode.h"

#include "mtUtil.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
public:
    void setup();
    void draw();
    void sendOsc();
    void keyDown( KeyEvent key );
    void setupMultichannelDevice();
    void setupDefaultChannelDevice();
    void before_quit();
    
    bool bStart = true;
    int stopFrame = 0;
    
    audio::FilePlayerNodeRef player;
    double seconds;
    
    osc::Sender sender1, sender2;
    std::string host1 = "localhost";
    std::string host2 = "10.0.0.2";
    int 		port1 = 12345;
    int         port2 = 12345;
    fs::path    path = "cs.wav";
  
    std::thread oscThread;
};

void cApp::setupDefaultChannelDevice(){
    auto ctx = audio::Context::master();
    audio::DeviceRef device = audio::Device::getDefaultOutput();
    //ctx->setOutput( device );
}

void cApp::setupMultichannelDevice(){
    console() << audio::Device::printDevicesToString();
    
    string rme = "Fireface UCX (23507154)";
    string def = "Built-in Output";
    audio::DeviceRef device = audio::Device::findDeviceByName( rme );
    
    console() << endl << "max output channels: " << device->getNumOutputChannels() << endl;
    getWindow()->setTitle( device->getName() );
    
    auto ctx = audio::master();
    audio::OutputDeviceNodeRef multichannelOutputDeviceNode = ctx->createOutputDeviceNode( device, audio::Node::Format().channels( device->getNumOutputChannels() ) );
    ctx->setOutput( multichannelOutputDeviceNode );
}


void cApp::setup(){
    
    setFrameRate(120);
    setWindowPos(0, 0);
    setWindowSize(300, 300);
    
    setupMultichannelDevice();

    auto ctx = audio::master();
    
    DataSourceRef data = loadAsset(path);
    audio::SourceFileRef src = audio::load( data , ctx->getSampleRate() );
    player = ctx->makeNode( new audio::FilePlayerNode(src) );
    player >> ctx->getOutput();
    
    ctx->enable();
    
    player->setLoopEnabled();
    player->start();

    sender1.setup( host1, port1);
    sender2.setup( host2, port2);
    
    oscThread = std::thread( &cApp::sendOsc, this );

}

void cApp::sendOsc(){

    int oscPerSec = 100;
    int interval_ms = 1000.0/oscPerSec;
    
    while (1) {
        seconds = player->getReadPositionTime();
        cinder::osc::Message mes;
        mes.addFloatArg( seconds );
        sender1.sendMessage( mes );
        sender2.sendMessage( mes );
        std::this_thread::sleep_for(chrono::milliseconds(interval_ms));
    }
}

void cApp::before_quit(){
    oscThread.join();

}

void cApp::draw(){
    
    gl::clear(Colorf(0.6,0.6,0.6));
    gl::color(1, 1, 1);
    
    int x = 20;
    int y = 20;
    int yp = 20;
    gl::drawString( "MASTER", Vec2i(x,y) ); y+=yp;
    gl::drawString( "osc1 host : " + host1, Vec2i(x,y) ); y+=yp;
    gl::drawString( "osc1 port : " + to_string(port1), Vec2i(x,y) ); y+=yp;
    gl::drawString( "osc2 host : " + host2, Vec2i(x,y) ); y+=yp;
    gl::drawString( "osc2 port : " + to_string(port2), Vec2i(x,y) ); y+=yp;
    gl::drawString( "frame     : " + to_string(seconds*25.0f), Vec2i(x,y) ); y+=yp;
    gl::drawString( "seconds   : " + to_string(seconds), Vec2i(x,y) ); y+=yp;
    gl::drawString( "fps       : " + to_string(getAverageFps()), Vec2i(x,y) ); y+=yp;
    gl::drawString( "file      : " + path.string(), Vec2i(x,y) ); y+=yp;
}

void cApp::keyDown( KeyEvent key ){

    char c = key.getChar();
    switch (c) {
        case '0': player->seek(0); break;
        case '1': player->seekToTime(60); break;
        case '2': player->seekToTime(60*2); break;
        case '3': player->seekToTime(60*3); break;
        case '4': player->seekToTime(60*4); break;
        case '5': player->seekToTime(60*5); break;
        case '6': player->seekToTime(60*6); break;
        case '7': player->seekToTime(60*7); break;
        case 'h': player->seekToTime( player->getReadPosition()+30); break;
        case 'q':
            before_quit();
            quit();
            break;
            
        case ' ':
            if(bStart){
                stopFrame = player->getReadPosition();
                player->stop();
                player->seek(stopFrame);
            }else{
                player->start();
                player->seek(stopFrame);
            }
            bStart = !bStart;
            break;
    }
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
