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

#include <iostream>
#include <fstream>
#include <stdio.h>

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
    void loadSimData( fs::path path, vector<vector<float>>& array );
    void makeVboPt( vector<vector<float>>& array );
    void makeVboLn();
    void makeVboBrd();
    void updateVbo();
    
    fs::path assetDir;
    MayaCamUI camUi;
    Perlin mPln;
    Exporter mExp;
    
    vector<vector<float>> array;
    vector<vector<Vec3f>> data3d;
    vector<vector<Vec3f>> data2d;
    vector<vector<Vec3f>> dataNow;
    vector<vector<ColorAf>> dataCol;

    vector<gl::VboMeshRef> vboPt;
    vector<gl::VboMeshRef> vboLn;
    gl::VboMeshRef vboBrd;
    
    bool bStart = false;
    int frame = 0;
};

void cApp::setup(){
    setWindowPos( 0, 0 );
    setWindowSize( 1920*0.4, 1080*3*0.4 );
    mExp.setup( 1920, 1080*3, 0, 1000, GL_RGB, mt::getRenderPath(), 0);
    
    CameraPersp cam( 1920, 1080*3, 54.4f, 0.1, 10000 ); //35mm
    cam.lookAt( Vec3f(0,50,0), Vec3f(0,0,0) );
    cam.setCenterOfInterestPoint( Vec3f(0,0,0) );
    camUi.setCurrentCam( cam );
    
    mPln.setSeed(123);
    mPln.setOctaves(4);
    
    assetDir = mt::getAssetPath();
    array.assign( 10, vector<float>() );
    
    data2d.assign(4, vector<Vec3f>() );
    data3d.assign(4, vector<Vec3f>() );
    dataNow.assign(4, vector<Vec3f>() );
    dataCol.assign(4, vector<ColorAf>() );
    
    string frame[] = {"00468", "01027", "01398"};
    loadSimData( assetDir/"sim"/"garaxy"/"bin"/"rho"/("rdr_"+frame[2]+"_l17.hydro_rho.bin"), array );
    makeVboPt( array );
    makeVboLn();
    makeVboBrd();
    
    array.clear();
    
#ifdef RENDER
    mExp.startRender();
#endif

}

void cApp::loadSimData(fs::path path, vector<vector<float>> & array){

    /*
        array[0,1,2]    pos x,y,z (kpc)
        array[3,4,5]    vel x,y,z (km/s-1)
        array[6]        rho, density   (cm-3)
        array[7]        N, AMR level
        array[8]        mass
        array[9]        temparatur (K)
     */
    
    cout << "loading binary file : " << path << endl;
    std::ifstream is( path.string(), std::ios::binary );
    if(is){
        ccout::b("load OK binary file");
    }else{
        ccout::r("load ERROR bin file");
        quit();
    }

    // get length of file:
    is.seekg (0, is.end);
    int fileSize = is.tellg();
    cout << "file size : " << fileSize << " byte" << endl;
    is.seekg (0, is.beg);
    
    int arraySize = fileSize / sizeof(float);
    cout << "found " << arraySize << " float numbers" << endl;

    vector<float> tmparray;
    tmparray.assign(arraySize, float(0) );
    is.read(reinterpret_cast<char*>(&tmparray[0]), fileSize);
    is.close();

    int numElem = arraySize/array.size();
    for( int i=0; i<array.size(); i++ ){
        array[i].assign( numElem, 0);
    }
    
    for( int y=0; y<array[0].size(); y++ ){
        
        //cout << "array " << y;
        
        for( int x=0; x<array.size(); x++ ){

            int id = y*array.size() + x;
            float data = tmparray[id];
            array[x][y] = data;
            //cout << " " << data;
        }
        //cout << endl;
    }
    
    tmparray.clear();
}

void cApp::makeVboPt(vector<vector<float> > &array){

    /*
     
        sample value
        array[212229]
            pos  : -2.63672 -12.4023 -13.5742
            vel  : -200.979 -408.506 69.3852
            rho  : 1.31536
            N    : 10
            mass : 316173
            K    : 1035.92
     */

    
    vector<float> prmMin;
    vector<float> prmMax;
    prmMin.assign( 4, numeric_limits<float>::max() );
    prmMax.assign( 4, numeric_limits<float>::min() );
    
    float N_min = numeric_limits<float>::max();
    float N_max = numeric_limits<float>::min();
    
    // store min, max of each param
   for( int i=0; i<array[0].size(); i++){
       float vx = array[3][i];
       float vy = array[4][i];
       float vz = array[5][i];
       float vlen = sqrt(vx*vx + vy*vy + vz*vz );
       float rho = array[6][i];
       float N = array[7][i];
       float mass = array[8][i];
       float K = array[9][i];

       prmMin[0] = MIN( prmMin[0], rho );
       prmMax[0] = MAX( prmMax[0], rho );

       prmMin[1] = MIN( prmMin[1], vlen );
       prmMax[1] = MAX( prmMax[1], vlen );

       prmMin[2] = MIN( prmMin[2], mass );
       prmMax[2] = MAX( prmMax[2], mass );

       prmMin[3] = MIN( prmMin[3], K );
       prmMax[3] = MAX( prmMax[3], K );
       
       N_min = MIN(N_min, N);
       N_max = MAX(N_max, N);
   }


    for( int i=0; i<4; i++ ){
        printf("prm[%d]  : %e ~ %e\n", i, prmMin[i], prmMax[i]);
    }
    
    
//    float rho_th = 0.6;
//    float v_th = 0.4;
//    float mass_th = 0.85;
//    float K_th = 0.3;

    vector<float> threshold = {  0.6f,  0.36f,   0.55f,  0.55f };
    vector<float> extrude   = { 20.0f,  50.0f,   30.0f,  5.0f };
    float alpha = 0.6;
    float offset_2d = 10;

    for( int i=0; i<array[0].size(); i++){
        
        // just read
        float x = array[0][i];
        float y = array[1][i];
        float z = array[2][i];
        float vx = array[3][i];
        float vy = array[4][i];
        float vz = array[5][i];
        float vlen = sqrt( vx*vx + vy*vy + vz*vz ) - prmMin[1];

        float rho = array[6][i] - prmMin[0];
        float mass = array[8][i] - prmMin[2];
        float K = array[9][i] - prmMin[3];
        float N = array[7][i] - N_min;

        float cellSize = 200.0f/N;
        
        // POSITION
        Vec3f pos( x, y, z );
        Vec3f noise = mPln.dfBm( x,y,z ) * 1.2;
        pos *= 0.5;
        pos += noise * lmap(cellSize, N_min, N_max, 0.0f, 1.0f)*0.5;


        vector<float> param{ rho, vlen, mass, K };
        vector<float> map{0,0,0,0};
        vector<float> log{0,0,0,0};
        
        for( int j=0; j<4; j++ ){

            map[j] = lmap( param[j], 0.0f, prmMax[j]-prmMin[j], 0.0f, 1.0f);
            log[j] = lmap( log10(param[j]), 0.0f, log10(prmMax[j]-prmMin[j]), 0.0f, 1.0f);
            
            if( threshold[j] < log[j]){
             
                data2d[j].push_back( Vec3f(x, y, log[j]-threshold[j]*extrude[j] - offset_2d) );
                data3d[j].push_back( pos );
                dataNow[j].push_back( pos );

                ColorAf col;

                switch ( j ) {
                    case 0: col = ColorAf( 1.0f-log[0]*0.5, log[0]*0.5+map[3], log[0]*0.5+map[1], alpha );  break;
                    case 1: col = ColorAf( 0.2, log[1]+0.1, log[3]-log[0]*0.7, alpha );                     break;
                    case 2: col = ColorAf( 0.4, 0.8f-log[2]*1.2, 0.1+map[2]*10, alpha );                    break;
                    case 3: col = ColorAf( log[3]*0.4, map[3]*0.5, 0.1f, alpha );                           break;
                }
                
                Vec3f n = mPln.dfBm( j+x*0.5, y*0.5, z*0.1 );
                col.r += n.x * 0.1;
                col.g += n.y * 0.1;
                col.b += n.z * 0.1;

                dataCol[j].push_back(col);
            }
        }
    }
    
    gl::VboMesh::Layout layout = mt::getVboLayout();

    for( int j=0; j<data2d.size(); j++ ){
        gl::VboMeshRef vbo = gl::VboMesh::create( data3d[j].size(), 0, layout, GL_POINTS );
        vboPt.push_back( vbo );
        
        gl::VboMesh::VertexIter itr = vboPt[j]->mapVertexBuffer();
        
        for( int i=0; !itr.isDone(); i++ ){
            itr.setPosition( data3d[j][i] );
            itr.setColorRGBA( dataCol[j][i] );
            ++itr;
        }
    }

    float pnum = array[0].size();
    for( int i=0; i<data3d.size(); i++ ){
        printf("VBO[%d]  : %8d verts, %0.2f%% visible\n", i, (int)vboPt[i]->getNumVertices(), vboPt[i]->getNumVertices()/pnum*100.0 );
    }
}

void cApp::makeVboLn(){
    
    for( auto v : vboLn ) v->reset();
    vboLn.clear();
    
    for (int i=0; i<vboPt.size(); i++ ) {

        // prepare data in vector
        vector<Vec3f> ps;
        vector<ColorAf> cs;

        int tryNum = 10000;
        for( int j=0; j<tryNum; j++){
            int nVerts = vboPt[i]->getNumVertices();
            int id1 = randInt( 0, nVerts-1 );
            int id2 = randInt( 0, nVerts-1 );
            if(id1 == id2 ) continue;
            Vec3f & p1 = dataNow[i][id1];
            Vec3f & p2 = dataNow[i][id2];
            ColorAf &c1 = dataCol[i][id1];
            ColorAf &c2 = dataCol[i][id2];
            float dist = p1.distance(p2);
            if( 1<dist && dist<3 ){
                ps.push_back( p1 );
                ps.push_back( p2 );
                cs.push_back( ColorAf(1,1,1,1) - c2 );
                cs.push_back( ColorAf(1,1,1,1) - c1 );
            }
        }
        
        gl::VboMesh::Layout layout;
        layout.setStaticColorsRGBA();
        layout.setStaticPositions();
        layout.setStaticIndices();
        gl::VboMeshRef ref = gl::VboMesh::create( ps.size(), 0, layout, GL_LINES );
        ref->bufferPositions( ps );
        ref->bufferColorsRGBA( cs );
        vboLn.push_back( ref );
    }
}

void cApp::makeVboBrd(){
    
    if( vboBrd ) vboBrd->reset();
    
    vector<Vec3f> ps;
    vector<ColorAf> cs;
    
    int tryNum = 10000;
    for (int i=0; i<tryNum; i++ ) {
    
        int vboId1 = randInt(0, 3+1);
        int vboId2 = randInt(0, 3+1);
        if( vboId1 == vboId2 ) continue;
        
        gl::VboMeshRef vbo1 = vboPt[vboId1];
        gl::VboMeshRef vbo2 = vboPt[vboId2];
        
        int vertId1 = randInt(0, vbo1->getNumVertices()-1 );
        int vertId2 = randInt(0, vbo2->getNumVertices()-1 );

        Vec3f & p1 = dataNow[vboId1][vertId1];
        Vec3f & p2 = dataNow[vboId2][vertId2];
        
        float dist = p1.distance( p2 );
        if( 1<dist && dist<3 ){
            
            ColorAf & c1 = dataCol[vboId1][vertId1];
            ColorAf & c2 = dataCol[vboId2][vertId2];
            ColorAf cMix = c1*0.5 + c2*0.5;
            cMix.a = 0.4;
            ps.push_back( p1 );
            ps.push_back( p2 );
            cs.push_back( cMix );
            cs.push_back( cMix );
        }
    }
    
    gl::VboMesh::Layout layout;
    layout.setStaticColorsRGBA();
    layout.setStaticPositions();
    layout.setStaticIndices();
    vboBrd = gl::VboMesh::create( ps.size(), 0, layout, GL_LINES );
    vboBrd->bufferPositions( ps );
    vboBrd->bufferColorsRGBA( cs );
}

void cApp::updateVbo(){
    
    frame++;
    float maxFrame = 50;
    float rate = frame/maxFrame;
    rate = MIN(rate, 1.0f);
    
    for( int j=0; j<data3d.size(); j++ ){
        gl::VboMesh::VertexIter itr = vboPt[j]->mapVertexBuffer();
        for( int i=0; !itr.isDone(); i++ ){
            Vec3f dir = data2d[j][i] - data3d[j][i];
            Vec3f dist = data2d[j][i] - *itr.getPositionPointer();
            if( dist.length()>1 ){
                dir.normalize();
                Vec3f p = data3d[j][i] + dir*frame;
                //Vec3f p = vecRho2d[i]*rate + vecRho[i]*( 1.0f-rate );
                itr.setPosition( p );
                dataNow[j][i] = p;
                
                if( 0 ){
                    const CameraPersp & cam = camUi.getCamera();
                    Vec2f sp = cam.worldToScreen( p, mExp.mFbo.getWidth(), mExp.mFbo.getHeight() );
                    sp.y = mExp.mFbo.getHeight() - sp.y;
                    if ( 1080*2<sp.y && 1920/2<sp.x ) {
                        itr.setColorRGBA( ColorAf(1,1,1.1) );
                    }else{
                        itr.setColorRGBA( dataCol[j][i] );
                    }
                }
            }
            ++itr;
        }
    }
}

void cApp::update(){
    if( bStart ){
        updateVbo();
        makeVboLn();
        makeVboBrd();
    }
}

void cApp::draw(){
    
    mExp.begin( camUi.getCamera() );{
        gl::clear( ColorA(0,0,0,1) );
        gl::enableAlphaBlending();
    
        // data
        gl::draw( vboLn[0] );
        gl::draw( vboLn[1] );
        gl::draw( vboLn[2] );
        gl::draw( vboLn[3] );
        
        glLineWidth(1);
       if( vboBrd ) gl::draw( vboBrd );
        
        gl::draw( vboPt[0] );
        gl::draw( vboPt[1] );
        gl::draw( vboPt[2] );
        gl::draw( vboPt[3] );
        
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
    gl::color( Color::white() );
    mExp.draw();

}

void cApp::keyDown( KeyEvent event ) {
    char key = event.getChar();
    switch (key) {

        case 'S':
            mExp.startRender();
            break;

        case 's':
            mExp.snapShot();
            break;

        case ' ':
            bStart = !bStart;
            break;
    }
}

void cApp::mouseDown( MouseEvent event ){
    camUi.mouseDown( event.getPos() );
}

void cApp::mouseDrag( MouseEvent event ){
    camUi.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
