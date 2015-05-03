#include "ContourMap.h"
#include "cinder/app/AppBasic.h"
#include "cinder/Path2d.h"
#include "cinder/cairo/Cairo.h"
#include "cinder/Rand.h"

using namespace ci;
using namespace ci::app;

ContourMap::ContourMap(){
}

void ContourMap::setImage(ci::Surface32f sur, bool convert2gray=true, cv::Size blurSize=cv::Size(2,2)){
    
    gl::Texture mTex = gl::Texture( sur );
    
    input = toOcv( sur );
    
    if( convert2gray )
        cv::cvtColor( input, input, CV_RGB2GRAY );
    
    cv::blur( input, input, blurSize );
}

void ContourMap::addContour( float threshold ){
    ContourGroup vec;
    vector<cv::Point> points;
    
    /* 
     *      findContour pre process
     *
     *      cv::threshold   - ok
     *      cv::Canny       - error
     *      cv::Sobel       - ?
     */
    
    cv::threshold( input, thresh, threshold, 1.0, CV_THRESH_BINARY );
    thresh.convertTo(thresh, CV_32SC1);
    
    if( thresh.type() == CV_32SC1 ){
        cv::findContours( thresh, vec, CV_RETR_CCOMP, CV_CHAIN_APPROX_NONE, cv::Point(0, 0));
        mCMapData.push_back( vec );
    }else{
        cout << "error : wrong image format " << endl;
    }
}

void ContourMap::drawContourAll(){

    glBegin( GL_POINTS );
    for ( auto & cg : mCMapData )
        for( auto & c : cg )
            for( auto & p : c )
                gl::vertex( fromOcv(p) );

    glEnd();
}

void ContourMap::drawContourGroup( int whichThreshold ){
    
    if( mCMapData.size() <= whichThreshold )
        return;
    
    glBegin( GL_POINTS );
    for( auto & c : mCMapData[whichThreshold] )
        for( auto p : c )
            gl::vertex( fromOcv(p) );
    
    glEnd();
    
}


void ContourMap::exportContour( string path, string fileType="svg" ){
 
    int w = 3000;
    int h = 3000;

    for ( auto & cg : mCMapData ){
        int nMap = &cg - &mCMapData[0];
 
        string fileName = path + "_" + toString(nMap);
        cairo::Context ctx;
        if( fileType == "svg" ){
            ctx = cairo::Context( cairo::SurfaceSvg( getHomeDirectory() / (fileName+=".svg"), w, h ) );
        }else if( fileType == "eps" ){
            ctx = cairo::Context( cairo::SurfaceEps( getHomeDirectory() / (fileName+=".eps"), w, h ) );
        }else if( fileType == "pdf" ){
            ctx = cairo::Context( cairo::SurfacePdf( getHomeDirectory() / (fileName+=".pdf"), w, h ) );
        }else{
            cout << "Error : exportContour, strange fileType of " << fileType << endl;
            return;
        }
        
        cout << "exportContour : " << fileName << endl;
        
        ctx.setSource( ColorAf(0,0,0,1) );
        ctx.setLineWidth(1);
        for( auto & c : cg ){
            ctx.newPath();
            for( auto & p : c )
                ctx.lineTo( p.x, p.y );
            ctx.closePath();
            ctx.stroke();
        }
    }
}