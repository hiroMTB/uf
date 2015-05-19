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

#include <sndfile.hh>

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
public:
    void setup();
    void writeWavHeader();
    
};

void cApp::setup(){
    
    // setting
    int nCh = 2;
    int samplingRate = 192000;

    Surface32f sur = Surface32f( loadImage(loadAsset("vela_scana_spire250_signal.tiff")));
    int w = sur.getWidth();
    int h = sur.getHeight();
    int nPix = w * h;

    vector<float> data;
    data.assign( nPix*nCh , 0);

    Surface32f::Iter itr = sur.getIter();

    int index = 0;
    while(itr.line()){
        while( itr.pixel() ){
            float r = itr.r();
            data[index*nCh+0] = r;
            data[index*nCh+1] = r;
            index++;
        }
    }

    string path = uf::getTimeStamp() + ".wav";
    
    /*
     *      Write WAV file with libsndfile
     */
    SNDFILE * file;
    SF_INFO sfinfo;
    sfinfo.samplerate = samplingRate;
    sfinfo.frames = nPix;
    sfinfo.channels = nCh;
    sfinfo.format = (SF_FORMAT_WAV | SF_FORMAT_FLOAT);  /* 32 bit float data */
    file = sf_open(path.c_str(), SFM_WRITE, &sfinfo);
    sf_count_t frameWrote = sf_write_float(file, &data[0], data.size());
    if(frameWrote != data.size()) puts(sf_strerror(file));
    sf_close(file);
    
    
    quit();
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
