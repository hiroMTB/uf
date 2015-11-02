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

#include "cinder/audio/Context.h"
#include "cinder/audio/Utilities.h"
#include "cinder/audio/SamplePlayerNode.h"

#include "csound.hpp"
#include "csound.h"

#include "ContourMap.h"
#include "ufUtil.h"
#include "ConsoleColor.h"
#include "CsoundOp.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
public:
    void setup();
    void draw();
    void listup( vector<fs::path>&srcList, fs::path assetPath );
    void write(fs::path srcPath, fs::path render_dir );
    
    Csound * csound;
    Perlin mPln;
    Perlin mPln2;
    
    vector<fs::path> srcList;
    fs::path assetPath;
};

void cApp::setup(){
    
    cout << mt::op( 1 ) << endl;
    cout << mt::op( 1.1, "Yes" ) << endl;
    cout << mt::op( 1, 2, 0.333 ) << endl;
    cout << mt::op( "one", "two", "three" ) << endl;
    cout << mt::op( 1, 2, 333, 4444, 5, 6, 7, 8 ) << endl;

    boost::format fmt("Yoooo!! %05.4f,%05.4f,%05.4f");
    cout << mt::op( fmt, 1, 2, 0.333 ) << endl;
    
    setWindowPos(0, 0);
    setWindowSize(600, 200);
    mPln.setSeed(123);
    mPln.setOctaves(4);
    mPln2.setSeed(555);
    mPln2.setOctaves(6);
    
    fs::path render_dir = uf::getRenderPath();
    bool dirok = createDirectories( render_dir.string() + "/" );
    if( !dirok ) quit();

    assetPath = "snd/data2wav";
    //assetPath = "snd/data2wav2fftwave";
    listup( srcList, assetPath );

    for( auto srcPath : srcList ){
        write(srcPath, render_dir );
    }
}

void cApp::listup( vector<fs::path>&srcList, fs::path assetPath){
    
    fs::path dir = loadAsset( assetPath )->getFilePath();
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
}

void cApp::write( fs::path srcFilePath, fs::path render_dir ) {
    
    csound = new Csound();
    //csound->SetOption( (char*)"-odac" );
    string srcFilename = srcFilePath.filename().string();
    fs::path renderFilePath = render_dir / srcFilename;
    cout << "srcFilePath =  " << srcFilePath.string() << endl;
    cout << "srcFilename =  " << srcFilename << endl;

    cout << "renderFilePath = " << renderFilePath.string() << endl;
    cout << "assetPath" << assetPath.string() << endl;
    
    int sampling_rate = 192000;
    int control_rate = 1;
    string sr      = "sr=" + toString(sampling_rate) + "\n";
    string ksmps   = "ksmps=" + toString(control_rate) + "\n";
    string nchnls  = "nchnls=" + toString(2) + "\n";
    string dbfs    = "0dbfs=1\n";
    string fileName = "-o" + renderFilePath.string() + "-ptk_" + toString(sampling_rate) + "_k" + toString(control_rate)+ "_" + ".wav";
    
    csound->SetOption( const_cast<char*>( fileName.c_str() ) );   // file name
    csound->SetOption( (char*)"-W" );   // Wav
    csound->SetOption( (char*)"-f" );   // 32float
    
    string orc = sr + ksmps + nchnls + dbfs;
    
    orc += "giFile	ftgen	0, 0, 0, 1, \"../../../assets/" + assetPath.string() + "/" + srcFilename + "\", 0, 0, 0";
    
    string orcMain = loadString( loadAsset("csound/partikkle_2_r1.orc") );
    
    orc += orcMain;
    
    {
        cout << orc << endl;
        int result = csound->CompileOrc( orc.c_str() );
        
        if( result ==0 ){
            ccout::b( "Orcestra file compile OK" );
        }else{
            ccout::r( "Orcestra file compile Failed" );
            quit();
        }
    }
    
    string sco =   R"dlm(
    ; score code
    ;i1	st	dur     speed	grate	gsize	cent	posrnd	cntrnd	pan	dist
    i1	0	10      1       20000      10     1200     1000	1400     1	1
    i2  0   10
    )dlm";
    
    {
        cout << sco << endl;
        int result = csound->ReadScore(sco.c_str());
        if( result ==0 ){
            ccout::b("Score file compile OK");
        }else{
            ccout::r("Score file compile Failed");
        }
    }
    
    csound->ReadScore(sco.c_str());
    csound->Start();
    
    //assetPath = "snd/data2wav2fftwave";
    audio::SourceFileRef sourceFileRef = audio::load( loadAsset( assetPath/srcFilePath.filename() ) );
    audio::BufferRef buf = sourceFileRef->loadBuffer();
    float * ch0 = buf->getChannel(0);
    int nFrame = buf->getNumFrames();
    
    int i = 0;
    MYFLT * cpp1, *cpp2, *cpp3;
    float n1 = 0.5;
    float n2 = 0.5;
    while ( csoundPerformKsmps(csound->GetCsound() ) == 0) {
        csoundGetChannelPtr( csound->GetCsound(), &cpp1, "cpp1", CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL );
        csoundGetChannelPtr( csound->GetCsound(), &cpp2, "cpp2", CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL );
        csoundGetChannelPtr( csound->GetCsound(), &cpp3, "cpp3", CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL );
    
        n1 = mPln.dfBm( 1 + i*0.001, n1 ).x;
        n2 = mPln2.dfBm( 2 + i*0.05, n1 ).x;
        n1 = (n1+1.0)*0.5;
        n2 = (n2+1.0)*0.5;
        
        *cpp1 = n1*0.9 + 0.1;
        *cpp2 = n2*0.9 + 0.1;
        *cpp3 = ch0[i%nFrame];
        
        //cout << *cpp1 << ", " << *cpp2 << endl;
        i++;
    }
    
    csoundDestroy( csound->GetCsound() );
}


void cApp::draw(){
    gl::clear( Colorf(1,0.4,0.3));
}


CINDER_APP_NATIVE( cApp, RendererGl(0) )
