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

#include "ContourMap.h"
#include "ufUtil.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
public:
    void setup();
    
    Perlin mPln;
};

void cApp::setup(){
    
    mPln.setSeed(123);
    mPln.setOctaves(4);

    // setting
    int nCh = 2;
    int samplingRate = 19200;
    float duration = 10;  // sec
    int totalSamp = duration * samplingRate;

    vector<float> data;
    data.assign( totalSamp*nCh , 0);
    
    float noise = 0.1;
    
    for( int i=0; i<totalSamp; i++){
        noise += (mPln.fBm(noise*0.1, i*0.01)) *0.02;
        noise = math<float>::clamp(noise, -1, 1);
        data[i*nCh+0] = noise;
        data[i*nCh+1] = noise;
    }

    string path = uf::getTimeStamp() + ".raw";
    FILE * file = fopen( path.c_str(), "wb");
    fwrite( &data[0], data.size(), sizeof(data[0]), file );
    fclose( file );
    quit();
}


CINDER_APP_NATIVE( cApp, RendererGl(0) )
