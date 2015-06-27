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

#include "ufUtil.h"
#include "ConsoleColor.h"
#include "Exporter.h"
#include "DataGroup.h"

#include <iostream>
#include <fstream>

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
public:
    void setup();
    void update();
    void draw();
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void keyDown( KeyEvent event );
    void resize();
    void loadSimulationData( string fileName );
    
    int mWin_w = 1920;
    int mWin_h = 1080;
    int boxelx, boxely, boxelz;
    
    MayaCamUI camUi;
    Perlin mPln;
    
    Exporter mExp;
    vector<DataGroup> mDataGroup;
    
    gl::VboMesh bridge;
    
};

void cApp::setup(){
    setWindowPos( 0, 0 );
    setWindowSize( 1080*3*0.5, 1920*0.5 );
    mExp.setup( 1080*3, 1920, 1, GL_RGB, uf::getRenderPath(), 0);
    
    CameraPersp cam(1080*3, 1920, 54.4f, 1, 100000 );
    cam.lookAt( Vec3f(0,0,600), Vec3f(0,0,0) );
    cam.setCenterOfInterestPoint( Vec3f(0,0,0) );
    //cam.setPerspective( 54.4f, getWindowAspectRatio(), 1, 100000 );     // 35mm
    camUi.setCurrentCam( cam );
    
    mPln.setSeed(123);
    mPln.setOctaves(4);
    
    boxelx = boxely = boxelz = 400;
    
    if( 1 ) loadSimulationData( "sim/Heracles/512.bin" );
}

void cApp::loadSimulationData(string fileName){
    
    string path = loadAsset( fileName )->getFilePath().string();
    cout << "loading binary file : " << path << endl;
    std::ifstream is( path, std::ios::binary );
    if(is){
        ccout::b("load OK binary file");
    }else{
        ccout::r("load ERROR bin file");
        quit();
    }
    
    // get length of file:
    is.seekg (0, is.end);
    int fileSize = is.tellg();
    cout << "length : " << fileSize << " byte" << endl;
    is.seekg (0, is.beg);
    
    int arraySize = arraySize = fileSize / sizeof(double);      // 400*400*400 = 64,000,000
    
    vector<double> rho;
    rho.assign(arraySize, double(0));
    is.read(reinterpret_cast<char*>(&rho[0]), fileSize);
    is.close();
    
    cout << "close binary file " << endl;
    cout << "Making point data... " << endl;
    
    double in_min = std::numeric_limits<double>::max();
    double in_max = std::numeric_limits<double>::min();
    
    for( auto r : rho ){
        in_min = MIN( in_min, r);
        in_max = MAX( in_max, r);
    }
    
    vector< tuple<float, float, ColorAf> > thresholds = {
        { 0.0006,   0.00066,    ColorAf( 0.7, 0.2, 0.6, 1) },
        { 0.00066,  0.00072,    ColorAf( 0.7, 0.6, 0.1, 1) },
        { 0.00075,  0.0008,     ColorAf( 0.8, 0.1, 0.1, 1) },
        { 0.01,     0.0102,     ColorAf( 0.2, 0.5, 0.5, 1) },
        { 0.02,     0.022,      ColorAf( 0.0, 0.4, 0.7, 1) },
        { 0.1,      0.24,       ColorAf( 0.3, 0.3, 0.3, 1) },
        { 0.24,     1.0,        ColorAf( 1.0, 0.0, 0.1, 1) }
    };
    
    vector< vector<Vec3f> > points( thresholds.size() );
    vector< vector<ColorAf> > colors( thresholds.size() );
    
    for( int i=0; i<boxelx; i++ ){
        for( int j=0; j<boxely; j++ ){
            for( int k=0; k<boxelx; k++ ){
                
                int index = i + j*boxely + k*boxelz*boxely;
                double rho_raw = rho[index];
                float rhof;
                
                bool logarizm = true;
                if( logarizm ){
                    double rho_map = lmap(rho_raw, in_min, in_max, 1.0, 10.0);
                    double rho_log = log10(rho_map);
                    rhof = rho_log;
                }else{
                    double rho_map = lmap(rho_raw, in_min, in_max, 0.0, 1.0);
                    rhof = rho_map;
                }
                
                for( int t=0; t<thresholds.size(); t++ ){
                    
                    float low = std::get<0>(thresholds[t]);
                    float high = std::get<1>(thresholds[t]);
                    
                    if( low<=rhof && rhof<high ){
                        
                        Vec3f noise = mPln.dfBm(k, j, i);
                        Vec3f weight(0, 0, 0);
                        
                        //rhof = lmap( rhof, visible_thresh, 1.0f, 0.005f, 0.4f);
                        weight.x = t * -600 + 200;
                        
                        Vec3f v = Vec3f(k-200, j-200, i-200) + noise + weight;
//                        v.rotate( Vec3f(1,0,0), t*90 );
                        points[t].push_back( v );
                        
                        ColorAf c = std::get<2>(thresholds[t]);
                        c.r += noise.x;
                        c.g += noise.y;
                        c.b += noise.z;
                        c.a = 0.1 + rhof*0.5;
                        colors[t].push_back( c );
                        break;
                    }
                }
            }
        }
    }

    int totalPoints = 0;
    for( int i=0; i<points.size(); i++ ){
        
        DataGroup dg = DataGroup();
        dg.createDot( points[i], colors[i], std::get<0>(thresholds[i]) );
        dg.createLine( points[i], colors[i] );
        
        mDataGroup.push_back( dg );
        totalPoints += points[i].size();
    }
    cout << "Particle Visible/Total : " << totalPoints << "/" << arraySize << endl;
    cout << "Visible Rate           : " << (float)totalPoints/arraySize*100.0f << endl;
    
    
    
    // make bridge
    vector<Vec3f> bv;
    vector<ColorAf> bc;
    for( int i=0; i<points.size()-1; i++ ){
    
        int num_try = MIN(points[i].size(),points[i+1].size()) * 0.02;
        
        for( int j=0; j<num_try; j++ ){
            int id1 = randInt(0, points[i].size() );
            int id2 = randInt(0, points[i+1].size() );
            Vec3f v1 = points[i][id1];
            Vec3f v2 = points[i+1][id2];
            
            float dist = v1.distance(v2);
            if( 20<dist && dist<1000 ){
                
                bv.push_back( v1 );
                bv.push_back( v2 );
                
                ColorAf &c1 = colors[i][id1];
                ColorAf &c2 = colors[i+1][id2];
                ColorAf c = (c1 + c2)*0.3;
                bc.push_back( c );
                bc.push_back( c );
            }
        }
        
        points[i].clear();
    }
    
    gl::VboMesh::Layout layout;
    layout.setStaticIndices();
    layout.setDynamicColorsRGBA();
    layout.setDynamicPositions();
    
    bridge = gl::VboMesh( bv.size(), 0, layout, GL_LINES );
    gl::VboMesh::VertexIter itr( bridge );
    for( int i=0; i<bridge.getNumVertices(); i++ ){
        itr.setPosition( bv[i] );
        itr.setColorRGBA( bc[i] );
        ++itr;
    }
    
    char m[255];
    sprintf(m, "create Bridge : %10d lines", bridge.getNumVertices()/2 );
    cout << m << endl;
}

void cApp::update(){
}

void cApp::draw(){
    
    mExp.begin( camUi.getCamera() );{
        gl::clear( ColorA(0,0,0,1) );
        gl::enableAlphaBlending();
        gl::enableDepthRead();
        gl::enableDepthWrite();
        
        if( !mExp.bSnap && !mExp.bRender ){
            // Guide
            uf::drawCoordinate( 10 );
        }
        
        // data
        glLineWidth( 1 );
        gl::draw( bridge );
        
        glLineWidth( 1 );
        for( int i=0; i<mDataGroup.size(); i++){
            gl::draw( mDataGroup[i].mLine );
        }

        glPointSize( 2 );
        for( int i=0; i<mDataGroup.size(); i++){
            gl::draw( mDataGroup[i].mDot );
        }
        
        
    }mExp.end();
    
    gl::clear( ColorA(1,1,1,1) );
    gl::color( Color::white() );
    mExp.draw();
}

void cApp::keyDown( KeyEvent event ) {
    char key = event.getChar();
    switch (key) {
        case 'S':
            mExp.snapShot();
            break;
    }
}

void cApp::mouseDown( MouseEvent event ){
    camUi.mouseDown( event.getPos() );
}

void cApp::mouseDrag( MouseEvent event ){
    camUi.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void cApp::resize(){
    //    CameraPersp cam = camUi.getCamera();
    //    cam.setAspectRatio( getWindowAspectRatio() );
    //    camUi.setCurrentCam( cam );
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
