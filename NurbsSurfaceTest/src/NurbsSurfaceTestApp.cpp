#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/Camera.h"
#include "cinder/MayaCamUI.h"
#include "cinder/Rand.h"

#include <OpenGL/glu.h>

using namespace ci;
using namespace ci::app;
using namespace std;

GLUnurbsObj *theNurb;
GLfloat ctlpoints[4][4][3];
GLfloat knots[8] = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};

class gluTestApp : public AppBasic {
public:
    void setup();
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void resize();
    void update();
    void draw();
    MayaCamUI mMayaCam;
    float mCameraDistance;
    Vec3f mLastEye, mEye, mCenter, mUp;
};

void nurbsError(GLenum errorCode)
{
    const GLubyte *estring;
    
    estring = gluErrorString(errorCode);
    //	fprintf (stderr, “Nurbs Error: %s\n”, estring);
    exit (0);
}

void gluTestApp::setup()
{
    setWindowSize(1280, 800);
    setWindowPos(0, 0);
    
    // SETUP CAMERA
    mCameraDistance = 500.0f;
    mEye = Vec3f( 0.0f, 0.0f, mCameraDistance );
    mCenter = Vec3f::zero();
    mUp = Vec3f::yAxis();
    CameraPersp cam;
    cam.setEyePoint(mEye);
    cam.setCenterOfInterestPoint(mCenter);
    cam.setPerspective( 60.0f, getWindowAspectRatio(), 5.0f, 5000.0f );
    mMayaCam.setCurrentCam( cam );
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_AUTO_NORMAL);
    glEnable(GL_NORMALIZE);
    
    theNurb = gluNewNurbsRenderer();
    gluNurbsProperty(theNurb, GLU_SAMPLING_TOLERANCE, 25.0);
    gluNurbsProperty(theNurb, GLU_DISPLAY_MODE, GLU_OUTLINE_POLYGON ); //GLU_FILL);
    gluNurbsCallback(theNurb, GLU_ERROR, (GLvoid (*)()) nurbsError);

    int u, v;
    for (u = 0; u < 4; u++) {
        for (v = 0; v < 4; v++) {
            ctlpoints[u][v][0] = 2.0*((GLfloat)u - 1.5) + randFloat(-1,1);
            ctlpoints[u][v][1] = 2.0*((GLfloat)v - 1.5) + randFloat(-1,1);
            ctlpoints[u][v][2] = randFloat(-4,4);
        }
    }
}

void gluTestApp::update()
{
    gl::setMatrices( mMayaCam.getCamera() );
}

void gluTestApp::draw()
{
    gl::clear( Color( 0, 0, 0 ) );
    CameraPersp cam = mMayaCam.getCamera();
    gl::setMatrices(cam);
    gl::scale(Vec3f(100.0f, 100.0f, 100.0f));
    gluBeginSurface(theNurb);
    gluNurbsSurface(theNurb,
                    8, knots, 8, knots,
                    4 * 3, 3, &ctlpoints[0][0][0], 
                    4, 4, GL_MAP2_VERTEX_3);
    gluEndSurface(theNurb);
}


void gluTestApp::resize()
{
    CameraPersp cam = mMayaCam.getCamera();
    cam.setAspectRatio( getWindowAspectRatio() );
    mMayaCam.setCurrentCam( cam );
}
void gluTestApp::mouseDown( MouseEvent event )
{
    mMayaCam.mouseDown(event.getPos());
}
void gluTestApp::mouseDrag( MouseEvent event )
{
    mMayaCam.mouseDrag(event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown());
}
CINDER_APP_BASIC( gluTestApp, RendererGl )
