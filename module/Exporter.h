#pragma once

#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/Camera.h"
#include "ufUtil.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class Exporter{
    
public:
    
    bool bRender;
    bool bSnap;
    int mFrame;
    int mExitFrame;
    gl::Fbo mFbo;
    fs::path mRenderPath;
    ImageTarget::Options mImgWOption;
    string snapFileName;
    Exporter(){}
    
    void setup( int width, int height, int exitFrame, GLenum colorInternalFormat, fs::path path, int aaSample, bool aFlip=false ){
        bRender = false;
        bSnap = false;
        mFrame = 1;
        mExitFrame = exitFrame;
        gl::Fbo::Format format;
        format.enableDepthBuffer( false );
        format.enableMipmapping( false );
        format.setTarget( GL_TEXTURE_RECTANGLE_ARB );
        format.setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );
        format.setMinFilter( GL_NEAREST );
        format.setMagFilter( GL_NEAREST );
        format.setColorInternalFormat( colorInternalFormat );
        format.setSamples( aaSample );
        mFbo = gl::Fbo( width, height, format );
        mFbo.getTexture(0).setFlipped(aFlip);
        mRenderPath = path;
        
        mImgWOption.quality(1);
        
        stringstream ss;
        ss << "setup FBO Renderer " << width << " x " << height << "px\n";
        ss << "exitFrame " << exitFrame << ", AntiAlias " << aaSample << "\n";
        ss << "renderPath " << mRenderPath;
        cout << ss.str() << endl;
    }
    
    void begin(){
        gl::pushMatrices();
        //gl::SaveFramebufferBinding bindingSaver;
        gl::setViewport( mFbo.getBounds() );
        
        mFbo.bindFramebuffer();
        gl::setMatricesWindow( mFbo.getSize() );
        gl::scale(1,-1,1);
        gl::translate( 0, -mFbo.getHeight() );
    }
    
    void begin( const Camera & cam){
        gl::pushMatrices();
        //gl::SaveFramebufferBinding bindingSaver;
        gl::setViewport( mFbo.getBounds() );
        
        mFbo.bindFramebuffer();
        gl::setMatrices( cam );
    }
    
    void end(){
        mFbo.unbindFramebuffer();
        gl::popMatrices();
        
        if( bRender || bSnap ){
            string frame_name = "f_" + toString( mFrame ) + ".png";

            if( bSnap && snapFileName!=""){
                frame_name = snapFileName;
            }
            writeImage( mRenderPath/frame_name,  mFbo.getTexture());
            cout << "Render Image : " << mFrame << endl;
            
            if( mExitFrame <= mFrame ){
                if( bSnap ){
                    cout << "Finish Snapshot " << frame_name << endl;
                }else{
                    cout << "Finish Rendering " << mFrame << " frames" << endl;
                    exit(1);
                }
            }
            
            bSnap = false;
            mFrame++;
        }
    }
    
    void startRender(){
        bRender = true;
        mFrame = 1;
    }
    
    void stopRender(){
        bRender = false;
        cout << "Stop Render : f_" << mFrame << endl;
    }
    
    void snapShot( string fileName=""){
        bSnap = true;
        snapFileName = fileName;
    }
    
    void draw(){
        gl::pushMatrices();
        gl::setMatricesWindow( mFbo.getSize() );
        gl::setViewport(getWindowBounds() );
        gl::draw( mFbo.getTexture() );
        gl::popMatrices();
    }
};

