//#define RENDER_SNAP

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "cinder/audio/Context.h"
#include "cinder/audio/NodeEffects.h"
#include "cinder/audio/SamplePlayerNode.h"
#include "cinder/audio/MonitorNode.h"
#include "cinder/audio/Utilities.h"

#include "FftWaveWriter.h"
#include "ConsoleColor.h"
#include "MonitorNodeNL.h"
#include "mtUtil.h"
#include "SoundWriter.h"
#include "VboSet.h"
#include "Exporter.h"

using namespace ci;
using namespace ci::app;
using namespace std;


class cApp : public AppNative {
  public:
	void setup();
    void update();
    void draw();
    void keyDown( KeyEvent e);
    void makeSpectrumVbo();
    
    vector<VboSet> vbos;
    
    float wW = 1920;
    float wH   = 1620;
    int fftSize = 8192/4;
    float samplingRate = 48000.0f;
    Exporter mExp;
    bool bLog = false;
    float logst = 1;
    vector<vector<float>> fftWave;
};

double myLog( double val, double m){
    return log10( 1+val*m) / log10(1+m) ;
}

void cApp::setup(){
    
    setWindowSize( wW/2, wH/2 );
    setWindowPos( 0, 0 );
 
    mExp.setup( wW, wH, 0, 1, GL_RGB, mt::getRenderPath()/"../", 0);
    
    // Audio Setup
    auto ctx = audio::Context::master();
    audio::DeviceRef device = audio::Device::getDefaultOutput();
    
    fs::path filePath = mt::getAssetPath()/"snd"/"7.1_sim_supernova_idft_48k"/"7.1_idft_vx_log_48k.wav";
    
    FftWaveWriter fww;
    fww.makeFftData( fftWave, ctx, filePath, samplingRate, 0.0f, fftSize );

    for( int i=0; i<fftWave.size(); i++ ){
        vbos.push_back( VboSet() );
    }
    
#ifdef RENDER_SNAP
    mExp.snapShot( mt::getTimeStamp()+".png" );
#endif
    
    makeSpectrumVbo();
}

void cApp::update(){
}

void cApp::makeSpectrumVbo(){
    
    float scale = wW/fftWave[0].size();
    printf("scale : %f\n", scale);
    
    float res = samplingRate/(float)fftSize;
    float x1 = samplingRate/2;
    float y1 = samplingRate/2;
    float x2 = 2.0f;
    float y2 = 2.0f;
    
    //
    // y = a exp bx
    //
    float b = log(y1/y2)/(x1-x2);
    float a = y1/exp(b*x1);
    
    printf("res: %d\n", res);
    cout << "a : " << a << ", b : " << b << endl;
    
    for( int i=0; i<fftWave.size(); i++ ){
        
        VboSet & vbo = vbos[i];
       
        
        float y;
        if( !bLog ){
            y = -(float)(i+1)/fftWave.size() * wH;
            y += wH;
            
        }else{
            float freq = (float)i * res;
            float freqLog = a * exp(b*freq);
            float rate = lmap(freqLog, 2.0f, samplingRate/2.0f, 0.0f, 1.0f);
            y = rate * -wH;
            y += wH;
            
            //printf("i: %d, freq: %f, freqLog: %f, rate: %f\n", i, freq, freqLog, rate);
        }
        
        for( int j=0; j<fftWave[i].size(); j++ ){
            Vec3f p;
            p.x = j*scale;
            p.y = y;
            p.z = 0;
            
            float data = fftWave[i][j];
            
            data = myLog(data, 2);
            
            vbo.addPos( p );
            
            Vec3f hsv( 0.31-data*0.2, 0.8, data );
            Colorf col = hsvToRGB( hsv );;
            vbo.addCol( col );
            
        }

        vbo.init( GL_POINTS);

    }
    printf("setup done\n");

   
}
void cApp::draw(){
    
    mExp.beginOrtho(); {
    
        gl::clear( Color(0,0,0) );
        
        glPushMatrix();

        if( bLog )
            glTranslatef(0,2,0);
        
        glPointSize(1);
        
        for( int i=0; i<vbos.size(); i++ ){
            vbos[i].draw();
        }

        // horizon tal gap
        if( 1 ){
            glPushMatrix();
            glTranslatef(1,0,0);
            for( int i=0; i<vbos.size(); i++ ){
                vbos[i].draw();
            }
            glPopMatrix();
        }
        
        // check vertical gap
        for( int i=0; i<vbos.size(); i++ ){
            if( i< vbos.size()-1 ){
                const Vec3f & pos1 =  vbos[i].getPos()[0];
                const Vec3f & pos2 =  vbos[i+1].getPos()[0];
                float dist = pos1.y - pos2.y;
                
                if(dist > 0.4){
                    
                    float gap = ceil(pos1.y) - ceil(pos1.y);
                    glPushMatrix();
                    glTranslatef(0, -gap, 0);
                    vbos[i].draw();
                    
                    int cnt = pos1.y - gap - ceil(pos2.y);
                    for( int j=0; j<cnt; j++ ){
                        glTranslatef(0, -1.0, 0);
                        vbos[i].draw();
                    }
                    glPopMatrix();
                }
            }
        }
        
        glPopMatrix();
    } mExp.end();

    gl::clear();
    glColor3f(1, 1, 1);
    mExp.draw();
}

void cApp::keyDown(KeyEvent e){
    
    switch (e.getChar()) {
        case 'S': mExp.snapShot(); break;
    }
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
