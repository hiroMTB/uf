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

    void setupCsound();
    void setupOrc();
    void setupSco();
    void startCsound();
    void prepareSim( int idump );
    
    const int numOcs= 60;
    int idump = 24;
    fs::path assetDir;
    fs::path renderDir;
    Csound * csound;
    mt::SimFrame<double> rho;
    mt::SimFrame<double> E;
    mt::SimFrame<double> fx;
};

void cApp::setup(){
    
    setWindowPos(0, 0);
    setWindowSize(400, 400);
    assetDir = mt::getAssetPath();
    renderDir = mt::getRenderPath();
    bool dirok = createDirectories( renderDir.string() + "/" );
    if( !dirok ) quit();
    
    setupCsound();
    setupOrc();
    setupSco();
    startCsound();

    csoundDestroy( csound->GetCsound() );
    
    ccout::b("\n\n DONE \n\n");
    quit();
}

void cApp::setupCsound(){
    
    csound = new Csound();
    
    fs::path renderFilePath = renderDir/(mt::getTimeStamp() + ".wav");
    string rnd = "-o" + renderFilePath.string();
    csound->SetOption( const_cast<char*>( rnd.c_str() ) );
    csound->SetOption( (char*)"-W" );   // Wav
    csound->SetOption( (char*)"-f" );   // -f:32float, -3:24bit, -s:16bit,
}

void cApp::setupOrc(){
    
    int sr = 48000;    // sampling rate
    int ksmps = 16;    // control rate
    int nChn = 2;
    int zdbfs = 1;
    string settings = mt::op_setting(sr, ksmps, nChn,zdbfs);
    
    
    string gen = R"dlm(
    
        giSine    ftgen     0, 0, 2^12, 10, 1
    
    )dlm";

    string instr1_top = R"dlm(
    
        instr 1
        ifrq  = p4
        print ifrq
    
        iFis  = ifrq * 1.414214
        iGis  = ifrq * 1.587401
        iA    = ifrq * 1.681773
        iCis  = ifrq * 1.059463
    
    )dlm";
    
    string instr1_kamp;
    for( int i=0; i<numOcs; i++){
        int id = i+1;
        string kamp = mt::op_orc("kamp"+to_string(id), "chnget", "\"camp"+ to_string(id)+"\"");
        instr1_kamp += kamp;
    }
    
    instr1_kamp += "\n";
    
    string instr1_aOsc;
    vector<string> scales = {"iFis", "iGis", "iA", "iCis"};
    
    for( int i=0; i<numOcs; i++){
        int id = i+1;
        int octave = floor(i/4);
        string scale = scales[id%4];
        
        int maxOctave = numOcs/4;
        double weight = (pow(2, maxOctave) - pow(2, octave) )/pow(2, maxOctave);
        //double weight = 1.0 - (double)octave/maxOctave;
        weight = pow(weight, 5);
        
        string aOsc = mt::op_orc("aOsc"+to_string(id), "poscil", "kamp"+to_string(id)+"*"+to_string(weight), scale+"*"+to_string(pow(2,octave)), "giSine" );
        instr1_aOsc += aOsc;
    }
    
    instr1_aOsc += "\n";
    
    string instr1_mix;
    string aL = "aL  sum ";
    string aR = "aR  sum ";
    
    for( int i=0; i<numOcs; i++ ){
        int id = i+1;
        if( id%2==1)
            aL += "aOsc" + to_string(id) + ",";
        else
            aR += "aOsc" + to_string(id) + ",";
    }
    
    aL.erase(aL.size()-1);
    aR.erase(aR.size()-1);
    aL += "\n";
    aR += "\n";
    instr1_mix = aL + aR;
    
    string instr1_out = R"dlm(

        outs   aL, aR
        endin
    
    )dlm";
    
    string orc = settings + gen + instr1_top + instr1_kamp + instr1_aOsc + instr1_mix + instr1_out;
    
    
    int r1 = csound->CompileOrc( orc.c_str() );
    mt::saveString( orc, "orc_src.txt" );
    
    if( r1 ==0 ){
        ccout::b( "Orcestra file compile OK" );
    }else{
        ccout::r( "Orcestra file compile Failed" );
        quit();
    }
}

void cApp::setupSco(){
    
    string sco = R"dlm(
    
        ;score
        ;          freq
        i 1 0 30   2.0439453125

    )dlm";
    
    cout << sco << endl;
    int r2 = csound->ReadScore(sco.c_str());
    if( r2 ==0 ){
        ccout::b("Score file compile OK");
    }else{
        ccout::r("Score file compile Failed");
    }
}

void cApp::startCsound(){
    
    csound->Start();
    
    int loop = 0;
    
    prepareSim( idump );
    
    MYFLT *camp[numOcs];
    
    vector<double> prevAmp;
    
    prevAmp.assign(numOcs, 0);
    
    bool init = false;
    while ( csoundPerformKsmps(csound->GetCsound() ) == 0) {
        
        if (loop >= 400*400) {
            idump++;
            if( idump == 94 )
                return;
            prepareSim(idump);
            loop = 0;
        }
        
        int offset = loop*400;
        
        for( int f=0; f<numOcs; f++ ){
            string chname = "camp" + to_string(f+1);
            csoundGetChannelPtr( csound->GetCsound(), &camp[f], chname.c_str(), CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL );
            
            double amp = rho.data[0][offset + f];
            if(init) amp = amp*0.0001 + prevAmp[f]*0.9999;
            *camp[f] = amp;
            prevAmp[f] = amp;
        }
        
        loop++;
        init = true;
    }
    
}

void cApp::prepareSim( int idump ){
    
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
}

void cApp::draw(){
    gl::clear();
}


CINDER_APP_NATIVE( cApp, RendererGl(0) )
