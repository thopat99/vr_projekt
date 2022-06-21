/*****************************************************************************

Copyright (c) 2004 SensAble Technologies, Inc. All rights reserved.

OpenHaptics(TM) toolkit. The material embodied in this software and use of
this software is subject to the terms and conditions of the clickthrough
Development License Agreement.

For questions, comments or bug reports, go to forums at: 
    http://dsc.sensable.com

Module Name:

  SimpleHapticScene.c

Description: 

  This example demonstrates some of the basic building blocks needed for a
  graphics + haptics application. One of the most important aspects is that
  it demonstrates how to map the workspace of the haptic device to
  the view and display a 3D cursor. It also shows how to properly sample and
  display state in the graphics thread that's changing in the haptic thread.

******************************************************************************/

#include <stdio.h>
#include <math.h>

#include <assert.h>

#include <HD/hd.h>
#include <HDU/hdu.h>
#include <HDU/hduError.h>
#include <HDU/hduVector.h>

#define CURL_STATICLIB
#include <curl\curl.h>

#define _USE_MATH_DEFINES // for C
#include <math.h>

#if defined(WIN32)
# include <windows.h>
#endif

#if defined(WIN32) || defined(linux)
# include <GL/glut.h>
#elif defined(__APPLE__)
# include <GLUT/glut.h>
#endif

static HHD ghHD = HD_INVALID_HANDLE;
static HDSchedulerHandle hUpdateDeviceCallback = HD_INVALID_HANDLE;

/* Transform that maps the haptic device workspace to world
   coordinates based on the current camera view. */
static HDdouble workspacemodel[16];

static double kCursorScreenSize = 20.0;
static GLuint gCursorDisplayList = 0;
static double gCursorScale = 1.0;


typedef struct
{
    hduVector3Dd position;
    hduVector3Dd jointAngles;
    hduVector3Dd gimbalAngles;
    //HDint currentButtons;

    HDdouble transform[16];
} HapticDisplayState;

/* Function prototypes. */
void glutDisplay(void);
void glutReshape(int width, int height);
void glutIdle(void);
void glutMenu(int);  

int exitHandler(void);

void initGL();
void initHD();
void initScene();
void drawScene();
void drawCursor(const HapticDisplayState *pState);
void updateWorkspace();

HDCallbackCode HDCALLBACK touchScene(void *pUserData);
HDCallbackCode HDCALLBACK copyHapticDisplayState(void *pUserData);

/*******************************************************************************
 Initializes GLUT for displaying a simple haptic scene.
*******************************************************************************/
int main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    glutInitWindowSize(800, 500);
    glutCreateWindow("Simple Haptic Scene");

    /* Set glut callback functions. */
    glutDisplayFunc(glutDisplay);
    glutReshapeFunc(glutReshape);
    glutIdleFunc(glutIdle);
    
    glutCreateMenu(glutMenu);
    glutAddMenuEntry("Quit", 0);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    /* Provide a cleanup routine for handling application exit. */
    atexit(exitHandler);

    initScene();

    glutMainLoop();

    return 0;
}

/*******************************************************************************
 GLUT callback for redrawing the view 
*******************************************************************************/
void glutDisplay()
{    
    drawScene();
    glutSwapBuffers();
}

/*******************************************************************************
 GLUT callback for reshaping the window. This is the main place where the 
 viewing and workspace transforms get initialized.
*******************************************************************************/
void glutReshape(int width, int height)
{
    static const double kPI = 3.1415926535897932384626433832795;
    static const double kFovY = 40;

    double nearDist, farDist, aspect;
    
    //Check for screen size before updating..
    if (width || height)
    {
    glViewport(0, 0, width, height);

    /* Compute the viewing parameters based on a fixed fov and viewing
       a canonical box centered at the origin. */

    nearDist = 1.0 / tan((kFovY / 2.0) * kPI / 180.0);
    farDist = nearDist + 2.0;
    aspect = (double) width / height;
   
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(kFovY, aspect, nearDist, farDist);

    /* Place the camera down the Z axis looking at the origin. */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();            
    gluLookAt(0, 0, nearDist + 1.0,
              0, 0, 0,
              0, 1, 0);
    
    updateWorkspace();
    }
}

/*******************************************************************************
 GLUT callback for idle state. Use this as an opportunity to request a redraw.
*******************************************************************************/
void glutIdle()
{
    glutPostRedisplay();
}

/******************************************************************************
 Popup menu handler
******************************************************************************/
void glutMenu(int ID)
{
    switch(ID) {
        case 0:
            exit(0);
            break;
    }
}

/*******************************************************************************
 This handler will get called when the application is exiting. This is our
 opportunity to cleanly shutdown the HD API.
*******************************************************************************/
int exitHandler()
{
    hdStopScheduler();
    hdUnschedule(hUpdateDeviceCallback);

    if (ghHD != HD_INVALID_HANDLE)
    {
        hdDisableDevice(ghHD);
        ghHD = HD_INVALID_HANDLE;
    }

    return 0;    
}

/*******************************************************************************
 Initialize the scene.  Handles initializing both OpenGL and HDAPI.
*******************************************************************************/
void initScene()
{
    initGL();
    initHD();
}

/*******************************************************************************
 Setup general OpenGL rendering properties, like lights, depth buffering, etc.
*******************************************************************************/
void initGL()
{
    static const GLfloat light_model_ambient[] = {0.3f, 0.3f, 0.3f, 1.0f};
    static const GLfloat light0_diffuse[] = {0.9f, 0.9f, 0.9f, 0.9f};   
    static const GLfloat light0_direction[] = {0.0f, -0.4f, 1.0f, 0.0f};    
    
    /* Enable depth buffering for hidden surface removal. */
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    
    /* Cull back faces. */
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    
    /* Other misc features. */
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);    
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light_model_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, light0_direction);
    glEnable(GL_LIGHT0);   
}

/*******************************************************************************
 Initializes the HDAPI.  This involves initing a device configuration, enabling
 forces, and scheduling a haptic thread callback for servicing the device.
*******************************************************************************/
void initHD()
{
    HDErrorInfo error;
    ghHD = hdInitDevice(HD_DEFAULT_DEVICE);
    if (HD_DEVICE_ERROR(error = hdGetError())) 
    {
        hduPrintError(stderr, &error, "Failed to initialize haptic device");
        fprintf(stderr, "\nPress any key to quit.\n");
        getchar(); 
        exit(-1);
    }


    hdEnable(HD_FORCE_OUTPUT);
        
    hUpdateDeviceCallback = hdScheduleAsynchronous(
        touchScene, 0, HD_MAX_SCHEDULER_PRIORITY);

    hdStartScheduler();
    if (HD_DEVICE_ERROR(error = hdGetError()))
    {
        hduPrintError(stderr, &error, "Failed to start the scheduler");
        exit(-1);
    }
}

/*******************************************************************************
 Uses the current OpenGL viewing transforms to initialize a transform for the
 haptic device workspace so that it's properly mapped to world coordinates.
*******************************************************************************/
void updateWorkspace()
{
    HDdouble screenTworkspace;
    GLdouble modelview[16];
    GLdouble projection[16];
    GLint viewport[4];

    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    /* Compute the transform for going from device coordinates to world
       coordinates based on the current viewing transforms. */
    hduMapWorkspaceModel(modelview, projection, workspacemodel);

    /* Compute the scale needed to display the cursor of a particular size
       in screen coordinates. */
    screenTworkspace = hduScreenToWorkspaceScale(
        modelview, projection, viewport, workspacemodel);

    gCursorScale = kCursorScreenSize * screenTworkspace;
}

/*******************************************************************************
 The main routine for displaying the scene.  Gets the latest snapshot of state
 from the haptic thread and uses it for displaying a 3D cursor.  Also, draws the
 anchor visual if the anchor is presently active.
*******************************************************************************/
void drawScene()
{
    HapticDisplayState state;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);        
    
    /* Obtain a thread-safe copy of the current haptic display state. */
    hdScheduleSynchronous(copyHapticDisplayState, &state,
                          HD_DEFAULT_SCHEDULER_PRIORITY);


    drawCursor(&state);

    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT);
    glPushMatrix();
    
    glEnable(GL_COLOR_MATERIAL);
    glColor3f(0.0, 1.0, 0.0);

    glTranslatef(-1.0, -1.0, 0.0);
    glRotatef(180, 0.0, 1.0, 0.0);
    
    glutSolidCube(0.2);
    
    glPopMatrix(); 
    glPopAttrib();

    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT);
    glPushMatrix();

    glEnable(GL_COLOR_MATERIAL);
    glColor3f(1.0, 1.0, 1.0);

    glutSolidSphere(0.01, 32, 32);

    glPopMatrix();
    glPopAttrib();

}

void sendData(const HapticDisplayState* pState)
{
    int base_deg = pState->jointAngles[0] * 180 / M_PI + 90;
    int shoulder_deg = pState->jointAngles[1] * 180 / M_PI + 90;
    int elbow_deg = pState->jointAngles[2] * 180 / M_PI;
    int wrist_rot_deg = pState->gimbalAngles[0] * 180 / M_PI + 90;
    int wrist_vert_deg = pState->gimbalAngles[1] * 180 / M_PI + 90;
    int grip_deg = pState->gimbalAngles[2] * 180 / M_PI + 90;


    fprintf(stdout, "Current Position: (%i, %i, %i) \n",
        pState->position[0],
        pState->position[1],
        pState->position[2]);

    fprintf(stdout, "Current Gimbal Angles (original): (%i, %i, %i) \n",
        wrist_vert_deg,
        wrist_rot_deg,
        grip_deg);

    // Convert angles of the input device to angles for the robot arm.
    base_deg = mapInterval(base_deg, 0, 180, 0, 180);
    // shoulder_deg = (shoulder_deg - 50) * (90 - 15) / (205 - 50) + 15;
    shoulder_deg = mapInterval(shoulder_deg, 50, 205, 15, 90);
    // elbow_deg = (elbow_deg + 21) * (90 - 0) / (167 + 21) + 0;
    elbow_deg = mapInterval(elbow_deg, -21, 167, 0, 90);
    // wrist_vert_deg = (wrist_vert_deg - 20) * (180 - 0) / (280 - 20) + 0;
    wrist_vert_deg = mapInterval(wrist_vert_deg, 40, 230, 0, 180);
    // wrist_rot_deg = (wrist_rot_deg + 90) * (0 - 180) / (395 - 85) + 180;
    wrist_rot_deg = mapInterval(wrist_rot_deg, -60, 230, 0, 180);
    // grip_deg = (grip_deg + 58) * (73 - 10) / (270 + 58) + 10;
    grip_deg = mapInterval(grip_deg, -58, 250, 10, 73);

    fprintf(stdout, "Current Joint Angles (mapped): (%i, %i, %i) \n",
        base_deg,
        shoulder_deg,
        elbow_deg);

    fprintf(stdout, "Current Gimbal Angles (mapped): (%i, %i, %i) \n",
        wrist_vert_deg,
        wrist_rot_deg,
        grip_deg);


    CURL* curl = curl_easy_init();
    if (curl) {
        char getParams[100];
        sprintf(getParams, "192.168.3.141:4200/?command=10,%d,%d,%d,%d,%d,%d",
            base_deg, shoulder_deg, elbow_deg, wrist_vert_deg, wrist_rot_deg, grip_deg);
        CURLcode res;
        curl_easy_setopt(curl, CURLOPT_URL, getParams);
        fprintf(stdout, "URL: %s", getParams);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
}

/*******************************************************************************
 Draws a 3D cursor for the haptic device using the current local transform,
 the workspace to world transform and the screen coordinate scale.
*******************************************************************************/
void drawCursor(const HapticDisplayState *pState)
{
    static const double kCursorRadius = 0.5;
    static const double kCursorHeight = 1.5;
    static const int kCursorTess = 15;

    GLUquadricObj *qobj = 0;

    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT);
    glPushMatrix();

    if (!gCursorDisplayList)
    {
        gCursorDisplayList = glGenLists(1);
        glNewList(gCursorDisplayList, GL_COMPILE);
        qobj = gluNewQuadric();
               
        gluCylinder(qobj, 0.0, kCursorRadius, kCursorHeight,
                            kCursorTess, kCursorTess);
            glTranslated(0.0, 0.0, kCursorHeight);
            gluCylinder(qobj, kCursorRadius, 0.0, kCursorHeight / 5.0,
                            kCursorTess, kCursorTess);
    
        gluDeleteQuadric(qobj);
        glEndList();
    }
    
    /* Apply the workspace view transform. */
    glMultMatrixd(workspacemodel);

    /* Apply the local transform of the haptic device. */
    glMultMatrixd(pState->transform);

    /* Apply the local cursor scale factor. */
    glScaled(gCursorScale, gCursorScale, gCursorScale);

    glEnable(GL_COLOR_MATERIAL);
    glColor3f(0.0, 0.5, 1.0);

    

    glCallList(gCursorDisplayList);

    glPopMatrix(); 
    glPopAttrib();

    sendData(pState);
}

/*******************************************************************************
 Draws a 3D cursor for the haptic device using the current local transform,
 the workspace to world transform and the screen coordinate scale.
*******************************************************************************/
//void drawCube()
//{
//    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT);
//    glPushMatrix();
//
//    glEnable(GL_COLOR_MATERIAL);
//    glColor3f(0.0, 0.5, 1.0);
//
//    glutSolidCube(0.5);
//
//    glPopMatrix(); 
//    glPopAttrib();
//}



/*******************************************************************************
 Use this scheduler callback to obtain a thread-safe snapshot of the haptic
 device state that we intend on displaying.
*******************************************************************************/
HDCallbackCode HDCALLBACK copyHapticDisplayState(void *pUserData)
{
    HapticDisplayState *pState = (HapticDisplayState *) pUserData;

    hdGetDoublev(HD_CURRENT_POSITION, pState->position);
    hdGetDoublev(HD_CURRENT_TRANSFORM, pState->transform);
    hdGetDoublev(HD_CURRENT_JOINT_ANGLES, pState->jointAngles);
    hdGetDoublev(HD_CURRENT_GIMBAL_ANGLES, pState->gimbalAngles);
    //hdGetIntegerv(HD_CURRENT_BUTTONS, pState->currentButtons);

    return HD_CALLBACK_DONE;
}

/*******************************************************************************
 This is the main haptic rendering callback. If the button is pressed, it will extract
 the joint and gimbal angles and send it to the robot arm.
*******************************************************************************/
HDCallbackCode HDCALLBACK touchScene(void *pUserData)
{
    static const HDdouble kAnchorStiffness = 0.2;

    int currentButtons, lastButtons;
    hduVector3Dd position;
    hduVector3Dd jointAngles;
    hduVector3Dd gimbalAngles;
    HDErrorInfo error;

    hdBeginFrame(ghHD);
    
    hdGetIntegerv(HD_CURRENT_BUTTONS, &currentButtons);
    hdGetIntegerv(HD_LAST_BUTTONS, &lastButtons);

    /* Detect button state transitions. */
    /* if ((currentButtons & HD_DEVICE_BUTTON_1) != 0 &&
        (lastButtons & HD_DEVICE_BUTTON_1) == 0)
    {*/
    //if ((currentButtons & HD_DEVICE_BUTTON_1) != 0) {
    //    
    //    
    //    hdGetDoublev(HD_CURRENT_JOINT_ANGLES, jointAngles);
    //    hdGetDoublev(HD_CURRENT_GIMBAL_ANGLES, gimbalAngles);

    //    // Convert angles to degree scale
    //    int base_deg = jointAngles[0] * 180 / M_PI + 90;
    //    int shoulder_deg = jointAngles[1] * 180 / M_PI + 90;
    //    int elbow_deg = jointAngles[2] * 180 / M_PI;
    //    int wrist_rot_deg = gimbalAngles[0] * 180 / M_PI + 90;
    //    int wrist_vert_deg = gimbalAngles[1] * 180 / M_PI + 90;
    //    int grip_deg = gimbalAngles[2] * 180 / M_PI + 90;

    //    fprintf(stdout, "Current Gimbal Angles (original): (%i, %i, %i) \n",
    //        wrist_vert_deg,
    //        wrist_rot_deg,
    //        grip_deg);

    //    // Convert angles of the input device to angles for the robot arm.
    //    base_deg = mapInterval(base_deg, 0, 180, 0, 180);
    //    // shoulder_deg = (shoulder_deg - 50) * (90 - 15) / (205 - 50) + 15;
    //    shoulder_deg = mapInterval(shoulder_deg, 50, 205, 15, 90);
    //    // elbow_deg = (elbow_deg + 21) * (90 - 0) / (167 + 21) + 0;
    //    elbow_deg = mapInterval(elbow_deg, -21, 167, 0, 90);
    //    // wrist_vert_deg = (wrist_vert_deg - 20) * (180 - 0) / (280 - 20) + 0;
    //    wrist_vert_deg = mapInterval(wrist_vert_deg, 40, 230, 0, 180);
    //    // wrist_rot_deg = (wrist_rot_deg + 90) * (0 - 180) / (395 - 85) + 180;
    //    wrist_rot_deg = mapInterval(wrist_rot_deg, -60, 230, 0, 180);
    //    // grip_deg = (grip_deg + 58) * (73 - 10) / (270 + 58) + 10;
    //    grip_deg = mapInterval(grip_deg, -58, 250, 10, 73);

    //    fprintf(stdout, "Current Joint Angles (mapped): (%i, %i, %i) \n",
    //        base_deg,
    //        shoulder_deg,
    //        elbow_deg);

    //    fprintf(stdout, "Current Gimbal Angles (mapped): (%i, %i, %i) \n",
    //        wrist_vert_deg,
    //        wrist_rot_deg,
    //        grip_deg);


    //    CURL* curl = curl_easy_init();
    //    if (curl) {
    //        char getParams[100];
    //        sprintf(getParams, "192.168.3.141:4200/?command=10,%d,%d,%d,%d,%d,%d",
    //            base_deg, shoulder_deg, elbow_deg, wrist_vert_deg, wrist_rot_deg, grip_deg);
    //        CURLcode res;
    //        curl_easy_setopt(curl, CURLOPT_URL, getParams);
    //        fprintf(stdout, "URL: %s", getParams);
    //        res = curl_easy_perform(curl);
    //        curl_easy_cleanup(curl);
    //    }

    //    

    //    
    //}
   
    hdEndFrame(ghHD);

    if (HD_DEVICE_ERROR(error = hdGetError()))
    {
        if (hduIsForceError(&error))
        {
            printf(stdout, "Maximum force was exceeded!\n");
        }
        else
        {
            /* This is likely a more serious error, so bail. */
            hduPrintError(stderr, &error, "Error during haptic rendering");
            exit(-1);
        }
    }

    return HD_CALLBACK_CONTINUE;
}

int mapInterval(int value, int A, int B, int a, int b) {
    int result = (value - A) * (b - a) / (B - A) + a;
    if (result < a) {
        return a;
    }
    if (result > b) {
        return b;
    }
    return result;
}