#pragma once

/*
 *      Import binary file for RF particle simulation
 *      No framework needed.
 */

#include <stdio.h>
#include <iostream>
#include <vector>

using namespace std;

class RfImporterBin{
    
public:
    
    void load( string fileName, vector<float> &position, vector<float> &velocity ){
        
        FILE * pFile;
        int     verificationCode;
        char    fluidName[250];
        short   version;
        float   scale;
        int     fluidType;
        float   elapTime;
        int     frameNum;
        int     fps;
        int     numParticles;
        float   radius;
        float   pressure[3];
        float   speed[3];
        float   temperature[3];
        float   emPosition[3];
        float   emRotation[3];
        float   emScale[3];
        
        pFile = fopen( fileName.c_str(), "rb");
        fread( &verificationCode,  sizeof(int),        1,   pFile );
        fread( fluidName,          sizeof(char),     250,   pFile );
        fread( &version,           sizeof(short),      1,   pFile );
        fread( &scale,             sizeof(float),      1,   pFile );
        fread( &fluidType,         sizeof(int),        1,   pFile );
        fread( &elapTime,          sizeof(float),      1,   pFile );
        fread( &frameNum,          sizeof(int),        1,   pFile );
        fread( &fps,               sizeof(int),        1,   pFile );
        fread( &numParticles,      sizeof(int),        1,   pFile );
        fread( &radius,            sizeof(float),      1,   pFile );
        fread( &pressure,          sizeof(float),      3,   pFile );
        fread( &speed,             sizeof(float),      3,   pFile );
        fread( &temperature,       sizeof(float),      3,   pFile );
        fread( &emPosition,        sizeof(float),      3,   pFile );
        fread( &emRotation,        sizeof(float),      3,   pFile );
        fread( &emScale,           sizeof(float),      3,   pFile );
        
        position.clear();
        velocity.clear();
        position.assign( numParticles*3, 0.0f );
        velocity.assign( numParticles*3, 0.0f );
        
        // Per Particle Data
        for( int i=0; i<numParticles; i++ ){
            float pPosition[3];
            float pVelocity[3];
            float pForce[3];
            float pVorticity[3];
            float pNormal[3];
            int   pNumNeihbors;
            float pTexVec[3];
            short pInfoBit;
            float pElapsedTime;
            float pIsoTime;
            float pViscosity;
            float pDensity;
            float pPressure;
            float pMass;
            float pTemperature;
            int   pId;
            
            fread( pPosition,      sizeof(float),      3,  pFile );
            fread( pVelocity,      sizeof(float),      3,  pFile );
            fread( pForce,         sizeof(float),      3,  pFile );
            fread( pVorticity,     sizeof(float),      3,  pFile );
            fread( pNormal,        sizeof(float),      3,  pFile );
            fread( &pNumNeihbors,  sizeof(int),        1,  pFile );
            fread( pTexVec,        sizeof(float),      3,  pFile );
            fread( &pInfoBit,      sizeof(short),      1,  pFile );
            fread( &pElapsedTime,  sizeof(float),      1,  pFile );
            fread( &pIsoTime,      sizeof(float),      1,  pFile );
            fread( &pViscosity,    sizeof(float),      1,  pFile );
            fread( &pDensity,      sizeof(float),      1,  pFile );
            fread( &pPressure,     sizeof(float),      1,  pFile );
            fread( &pMass,         sizeof(float),      1,  pFile );
            fread( &pTemperature,  sizeof(float),      1,  pFile );
            fread( &pId,           sizeof(int),        1,  pFile );
            
            position[i*3+0] = pPosition[0];
            position[i*3+1] = pPosition[1];
            position[i*3+2] = pPosition[2];
            
            velocity[i*3+0] = pVelocity[0];
            velocity[i*3+1] = pVelocity[1];
            velocity[i*3+2] = pVelocity[2];
        }
        
        // Ignore Footer
        /*
        int additioanl_data_per_particle;
        bool RF4_internal_data;
        bool RF5_internal_data;
        int dummyInt;
        
        fread( &additioanl_data_per_particle, sizeof(int), 1, pFile );
        fread( &RF4_internal_data, sizeof(bool),       1, pFile );
        fread( &RF5_internal_data, sizeof(bool),       1, pFile );
        fread( &dummyInt,          sizeof(int),        1, pFile );
        */
  
        fclose(pFile);
    }
};