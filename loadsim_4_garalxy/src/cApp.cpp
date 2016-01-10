#include "cinder/app/AppNative.h"
#include "cinder/Utilities.h"
#include "mtUtil.h"
#include "ConsoleColor.h"
#include "cinder/Xml.h"

#include <iostream>
#include <fstream>
#include <stdio.h>

#define USE_GZIP
#ifdef USE_GZIP
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#endif

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
    
public:
    void setup();

    void loadSimData_ascii( fs::path path, vector<vector<float>> & array );
#ifdef USE_GZIP
    void loadSimData_gzip( fs::path path, vector<vector<float>> & array );
#endif
    void convert2bin( fs::path renderTopDir, string frameName, vector<vector<float>> &array );
    
    void loadingSpeedTest();
    
};

void cApp::setup(){
    setWindowPos( 0, 0 );
    setWindowSize( 100,100 );

    //loadingSpeedTest();
    
    fs::path assetDir = mt::getAssetPath();
    fs::path simDir = assetDir/"sim"/"garaxy";
    
    vector<fs::path> simFilePathList;
    
#ifndef USE_GZIP
    fs::recursive_directory_iterator itr(simDir/"ascii"), eof;
#else
    fs::recursive_directory_iterator itr(simDir/"original_gz"), eof;
#endif
    while ( itr!= eof ) {
        
        fs::path filePath = itr->path();
        if( fs::is_regular_file( filePath ) ){
            simFilePathList.push_back( filePath );
        }
        ++itr;
    }
    
    printf("found %d files\n", (int)simFilePathList.size() );
    
    fs::path renderTopPath = assetDir/"sim"/"garaxy"/"bin/";
    createDirectories( renderTopPath );

    {
        for( auto path : simFilePathList ){
            cout << endl;
            vector< vector<float> > array;
            array.assign(10, vector<float>() );
#ifndef USE_GZIP
            loadSimData_ascii( path, array );
#else
            loadSimData_gzip( path, array );
#endif
            convert2bin( renderTopPath, path.filename().string(), array);
        }
    }
    
    cout << "\nQUIT\n" << endl;
    quit();
}

void cApp::loadSimData_ascii( fs::path path, vector<vector<float>> & array ){
    
    printf("start loading ASCII sim file : %s", path.filename().string().c_str() );
    
    std::ifstream pFile( path.string() );
    
    string line;
    int row,col;
    
    if ( pFile.is_open() ){
        row=0;
        
        while(!pFile.eof()) {
            getline(pFile, line);
            stringstream ss(line);
            col=0;
            float data;
            while( ss >> data ){
                array[col].push_back( data );
                col++;
            }
            row++;
        }
        
        printf(" - DONE\n");
        printf("found %d lines, close file\n", row);
        pFile.close();

    }else{
        cout << "Unable to open file";
    }
    
}

#ifdef USE_GZIP
void cApp::loadSimData_gzip( fs::path path, vector<vector<float>> & array ){
    
    printf("start loading GZIP sim file : %s", path.filename().string().c_str() );
    
    std::ifstream file(path.string().c_str(), std::ios_base::in | std::ios_base::binary);
    try {
        boost::iostreams::filtering_istream in;
        in.push(boost::iostreams::gzip_decompressor());
        in.push(file);
        
        std::string line;
        int row, col;
        
        while( std::getline(in, line) ){
            stringstream ss(line);
            col = 0;
            float data;
            while ( ss >> data ) {
                array[col].push_back( data );
                col++;
            }
            row++;
        }
        printf(" - DONE\n");
        printf("found %d lines, close file\n", row);
    }
    catch(const boost::iostreams::gzip_error& e) {
        std::cout << e.what() << '\n';
    }
}
#endif

void cApp::loadingSpeedTest(){
    fs::path assetDir = mt::getAssetPath();
    fs::path simDir = assetDir/"sim"/"garaxy";
    string frameName = "rdr_01027_l17.hydro";
    fs::path asciiPath = simDir/"ascii"/frameName;
    fs::path gzipPath = simDir/"original_gz"/(frameName+".gz");
    
    vector< vector<float> > array;
    array.assign(10, vector<float>() );
    
    {
        mt::timer_start();
        loadSimData_ascii( asciiPath, array );
        mt::timer_end();
    }
    
    printf("\n\n");
    
    for( auto a : array ) a.clear();
    
    sleep(100.0f);
    
#ifdef USE_GZIP
    {
        mt::timer_start();
        loadSimData_gzip( gzipPath, array );
        mt::timer_end();
    }
#endif
    quit();
    
}

void cApp::convert2bin( fs::path renderTopDir, string frameName,vector<vector<float>> & array ){
    
    /*
     
        + calculate min, max value
        + re-make pos, vel, vel_length vector
     
     */
    int row = array[0].size();
    vector<float> min, max;
    min.assign( 11, numeric_limits<float>::max() );
    max.assign( 11, numeric_limits<float>::min() );

    vector<float> v_pos;
    vector<float> v_vel;
    vector<float> v_vlen;

    for( int i=0; i<row; i++){
        float posx = array[0][i];
        float posy = array[1][i];
        float posz = array[2][i];
        v_pos.push_back( posx );
        v_pos.push_back( posy );
        v_pos.push_back( posz );
        
        float vx = array[3][i];
        float vy = array[4][i];
        float vz = array[5][i];
        float vlen = sqrt( vx*vx + vy*vy + vz*vz );
        v_vel.push_back( vx );
        v_vel.push_back( vy );
        v_vel.push_back( vz );
        v_vlen.push_back( vlen );
        
        for( int j=0; j<min.size()-1; j++ ){
            min[j] = MIN(min[j], array[j][i] );
            max[j] = MAX(max[j], array[j][i] );
        }
        
        min[10] = MIN(min[10], vlen);
        max[10] = MAX(max[10], vlen);
    }
    
    /*
     
        save min-max XML file
     
     */
    {
        printf("start writing XML min-max file ");

        fs::path xmlDir = renderTopDir / "_settings/";
        createDirectories( xmlDir );
        fs::path xmlFileName = frameName + ".settings.xml";
        string xmlFilePath = (xmlDir/xmlFileName).string();

        
        vector<string> xmlPrmName = { "pos.x", "pos.y", "pos.z", "vel.x", "vel.y", "vel.z", "rho", "N", "mass", "K", "vel_len" };
        
        XmlTree xml( "settings", "" );
        xml.push_back( XmlTree( "particle_num", std::to_string(array[6].size())) );
        
        for (int i=0; i<xmlPrmName.size(); i++ ) {
            
            std::ostringstream min_s; min_s << std::scientific << min[i];
            std::ostringstream max_s; max_s << std::scientific << max[i];
            
            XmlTree prm( xmlPrmName[i], "");
            prm.push_back( XmlTree( "min", min_s.str() ) );
            prm.push_back( XmlTree( "max", max_s.str() ) );
            xml.push_back( prm );
            printf(".");
        }
        xml.write( writeFile(xmlFilePath) );
        
        printf("  DONE\n");
    }
    
    /*
     
        save binary file
     
     */
    {
        printf("start writing binary files ");

        map<string, vector<float>& > binPrms{
            pair<string, vector<float>& >( "pos", v_pos ),
            pair<string, vector<float>& >( "vel", v_vel ),
            pair<string, vector<float>& >( "rho", array[6] ),
            pair<string, vector<float>& >( "N", array[7] ),
            pair<string, vector<float>& >( "mass", array[8] ),
            pair<string, vector<float>& >( "K", array[9] ),
            pair<string, vector<float>& >( "vel_length", v_vlen ),
        };
        
        map<string, vector<float>& >::iterator itr = binPrms.begin();
        for( ; itr!=binPrms.end(); itr++ ) {
     
            string prmName = itr->first;
            vector<float> & data = itr->second;
            
            fs::path sPrmDir = renderTopDir/ (prmName + "/");
            createDirectories( sPrmDir );
            fs::path sPath = sPrmDir/(frameName+"_"+prmName+".bin");
            FILE * wFile = fopen( sPath.string().c_str(), "wb");
            fwrite( data.data(), sizeof(float), data.size(), wFile );
            fclose( wFile );
            printf(".");
        }
        
        printf("  DONE\n");
    }
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
