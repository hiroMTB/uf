#include "ContourMap.h"


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

