/#include "cinder/app/AppNative.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Texture.h"
#include "cinder/Camera.h"
#include "cinder/MayaCamUI.h"
#include "cinder/Perlin.h"
#include "cinder/params/Params.h"
#include "cinder/Xml.h"

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
    void writeWavFromSim( fs::path srcPath, fs::path renderdir, double setting_min, double setting_max, int nCh, int samplingRate );
    void writeSettingXml( fs::path xmlFilePath, double min, double max);
    
    void prepareSettings( string prmName );
    
    const int       samplingRate    = 48000;
    double          screen_w        = -123;        // depends on sound sample length
    const double    screen_h        = 1000;
    const double    xScale          = 0.0002;
    const double    yScale          = screen_h/2;
    const double    margin          = 40;

    fs::path assetDir;
    fs::path simDir;
    vector<string> prmNames = { "rho", "E", "fx", "u"};
    vector<vector<fs::path>> simFilePathList;
};

void cApp::setup(){
    
    // fetch image file
    assetDir = mt::getAssetPath();
    simDir = assetDir/"sim"/"Heracles"/"simu_mach4_split";
    
    for( int j=0; j<prmNames.size(); j++ ){
        simFilePathList.push_back( vector<fs::path>()) ;
                                  
        for( int i=24; i<95; i++  ){
            string dumpName ="0"+to_string(i);
            fs::path prmDir = simDir/prmNames[j];
            simFilePathList[j].push_back( prmDir/(prmNames[j] + "_"+ dumpName + ".bin") );
        }
    }
    
    // prepare min - max xml
    if(0){
        for( int j=0; j<prmNames.size(); j++ ){
            prepareSettings(prmNames[j]);
        }
    }
    
    // prepare render dir
    fs::path renderDir = mt::getRenderPath();
    
    // write snd
    for( int j=0; j<prmNames.size(); j++ ){
        string prmName = prmNames[j];
        double min, max;
        fs::path xmlFilePath = simDir/("settings_" + prmName + ".xml") ;
        if( !fs::exists(xmlFilePath ) ){
            //prepareSettings( prmName );
        }

        XmlTree settings( loadFile(xmlFilePath) );
        min = std::stod( settings.getChild("settings/min").getValue() );
        max = std::stod( settings.getChild("settings/max").getValue() );

        for( auto simFilePath : simFilePathList[j] ){
            writeWavFromSim(simFilePath, renderDir, min, max, 1, samplingRate);
        }
    }

    quit();

}

void cApp::prepareSettings( string prmName ){
    // prepare min / max value for scaling
    fs::path prmDir = simDir/prmName;
    
    mt::SimFrame<double> simf;
    simf.purseMinMax( prmDir );
    double min = simf.min;
    double max = simf.max;
    writeSettingXml( simDir/("settings_" + prmName+ ".xml"), min, max );
    
}


void cApp::writeSettingXml(fs::path xmlFilePath, double min, double max ){
    //console() << xmlFilePath.string() << endl;
    XmlTree xml( "settings", "" );
    
    std::ostringstream min_s; min_s << std::scientific << min;
    std::ostringstream max_s; max_s << std::scientific << max;
    
    xml.push_back( XmlTree( "min", min_s.str() ) );
    xml.push_back( XmlTree( "max", max_s.str() ) );
    xml.write( writeFile(xmlFilePath) );
}


void cApp::writeWavFromSim( fs::path simFilePath, fs::path renderDir, double setting_min, double setting_max, int nCh=1, int samplingRate=48000 ){

    fs::path renderDirSnd = renderDir / "snd/";
    fs::path renderDirImg = renderDir / "img/";
    if( !fs::exists( renderDirSnd) )
        createDirectories( renderDirSnd );

    if( !fs::exists( renderDirImg) )
        createDirectories( renderDirImg );
    
    
    // LOAD Sim
    mt::SimFrame<float> simf;
    
    simf.min = setting_min;
    simf.max = setting_max;
    
    double min = std::numeric_limits<double>::min();
    double max = std::numeric_limits<double>::max();
    simf.load( simFilePath.string(), 400, 400, 400, min, max, mt::SimFrame<float>::DATA_TYPE::DATA_MAP );
    
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
