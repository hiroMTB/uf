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

#include "mtUtil.h"
#include "SoundWriter.h"
#include "Exporter.h"

#include "SimFrame.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
public:
    void setup();
    void writeWavFromSim(fs::path srcPath, fs::path renderdir, int nCh, int samplingRate );
    
    const int       samplingRate    = 192000;
    double          screen_w        = -123;        // depends on sound sample length
    const double    screen_h        = 1000;
    const double    xScale          = 0.0002;
    const double    yScale          = screen_h/2;
    const double    margin          = 40;
};

void cApp::setup(){
    
    // fetch image file
    fs::path assetDir = mt::getAssetPath();
    vector<fs::path> simFilePathList;
    fs::path simDir = assetDir/("sim/all/");

    for( int i=24; i<95; i++  ){
        string dumpName ="0"+to_string(i);
        //fs::path rho =  assetDir/"sim"/"Heracles"/"simu_mach4_split"/"rho"/("rho_"+ dumpName + ".bin");
        //fs::path E =    assetDir/"sim"/"Heracles"/"simu_mach4_split"/"E"  /("E_"  + dumpName + ".bin");
        //fs::path fx =   assetDir/"sim"/"Heracles"/"simu_mach4_split"/"fx" /("fx_" + dumpName + ".bin");
        fs::path u =    assetDir/"sim"/"Heracles"/"simu_mach4_split"/"u"  /("u_"  + dumpName + ".bin");
        
        //simFilePathList.push_back( rho );
        //simFilePathList.push_back( E );
        //simFilePathList.push_back( fx );
        simFilePathList.push_back( u );
    }
    
    // prepare render dir
    fs::path renderDir = mt::getRenderPath();
    
    // write
    for( auto simFilePath : simFilePathList ){
        writeWavFromSim(simFilePath, renderDir, 1, samplingRate);
    }

    quit();

}

void cApp::writeWavFromSim( fs::path simFilePath, fs::path renderDir, int nCh=1, int samplingRate=192000 ){

    fs::path renderDirSnd = renderDir / "snd/";
    fs::path renderDirImg = renderDir / "img/";
    if( !fs::exists( renderDirSnd) )
        createDirectories( renderDirSnd );

    if( !fs::exists( renderDirImg) )
        createDirectories( renderDirImg );
    
    
    // LOAD Sim
    mt::SimFrame<float> simf;
    double min = std::numeric_limits<double>::min();
    double max = std::numeric_limits<double>::max();
    simf.load( simFilePath.string(), 400, 400, 400, min, max, mt::SimFrame<float>::DATA_TYPE::DATA_LOG );
    
    unsigned int dimemsion = simf.dimension;
    for( int dim=0; dim<dimemsion; dim++){
    
        vector<float> & data = simf.data[dim];
        
        string fileName = simFilePath.filename().string();
        
        if( dimemsion!= 1 )
            fileName.replace(0, 1, "u"+to_string(dim));
        
        // Write Sound
        unsigned int frameNum = simf.grid_size;
        fs::path path_snd = renderDirSnd / (fileName+".wav");
        SoundWriter::writeWav32f(data, nCh, samplingRate, frameNum/nCh, path_snd.string() );
        
        screen_w = frameNum * xScale;
        
        // Write image
        Exporter mExp;
        setWindowPos(0, 0);
        setWindowSize( (screen_w+ margin*2)/10, (screen_h+margin*2)/10);
        mExp.setup(screen_w+margin*2, screen_h+margin*2, 1, GL_RGB, "", 0);
        
        fs::path renderImgPath = renderDirImg / (fileName+ ".png");
        mExp.snapShot( renderImgPath.string() );
        
        gl::enableAlphaBlending();
        gl::clear();
        
        mExp.begin();{
            gl::clear( Colorf(0,0,0) );
            glPointSize(1);
            gl::color(1,1,1,0.5);
            gl::translate( Vec2f(margin, margin+mExp.mFbo.getHeight()/2) );
            
            glBegin( GL_POINTS );
            for( int i=0; i<data.size(); i++ ){
                float x = i * xScale;
                float y = data[i] * yScale;
                glVertex3f( x, -y, 0 );
            }
            glEnd();
            
            // gl::color( Colorf(1,0,0) );
            // gl::drawSolidRect( Rectf(100, -100, 400, -400) );
        }
        mExp.end();
    }

}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
