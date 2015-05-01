#pragma once

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Vbo.h"

using namespace ci;
using namespace ci::app;
using namespace std;

typedef std::function<float (float)> EaseFunc;

namespace uf {
    
    gl::VboMesh::Layout getVboLayout(){
        gl::VboMesh::Layout layout;
        layout.setDynamicColorsRGBA();
        layout.setDynamicPositions();
        layout.setStaticIndices();
        return layout;
    }
    
    
    string getTimeStamp(){        
        time_t curr;
        tm local;
        time(&curr);
        local =*(localtime(&curr));
        int yday = local.tm_yday;
        int hours = local.tm_hour;
        int min = local.tm_min;
        int sec = local.tm_sec;
        
        char stamp[16];
        sprintf(stamp, "%03d_%02d%02d_%02d", yday, hours, min, sec);
        return string(stamp);
    }
    
    string getTimeStampU(){
        return toString( time(NULL) );
    }
 
    
    void renderScreen( fs::path path, int frame ){
        string frame_name = "f_" + toString( frame ) + ".png";
        writeImage( path/frame_name, copyWindowSurface() );
        cout << "Render Image : " << frame << endl;
    }
    
    fs::path getRenderPath(){
        return expandPath("../../../_render/")/getTimeStamp();
    }
    
    float distanceToLine(Ray ray, Vec3f point){
        return cross(ray.getDirection(), point - ray.getOrigin()).length();
    }
    
    void fillSurface( Surface16u & sur, const ColorAf & col){

        Surface16u::Iter itr = sur.getIter();
        while (itr.line() ) {
            while( itr.pixel()){
                //setColorToItr( itr, col );
                sur.setPixel(itr.getPos(), col);
            }
        }
    }
    
    
    void drawCoordinate( float length=100 ){
        glBegin( GL_LINES ); {
            glColor3f( 1, 0, 0 );
            glVertex3f( 0, 0, 0 );
            glVertex3f( length, 0, 0 );
            glColor3f( 0, 1, 0 );
            glVertex3f( 0, 0, 0 );
            glVertex3f( 0, length, 0 );
            glColor3f(  0, 0, 1 );
            glVertex3f( 0, 0, 0 );
            glVertex3f( 0, 0, length );
        } glEnd();
    }
  
    // this does work but strange behavior !!
    /*
     inline ColorAf getColorFromItr( const Surface16u::Iter & itr ){
     return ColorAf(itr.r(), itr.g(), itr.b(), itr.a() );
     }
     */
    
    // this does not work!!
    /*
     inline void setColorToItr( Surface16u::Iter & itr, const ColorAf & col ){
     itr.r() = col.r;
     itr.g() = col.g;
     itr.b() = col.b;
     itr.a() = col.a;
     }
     */
    
 }