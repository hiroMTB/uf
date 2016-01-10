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
#include "DataGroup.h"
#include "RfExporterBin.h"
#include "RfImporterBin.h"

using namespace ci;
using namespace ci::app;
using namespace std;


struct Particle{

public:
    Vec3f pos;
    Vec3f vel;
    ColorAf col;
    float key;

    bool operator<(const Particle& p ) const {
        return key < p.key;
    }

    bool operator>(const Particle& p ) const {
        return key > p.key;
    }

};

class cApp : public AppNative {
    
public:
    void setup();
    
    void create_from_perlin();
    void create_from_grid();
    void create_from_image();
    
    void update();
    void draw();
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void keyDown( KeyEvent event );
    
    MayaCamUI camUi;
    Perlin mPln;
    
    DataGroup mDg;
    vector<Vec3f> vs;
    vector<ColorAf> cs;
    vector<float> pos;
    vector<float> vel;
    
    string renderFileName;
    
    
    vector<Particle> part;
};

void cApp::setup(){
    setWindowPos( 0, 0 );
    setWindowSize( 1920, 1080 );
    
    CameraPersp cam( 1920, 1080, 54.4f, 1, 100000 );
    cam.lookAt( Vec3f(0,0,100), Vec3f(0,0,0) );
    cam.setCenterOfInterestPoint( Vec3f(0,0,0) );
    camUi.setCurrentCam( cam );
    
    mPln.setSeed(123);
    mPln.setOctaves(4);
    
    //create_from_perlin();
    //create_from_grid();
    create_from_image();
    
    mDg.createDot( vs, cs, 0.0 );
    
//    RfExporterBin rfOut;
//    renderFileName = mt::getTimeStamp()+"_p_00000.bin";
//    rfOut.write( renderFileName, pos, vel );
    
    vs.clear();
    //cs.clear();
    pos.clear();
    vel.clear();
    
}

void cApp::create_from_image(){
    
    //  RF
    //  max width -100~100
    double scale = 0.1; // 100/4320 = 0.023
    
    // make particle set
    printf("start loading image ... ");
    fs::path assetDir = mt::getAssetPath();
    string imgName = "earth_15_4320x1920";
    Surface32f sur( loadImage( assetDir/"img"/(imgName+".png") ));

    float iW = sur.getWidth();
    float iH = sur.getHeight();
    printf("DONE\n");

    printf("start making particle data ... ");
    Surface32f::Iter itr = sur.getIter();
    while( itr.line() ){
        while ( itr.pixel() ){
            
            Particle p;
            
            Vec2i iPos = itr.getPos();
            iPos.x -= iW/2;
            iPos.y -= iH/2;
            p.pos.set( iPos.x*scale, 0, iPos.y*scale);  // X-Z coord

            Colorf color(itr.r(), itr.g(), itr.b() );
            p.col = color;
            
            Vec3f hsb = color.get( CM_HSV );
            p.key = hsb.x;
            
            part.push_back( p );
        }
    }
    
    unsigned long num = part.size();
    printf("DONE, num : %ld\n", num);
    
    
    // sort
    printf("start sort ... " );
    std::sort( part.begin(), part.end() );
    printf("DONE \n" );

    int separator = 128;
    int numPerSep = num/separator;
    
    printf("start making RF data, sep %d, num particle per sep %d\n", separator, numPerSep);
    
    // print and check
    for (int s=0; s<separator; s++ ) {

        vector<float> pos;
        vector<float> vel;
        vector<float> mass;
        

        for (int i=0; i<numPerSep; i++ ) {

            float rf = randFloat();
            int index = i + s*numPerSep;
            if( index> num )
                break;
            
            Particle & p = part[index];
            const Vec3f & pPos = p.pos;
            const Colorf & pCol = p.col;
            
            Vec3f v = pCol.get( CM_HSV );
            
            pos.push_back( pPos.x );
            pos.push_back( pPos.y + mPln.noise(v.x, v.y*0.45, rf)*15 );
            pos.push_back( pPos.z );
            
            Vec3f n = mPln.dfBm( pCol.r, pCol.g, pCol.b);
            n.x *= 10.0f;
            n.z *= 10.0f;
            
            vel.push_back( n.x );
            vel.push_back( n.y );
            vel.push_back( n.z );
            
            mass.push_back( (mPln.noise( v.x+v.y )+1.0)*0.55 );
        }
        RfExporterBin rfOut;
        renderFileName = imgName+"_sep_"+ to_string(s)+"_00000.bin";
        rfOut.write( renderFileName, pos, vel, mass );
        printf( "finish writting RF data : %s\n", renderFileName.c_str() );
    }

    quit();
}

void cApp::create_from_grid(){
    
    int w = 1080*4/4;
    int h = 1920/4;
    float scale = 0.2;
    
    for( int i=0; i<w; i++ ){
        for( int j=0; j<h; j++ ){
            
            Vec3f p(i, 0, j );
            p.x -= w/2;
            p.z -= h/2;

            p.x *= scale;
            p.z *= scale;

            vs.push_back( p );
            
            ColorAf c( randFloat(), randFloat(), randFloat(), 1 );
            cs.push_back( c );
            
            pos.push_back( p.x );
            pos.push_back( p.y );
            pos.push_back( p.z );
            
            Vec3f v(0,0,0);
            vel.push_back( v.x );
            vel.push_back( v.y );
            vel.push_back( v.z );
        }
    }
}

void cApp::create_from_perlin(){

    float rf = randFloat();
    for( int i=0; i<50; i++ ){
        for( int j=0; j<50; j++ ){
            
            Vec3f p = mPln.dfBm(rf, rf+i*0.04, rf+j*0.04 );
            p *= 5.0f;
            vs.push_back( p );
            
            ColorAf c( randFloat(), randFloat(), randFloat(), 1 );
            cs.push_back( c );
            
            pos.push_back( p.x );
            pos.push_back( p.y );
            pos.push_back( p.z );
            
            Vec3f v = mPln.dfBm(rf*0.5, rf+i*0.08, -rf+j*0.02 );
            vel.push_back( v.x );
            vel.push_back( v.y );
            vel.push_back( v.z );
        }
    }
}

void cApp::update(){
}

void cApp::draw(){
    
    gl::clear( ColorA(1,1,1,1) );

    gl::setMatrices( camUi.getCamera() );
    mt::drawCoordinate( 10 );
    
    // data
    if( mDg.mDot ){
        glPointSize( 1 );
        gl::draw( mDg.mDot );
    }
}

void cApp::keyDown( KeyEvent event ) {
    char key = event.getChar();
    switch (key) {
        case 'l':
        {
            vs.clear();
            RfImporterBin rfIn;
            rfIn.load( renderFileName );
            
            pos = rfIn.pPosition;
            vel = rfIn.pVelocity;
            for( int i=0; i<pos.size()/3; i++){
                float x = pos[i*3+0];
                float y = pos[i*3+1];
                float z = pos[i*3+2];
                
                vs.push_back( Vec3f( x, y, z) );
            }
            
            mDg.createDot( vs, cs, 0.0 );
        }
        break;
            
        case 'r':
        {
            vs.clear();
            //cs.clear();
            pos.clear();
            vel.clear();
            mDg.mDot.reset();
        }
    }
}

void cApp::mouseDown( MouseEvent event ){
    camUi.mouseDown( event.getPos() );
}

void cApp::mouseDrag( MouseEvent event ){
    camUi.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
