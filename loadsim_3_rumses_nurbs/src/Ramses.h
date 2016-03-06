#pragma once

#include <iostream>
#include <fstream>
#include "mtUtil.h"
#include "cinder/Perlin.h"

using namespace ci;
using namespace ci::app;

class Particle{

public:
    Vec3f pos;
    ColorAf col;
    float dist;
    float rho_map;
    double raw;
};

class Ramses{
    
public:
    
    Ramses( int _simType, int _prmType){
        eSimType = _simType;
        ePrm = _prmType;
        
        bShow = false;
        bPolar = true;
        bAutoMinMax = false;
        extrude = 200.0f;
        scale = 100;
        in_min = -9.0;
        in_max = 3.0;
        visible_thresh = 0.1f;
        eStretch = 1;
        xoffset = yoffset = zoffset = 0;
        inAngle = -180;
        outAngle = 180;
        rotateSpeed = 0.0f;
        offsetRotateAngle = 0.0f;
        loadPlotData();        
        
        mPln.setSeed(12345);
        mPln.setOctaves(4);
        
    }

    Perlin mPln;
    
    void loadPlotData(){
        if( pR.size() != 0) return;
            
        string simu_name = simType[eSimType];
        
        // load .r, .thata
        vector<string> exts = { ".r", ".theta"};
        for( auto & ext : exts ){            
            string path = (simRootDir/"plot"/(simu_name + ext)).string();
            cout << "start loading " << ext << " file : " << path << "...";
            std::ifstream is( path, std::ios::binary );
            if(is){ cout << " done" << endl;
            }else{ cout << " ERROR" << endl;}
            
            is.seekg (0, is.end);
            int fileSize = is.tellg();
            is.seekg (0, is.beg);
            int arraySize = arraySize = fileSize / sizeof(double);
            
            vector<double> & vec = (ext == ".r")? pR : pTheta;
            vec.clear();
            vec.assign(arraySize, double(0));
            is.read(reinterpret_cast<char*>(&vec[0]), fileSize);
            is.close();
            
            printf( "arraySize %d, %e~%e\n\n", (int)vec.size(), vec[0], vec[vec.size()-1]);
        }
        
//        double Rmin = pR[0];
//        double Rmax = pR[pR.size()-1];
//        for( auto& p : pR ){
//            p = lmap(p, Rmin, Rmax, 0.0, 1.0 );
//        }
        
        boxelx = pR.size();
        boxely = pTheta.size();
    }
    
    bool loadSimData( int _frame ){
        
        frame = _frame;
        
        if( frame>endFrame[eSimType]){
            return false;
        }
        
        string fileName = (simType[eSimType] + "_polar_" +stretch[eStretch]+ "_" + prm[ePrm] + "_00" + to_string(frame) + ".bin");
        fs::path dir = simRootDir/simType[eSimType]/stretch[eStretch]/prm[ePrm];
        fs::path path = dir/fileName;
        
        std::ifstream is( path.string(), std::ios::binary );
        if(is){
            // cout << " done" << endl;
        }else{
            cout << " ERROR" << endl;
            return false;
        }
        
        // get length of file:
        is.seekg ( 0, is.end );
        int fileSize = is.tellg();
        is.seekg ( 0, is.beg );
        
        arraySize = arraySize = fileSize / sizeof(double);

        //data.clear();
        if(data.size()==0)
            data.assign(arraySize, double(0));
        is.read( reinterpret_cast<char*>(&data[0]), fileSize );
        is.close();
        return true;
    }
    
//    void move(){
//        {
//            auto itr = vbo->mapAttrib3f( geom::POSITION );
//            for( int i=0; i<pos.size(); i++ ){
//                
//                {
//                    float val = filData[i];
//                    val = pow(val,5.0f);
//                    val = max(0.1f, val);
//                    vec2 n2 = mPln.dfBm(0.1+i*0.01f, 0.1+val*0.01);
//                    float n = n2.x + n2.y;
//                    float vel = (n>0)?-abs(val):abs(val);
//                    pos[i].z += vel*0.01*0.25;
//                }
//                
//                Vec3f p = pos[i];
//
//                if(bPolar){
//                    p.x = p.x*scale+xoffset;
//                    p.y = p.y*scale+yoffset;
//                    p.z = p.z*extrude + zoffset;
//                }else{
//                    p.x = p.x + xoffset;
//                    p.y = p.y + yoffset;
//                    p.z = p.z * extrude + zoffset;
//                }
//                
//                p *= globalScale;
//                *itr++ = p;
//            }
//            itr.unmap();
//        }
//
//        {
//            auto itr = vbo->mapAttrib4f( geom::COLOR);
//            for( int i=0; i<col.size(); i++ ){ *itr++ = col[i];}
//            itr.unmap();
//        }
//    }
    
    void updateVbo( int res, const Vec3f & eye ){
        if( data.size()==0 ) return;
        
        float inRad = toRadians(inAngle);
        float outRad = toRadians(outAngle);
        
        if( bAutoMinMax ){
            in_min = numeric_limits<double>::max();
            in_max = numeric_limits<double>::min();
            
            for( auto d : data ){
                in_min = min( in_min, d );
                in_max = max( in_max, d );
            }
        }
        
        part.clear();
        filData.clear();
        
        for( int j=0; j<boxely; j+=res ){
            for( int i=0; i<boxelx; i+=res ){
                
                int index = j + i*boxely;
                
                double rho_raw = data[index];
                float rho_map = lmap(rho_raw, in_min, in_max, 0.0, 1.0);
                
                //if( visible_thresh<rho_map && rho_map <1.00 ){
                if( 0.0<rho_map && rho_map <=1.00 ){
                    
                    filData.push_back(rho_map);
                    
                    float hue = lmap( rho_map, visible_thresh, 1.0f, 0.83f, 0.0f);
                    if( bPolar ){
                        double r = pR[i];
                        double theta = pTheta[j];
                        
                        if( inRad<=theta&&theta<=outRad){
                            
                            double x = r * cos( theta );
                            double y = r * sin( theta );
                            
                            rho_map -= visible_thresh;
                            Vec3f p = Vec3f( x, y, rho_map);
                            float angle = offsetRotateAngle + rotateSpeed*(frame-100);
                            p = glm::rotateZ( p, toRadians(angle) );
                            
                            Particle pt;
                            pt.pos = p;
                            pt.col = ColorAf(CM_HSV, hue, 0.8f, 0.7f);
                            pt.rho_map = rho_map;
                            pt.raw = rho_raw;
                            
                            Vec3f tmp = p;
                            tmp.x = p.x*scale+xoffset;
                            tmp.y = p.y*scale+yoffset;
                            tmp.z = p.z*extrude + zoffset;
                            tmp *= globalScale;
                            pt.dist = glm::distance(tmp, eye);
                            part.push_back(pt);
                        }
                    }else{
                        Particle pt;
                        Vec3f p(i, j, rho_map);
                        pt.pos = p;
                        pt.col = ColorAf(CM_HSV, hue, 0.8f, 0.7f);
                        pt.rho_map = rho_map;
                        pt.raw = rho_raw;
                        Vec3f tmp = p;
                        tmp.x = p.x + xoffset;
                        tmp.y = p.y + yoffset;
                        tmp.z = p.z * extrude + zoffset;
                        pt.dist = glm::distance(tmp, eye);
                        part.push_back(pt);
                    }
                }
            }
        }
        
        // sort based on eye
        std::sort(part.begin(), part.end(), [&eye](const Particle&lp, const Particle&rp){ return lp.dist > rp.dist;} );
        
        
        gl::VboMesh::Layout play, clay;
        play.usage(GL_STATIC_DRAW).attrib(geom::POSITION,3);
        clay.usage(GL_STATIC_DRAW).attrib(geom::COLOR,4);
        
        vbo.reset();
        vbo = gl::VboMesh::create( part.size(), GL_POINTS, {play, clay} );
        {
            auto itr = vbo->mapAttrib3f( geom::POSITION );
            for( int i=0; i<part.size(); i++ ){
                
                Vec3f p = part[i].pos;
                if(bPolar){
                    p.x = p.x*scale+xoffset;
                    p.y = p.y*scale+yoffset;
                    p.z = p.z*extrude + zoffset;
                }else{
                    p.x = p.x + xoffset;
                    p.y = p.y + yoffset;
                    p.z = p.z * extrude + zoffset;
                }
                
                p *= globalScale;
                *itr++ = p;
            }
            itr.unmap();
        }
        {
            auto itr = vbo->mapAttrib4f( geom::COLOR);
            for( int i=0; i<part.size(); i++ ){ *itr++ = part[i].col;}
            itr.unmap();
        }
        
        nParticle = vbo->getNumVertices();
        visible_rate = nParticle/(float)arraySize*100.0f;
       
    }
    
    
    void draw(){
        if(vbo && bShow ){
            gl::draw(vbo);
        }
    }

    vector<double> data;
    
    vector<Particle> part;
    vector<float> filData;
    
    
    int frame;
    bool bShow;
    bool bAutoMinMax;
    bool bPolar;
    float extrude;

    float xoffset;
    float yoffset;
    float zoffset;
    float scale;
    double in_min;
    double in_max;
    int nParticle;
    int eSimType;
    int ePrm;
    int eStretch;
    float visible_thresh;
    float visible_rate;
    int arraySize;
    
    float inAngle, outAngle;
    float rotateSpeed;
    float offsetRotateAngle;
    
    gl::VboMeshRef vbo;
    
    const fs::path simRootDir = mt::getAssetPath()/fs::path("sim/supernova");
    static vector<string> simType;
    static vector<string> stretch;
    static vector<string> prm;
    static vector<int> endFrame;

    static vector<double> pR, pTheta;
    static int boxelx, boxely;
    static float globalScale;
};


vector<string>  Ramses::simType = {"simu_1", "simu_2", "simu_3","simu_4","simu_5" };
vector<string>  Ramses::stretch = { "linear", "log" };
vector<string>  Ramses::prm = {"rho", "vx", "vy", "S", "P", "c"};
vector<double>  Ramses::pR;
vector<double>  Ramses::pTheta;
vector<int>     Ramses::endFrame = { 650, 350, 452, 514, 750 };
int             Ramses::boxelx = -123;
int             Ramses::boxely = -123;
float           Ramses::globalScale = 1.0f;

