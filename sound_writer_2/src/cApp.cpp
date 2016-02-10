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

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
public:
    void setup();
    void writeWavFromImg(fs::path srcPath, fs::path renderdir, int nCh, int samplingRate );
    
    const int       samplingRate    = 192000;
    double          screen_w        = -123;        // depends on sound sample length
    const double    screen_h        = 1000;
    const double    xScale          = 0.001;
    const double    yScale          = screen_h;
    const double    margin          = 0;
};

void cApp::setup(){
    
    // fetch image file
    fs::path assetPath = mt::getAssetPath();
    vector<fs::path> srcList;
    fs::path dir = assetPath/("img/all/");
    fs::recursive_directory_iterator it(dir), eof;
    while( it!= eof){
        if( !fs::is_directory(it.status() ) ){
            
            if( it->path().filename().string().at(0) != '.' ){
                cout << "add to process list : " << srcList.size() << " " << it->path().filename() << endl;
                srcList.push_back( *it );
            }
        }
        ++it;
    }

    // prepare render dir
    fs::path render_dir = mt::getRenderPath();
    
    // write
    for( auto srcPath : srcList ){
        writeWavFromImg(srcPath, render_dir, 1, samplingRate);
    }

    quit();

}

void cApp::writeWavFromImg( fs::path srcPath, fs::path render_dir, int nCh=1, int samplingRate=192000 ){

    fs::path dir_snd = render_dir / "snd/";
    fs::path dir_img = render_dir / "img/";
    if( !fs::exists(dir_snd) )
        createDirectories( dir_snd );

    if( !fs::exists(dir_img) )
        createDirectories( dir_img );
    
    
    // we use float for all data type
    vector<float> data;

    // LOAD Image
    ImageSourceRef srcRef = loadImage(srcPath);

    Surface32f sur = Surface32f( srcRef );
    Surface32f::Iter itr = sur.getIter();
    
    while(itr.line()){
        while( itr.pixel() ){
            data.push_back( itr.r() );
        }
    }

    string fileName = srcPath.filename().string();

    
    // Write Sound
    if(0){
        fs::path path_snd = dir_snd / (fileName+".wav");
        SoundWriter::writeWav32f(data, nCh, samplingRate, data.size()/nCh, path_snd.string() );
    }
    
    screen_w = data.size() * xScale;

    // Write sound data PNG
    Exporter mExp;
    setWindowPos(0, 0);
    setWindowSize( (screen_w+ margin*2)/10, (screen_h+margin*2)/10);
    mExp.setup(screen_w+margin*2, screen_h+margin*2, 0, 2, GL_RGB, "", 0);

    fs::path path_img = dir_img / (fileName+ ".png");
    mExp.snapShot( path_img.string() );
    
    gl::enableAlphaBlending();
    gl::clear();
    
    mExp.beginPersp();{
        gl::clear( Colorf(0,0,0) );
        glPointSize(1);
        gl::color(1,1,1,0.5);
        gl::translate( Vec2f(margin, margin+mExp.mFbo.getHeight()) );
        
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

CINDER_APP_NATIVE( cApp, RendererGl(0) )
