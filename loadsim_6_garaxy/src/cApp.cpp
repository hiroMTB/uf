//#define RENDER
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
#include "mtUtil.h"
#include "ConsoleColor.h"
#include "Exporter.h"
#include "VboSet.h"
#include "cinder/Xml.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
    typedef tuple<string, vector<float>, float, float, float, float, VboSet, int> Gdata;

public:
    void setup();
    void update();
    void draw();
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void keyDown( KeyEvent event );

    void prepare();
    void loadXml( fs::path path );
    void loadBin( fs::path path, vector<float> & array );
    
    void makeVbo( Gdata & gd);
    void updateVbo();
    
    fs::path assetDir;
    MayaCamUI camUi;
    Perlin mPln;
    Exporter mExp;
    
    float fmax = numeric_limits<float>::max();
    float fmin = numeric_limits<float>::min();
    
    /*
         0 : prmNam
         1 : data
         2 : data min
         3 : data max
         4 : range in
         5 : range out
     */
    vector<Gdata> v_gd{
        Gdata( "pos",        vector<float>(), fmax, fmin, 0.0f, 1.0f, VboSet(),     0),
<<<<<<< HEAD
        Gdata( "vel_length", vector<float>(), fmax, fmin, 0.31f, 0.9f, VboSet(),     1),
        Gdata( "rho",        vector<float>(), fmax, fmin, 0.08f, 1.0f, VboSet(),     2),
        Gdata( "N",          vector<float>(), fmax, fmin, 0.0f, 1.0f, VboSet(),     3),
        Gdata( "mass",       vector<float>(), fmax, fmin, 0.28f, 1.0f, VboSet(),     4),
        Gdata( "K",          vector<float>(), fmax, fmin, 1.0f, 1.0f, VboSet(),     5)
=======
        Gdata( "vel_length", vector<float>(), fmax, fmin, 0.31f, 0.9f, VboSet(),    1),
        Gdata( "rho",        vector<float>(), fmax, fmin, 0.0001f, 1.0f, VboSet(),  2),
        Gdata( "N",          vector<float>(), fmax, fmin, 0.0f, 1.0f, VboSet(),     3),
        Gdata( "mass",       vector<float>(), fmax, fmin, 0.4f, 1.0f, VboSet(),     4),
        Gdata( "K",          vector<float>(), fmax, fmin, 0.3f, 1.0f, VboSet(),     5)
>>>>>>> master
    };
    
    bool bStart = false;
    int frame = 0;
};

void cApp::setup(){
    setWindowPos( 0, 0 );
    setWindowSize( 1920*0.4, 1080*3*0.4 );
    mExp.setup( 1920, 1080*3, 1000, GL_RGB, mt::getRenderPath(), 0);
    
<<<<<<< HEAD
    CameraPersp cam( 1920, 1080*3, 54.4f, 0.1, 10000 ); //35mm
    cam.lookAt( Vec3f(0,0,50), Vec3f(0,0,0) );
=======
    CameraPersp cam( 1920, 1080*3, 54.4f, 1, 1000 ); //35mm
    cam.lookAt( Vec3f(0,30,0), Vec3f(0,0,0) );
>>>>>>> master
    cam.setCenterOfInterestPoint( Vec3f(0,0,0) );
    camUi.setCurrentCam( cam );
    
    mPln.setSeed(123);
    mPln.setOctaves(4);
    
    assetDir = mt::getAssetPath();
    
    prepare();
    
#ifdef RENDER
    mExp.startRender();
#endif

}

void cApp::prepare(){
    
    string frameName = "rdr_00468_l17.hydro";
    
    /*
            XML
     */
    printf(")\nstart loading XML file\n");
    loadXml(assetDir/"sim"/"garaxy"/"bin"/"_settings"/(frameName+".settings.xml") );

    /*
            Bin
     */
    printf("\nstart loading binary file\n");
    for( auto & gd : v_gd ){
        string & prmName = std::get<0>(gd);
        vector<float> & data = std::get<1>(gd);
        loadBin(assetDir/"sim"/"garaxy"/"bin"/prmName/(frameName+"_"+ prmName + ".bin"), data );
    }
    
    /*
            Vbo
     */
    printf("\nstart making Vbo\n");
    makeVbo( v_gd[1] ); // 1 : vel_length
    makeVbo( v_gd[2] ); // 2 : rho
    makeVbo( v_gd[4] ); // 4 : mass
<<<<<<< HEAD
    makeVbo( v_gd[5] ); // 5 : K
=======
    //makeVbo( v_gd[5] ); // 5 : K
>>>>>>> master
}

void cApp::loadXml( fs::path path ){
    
    printf("%s\n", path.filename().string().c_str() );
    XmlTree settings( loadFile(path) );

    for( auto & gd : v_gd ){
        string & prmName = std::get<0>(gd);

        if( prmName != "pos" && prmName != "vel"){        
            float & min = std::get<2>(gd) = std::stod( settings.getChild("settings/"+ prmName+"/min").getValue() );
            float & max = std::get<3>(gd) = std::stod( settings.getChild("settings/"+ prmName+"/max").getValue() );
            printf("%-10s  : %e - %e\n", prmName.c_str(), min, max );
        }
    }
}

void cApp::loadBin( fs::path path, vector<float> & array ){
    
    printf("%-34s", path.filename().string().c_str() );
    std::ifstream is( path.string(), std::ios::binary );
    if( is ){   printf(" - DONE, "); }
    else{       printf(" - ERROR\nquit()"); quit(); }

    is.seekg (0, is.end);
    int fileSize = is.tellg();
    printf("%d byte, ", fileSize);
    is.seekg (0, is.beg);
    
    int arraySize = fileSize / sizeof(float);
    printf("%d float numbers\n", arraySize);
    
    array.assign(arraySize, float(0) );
    is.read(reinterpret_cast<char*>(&array[0]), fileSize);
    is.close();
}

void cApp::makeVbo( Gdata & gd ){
    vector<float> & posdata = std::get<1>( v_gd[0] );
    string prmName = std::get<0>(gd);
    vector<float> & data = std::get<1>(gd);
    float min = std::get<2>(gd);
    float max = std::get<3>(gd);
    float in  = std::get<4>(gd);
    float out = std::get<5>(gd);
    VboSet & vs = std::get<6>(gd);
    int id = std::get<7>(gd);
<<<<<<< HEAD
    printf("\nprmName   : %s\nmin-max   : %e - %e\nin-out    : %e - %e\n", prmName.c_str(), min, max, in, out );
    
    for( int i=0; i<data.size(); i++ ) {
        float d = data[i] - min;
        float log = lmap( log10(d), 0.0f, log10(max-min), 0.0f, 1.0f );
        float map = lmap( d,        min,  max,            0.0f, 1.0f );
        float alpha = 0.6;
        if( in<=log && log<=out ){

            Vec3f p( posdata[i*3+0], posdata[i*3+1], posdata[i*3+2] );
            float N = std::get<1>(v_gd[3])[i];
            N = lmap(N, std::get<2>(v_gd[3]), std::get<3>(v_gd[3]), 0.0f, 1.0f);
            Vec3f n = mPln.dfBm( p ) * 0.1 * (1.0f-N);
            vs.addPos( p*0.5 + n);
            
            ColorAf c;
            switch ( id ) {
                case 1: c = ColorAf( 1.0f-log*0.5, log*0.5+map, log*0.5+map, alpha );  break;
                case 2: c = ColorAf( 0.2, log+0.1, 1.0f-log*0.7, alpha );            break;
                case 4: c = ColorAf( 0.4, 0.8f-log*1.2, 0.1+map*10, alpha );           break;
                case 5: c = ColorAf( log*0.4, map*0.5, 0.1f, alpha );                  break;
=======
    printf("\nprmName   : %s\nmin-max   : %e - %e\nin-out(log): %0.4f - %0.4f\n", prmName.c_str(), min, max, in, out );
    
    for( int i=0; i<data.size(); i++ ) {
        float d = data[i] - min;
        float log = lmap( log10(1+d), 0.0f, log10(1+max-min), 0.0f, 1.0f );
        float map = lmap( d, 0.0f, max-min, 0.0f, 1.0f );
        float alpha = 0.8f;
        float N = std::get<1>(v_gd[3])[i];
        N = lmap(N, std::get<2>(v_gd[3]), std::get<3>(v_gd[3]), 0.0f, 1.0f);
        N = 1.0f - N;
        if( 0.7 <= N) continue;
        
        if( in<=log && log<=out ){

            Vec3f p( posdata[i*3+0], posdata[i*3+1], posdata[i*3+2] );
            p *= 0.5;
            Vec3f n = mPln.dfBm( p*0.5 ) * N * 0.1;
            p += n;
            vs.addPos( p );
            
            float rlog = lmap(log, in, out, 0.0f, 1.0f);
            rlog *= 8;
            
            ColorAf c(1,1,1,1);
            switch ( id ) {
                case 1: c = ColorAf( CM_HSV, MIN(1.0f, 0.5f+rlog), MIN(1.0f, 0.3f+rlog), 0.6f, alpha );            break;
                case 2: c = ColorAf( CM_HSV, MIN(1.0f, 0.3f+rlog), MIN(1.0f, 0.3f+rlog*0.7f), 0.5f, alpha );        break;
                case 4: c = ColorAf( CM_HSV, MIN(1.0f, 0.1f+rlog*0.1), MIN(1.0f, 0.4f+rlog), 0.7f, alpha );           break;
                case 5: c = ColorAf( log, log, log, alpha );             break;
>>>>>>> master
            }
            vs.addCol( c );
        }
    }
    
<<<<<<< HEAD
    vs.init( false, false, true, GL_POINTS );

    int nVerts = vs.vbo->getNumVertices();
=======
    vs.init( true, true, true, GL_POINTS );

    int nVerts = vs.getPos().size();
>>>>>>> master
    printf("add %d vertices, %0.4f %% visible\n", nVerts, (float)nVerts/data.size()*100.0f);

}

void cApp::update(){
    if( bStart ){
    }
}

void cApp::draw(){
    
    mExp.begin( camUi.getCamera() );{
        gl::clear( ColorA(0,0,0,1) );
        gl::enableAlphaBlending();
        
        glLineWidth( 1 );
        glPointSize( 1 );
        
<<<<<<< HEAD
        for( auto & gd : v_gd ){
            VboSet & vs = std::get<6>( gd );
=======
        for( int i=0; i<v_gd.size(); i++ ){
            VboSet & vs = std::get<6>( v_gd[i] );
>>>>>>> master
            if( vs.vbo ){
                gl::draw( vs.vbo );
            }
        }
        
<<<<<<< HEAD
        glColor3f(0,0,1);
        //gl::drawCube( Vec3f(0,0,0), Vec3f(10,10,10) );
        
=======
>>>>>>> master
        glPushMatrix();
        gl::setMatricesWindow(mExp.mFbo.getWidth(), mExp.mFbo.getHeight() );
        glLineWidth(3);
        glColor3f(1, 0, 0);
        gl::drawLine( Vec2f(0, 1080), Vec2f(1920, 1080) );
        gl::drawLine( Vec2f(0, 1080*2), Vec2f(1920, 1080*2) );
        //gl::drawLine( Vec2f(1920/2, 0), Vec2f(1920/2, 1080*3) );
        glPopMatrix();

    }mExp.end();
    
    gl::clear( ColorA(1,1,1,1) );
    glColor3f( 1,1,1 );
    mExp.draw();

}

void cApp::keyDown( KeyEvent event ) {
    switch ( event.getChar() ) {
        case 'S': mExp.startRender();       break;
        case 's': mExp.snapShot();          break;
        case ' ': bStart = !bStart;         break;
    }
}

void cApp::mouseDown( MouseEvent event ){
    camUi.mouseDown( event.getPos() );
}

void cApp::mouseDrag( MouseEvent event ){
    camUi.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
