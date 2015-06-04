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
        int month = local.tm_mon;
        int day = local.tm_mday;
        int hours = local.tm_hour;
        int min = local.tm_min;
        int sec = local.tm_sec;
        
        char stamp[16];
        sprintf(stamp, "%02d%02d_%02d%02d_%02d", month+1, day, hours, min, sec);
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
    
    fs::path getProjectPath(){
        return expandPath("../../../");
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
  
    
    void drawScreenGuide(){

        float w = getWindowWidth();
        float h = getWindowHeight();
        gl::pushMatrices();
        gl::setMatricesWindow( getWindowSize() );
        glBegin( GL_LINES ); {
            glColor3f( 1,1,1 );
            glVertex3f( w*0.5, 0, 0 );
            glVertex3f( w*0.5, h, 0 );
            glVertex3f( 0, h*0.5, 0 );
            glVertex3f( w, h*0.5, 0 );
        } glEnd();
        gl::popMatrices();
    }
 }