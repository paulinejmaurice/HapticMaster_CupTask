#ifndef DISPLAY_H_INCLUDED
#define DISPLAY_H_INCLUDED

//#include <windows.h> 
#include "stdlib.h" // needed otherwise conflict with glut
#include <GL/glut.h>
#include <string>
#include <queue>
#include <vector>
#include <iostream>
#include <fstream>
#include "time.h"
#include "math.h"
#include "model.h"
#include "haptic.h"

// Define status
#define INITIALIZING 0
#define GOTONEXT 1
#define WAITFORSTART 2
#define STARTMOTION 3
#define INITIATEMOTION 4
#define INMOTION 5
#define TERMINATEMOTION 6
#define ENDOFTRIAL 7
#define END 8

#define M_PI 3.1415926

class Display
{
	// Methods
public:
	// Constructor
	Display(int mainLoopPeriod, int mainLoopTimerID, std::string nameOfBlock, int nbTrialsInBlock, double goalTimeForTrial, double floorHeight, double startToTargetDistance, double arcCup, double lengthPendulum, double massPendulum, double dampingPendulum, double pendulumInitAngle, double pendulumInitVelocity, double inertiaOfHM, double accuracyFactor = 1.5, double accelerationAmplification = 1.0, bool ballCanEscape = true, bool autoStart = false, bool dampMotion = false, double scalingFactorVisual = 1., double cupAddScalingFactorVisual = 1., bool projector = false, bool sound = true, int pX = 0, int pY = 1, int pZ = 2);
	// Destructor 
	~Display();

	// This Is A Timer Function Which Call The HapticMASTER To Get The New EndEffector Position
	void Timer(int iTimer);

	// This Function Is Called By OpenGl WhenEver A Key Was Hit
	void Keyboard(unsigned char ucKey, int iX, int iY);

	// The Function Is Called By OpenGL Whenever The Window Is Resized
	void Reshape(int iWidth, int iHeight);

	// This Function Is CAlled By OpenGL To Redraw The Scene Here's Where You Put The EndEffector And Block Drawing FuntionCalls
	void UpdateDisplay(void);

	// Affect glut functions and Initializes the OpenGl Graphics Engine
	int Initialize(int argc, char** argv);

	// Start OpenGL main loop
	void LaunchLoop();

	// Set parameters for a perturbation in the motion
	// If this function is called, perturbation can happen in the movement. If not perturbation is wanted in this block, do not call this function
	void SetPerturbationParameters(double duration, double magnitude, bool randomDirection, bool randomDistance, bool randomEvent,  bool visible = false, int direction = 1, double distance = 0.5);

	// Attributes
private:

	// Static is needed for glut callback function
	static void InternalTimer(int iTimer);
	static void InternalReshape(int iWidth, int iHeight);
	static void InternalKeyboard(unsigned char ucKey, int iX, int iY);
	static void InternalUpdateDisplay(void);

	void DrawFloor();
	void DrawBall(GLfloat color[3]);
	void DrawCup(GLfloat color[3]);
	// Draw a square representing the target to reach (updated at each new trial)
	void DrawTargetBlock();
	// Draw a square representing the start position 
	void DrawStartBlock();
	// Draw a moving square representing the time remaining to perform the motion
	void DrawTimingBox(double heightPosition, GLfloat color[3]);
	// Display a text on the screen
	void DrawStatus(std::string text, GLfloat color[3], double position);
	// Display a vertical line to indicate where the perturbation will happen (can be activated or not)
	void DrawPerturbation();
	// Display the cup and ball
	void DrawWindow(GLfloat currentStateColor[3], bool drawTimingBox, double timingBoxHeight = 0.);
	// Re-initialize perturbation for next trial
	void ResetPerturbation();
	// Write motion data in a file
	void WriteDataInFile();
	void StartRecording();
	void StopRecording();
	void RecordMotionData();
	void ClearDataBuffer();

	// Task parameter
	Haptic *pHaptic; // interaction with the HapticMaster
	Model *pModel; // mathematical model of the cup-task (cart-pendulum)
			
	int posX, posY, posZ; // define axes orientation
	
	int loopPeriod; // Timing parameter for the control loop
	int loopTimerID;
		
	int status;	
	bool autoStartMode; // whether the time starts when the visual/auditive cue is given (auto) or when you want and start moving the HM (non-auto)
	bool canBallEscape; // whether the ball can escape or not
	bool ballEscape; // whether the ball has escaped or not
	bool canEndMotionBeDamped; // whether damping is added to help stop the motion when cup is inside target box
	bool isEndMotionDamped;
	bool isWaitingAtTarget;
	int maxNbTrials; // Nb of trials in one block
	int trialNb;
		
	double targetAccuracyFactor; // ratio between the target width and the cup width
	double accelerationAmplificationFactor;
	
	double gravity;
	double arcOfCup; // (rad) angle of the whole circle which is kept for the cup
	double cupWidth; // (m) Horizontal width of the cup (used to scale ball and target width)
	double cupHeight; // (m) Vertical height of the cup (used to choose vertical target block size)
	double inertiaHM; // Include cup and ball?
	double pendulumMass; // (kg)
	double pendulumDamping; // (N.m.s)
	double pendulumLength; // (m)
	double pendulumInitialAngle; // (rad)
	double pendulumInitialVelocity; // (rad)
	
	unsigned __int64 timerFrequency;
	double goalTime; // (s) for one trial
	double currentTime;
	double startTime; // time at which you can start to move (unlock HM and cue to start) and teh recording start
	double userStartTime; // time when the user really starts moving the HM (if in automatic-start mode, it is equal to startTime). Used for computing score 	
	double startWaitTime;
	double escapeTime;  // in case ball escape, time when it happens (used to compute and show the motion of the free flying ball, just for visual effect)
	double startPerturbationTime;
	double motionDuration;
	double durationWaitForStart;
	double durationDisplaySuccess;
	double durationAfterEndMotion;
	double durationWaitAtTarget;
	
	int axisOfMotion;
	double targetWidth;
	double timingBoxStartHeight;
	double startPosition[3];
	double targetPosition[3];	
	double escapePosition[3]; // in case ball escape, 3D Cartesian position in the cup at time of escape (used to compute and show the motion of the free flying ball, just for visual effect)
	double escapeVelocity[3]; // in case ball escape, 3D Cartesian velocity at time of escape (used to compute and show the motion of the free flying ball, just for visual effect)

	double distanceTolerance; // distance to target
	double velocityTolerance; // compared to zero
	
	int trialScore;
	int totalScore;
	int scoreFailure;  // minimal score (negative) you get if you lose the ball or if you are very far from the time constraint
	int scoreFullSuccess; // score you would get if you exactly respect the time constraint
	
	bool playSound; 
	const char* successSound;
	const char* failureSound;
	const char* neutralSound;
	const char* goSound;

	// Perturbation parameters
	bool applyPerturbation; // whether any perturbation should occur during this block or not
	bool isPerturbationActive;
	bool isPerturbationDue; // whether a perturbation is still "due" (whether one should happen in this trial, and if yes, whether it has already happened or not)
	bool isPerturbationInCurrentTrial; // seems redundant but needed to put in output file
	bool isRandomDistancePerturbation; // whether pertubation happens at a given position in the motion or anywhere
	bool isRandomDirectionPerturbation; // whether the directtion (left or right) of the perturbation is random
	bool isRandomEventPerturbation; // whether perturbation happens at each trial or not
	bool isPerturbationVisible; // whether a visual cue must indicate where the perturbation will happen
	double perturbationForce[3]; // used to store the 3D force at each trial
	double perturbationMagnitude; // (N)
	double perturbationDuration; // (s)
	double perturbationDistance; // in percentage of the start-to-target distance. The perturbation only happens the first time you cross this distance
	double perturbationPosition[3]; // used to store 3D position of where the perturbation should happen (computed from perturbationDistance and start and target positions). Useful especially if the axis of motion is not aligned with one axis of the HM
	int perturbationDirection; // whether the perturbation is towards the right (+1) or left (-1). For now these are the only possibilities, other could be added later
	
	// Visual parameters
	double visualScalingFactor;
	double cupAdditionalVisualScalingFactor;
	double ballRadius;
	int ballNbSlices; // glutSphere parameter
	double blockLineWidth;
	std::vector<std::pair<double, double> > cupShapePoints; // array of points which - once joined - form the arc for the cup
		
	int windowSizeX;
	int windowSizeY;
	int windowPosX;
	int windowPosY;
	const char* windowName;
	double gNear;
	double gFar;
	double gOrtLeft;
	double gOrtRight;
	double gOrtBottom;
	double gOrtTop;
	
	double screenDistance, screenWidth, screenHeight;
	double eyeX, eyeY, eyeZ; // position of the eye point
	double centerX, centerY, centerZ; // position of the reference point
	double upX, upY, upZ; // direction of the up vector
	double red, green, blue, alpha; // specify the values used when the color buffers are cleared

	GLfloat floorColor[3];
	GLfloat startBlockColor[3];
	GLfloat targetBlockColor[3];
	GLfloat cupColor[3];
	GLfloat activeColor[3];
	GLfloat waitingColor[3];
	GLfloat successColor[3];
	GLfloat textColor[3];
	GLfloat perturbationColor[3];
	GLfloat backgroundColor[4]; // RGBA
	
	// Recording parameters
	std::string blockName; // Name you want for the output data file (a separate file is created for each trial in the block and the number of the trial is appended to the file name)
	bool isRecording;
	std::queue<double> timeData;
	std::queue<double> pendulumAngleData;
	std::queue<double> pendulumAngularVelocityData;
	std::queue<double> pendulumAngularAccelerationData;
	std::queue<double> cartPositionData; // though the HM can move in 3D, the cart model has only a 1D motion. Only this direction is recorded so that the file is not too big
	std::queue<double> cartVelocityData;
	std::queue<double> cartAccelerationData;
	std::queue<double> pendulumForceData; // this is not similar to what we get with the force in HapticMaster (force along Y is zero since no haptic objects, except constant force but is not included apparently)

};

#endif // DISPLAY_H_INCLUDED
