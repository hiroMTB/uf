//#include "cinder/app/AppNative.h"
//#include "cinder/Rand.h"
//#include "cinder/Utilities.h"
//#include "cinder/gl/gl.h"
//#include "cinder/gl/Fbo.h"
//#include "cinder/gl/Texture.h"
//#include "cinder/Camera.h"
//#include "cinder/MayaCamUI.h"
//#include "cinder/Perlin.h"
//#include "cinder/params/Params.h"
//#include "CinderOpenCv.h"
//
//#include "csound.hpp"
//#include "csound.h"
//
//#include "ContourMap.h"
//#include "ufUtil.h"
//#include "ConsoleColor.h"
//
//using namespace ci;
//using namespace ci::app;
//using namespace std;
//
//class cApp : public AppNative {
//    
//public:
//    void setup();
//    void draw();
//    void write(fs::path srcPath, fs::path render_dir );
//    
//    Csound * csound;
//    Perlin mPln;
//    Perlin mPln2;
//    
//    fs::path assetPath;
//};
//
//void cApp::setup(){
//    mPln.setSeed(123);
//    mPln.setOctaves(4);
//    mPln2.setSeed(555);
//    mPln2.setOctaves(6);
//    fs::path render_dir = uf::getRenderPath();
//    
//    vector<fs::path> srcList;
//    assetPath = "snd/data2wav";
//    fs::path dir = loadAsset( assetPath )->getFilePath();
//    fs::recursive_directory_iterator it(dir), eof;
//    while( it!= eof){
//        if( !fs::is_directory(it.status() ) ){
//            
//            if( it->path().filename().string().at(0) != '.' ){
//                cout << "add to process list : " << srcList.size() << " " << it->path().filename() << endl;
//                srcList.push_back( *it );
//            }
//        }
//        ++it;
//    }
//    
//    
//    
//    for( auto srcPath : srcList ){
//        write(srcPath, render_dir );
//    }
//    quit();
//    
//}
//
//void cApp::write( fs::path srcFilePath, fs::path render_dir ) {
//    
//    csound = new Csound();
//    //csound->SetOption( (char*)"-odac" );
//    
//    string srcFilename = srcFilePath.filename().string();
//    fs::path renderFilePath = render_dir / srcFilename;
//    
//    int sampling_rate = 192000;
//    int control_rate = 32;
//    string sr      = "sr=" + toString(sampling_rate) + "\n";
//    string ksmps   = "ksmps=" + toString(control_rate) + "\n";
//    string nchnls  = "nchnls=" + toString(2) + "\n";
//    string dbfs    = "0dbfs=1\n";
//    string fileName = "-o" + renderFilePath.string() + "-ptk_" + toString(sampling_rate) + "_k" + toString(control_rate)+ "_" + uf::getTimeStamp() + ".wav";
//    
//    csound->SetOption( const_cast<char*>( fileName.c_str() ) );   // file name
//    csound->SetOption( (char*)"-W" );   // Wav
//    csound->SetOption( (char*)"-f" );   // 32float
//    
//    
//    string orc = sr + ksmps + nchnls + dbfs;
//    
//    orc += "giFile	ftgen	0, 0, 0, 1, \"" + assetPath.string() + "/" + srcFilename + "\", 0, 0, 0";
//    
//    string orcMain = loadString( loadAsset("csound/partikkle_2_r1.orc") );
//    
//    orc += orcMain;
//    
//    {
//        cout << orc << endl;
//        int result = csound->CompileOrc( orc.c_str() );
//        
//        if( result ==0 ){
//            ccout::b( "Orcestra file compile OK" );
//        }else{
//            ccout::r( "Orcestra file compile Failed" );
//            quit();
//        }
//    }
//    
//    string sco =   R"dlm(
//    ; score code
//    ;i1	st	dur     speed	grate	gsize	cent	posrnd	cntrnd	pan	dist
//    i1	0	10      0.001    1000    0.1     1200     10000	1400     1	1
//    i2  0   10
//    )dlm";
//    
//    {
//        cout << sco << endl;
//        int result = csound->ReadScore(sco.c_str());
//        if( result ==0 ){
//            ccout::b("Score file compile OK");
//        }else{
//            ccout::r("Score file compile Failed");
//        }
//    }
//    
//    csound->ReadScore(sco.c_str());
//    csound->Start();
//    
//    int i = 0;
//    MYFLT * cpp1, *cpp2;
//    float n1 = 0.5;
//    float n2 = 0.5;
//    while ( csoundPerformKsmps(csound->GetCsound() ) == 0) {
//        csoundGetChannelPtr( csound->GetCsound(), &cpp1, "cpp1", CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL );
//        csoundGetChannelPtr( csound->GetCsound(), &cpp2, "cpp2", CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL );
//        
//        n1 = mPln.dfBm( 1 + i*0.001, n1 ).x;
//        n2 = mPln2.dfBm( 2 + i*0.05, n1 ).x;
//        n1 = (n1+1.0)*0.5;
//        n2 = (n2+1.0)*0.5;
//        
//        *cpp1 = n1*0.9 + 0.1;
//        *cpp2 = n2*0.9 + 0.1;
//        
//        //cout << *cpp1 << ", " << *cpp2 << endl;
//        i++;
//    }
//    
//    csoundDestroy( csound->GetCsound() );
//}
//
//
//void cApp::draw(){
//    gl::clear();
//}
//
//
//CINDER_APP_NATIVE( cApp, RendererGl(0) )
