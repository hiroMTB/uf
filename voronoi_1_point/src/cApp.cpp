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
#include "boost/polygon/voronoi.hpp"
#include "boost/polygon/segment_data.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;

using boost::polygon::voronoi_builder;
using boost::polygon::voronoi_diagram;
typedef boost::polygon::point_data<float> vPoint;
typedef boost::polygon::segment_data<float> vSegment;
typedef voronoi_diagram<double>::cell_type cell_type;
typedef voronoi_diagram<double>::edge_type edge_type;
typedef voronoi_diagram<double>::vertex_type vertex_type;

class cApp : public AppNative {
    
public:
    void setup();
    void keyDown( KeyEvent event );
    void update();
    void draw();
  	void resize();
    
    int mWin_w = 1920;
    int mWin_h = 1080;
    vector<vPoint> vPs;
    vector<vSegment> vSegs;
    
    voronoi_diagram<double> vd;
};

void cApp::setup(){
    setWindowPos( 0, 0 );
    setWindowSize( mWin_w, mWin_h );

    for(int i=0; i<20; i++){
        vPs.push_back( vPoint(randFloat()*mWin_w, randFloat()*mWin_h) );
    }
    
//    for(int i=0; i<5; i++){
//        vPoint v1(randFloat()*mWin_w, randFloat()*mWin_h);
//        vPoint v2 = v1;
//        v2.x( v2.x() + randFloat()*100 );
//        v2.y( v2.y() + randFloat()*100 );
//        vSegs.push_back( vSegment(v1, v2) );
//    }
    
    construct_voronoi(vPs.begin(), vPs.end(), &vd);
    //construct_voronoi(vPs.begin(), vPs.end(), vSegs.begin(), vSegs.end(), &vd);
}


void cApp::update(){
}

void cApp::draw(){
    gl::clear(ColorA(1,1,1,1));
    gl::setMatricesWindow( getWindowSize() );
    gl::translate( 0, 0 ,0 );
    glScalef(0.5, 0.5, 0.5);
    
    gl::color(1, 0, 0);
    gl::lineWidth(1);
    gl::begin(GL_LINES);
    voronoi_diagram<double>::const_cell_iterator it = vd.cells().begin();
    for (; it!= vd.cells().end(); ++it) {

        const cell_type& cell = *it;
        const edge_type* edge = cell.incident_edge();
    
        do{
            if(edge->is_primary()){
                if( edge->is_finite() ){
                    if (edge->cell()->source_index() < edge->twin()->cell()->source_index()){
                        float x0 = edge->vertex0()->x();
                        float y0 = edge->vertex0()->y();
                        float x1 = edge->vertex1()->x();
                        float y1 = edge->vertex1()->y();
                        glVertex3f(x0, y0, 0);
                        glVertex3f(x1, y1, 0);
                    }
                }else{
                    const vertex_type * v0 = edge->vertex0();
                    if( v0 ){
                        vPoint p1 = vPs[edge->cell()->source_index()];
                        vPoint p2 = vPs[edge->twin()->cell()->source_index()];
                        float x0 = edge->vertex0()->x();
                        float y0 = edge->vertex0()->y();
                        float end_x = (p1.y() - p2.y()) * mWin_w;
                        float end_y = (p1.x() - p2.x()) * -mWin_w;
                        glVertex3f(x0, y0, 0);
                        glVertex3f(end_x, end_y, 0);
                    }
                }
            }
            edge = edge->next();
        }while (edge != cell.incident_edge());
    }
    glEnd();

    
    // draw Point
    glPointSize(3);
    gl::color(0, 0, 1);
    glBegin(GL_POINTS);
    for( auto v : vPs )
        glVertex3f( v.x(), v.y(), 0);
    glEnd();

//    // draw Segment
//    gl::lineWidth(3);
//    glBegin(GL_LINES);
//    for( auto s : vSegs ){
//        glVertex3f(s.low().x(), s.low().y(), 0);
//        glVertex3f(s.high().x(), s.high().y(), 0);
//    }
    glEnd();
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

CINDER_APP_NATIVE( cApp, RendererGl(0) )
