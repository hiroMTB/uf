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
  	void resize();
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void keyDown( KeyEvent event );
    
    int mWin_w = 1920;
    int mWin_h = 1080;

    gl::VboMesh vbo;
    int boxelx, boxely, boxelz;
    
    MayaCamUI camUi;
};

void cApp::setup(){
    setWindowPos( 0, 0 );
    setWindowSize( mWin_w, mWin_h );
    
    boxelx = boxely = boxelz = 400;
    
    // laod binary
    string path = loadAsset("sim/Heracles/512.bin")->getFilePath().string();
    cout << "loading binary file : " << path << endl;
    std::ifstream is( path, std::ios::binary );
    if(is){
        ccout::b("load OK bin file");
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
    cout << "arraySize : " << arraySize << endl;

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

    vector<Vec3f> points;
    vector<ColorAf> colors;
    
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
                
                float visible_thresh = 0.0005f;
                if( rhof>visible_thresh ){
                    rhof = lmap( rhof, visible_thresh, 1.0f, 0.01f, 0.4f);
                    colors.push_back( ColorAf(1,1,1,rhof) );
                    points.push_back( Vec3f(k-200, j-200, i-200) );
                }
            }
        }
    }
    
    gl::VboMesh::Layout layout;
    layout.setStaticColorsRGBA();
    layout.setStaticPositions();
    
    vbo = gl::VboMesh(points.size(), 0, layout, GL_POINTS);
    vbo.bufferPositions(points);
    vbo.bufferColorsRGBA(colors);

    cout << "create VBO : " << vbo.getNumVertices() << " verts" << endl;
    cout << "Visible particle rate : " << vbo.getNumVertices()/(float)arraySize*100.0 << " %" << endl;
    
    CameraPersp cam;
    cam.lookAt( Vec3f(0,0,600), Vec3f(0,0,0) );
    cam.setCenterOfInterestPoint( Vec3f(0,0,0) );
    cam.setPerspective( 54.4f, getWindowAspectRatio(), 1, 100000 );     // 35mm
    camUi.setCurrentCam( cam );
}


void cApp::update(){
}

void cApp::draw(){
    gl::clear(ColorA(0,0,0,1));
    gl::enableAlphaBlending();
    
    gl::pushMatrices();
    gl::setMatrices( camUi.getCamera() );

    {
        // base rect
        gl::lineWidth(1);
        gl::color(0.25, 0.25, 0.25);
        gl::pushModelView();
        gl::translate(Vec3f(210,0,0));
        gl::rotate(Vec3f(0,90,0));
        gl::drawStrokedRect( Rectf( -200,200, 200,-200) );
        gl::popModelView();
    }
    
    {
        // data
        glPointSize(1);
        gl::draw(vbo);
    }
    
    {
        // Guide
        uf::drawCoordinate(10);
    }
    
    gl::popMatrices();
    
}

void cApp::keyDown( KeyEvent event ) {
    char key = event.getChar();
    switch (key) {
        case 'S':
            break;
        }
}

void cApp::resize(){
}

void cApp::mouseDown( MouseEvent event ){
    camUi.mouseDown( event.getPos() );
}

void cApp::mouseDrag( MouseEvent event ){
    camUi.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
