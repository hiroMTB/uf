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
    
    void addIndicesRectMesh(VboSet & vboset, VboSet & ret,  float threthold, int w, int h, Area area);
    void addIndicesTriMesh(VboSet & vboset, VboSet & ret, float threthold, int w, int h, Area area);
    void addIndiceSpline(VboSet & vboset, VboSet & ret, float threthold, int w, int h, Area area);
    void addIndicesLineVertical(VboSet & vboset, VboSet & ret, float threthold, int w, int h, Area area);
    void addIndicesLineHorizontal(VboSet & vboset, VboSet & ret, float threthold, int w, int h, Area area);
    void addIndicesPoint(VboSet & vboset, VboSet & ret, float threthold, int w, int h, Area area);
    
    bool bStart = false;
    int imgW;
    int imgH;
    
    MayaCamUI camUi;
    Perlin mPln;
    
    Exporter mExp;
    VboSet vboOrig;
    vector<vector<float>> data;
    vector<VboSet> vboRectMesh; // rect mesh
    vector<VboSet> vboTriMesh;  // tri  mesh
    vector<VboSet> vboCross;    // + + +
    vector<VboSet> vboPoint;    // ...
    vector<VboSet> vboLine;     // ---
    vector<VboSet> vboSpline;   // ~~~
    
    vector< vector<Colorf> > mColorSample;
    
};

void cApp::setup(){
    
    randSeed(4982);
    
    setWindowPos( 0, 0 );
    setWindowSize( 1080*3*0.5, 1920*0.5 );
    mExp.setup( 1080*3, 1920, 3000, GL_RGB, mt::getRenderPath(), 0);
    
    CameraPersp cam( 1080*3, 1920, 39.6f, 1, 100000 );
    cam.lookAt( Vec3f(0,0, -3300), Vec3f(0,0,0), Vec3f(0,1,0) );
    cam.setCenterOfInterestPoint( Vec3f(0,0,0) );
    //cam.setLensShift( 0, 1 );
    
    camUi.setCurrentCam( cam );
    
    mPln.setSeed(123);
    mPln.setOctaves(4);
    
    fs::path assetPath = mt::getAssetPath();
    mt::loadColorSample( "img/Vela_colorMap_x2.tif", mColorSample);
    
    {
        // make point from iamge
        fs::path imgPath1 = assetPath/"img"/"04"/"THY_Bfort 031_blk_x3.png";
        fs::path imgPath2 = assetPath/"img"/"04"/"XZ"/"x3"/"XZ_00047.png";
        
        Surface32f surf = Surface32f( loadImage( imgPath1 ) );
        imgW = surf.getWidth();
        imgH = surf.getHeight();
        
        data.assign(imgW, vector<float>() );
        for(auto & d: data) d.assign(imgH, 0);
        
        Surface32f::Iter itr = surf.getIter();
        float extrusion = 300;
        
        while ( itr.line() ) {
            while( itr.pixel() ){
                float yellow = (itr.r() + itr.g())*0.5f;
                //if( threashold < yellow ){
                {
                    Vec2i pos = itr.getPos();
                    data[pos.x][pos.y] = yellow;
                    
                    Vec3f v( pos.x, pos.y, yellow*extrusion- extrusion/2 );
                    Vec3f noise = mPln.dfBm( Vec3f(pos.x, pos.y, yellow) ) * 0.0f;
                    vboOrig.addPos( v + noise );
                    
                    pos *= 1.9f;
                    pos.x += 1000;
                    noise *= 0.15;
                    ColorAf col = mColorSample[pos.x][pos.y];
                    
                    col.r += noise.x*0.5;
                    col.g += noise.y*0.5;
                    col.b += noise.z*0.5;
                    col.a = lmap(yellow, 0.0f, 1.0f, 0.1f, 0.9f);
                    //col = ColorAf(1,1,1,0.8);
                    vboOrig.addCol( col );
                }
            }
        }
        
        printf("w: %d, h:%d\n", imgW, imgH);
        vboOrig.init(true, true, true, GL_POINTS);
        
        float wRange = imgW*0.3;
        float hRange = imgH*0.7;
        
        for (int i=0; i<12;i++) {
            Vec2i ul( randInt(0,imgW),randInt(0,imgH) );
            Vec2i lr = ul + Vec2i( randInt(-wRange, wRange), randInt(-hRange, hRange) );
            Area area( ul, lr );
            vboLine.push_back( VboSet() );
            addIndicesLineHorizontal(vboOrig, vboLine[vboLine.size()-1], 0.4, imgW, imgH, area);
        }
        
        for (int i=0; i<8;i++) {
            Vec2i ul( randInt(0,imgW),randInt(0,imgH) );
            Vec2i lr = ul + Vec2i( randInt(-wRange, wRange), randInt(-hRange, hRange) );
            Area area( ul, lr );
            vboTriMesh.push_back( VboSet() );
            addIndicesTriMesh(vboOrig, vboTriMesh[vboTriMesh.size()-1], 0.5, imgW, imgH, area);
        }

        for (int i=0; i<20; i++) {
            Vec2i ul( randInt(0,imgW),randInt(0,imgH) );
            Vec2i lr = ul + Vec2i( randInt(-wRange, wRange), randInt(-hRange, hRange) );
            Area area( ul, lr );
            vboSpline.push_back( VboSet() );
            addIndiceSpline(vboOrig, vboSpline[vboSpline.size()-1], 0.4, imgW, imgH, area);
        }
        
        for (int i=0; i<16; i++) {
            Vec2i ul( randInt(0,imgW),randInt(0,imgH) );
            Vec2i lr = ul + Vec2i( randInt(-wRange, wRange), randInt(-hRange, hRange) );
            Area area( ul, lr );
            vboPoint.push_back( VboSet() );
            addIndicesPoint(vboOrig, vboPoint[vboPoint.size()-1], 0.5, imgW, imgH, area);
        }
    }
}

void cApp::update(){
    if( !bStart )
        return;
    
}

void cApp::keyDown( KeyEvent event ) {
    char key = event.getChar();
    switch (key) {
        case 'S': mExp.snapShot();      break;
        case 'R': mExp.startRender();   break;
        case 'T': mExp.stopRender();    break;
        case ' ': bStart = !bStart;     break;
    }
}

void cApp::mouseDown( MouseEvent event ){
    camUi.mouseDown( event.getPos() );
}

void cApp::mouseDrag( MouseEvent event ){
    camUi.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void cApp::addIndicesRectMesh( VboSet & vboset, VboSet & ret, float threthold, int w, int h, Area area ){
    for( auto & v: vboset.getPos()) ret.addPos( v );
    for( auto & c: vboset.getCol()) ret.addCol( c );
    
    for( int y=0; y<h; y++){
        for( int x=0; x<w-1; x++){
            if ( area.contains(Vec2i(x, y)) ){
                float d = data[x][y];
                if( d > threthold ){
                    int ind0 = x + y*w;
                    int ind1 = ind0 + 1;
                    ret.addInd(ind0);
                    ret.addInd(ind1);
                }
            }
        }
    }
    
    // vertical line
    for( int x=0; x<w; x++){
        for( int y=0; y<h-1; y++){
            if ( area.contains(Vec2i(x, y)) ){
                float d = data[x][y];
                if( d > threthold ){
                    int ind0 = x + y*w;
                    int ind1 = ind0 + w;
                    ret.addInd(ind0);
                    ret.addInd(ind1);
                }
            }
        }
    }
    ret.init(true, true, true, GL_LINES);
}

void cApp::addIndicesLineHorizontal(VboSet & vboset, VboSet & ret, float threthold, int w, int h, Area area){
    for( auto & v: vboset.getPos()) ret.addPos( v );
    for( auto & c: vboset.getCol()) ret.addCol( c );
    
    for( int y=0; y<h; y++){
        for( int x=0; x<w-1; x++){
            if ( area.contains(Vec2i(x, y)) ){
                float d = data[x][y];
                if( d > threthold ){
                    int ind0 = x + y*w;
                    int ind1 = ind0 + 1;
                    ret.addInd(ind0);
                    ret.addInd(ind1);
                }
            }
        }
    }
    
    ret.init(true, true, true, GL_LINES);
}

void cApp::addIndicesLineVertical( VboSet & vboset, VboSet & ret, float threthold, int w, int h, Area area){
    
    for( auto & v: vboset.getPos()) ret.addPos( v );
    for( auto & c: vboset.getCol()) ret.addCol( c );
    
    // vertical line
    for( int x=0; x<w; x++){
        for( int y=0; y<h-1; y++){
            if ( area.contains(Vec2i(x, y)) ){
                float d = data[x][y];
                if( d > threthold ){
                    int ind0 = x + y*w;
                    int ind1 = ind0 + w;
                    ret.addInd(ind0);
                    ret.addInd(ind1);
                }
            }
        }
    }
    ret.init(true, true, true, GL_LINES);
}

void cApp::addIndicesTriMesh(VboSet & vboset, VboSet & ret, float threthold, int w, int h, Area area){
    
    for( auto & v: vboset.getPos()) ret.addPos( v );
    for( auto & c: vboset.getCol()) ret.addCol( c );
    
    // GL_TRIANGLES
    for( int y=0; y<h-1; y++){
        for( int x=0; x<w-1; x++){
            if ( area.contains(Vec2i(x, y)) ){
                float d = data[x][y];
                if( d > threthold ){
                    int ind0 = x + y*w;
                    int ind1 = ind0 + 1;
                    int ind2 = ind1 + w;
                    ret.addInd(ind0);
                    ret.addInd(ind1);
                    ret.addInd(ind2);
                    
                    int ind3 = ind0;
                    int ind4 = ind3 + w;
                    int ind5 = ind4 + 1;
                    ret.addInd(ind3);
                    ret.addInd(ind4);
                    ret.addInd(ind5);
                }
            }
        }
    }
    
    ret.init(true, true, true, GL_TRIANGLES);
}

void cApp::addIndiceSpline(VboSet & vboset, VboSet & ret,  float threthold, int w, int h, Area area){
    
    // make spline first
    vector<BSpline3f> sps;
    const vector<Vec3f> & pos = vboset.getPos();
    for( int y=0; y<h; y++){
        vector<Vec3f> ctrls;
        for( int x=0; x<w; x++){
            float d = data[x][y];
            if( d > threthold ){
                if ( area.contains(Vec2i(x, y)) ){
                    int index = x + y*w;
                    ctrls.push_back( pos[index] );
                }
            }
        }
        if(ctrls.size()>4)
            sps.push_back(BSpline3f(ctrls, 3, false, true));
    }
    
    int resolution = 1000;
    for( int i=0; i<sps.size(); i++ ){
        for( int j=0; j<resolution-1; j++ ){
            float rate1 = (float)j/resolution;
            float rate2 = (float)(j+1)/resolution;
            Vec3f v1 = sps[i].getPosition( rate1 );
            Vec3f v2 = sps[i].getPosition( rate2 );
            ret.addPos(v1);
            ret.addPos(v2);
   
            ColorAf white(1,1,1,0.4);
            ColorAf black(0,0,0,0.9);
            ret.addCol( white ); ret.addCol( white );
            //ret.addCol( black );ret.addCol( black );

        }
    }
    
    // GL_LINES
    for( int y=0; y<sps.size(); y++){
        int nCp = sps[y].getNumControlPoints();
        if( nCp > 3 ){
            for( int x=0; x<resolution-2; x++){
                int ind0 = x + y*(resolution-1);
                int ind1 = ind0 + 1;
                ret.addInd(ind0);
                ret.addInd(ind1);
            }
        }
    }
    
    ret.init(true, true, true, GL_LINES);
}

void cApp::addIndicesPoint(VboSet & vboset, VboSet & ret, float threthold, int w, int h, Area area){
    
    const vector<Vec3f> & pos = vboset.getPos();
    const vector<ColorAf> & col = vboset.getCol();
    
    for( int y=0; y<h-1; y++){
        for( int x=0; x<w-1; x++){
            if ( area.contains(Vec2i(x, y)) ){
                float d = data[x][y];
                if( d > threthold ){
                    
                    int index = x + y*w;
                    const Vec3f & v = pos[index];
                    const ColorAf & c = col[index];
                    
                    ret.addPos(v);
                    ret.addCol(c);
                }
            }
        }
    }
    
    ret.init(true, true, true, GL_POINTS);
}

void cApp::draw(){
    
    mExp.begin( camUi.getCamera() );{
        gl::clear( ColorA(0,0,0,1) );
        gl::enableAlphaBlending();
        gl::enableDepthRead();
        gl::enableDepthWrite();
        
        if( !mExp.bSnap && !mExp.bRender ){
            mt::drawCoordinate( 100 );
        }
        
        glPointSize(1);
        gl::enableWireframe();
        

        glRotatef(90, 0, 0, 1);
//        glTranslatef(-imgW/2, -imgH/2, 0);
        //vboOrig.draw();
        
        float i=0.1;
        float zscale = 1000;
        for (auto & v: vboLine) {
            if (v.vbo->getNumIndices()>10) {
                //glTranslatef(0, 0, mPln.noise(i) * zscale);
                v.draw();
                i+=0.3;
            }
        }
        
        for (auto & v: vboPoint){
            //glTranslatef(0, 0, mPln.noise(i) * zscale);
            v.draw();
            i+=0.3;
        }
        
        for (auto & v: vboRectMesh){
            if (v.vbo->getNumIndices()>10) {
                //glTranslatef(0, 0, mPln.noise(i) * zscale);
                v.draw();
                i+=0.3;
            }
        }
        
        for (auto & v: vboTriMesh){
            if (v.vbo->getNumIndices()>10) {
                //glTranslatef(0, 0, mPln.noise(i) * zscale);
                v.draw();
                i+=0.3;
            }
        }
        
        for (auto & v: vboSpline){
            if (v.vbo->getNumIndices()>10) {
                //glTranslatef(0, 0, mPln.noise(i) * zscale);
                v.draw();
                i+=0.3;
            }
        }
    
    }mExp.end();
    
    gl::disableWireframe();
    gl::clear( ColorA(1,1,1,1) );
    gl::color( Color::white() );
    mExp.draw();
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
