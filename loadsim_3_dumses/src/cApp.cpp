#define RENDER

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
#include "ConsoleColor.h"
#include "Exporter.h"

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

    void loadPlotData( string fileName );
    void loadSimulationData( string fileName );
    
    int mWin_w = 1920;
    int mWin_h = 1080;

    gl::VboMesh vbo;
    MayaCamUI camUi;
    Perlin mPln;
    
    Exporter mExp;
    int boxelx, boxely, boxelz;
    
    vector<double> pR, pTheta;
    
    int frame = 100;

    string simType = "simu_1";
    string log = "log";
    string prm = "rho";

};

void cApp::setup(){
    setWindowPos( 0, 0 );
    setWindowSize( 1080*3*0.5, 1920*0.5 );
    mExp.setup( 1080*3, 1920, 0, 3000, GL_RGB, mt::getRenderPath(), 0);
    
    CameraPersp cam(1080*3, 1920, 60.0f, 1, 100000 );
    cam.lookAt( Vec3f(0,0,200), Vec3f(0,0,0) );
    cam.setCenterOfInterestPoint( Vec3f(0,0,0) );
    camUi.setCurrentCam( cam );
    
    mPln.setSeed(123);
    mPln.setOctaves(4);
    
    loadPlotData( "sim/supernova/plot/" + simType);
    
    boxelx = pR.size();
    boxely = pTheta.size();
    boxelz = 1;
    
#ifdef RENDER
    mExp.startRender();
#endif
    
}

void cApp::loadPlotData(string simu_name){
    
    // load .r, .thata
    vector<string> exts = { ".r", ".theta"};
    for( auto & ext : exts ){
    
        fs::path assetPath = mt::getAssetPath();
        string path = (assetPath/(simu_name + ext)).string();
        cout << "start loading " << ext << " file : " << path << "...";
        std::ifstream is( path, std::ios::binary );
        if(is){ cout << " done" << endl;
        }else{ cout << " ERROR" << endl; quit(); }
        
        is.seekg (0, is.end);
        int fileSize = is.tellg();
        is.seekg (0, is.beg);
        int arraySize = arraySize = fileSize / sizeof(double);

        vector<double> & vec = (ext == ".r")? pR : pTheta;
        vec.clear();
        vec.assign(arraySize, double(0));
        is.read(reinterpret_cast<char*>(&vec[0]), fileSize);
        is.close();
        
        printf( "arraySize %d, %e~%e\n\n", (int)vec.size(), vec[0], vec[vec.size()-1]);
    }
    
    
}


void cApp::loadSimulationData(string fileName){

    fs::path assetPath = mt::getAssetPath();
    string path = (assetPath/fileName).string();
    //cout << "start loading binary file : " << path << "...";
    std::ifstream is( path, std::ios::binary );
    if(is){
       // cout << " done" << endl;
    }else{
        cout << " ERROR" << endl;
        quit();
    }
    
    // get length of file:
    is.seekg (0, is.end);
    int fileSize = is.tellg();
    is.seekg (0, is.beg);
    
    int arraySize = arraySize = fileSize / sizeof(double);
    //cout << "arraySize : " << arraySize << endl;
    
    vector<double> rho;
    rho.assign(arraySize, double(0));
    is.read(reinterpret_cast<char*>(&rho[0]), fileSize);
    is.close();
    
    //cout << "Making point data... " << endl;
    
    double in_min =  numeric_limits<double>::max();
    double in_max =  numeric_limits<double>::min();
    
    for( auto r : rho ){
        in_min = MIN( in_min, r);
        in_max = MAX( in_max, r);
    }
    
    vector<Vec3f> points;
    vector<ColorAf> colors;
    
    double scale = 600.0;
    double extrude = 900.0;
    for( int j=0; j<boxely; j++ ){
        for( int i=0; i<boxelx; i++ ){
            
            int index = j + i*boxely;
            
            double rho_raw = rho[index];
            float rho_map = lmap(rho_raw, in_min, in_max, 0.0, 1.0);
            float visible_thresh = 0.2f;
            
            if( visible_thresh<rho_map && rho_map <1.00 ){
                rho_map = lmap( rho_map, visible_thresh, 1.0f, 0.8f, 0.0f);
                ColorAf color(CM_HSV, rho_map, 0.8f, 0.8);
                
                bool polar = true;
                if( polar ){
                    double r = pR[i];
                    double theta = pTheta[j];
                    double x = r * cos( theta );
                    double y = r * sin( theta );
                    
                    points.push_back( Vec3f( x*scale, y*scale, -rho_map*extrude) );
                }else{
                    points.push_back( Vec3f(i, j, 0) * scale);
                }
                colors.push_back( color );
            }
        }
    }


    gl::VboMesh::Layout layout;
    layout.setStaticColorsRGBA();
    layout.setStaticPositions();

    vbo.reset();
    vbo = gl::VboMesh(points.size(), 0, layout, GL_POINTS);
    vbo.bufferPositions(points);
    vbo.bufferColorsRGBA(colors);
    
    //cout << "create VBO : " << vbo.getNumVertices() << " verts" << endl;
    cout << "Visible particle rate : " << vbo.getNumVertices()/(float)arraySize*100.0 << " %" << endl;
    
}

void cApp::update(){
    
    fs::path p("sim");
    p = p/"supernova"/simType/log/prm/(simType + "_polar_" +log+ "_" + prm + "_00" + to_string(frame) + ".bin");
    loadSimulationData(  p.string() );

}

void cApp::draw(){

    
    gl::enableAlphaBlending();
    gl::enableDepthRead();
    gl::enableDepthWrite();
    glPointSize(1);
    glLineWidth(1);
    
    
    //mExp.begin( camUi.getCamera() );{
    mExp.beginOrtho();{
        
        gl::clear( ColorA(0,0,0,1) );
        
        //gl::translate(mExp.mFbo.getWidth()/2, mExp.mFbo.getHeight()/2);
        gl::rotate(Vec3f(-90,0,0));
        
        if( !mExp.bSnap && !mExp.bRender )
            mt::drawCoordinate(1000);
        
        if(vbo){
            gl::draw(vbo);
            gl::translate(0.5f, 0.5f);
            gl::draw(vbo);
        }
        

    }mExp.end();
    
    mExp.draw();
    
    gl::pushMatrices();
    gl::setMatricesWindow(getWindowSize() );
    gl::drawSolidRect(Rectf(Vec2i(0,0), Vec2i(120,50)));
    gl::color(1, 1, 1);
    gl::drawString("frame " + to_string(frame), Vec2f(20,20));
    gl::popMatrices();
    
    frame++;
}

void cApp::keyDown( KeyEvent event ) {
    char key = event.getChar();
    switch (key) {
        case 'S': mExp.snapShot();  break;
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
