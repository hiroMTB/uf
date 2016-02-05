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
    void loadSimulationData( int idump );
    void setupGui();
    void updateRotation();
    
    int boxelx, boxely, boxelz;
    
    MayaCamUI camUi;
    Perlin mPln;
    
    Exporter mExp;
    vector<DataGroup> mDataGroup;
    
    gl::VboMesh bridge;
    unsigned int idump = 24;
    
    bool bStart = false;

    float angle1 = 40;
    float angle2 = 40;
    float angleSpd1 = 0;
    float angleSpd2 = 0.5f;
    Vec3f axis1 = Vec3f(-0.42, -0.84, 0.34);
    Vec3f axis2 = Vec3f(-0.52, -0.83, -0.18);
    
    int rotateStartFrame = 25 + 50;
    
    params::InterfaceGlRef gui;
    
};

void cApp::setup(){
    setWindowPos( 0, 0 );
    setWindowSize( 1920, 1080 );
    mExp.setup( 1920, 1080, 0, 1000, GL_RGB, mt::getRenderPath(), 0);
    
    CameraPersp cam(1080*3, 1920, 50, 1, 1000000 );
    cam.lookAt( Vec3f(0,0,1600), Vec3f(0,0,0) );
    cam.setCenterOfInterestPoint( Vec3f(0,0,0) );
    //cam.setPerspective( 54.4f, getWindowAspectRatio(), 1, 100000 );     // 35mm
    camUi.setCurrentCam( cam );
    
    setupGui();
    
    mPln.setSeed(123);
    mPln.setOctaves(4);
    
    boxelx = boxely = boxelz = 400;
    
    //loadSimulationData( idump );
    
#ifdef RENDER
    mExp.startRender();
#endif
}

void cApp::setupGui(){

    gui = params::InterfaceGl::create( getWindow(), "settings", Vec2i(300,400) );
    
    gui->addText("main");
    gui->addParam("idum", &idump);
    gui->addParam("start", &bStart);

    gui->addText("Rotation1");
    gui->addParam("axis1", &axis1);
    gui->addParam("angle1", &angle1);
    gui->addParam("angleSpeed1", &angleSpd1);
    
    gui->addText("Rotation2");
    gui->addParam("axis2", &axis2);
    gui->addParam("angle2", &angle2);
    gui->addParam("angleSpeed2", &angleSpd2);

}

void cApp::update(){
   
    if( bStart ){
        for( auto dg : mDataGroup )
            dg.clear();
    
        mDataGroup.clear();
        
        idump++;
        loadSimulationData( idump );
    }
    
    updateRotation();
}

void cApp::updateRotation(){

    angle2 = angleSpd2*idump;

}

void cApp::loadSimulationData( int idump){
    
    stringstream fileName;
    fileName << "sim/Heracles/simu_mach4_split/rho/rho_" << setw(3) << setfill('0') << idump << + ".bin";
    
    fs::path assetPath = mt::getAssetPath();
    string path = ( assetPath/fileName.str() ).string();
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
    //cout << "length : " << fileSize << " byte" << endl;
    is.seekg (0, is.beg);
    
    int arraySize = arraySize = fileSize / sizeof(double);      // 400*400*400 = 64,000,000
    
    vector<double> rho;
    rho.assign(arraySize, double(0));
    is.read(reinterpret_cast<char*>(&rho[0]), fileSize);
    is.close();
    
    double in_min = std::numeric_limits<double>::max();
    double in_max = std::numeric_limits<double>::min();
    
    for( auto r : rho ){
        in_min = MIN( in_min, r);
        in_max = MAX( in_max, r);
    }
    
    //float alpha = 0.56f;
    vector< tuple<float, float, ColorAf> > thresholds = {
        { 0.000645,   0.00066,    ColorAf( 0.7, 0.2, 0.6, 1) },
        { 0.00067,  0.00072,    ColorAf( 0.7, 0.6, 0.1, 1) },
        { 0.00075,  0.0008,     ColorAf( 0.8, 0.1, 0.1, 0.1) },
        { 0.01,     0.0102,     ColorAf( 0.2, 0.5, 0.5, 1) },
        { 0.02,     0.022,      ColorAf( 0.0, 0.4, 0.7, 1) },
        { 0.1,      0.24,       ColorAf( 0.3, 0.3, 0.3, 1) },
        { 0.24,     1,        ColorAf( 1.0, 0.0, 0.1, 1) }
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
                        
                        Vec3f noise = mPln.dfBm(k, j, i)*0.3f;
                        Vec3f v = Vec3f(k, j, i) + Vec3f(-200,-200,-200) + noise;
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
        //dg.createLine( points[i], colors[i] );
        
        mDataGroup.push_back( dg );
        totalPoints += points[i].size();
    }
    cout << "Particle Visible/Total : " << totalPoints << "/" << arraySize << endl;
    cout << "Visible Rate           : " << (float)totalPoints/arraySize*100.0f << endl;
    
    
    // make bridge
    if( 0 ){
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
        sprintf(m, "create Bridge : %10lu lines", bridge.getNumVertices()/2 );
        cout << m << endl;
    }
}

void cApp::draw(){
    
    mExp.begin( camUi.getCamera() );{
    
        
        //gl::clear( ColorA(0,0,0,1) );
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

        gl::enableAlphaBlending();
        gl::enableDepthRead();
        gl::enableDepthWrite();
        
        gl::rotate(Quaternion<float>(axis1, toRadians(angle1)) );
        gl::rotate(Quaternion<float>(axis2, toRadians(angle2)) );

        if( !mExp.bSnap && !mExp.bRender ){
            mt::drawCoordinate( 200 );
        }
        
        // data
//        if(bridge){
//            glLineWidth( 1 );
//            gl::draw( bridge );
//        }

        glLineWidth( 1 );
//        for( int i=0; i<mDataGroup.size(); i++){
//            if(mDataGroup[i].mLine) gl::draw( mDataGroup[i].mLine );
//        }

        glPointSize( 1 );
        for( int i=0; i<mDataGroup.size(); i++){
            gl::draw( mDataGroup[i].mDot );            
        }
        
        glFrontFace(GL_CCW);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        gl::color(1, 1, 1);
        mt::drawCube( 400 );
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_CULL_FACE);

    }mExp.end();
    mExp.draw();
    
    gui->draw();
}

void cApp::keyDown( KeyEvent event ) {
    char key = event.getChar();
    switch (key) {
        case 's': mExp.snapShot(); break;
        case ' ': bStart = !bStart; break;
    }
}

void cApp::mouseDown( MouseEvent event ){
    camUi.mouseDown( event.getPos() );
}

void cApp::mouseDrag( MouseEvent event ){
    camUi.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void cApp::resize(){
    CameraPersp cam = camUi.getCamera();
    cam.setAspectRatio( getWindowAspectRatio() );
    camUi.setCurrentCam( cam );
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
