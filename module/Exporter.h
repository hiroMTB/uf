//
//  cs_rnd.h
//  gradient
//
//  Created by MatobaHiroshi on 2/2/15.
//
//

#pragma once

#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
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
    
    Exporter(){}
    
    void setup( int width, int height, int exitFrame, GLenum colorInternalFormat, fs::path path, int aaSample ){
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
        mRenderPath = path;
        
        mImgWOption.quality(1);
        
        stringstream ss;
        ss << "setup FBO Renderer " << width << " x " << height << "px\n";
        ss << "exitFrame " << exitFrame << ", AntiAlias " << aaSample << "\n";
        ss << "renderPath " << mRenderPath;
        cout << ss.str() << endl;
    }
    
    
    void begin(){
        mFbo.bindFramebuffer();
    }
    
    void end(){
        mFbo.unbindFramebuffer();
        
        if( bRender || bSnap ){
            Surface16u sur( mFbo.getTexture() );
            string frame_name = "f_" + toString( mFrame ) + ".png";
            writeImage( mRenderPath/frame_name,  sur);
            cout << "Render Image : " << mFrame << endl;
            mFrame++;
            
            if( mExitFrame <= mFrame ){
                cout << "Finish Rendering " << mFrame << " frames" << endl;
                exit(1);
            }
            
            bSnap = false;
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
    
    void snapShot(){
        bSnap = true;
    }
    
    void draw(){
        gl::pushMatrices();
        gl::setMatricesWindow( mFbo.getSize() );
        gl::draw( mFbo.getTexture() );
        gl::popMatrices();
    }
};
