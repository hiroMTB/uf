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
#include "cinder/Xml.h"
#include "csound.hpp"
#include "csound.h"
#include "ContourMap.h"
#include "mtUtil.h"
#include "ConsoleColor.h"
#include "CsoundOp.h"
#include "SimFrame.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
public:
    void setup();
    void draw();
    void prepare_sim( int idump );
    string writeOrc();
    string writeSco();
    void startCsound( string orc, string sco);
    void performCsound();

    const int numFilter = 100;
    int idump = 24;
    fs::path assetDir;
    Csound * csound;
    mt::SimFrame<double> rho;
    mt::SimFrame<double> E;
    mt::SimFrame<double> fx;

};

void cApp::setup(){
    
    setWindowPos(0, 0);
    setWindowSize(400, 400);
    assetDir = mt::getAssetPath();
    
    fs::path render_dir = mt::getRenderPath();
    bool dirok = createDirectories( render_dir.string() + "/" );
    if( !dirok ) quit();

    fs::path srcDirPath = assetDir/"sim"/"Heracles"/"simu_mach4_split"/"rho";

    string orc = writeOrc();
    string sco = writeSco();
    startCsound( orc, sco );
    performCsound();
    
    csoundDestroy( csound->GetCsound() );
    
    ccout::b("\n\n DONE \n\n");
    quit();
}

void cApp::prepare_sim( int idump ){

    // LOAD Sim
    string dumpName = "0" + to_string(idump);
    double range_min = std::numeric_limits<double>::min();
    double range_max = std::numeric_limits<double>::max();

    fs::path simDir = assetDir/"sim"/"Heracles"/"simu_mach4_split";
    
    {
        string prmName = "rho";
        fs::path xmlFilePath = simDir/("settings_" + prmName + ".xml");
        XmlTree settings( loadFile(xmlFilePath) );
        double dmin = std::stod( settings.getChild("settings/min").getValue() );
        double dmax = std::stod( settings.getChild("settings/max").getValue() );
        rho.min = dmin;
        rho.max = dmax;
        fs::path filePath = assetDir/"sim"/"Heracles"/"simu_mach4_split"/prmName  /(prmName+"_"  + dumpName + ".bin");
        rho.load( filePath.string(), 400, 400, 400, range_min, range_max, mt::SimFrame<double>::DATA_TYPE::DATA_LOG );
    }
    {
        string prmName = "E";
        fs::path xmlFilePath = simDir/("settings_" + prmName + ".xml");
        XmlTree settings( loadFile(xmlFilePath) );
        double dmin = std::stod( settings.getChild("settings/min").getValue() );
        double dmax = std::stod( settings.getChild("settings/max").getValue() );
        E.min = dmin;
        E.max = dmax;
        
        fs::path filePath = assetDir/"sim"/"Heracles"/"simu_mach4_split"/prmName  /(prmName+"_"  + dumpName + ".bin");
        E.load( filePath.string(), 400, 400, 400, range_min, range_max, mt::SimFrame<double>::DATA_TYPE::DATA_LOG );
    }
    
    {
        string prmName = "fx";
        fs::path xmlFilePath = simDir/("settings_" + prmName + ".xml");
        XmlTree settings( loadFile(xmlFilePath) );
        double dmin = std::stod( settings.getChild("settings/min").getValue() );
        double dmax = std::stod( settings.getChild("settings/max").getValue() );
        fx.min = dmin;
        fx.max = dmax;

        fs::path filePath = assetDir/"sim"/"Heracles"/"simu_mach4_split"/prmName  /(prmName+"_"  + dumpName + ".bin");
        fx.load( filePath.string(), 400, 400, 400, range_min, range_max, mt::SimFrame<double>::DATA_TYPE::DATA_LOG );
    }
}

string cApp::writeOrc(){
    
    int sr = 48000;    // sampling rate
    int ksmps = 16;      // control rate
    int nChn = 2;        //
    int zdbfs = 1;
    string settings = mt::op_setting(sr, ksmps, nChn,zdbfs);

    string instr1 = R"dlm(
    
        ; instr1
        instr 1                                     ;triggers notes in instrument 2 with randomised p-fields
        krate  randomi 0.2,0.4,0.1                  ;rate of note generation
        ktrig  metro  krate                         ;triggers used by schedkwhen
        koct   random 5,12                          ;fundemental pitch of synth note
        kdur   random 15,30                         ;duration of note
        schedkwhen ktrig,0,0,2,0,kdur   ;,cpsoct(koct)  ;trigger a note in instrument 2
        endin
    
    )dlm";
    
    string instr2_top = R"dlm(
    
        ; instr2
        instr 2 ; subtractive synthesis instrument
        aNoise  pinkish  1                     ;a noise source sound: pink noise
        kGap    rspline  0.3,0.05,0.2,2        ;time gap between impulses
        aPulse  mpulse   1, kGap              ;a train of impulses
        kCFade  rspline  0,1,0.1,1             ;crossfade point between noise and impulses
        aInput  ntrpol   aPulse,aNoise,kCFade  ;implement crossfade

        ; cutoff frequencies for low and highpass filters
        ;kLPF_CF  rspline  13,8,0.1,0.4
        ;kHPF_CF  rspline  5,10,0.1,0.4

        ; filter input sound with low and highpass filters in series -
        ; - done twice per filter in order to sharpen cutoff slopes
        ;aInput    butlp    aInput, cpsoct(kLPF_CF)
        ;aInput    butlp    aInput, cpsoct(kLPF_CF)
        ;aInput    buthp    aInput, cpsoct(kHPF_CF)
        ;aInput    buthp    aInput, cpsoct(kHPF_CF)
    
        aInput = aInput * 0.1
        ;kcf     rspline  p4*1.05,p4*0.95,0.01,0.1 ; fundemental
    
    )dlm";
    instr2_top += "\n";
    
    
    string instr2_kamp;
    for( int i=0; i<numFilter; i++){
        int id = i+1;
        string kamp = mt::op_orc("kamp"+to_string(id), "chnget", "\"camp"+ to_string(id)+"\"");
        instr2_kamp += kamp;
    }
    
    instr2_kamp += "\n";
    
    string instr2_kcf;
    for( int i=0; i<numFilter; i++){
        int id = i+1;
        string kcf = mt::op_orc("kcf"+to_string(id), "chnget", "\"ccf"+ to_string(id)+"\"");
        instr2_kcf += kcf;
    }
    
    instr2_kcf += "\n";
    
    
    string instr2_kbw;
    for( int i=0; i<numFilter; i++){
        int id = i+1;
        string kbw = mt::op_orc( "kbw"+to_string(id), "chnget", "\"cbw"+to_string(id)+"\"" );
        instr2_kbw += kbw;
    }
    
    instr2_kbw += "\n";
    
    
    string instr2_reson;
    instr2_reson += "imode = 0\n";
    for( int i=0; i<numFilter; i++){
        int id = i+1;
        string reson = mt::op_orc("a"+toString(id), "reson", "aInput", "kcf"+to_string(id), "kbw"+to_string(id), "imode" );
        instr2_reson += reson;
    }
    
    instr2_reson += "\n";
    
    string instr2_mix;
    string mixL = "aMixL  sum ";
    string mixR = "aMixR  sum ";

    for( int i=1; i<numFilter+1; i++ ){
        if( i%2==1)
            mixL += "a" + to_string(i) + "*kamp" + to_string(i) + ",";
        else
            mixR += "a" + to_string(i) + "*kamp" + to_string(i) + ",";
    }

    mixL.erase(mixL.size()-1);
    mixR.erase(mixR.size()-1);
    mixL += "\n";
    mixR += "\n";
    instr2_mix = mixL + mixR;
  
    
    string instr2_out = R"dlm(
    
    ;kEnv     linseg   0, p3*0.5, 1,p3*0.5,0,1,0       ; global amplitude envelope

    ;aMixL  = aMixL * kEnv
    ;aMixR  = aMixR * kEnv
    aMixL  = aMixL * 0.0008
    aMixR  = aMixR * 0.0008
    
    outs   aMixL,aMixR
    ;outs   aNoise,  aNoise
    ;outs   aPulse, aPulse
    ;outs   aInput, aInput
    endin

    )dlm";
    
    string instr2;
    instr2 += instr2_top;
    instr2 += instr2_kamp;
    instr2 += instr2_kcf;
    instr2 += instr2_kbw;
    instr2 += instr2_reson;
    instr2 += instr2_mix;
    instr2 += instr2_out;
    
    string orc = settings + instr1 + instr2;
    return orc;
}

string cApp::writeSco(){

    string sco = R"dlm(

        ;score code
        ;	st	dur
        i1	0	30

    )dlm";

    return sco;
}

void cApp::startCsound( string orc, string sco ){
    csound = new Csound();
    //csound->SetOption( (char*)"-odac" );
    string renderFileName = "-o" + mt::getTimeStamp() + ".wav";
    csound->SetOption( const_cast<char*>( renderFileName.c_str() ) );
    csound->SetOption( (char*)"-W" );   // Wav
    csound->SetOption( (char*)"-f" );   // -f:32float, -3:24bit, -s:16bit,

    int r1 = csound->CompileOrc( orc.c_str() );
    
    if( r1 ==0 ){
        ccout::b( "Orcestra file compile OK" );
        mt::saveString( orc, "orc_src.txt" );
    }else{
        ccout::r( "Orcestra file compile Failed" );
        quit();
    }
    
    cout << sco << endl;
    int r2 = csound->ReadScore(sco.c_str());
    if( r2 ==0 ){
        ccout::b("Score file compile OK");
    }else{
        ccout::r("Score file compile Failed");
    }
    
    //csound->ReadScore(sco.c_str());
    csound->Start();
}

void cApp::performCsound() {
    
    int loop = 0;
    
    prepare_sim( idump );
    
    MYFLT *cbw[numFilter];
    MYFLT *camp[numFilter];
    MYFLT *ccf[numFilter];
    
    vector<double> prevAmp;
    vector<double> prevCf;
    vector<double> prevBw;

    prevAmp.assign(numFilter, 0.1);
    prevCf.assign(numFilter, 0.1);
    prevBw.assign(numFilter, 0.1);

    bool init = false;
    while ( csoundPerformKsmps(csound->GetCsound() ) == 0) {

        if (loop >= 400*400) {
            idump++;
            if( idump == 94 )
                return;
            prepare_sim(idump);
            loop = 0;
        }
        
        int offset = loop*400;
        
        for( int f=0; f<numFilter; f++ ){
            string chname = "camp" + to_string(f+1);
            csoundGetChannelPtr( csound->GetCsound(), &camp[f], chname.c_str(), CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL );
            
            double amp = rho.data[0][offset + f] * 0.1;
            if(init) amp = amp*0.0001 + prevAmp[f]*0.9999;
            *camp[f] = amp;
            prevAmp[f] = amp;
        }
        
        for( int f=0; f<numFilter; f++ ){
            string chname = "ccf" + to_string(f+1);
            csoundGetChannelPtr( csound->GetCsound(), &ccf[f], chname.c_str(), CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL );
            
            double cf = 20 + rho.data[0][offset + f] * 400000.0;
            if(init) cf = cf*0.1 + prevCf[f]*0.9;
            *ccf[f] = cf;
            prevCf[f] = cf;
        }
        
        for( int f=0; f<numFilter; f++ ){
            string chname = "cbw" + to_string(f+1);
            csoundGetChannelPtr( csound->GetCsound(), &cbw[f], chname.c_str(), CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL );
            
            double bw = fx.data[0][offset + f];
            if(init) bw = bw*0.01 + prevBw[f] * 0.99;
            *cbw[f] = bw;
            prevBw[f] = bw;
            
        }
        
        loop++;
        init = true;
    }
    
}

void cApp::draw(){
    gl::clear( Colorf(0,0,0));
    
}


CINDER_APP_NATIVE( cApp, RendererGl(0) )
