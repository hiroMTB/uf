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

    gl::VboMesh vbo;
    MayaCamUI camUi;
    Perlin mPln;
    
    Exporter mExp;
    int boxelx, boxely, boxelz;
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
    
    if( 1 ) loadSimulationData( "sim/Dumses/test/simu_2_rho_p_300f.bin" );
    
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
    
    int arraySize = arraySize = fileSize / sizeof(double);
    cout << "arraySize : " << arraySize << endl;
    
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

    cout << "min : " << in_min << endl;
    cout << "max : " << in_max << endl;

    vector<Vec3f> points;
    vector<ColorAf> colors;
    
    for( int i=0; i<boxelx; i++ ){
        for( int j=0; j<boxely; j++ ){
            for( int k=0; k<boxelx; k++ ){
                
                int index = i + j*boxely + k*boxelz*boxely;

                int dimension = 3;
                switch ( dimension) {
                    case 1:
                    {
                        double rho_raw = rho[index];
                        float rhof;
                        
                        bool logarizm = true;
                        if( logarizm ){
                            double rho_map = lmap(rho_raw, in_min, in_max, 1.0, 10.0);
                            rhof = log10(rho_map);
                        }else{
                            double rho_map = lmap(rho_raw, in_min, in_max, 0.0, 1.0);
                            rhof = rho_map;
                        }
                        
                        float visible_thresh = 0.005f;
                        
                        if( rhof>visible_thresh ){
                            rhof = lmap( rhof, visible_thresh, 1.0f, 0.001f, 0.7f);
                            
                            Vec3f noise = mPln.dfBm(k, j, i);
                            
                            ColorAf color(1,1,1,rhof);
                            points.push_back( Vec3f(k-200, j-200, i-200) + noise );
                            colors.push_back( color );
                        }
                        break;
                    }
                        
                    case 3:
                    {
                        double x = rho[ index*3 + 0];
                        double y = rho[ index*3 + 1];
                        double z = rho[ index*3 + 2];
                        double x_map = lmap( x, in_min, in_max, 1.0, 10.0 );
                        double y_map = lmap( y, in_min, in_max, 1.0, 10.0 );
                        double z_map = lmap( z, in_min, in_max, 1.0, 10.0 );
                        float xf = log10( x_map );
                        float yf = log10( y_map );
                        float zf = log10( z_map );
 
                        
                        double ulen = sqrt(x*x + y*y + z*z);
                        double ulen_map = lmap( ulen, in_min, in_max, 1.0, 10.0 );
                        float ulenf = log10(ulen_map);
                        float visible_thresh = 0.78f;
                        
                        if( ulenf>visible_thresh ){
                            ulenf = lmap( ulenf, visible_thresh, 1.0f, 0.000001f, 0.2f);
                            
                            Vec3f noise = mPln.dfBm(k, j, i) * 1.5;
                            ColorAf color( xf, yf, zf, ulenf);
                            points.push_back( Vec3f(k-200, j-200, i-200) + noise );
                            colors.push_back( color );
                        }
                        break;
                    }
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
    
}

void cApp::update(){
}

void cApp::draw(){

    mExp.begin( camUi.getCamera() );{
        gl::clear( ColorA(0,0,0,1) );
        gl::enableAlphaBlending();
        
        if( !mExp.bSnap && !mExp.bRender ){
            // Guide
            uf::drawCoordinate(10);
        
            // base rect
//            gl::lineWidth(1);
//            gl::color(0.25, 0.25, 0.25);
//            gl::pushModelView();{
//                gl::translate(Vec3f(210,0,0));
//                gl::rotate(Vec3f(0,90,0));
//                gl::drawStrokedRect( Rectf( -200,200, 200,-200) );
//            }gl::popModelView();
        }
        
        {
            // data
            if(vbo){
                glPointSize(1);
                gl::draw(vbo);
            }
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
