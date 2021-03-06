#include "display.h"
#include "Mmsystem.h" // required for PlaySound and must be placed here and not in header file (don't know why but otherwise compilation errors)

extern Display* pDisplay; // no other choice due to GLUT functions

Display::Display(int mainLoopPeriod, int mainLoopTimerID, std::string nameOfBlock, int nbTrialsInBlock, double durationOfOneTrial, double goalFrequencyOfOscillations, double floorHeight, double startToTargetDistance, double arcCup, double lengthPendulum, double massPendulum, double dampingPendulum, double pendulumInitAngle, double pendulumInitVelocity, double inertiaOfHM, double accuracyFactorAmplitude, double accelerationAmplification, bool isSelfPaced, bool ballCanEscape, bool autoStart, double scalingFactorVisual, double cupAddScalingFactorVisual, bool projector, bool sound, bool isSpeedHint, int pX, int pY, int pZ)
{
	posX = pX;
	posY = pY;
	posZ = pZ;

	loopPeriod = mainLoopPeriod; // (ms)
	loopTimerID = mainLoopTimerID;
	
	QueryPerformanceFrequency((LARGE_INTEGER*)&timerFrequency); // initialization of time counter

	/* Task related parameters */	
	gravity  = 9.81; 
	pModel = new Model(massPendulum, lengthPendulum, dampingPendulum, pendulumInitAngle, pendulumInitVelocity, gravity); // timeStep is in sec 
	pHaptic = new Haptic(inertiaOfHM, floorHeight, pX, pY, pZ);

	// options 
	blockName = nameOfBlock;	
	selfPaced = isSelfPaced;
	speedHint = isSpeedHint;
	autoStartMode = autoStart;	
	canBallEscape = ballCanEscape;	
	isRecording = false;
	ballEscape = false;	
	isEndMotionDamped = false;
	maxNbTrials = nbTrialsInBlock;
	trialNb = 0;
	status = INITIALIZING;
		
	// time related parameters
	goalOscillationPeriod = 1. / goalFrequencyOfOscillations;
	durationOneTrial = durationOfOneTrial;
	startTime = 0.;
	userStartTime = 0.;	
	startWaitTime = 0.;
	escapeTime = 0.;
	durationWaitForStart = 2.;
	durationEndOfTrial = 2.;
	durationDamping = 0.5;
	
	// movement related parameters
	axisOfMotion = posY;
	targetPosition[posX] = 0;
	startPosition[posX] = 0;
	targetPosition[posY] = 0;
	startPosition[posY] = 0;
	startPosition[posZ] = floorHeight;
	targetPosition[posZ] = floorHeight;
	startPosition[axisOfMotion] += -0.5 * startToTargetDistance; // Compute a symetric start and target position (motion is constrained on the Y axis)
	targetPosition[axisOfMotion] += 0.5 * startToTargetDistance; 
	
	for (int i=0; i<3; i++)
	{
		escapePosition[i] = 0.;
		escapeVelocity[i] = 0.;
	}
	
	// physical model parameter (all model parameters must be stored to write in output file)
	accelerationAmplificationFactor = accelerationAmplification;
	arcOfCup = arcCup;
	inertiaHM = inertiaOfHM;
	pendulumMass = massPendulum;
	pendulumDamping = dampingPendulum;
	pendulumLength = lengthPendulum;
	pendulumInitialAngle = pendulumInitAngle;
	pendulumInitialVelocity = pendulumInitVelocity;
	
	cupWidth = 2 * pendulumLength * sin(0.5 * arcCup);
	cupHeight = pendulumLength * (1 - cos(0.5 * arcCup));
	amplitudeFactorAccuracy = accuracyFactorAmplitude;
	targetWidth = max(amplitudeFactorAccuracy, 1.1) * cupWidth; // target must be larger that cup so that cup can fits entirely into target box (this is how motion is target is considered to be reached)

	// tolerance parameters (used to bring HM back to init pos) and parameters to compute average frequency for feedback to user during trial
	distanceTolerance = 0.01; 
	velocityTolerance = 0.005; // also used when autoStart is off to detect beginning of motion
	frequencyTolerance = 0.1; // percentage of the goal frequency
	nbCyclesForAverageFrequency = 2;
	motionDirection = 1;
	averageUserFrequency = 0.;
	timeStartCycle = 0.;
	
	// sounds for auditory cues
	playSound = sound;
	failureSound = "Sounds\\failure.wav";
	bipSound = "Sounds\\bip.wav";
	endSound  = "Sounds\\neutral.wav";

	/* Graphic parameters */			
	// window size and projection parameters
	windowName = "Cup Task";
	visualScalingFactor = scalingFactorVisual;
	cupAdditionalVisualScalingFactor = cupAddScalingFactorVisual;
	double physicalScreenWidth; // (m) the real width of the screen (must be measured if a new screen is used)
	if (projector)
	{
		windowSizeX = 1024;
		windowSizeY = 768;
		windowPosX = 1920 + 0.5 * (1024 - windowSizeX);
		windowPosY = 0.5 * (768 - windowSizeY);
		physicalScreenWidth = 2.46;
	}
	else // Main monitor
	{	
		windowSizeX = glutGet(GLUT_SCREEN_WIDTH);
		windowSizeY = glutGet(GLUT_SCREEN_HEIGHT);
		windowPosX = glutGet(GLUT_SCREEN_WIDTH) - windowSizeX;
		windowPosY = glutGet(GLUT_SCREEN_HEIGHT) - windowSizeY;
		physicalScreenWidth = 0.6;
	}	
	double ratio = (double)glutGet(GLUT_SCREEN_WIDTH)/(double)glutGet(GLUT_SCREEN_HEIGHT);
	screenDistance = 1.0; // (m) with screenDistance = 1 and a visualScalingFactor = 1, the distances on the screen are equal to the physical distances in the model/HM
	// All the scaling is done here (size of the cup + start-to-target distance)
	screenWidth = physicalScreenWidth / visualScalingFactor; 
	screenHeight = screenWidth / ratio; //m	
	gOrtLeft = -screenWidth / 2.0;
	gOrtRight = screenWidth / 2.0;
	gOrtBottom = -screenHeight / 2.0;
	gOrtTop = screenHeight / 2.0;
	// Define position of observer wrt screen 
	eyeX = screenDistance;
	eyeY = 0.;
	eyeZ = floorHeight + 0.15 * screenHeight;
	centerX = 0.0;
	centerY =  eyeY;
	centerZ = eyeZ;
	upX = 0.0;
	upY = 0.0;
	upZ = 1.0;
	
	// Update target width and height for visual block 
	targetWidth *= cupAdditionalVisualScalingFactor;
	cupHeight *= cupAdditionalVisualScalingFactor;

	ballRadius = 0.1 * cupWidth * cupAdditionalVisualScalingFactor; // size of ball does not have any impact on the mechanical model. Defined as a percentage of the horizontal width of the cup
	ballNbSlices = 20;
	blockLineWidth = 4.0;

	// Create an array of points which - once joined - form the arc for the cup (centered on (0,0), real position adapted after according to the position of the HM end-effector)
	// The cup is defined by both the pendulum length (curvature of the cup) and the portion of the total circle which is kept to represent the cup (defined by an angle)
	cupShapePoints.clear();
	int nbPoints = 100;	
	for (int i=0; i<nbPoints+1; i++)
	{
		double angle = ((i *1.0) / nbPoints * arcCup - arcCup / 2.);
		cupShapePoints.push_back(std::pair<double,double>(cupAdditionalVisualScalingFactor * (pendulumLength + ballRadius) * sin(angle), cupAdditionalVisualScalingFactor * (pendulumLength + ballRadius) * (1 - cos(angle)) - ballRadius));
	}
	
	// color parameters
	floorColor[0] = 0.;
	floorColor[1] = 1.;
	floorColor[2] = 1.;
	startBlockColor[0] = 0.;
	startBlockColor[1] = 1.;
	startBlockColor[2] = 0.;
	targetBlockColor[0] = 0.;
	targetBlockColor[1] = 1.;
	targetBlockColor[2] = 0.;
	cupColor[0] = 1.;
	cupColor[1] = 1.;
	cupColor[2] = 0.;
	activeColor[0] = 1.;
	activeColor[1] = 1.;
	activeColor[2] = 1.;
	waitingColor[0] = 1.;
	waitingColor[1] = 0.;
	waitingColor[2] = 0.;
	successColor[0] = 0.;
	successColor[1] = 0.;
	successColor[2] = 1.;
	textColor[0] = 1.;
	textColor[1] = 1.;
	textColor[2] = 1.;
	backgroundColor[0] = 0.;
	backgroundColor[1] = 0.;
	backgroundColor[2] = 0.;
	backgroundColor[3] = 0.;
}

Display::~Display()
{
	if (pModel != NULL)
		delete pModel;
	if (pHaptic != NULL)
		delete pHaptic;
}

void Display::Timer(int iTimer)
{
	unsigned __int64 currentTimeStamp;
	double timeStep, previousTime;
	double distanceNorm, velocityNorm;
	double *cupAcceleration, *cupPosition, *cupVelocity;
	double pendulumForce[3];
	for (int i=0; i<3; i++)
		pendulumForce[i] = 0;

	if (pHaptic != NULL)
	{
		// Get time
		previousTime = currentTime;
		QueryPerformanceCounter((LARGE_INTEGER *)&currentTimeStamp);
		currentTime = (1. * currentTimeStamp) / timerFrequency;
		timeStep = currentTime - previousTime; // need to be computed because the theoretical simestep given as parameter in the glutTimerFunc is not well respected
		
		// Get The Current EndEffector Position/Velocity/Acceleration from THe HapticMASTER
		// (force if needed but be careful, with the old software these are the virtual forces, not the forces actually applied by the user)
		pHaptic->UpdateForcePositionVelocityAcceleration();

		switch (status)
		{
		case INITIALIZING:
			// Move end effector to start position
			pHaptic->UpdateStartPositionSpring(startPosition); // spring reference position can be modified (scaled) to prevent high force in spring
			pHaptic->EnableStartPositionSpring();
			status = GOTONEXT; // will ends up in a "break" state before first trial
			break;
			
		case GOTONEXT:
			if (trialNb == maxNbTrials)
			{
				status = END;
				break;
			}
			else
			{
				// Update reference position for spring_Y to move end-effector smoothly back to start position (with a force limit in the spring)
				pHaptic->UpdateStartPositionSpring(startPosition);
				cupPosition = pHaptic->GetCurrentPosition();
				cupVelocity = pHaptic->GetCurrentVelocity();
				distanceNorm = sqrt(pow(cupPosition[posX] - startPosition[posX], 2) + pow(cupPosition[posY] - startPosition[posY], 2) + pow(cupPosition[posZ] - startPosition[posZ], 2));
				velocityNorm = sqrt(pow(cupVelocity[posX], 2) + pow(cupVelocity[posY], 2) + pow(cupVelocity[posZ], 2));
				// The 3D position is correct (not only in the motion direction)
				if (distanceNorm <= distanceTolerance && velocityNorm <= velocityTolerance)
				{		
					// Make sure vectors where recorded data are stored are empty
					ClearDataBuffer();
					// Re-initialize state
					ballEscape = false;
					motionDirection = 1;
					averageUserFrequency = 0.;
					timeStartCycle = 0.;
					storageCycleDuration.clear(); 
					pModel->InitializeState(pendulumInitialAngle, pendulumInitialVelocity); // the start state is the same for all trials in block
					pHaptic->UpdateStartPositionSpring(startPosition);
					pHaptic->EnableRestrict1DMotion();
					startWaitTime = currentTime; 
					status = WAITFORSTART;
				}
				break;
			}

		case WAITFORSTART:
			if (currentTime - startWaitTime >= durationWaitForStart)
				status = STARTMOTION;
			break;

		case STARTMOTION:
			// Deactivate the spring which keep the EE at the start position
			pHaptic->DisableStartPositionSpring();
			// Enable force feedback
			pHaptic->EnableBallForce();
			// Play a sound saying you can start 
			if(playSound)
				PlaySoundA(bipSound, NULL, SND_ASYNC);

			// Start recording (as soon as you are allowed to move, even if you are not really starting, so that you don't miss the very beginning of the motion)
			startTime = currentTime; 	
			StartRecording();
			status = INITIATEMOTION;
			break;

		case INITIATEMOTION:
			if (autoStartMode || pHaptic->GetCurrentVelocity()[axisOfMotion] >= velocityTolerance) // starts automatically or the time is triggered only when the user actually moves the HM 
			{
				userStartTime = currentTime;
				startWaitTime = currentTime; // used later to compute when bip sound should be played
				status = INMOTION;				
			}
			break;
			
		case INMOTION:
			cupPosition = pHaptic->GetCurrentPosition();
			cupVelocity = pHaptic->GetCurrentVelocity();	
			cupAcceleration = pHaptic->GetCurrentAcceleration();
			cupAcceleration[axisOfMotion] *= accelerationAmplificationFactor; // if motion is not along Y only, modify that to scale all the components needed
			pModel->UpdatePendulumState(cupAcceleration[axisOfMotion], timeStep);
			
			pendulumForce[axisOfMotion] = pModel->ComputePendulumForceOnCart(cupAcceleration[axisOfMotion]);// Inertial force which tends to put the cart in motion (opposite of resistive force of the cup wall)
			pHaptic->UpdateBallForce(pendulumForce);
			
			// Check if ball escape
			if (canBallEscape && abs(pModel->GetPendulumAngle()) > arcOfCup/ 2.)
			{
				ballEscape = true;
				escapeTime = currentTime;
				double angle = pModel->GetPendulumAngle();
				double angularVelocity = pModel->GetPendulumAngularVelocity();

				escapePosition[posX] = cupPosition[posX]; // 2D model
				escapePosition[posY] = cupPosition[posY] + cupAdditionalVisualScalingFactor * pendulumLength * sin(angle);
				escapePosition[posZ] = cupPosition[posZ] + cupAdditionalVisualScalingFactor * pendulumLength * (1 - cos(angle)); 
					
				escapeVelocity[posX] = cupVelocity[posX]; // TODO should the velocity also be scaled ??
				escapeVelocity[posX] = cupVelocity[posX] + cupAdditionalVisualScalingFactor * pendulumLength * cos(angle) * angularVelocity;
				escapeVelocity[posX] = cupVelocity[posX] + cupAdditionalVisualScalingFactor * pendulumLength * sin(angle) * angularVelocity;
				
				if (playSound)
					PlaySoundA(failureSound, NULL, SND_ASYNC);
				status = TERMINATEMOTION;
				break;
			}

			// Check whether a new cycle has started. Cycles are computed based on when half start to target distance is crossed. This trick assumes that you do not go back while you haven't reached the target. Probably won't work with perturbation. Maybe check whether you also have reached start/target in between?
			// Used for computing average frequency to compare with metronome frequency so only used for metronome paced trials
			if (!selfPaced && speedHint && motionDirection * cupPosition[axisOfMotion] >= motionDirection * (targetPosition[axisOfMotion] + startPosition[axisOfMotion]) / 2.)
			{
				if (motionDirection == 1) // a cycle is a back and forth motion so only count when going left to right
				{
					if (timeStartCycle != 0.) // first time crossing, no cycle terminated
					{
						double averageCycleTime = 0.;
						storageCycleDuration.push_back(currentTime - timeStartCycle);						
						for (unsigned int i=0; i< storageCycleDuration.size(); i++)
							averageCycleTime += storageCycleDuration[i];
						averageUserFrequency = 1. / (averageCycleTime / storageCycleDuration.size());
					
						if (storageCycleDuration.size() > nbCyclesForAverageFrequency)
							storageCycleDuration.pop_front(); // remove oldest element	
					}
					timeStartCycle = currentTime;
				}
				motionDirection *= -1;
			}
			
			// if metronome paced, check whether a metronome bip should be played (each time direction must change so half-period)
			if (!selfPaced && currentTime - startWaitTime >= goalOscillationPeriod / 2.)	
			{
				PlaySoundA(bipSound, NULL, SND_ASYNC);
				startWaitTime = currentTime;
			}
			
			// Check whether duration of trial is reached
			if (currentTime - userStartTime >= durationOneTrial)
			{
				if (playSound)
					PlaySoundA(endSound, NULL, SND_ASYNC);
				status = TERMINATEMOTION;
			}
			// Check whether end of trial is close and damping should be activated to make the stop smooth
			else if (!isEndMotionDamped && currentTime - userStartTime >= durationOneTrial - durationDamping)
			{
				pHaptic->EnableDamper();
				isEndMotionDamped = true;
			}
			break;

		case TERMINATEMOTION:
			// Motion ended: deactivate force feedback (force from ball on cup) and lock HM
			pHaptic->DisableBallForce();
			pHaptic->UpdateStartPositionSpring(pHaptic->GetCurrentPosition()); // lock the robot at its current position (target reached)
			pHaptic->EnableStartPositionSpring();
			if (isEndMotionDamped)
			{
				pHaptic->DisableDamper();
				isEndMotionDamped = false;
			}

			// Stop recording data and write the recorded data in a file
			StopRecording();			
			WriteDataInFile();

			startWaitTime =  currentTime;
			status = ENDOFTRIAL;	
			break;

		case ENDOFTRIAL:
			if (currentTime - startWaitTime >= durationEndOfTrial)
			{
				trialNb++;
				status = GOTONEXT;
			}
			break;

		case END: // Exit
			pHaptic->Terminate();
			exit(0);
			break;
		}

		// Record current data if recording is active
		if (isRecording)
			RecordMotionData();
	}
	
	// Set The Timer For This Function Again
	// 1st param = callback time in msec, 2nd param = function to call at callback, 3rd param : timer ID (can have several differemt timers)
	// Note: GLUT attempts to deliver the timer callback as soon as possible after the expiration of the callback's time interval (time is not exactly guaranteed)
	// Callback function cannot be a method (must be either static method or regular function), because the callback function can take only one parameter (here loopTimerID),		// whereas a method also has "this" as a parameter
	glutTimerFunc(loopPeriod, InternalTimer, loopTimerID);
}


void Display::InternalTimer(int iTimer)
{
	// Method being static, it cannot use attributes of the class, so a global variable is needed here
	if (pDisplay != NULL)
		pDisplay->Timer(iTimer);
}


void Display::Keyboard(unsigned char ucKey, int iX, int iY)
{
	switch (ucKey)
	{		
		// Exit
	case 27:
		if (pHaptic != NULL)
			pHaptic->Terminate();
		exit(0);
		break;
	}
}


void Display::InternalKeyboard(unsigned char ucKey, int iX, int iY)
{
	if (pDisplay != NULL)
		pDisplay->Keyboard(ucKey, iX, iY);
}



void Display::DrawFloor()
{
	glLineWidth(blockLineWidth);
	glColor3f(floorColor[0], floorColor[1], floorColor[2]);
	glBegin(GL_LINES); 
	glVertex3f(startPosition[posX], startPosition[posY] - targetWidth / 2., startPosition[posZ] - ballRadius);
	glVertex3f(targetPosition[posX], targetPosition[posY] + targetWidth / 2., targetPosition[posZ] - ballRadius);
	glEnd();
}

void Display::DrawBall(GLfloat color[3])
{
	double *cup = pHaptic->GetCurrentPosition();
	double angle  = pModel->GetPendulumAngle(); 	
	double ballPosition[3];
	glPushMatrix();	// push and pop matrix are needed here because you do a translation of the frame (whereas you don't do any modification when drawing blocks etc...)
	glColor3f(color[0], color[1], color[2]); 
	// Compute ball position
	if (!ballEscape) // ball in cup
	{		
		ballPosition[posX] = cup[posX]; // 2D model
		ballPosition[posY] = cup[posY] + cupAdditionalVisualScalingFactor * pendulumLength * sin(angle);
		ballPosition[posZ] = cup[posZ] + cupAdditionalVisualScalingFactor * pendulumLength * (1 - cos(angle));
	}
	else // flying ball motion
	{
		unsigned __int64 currentTimeStamp;
		QueryPerformanceCounter((LARGE_INTEGER *)&currentTimeStamp);
		double currentTime = (1. * currentTimeStamp) / timerFrequency; 
		ballPosition[posX] = escapePosition[posX] + escapeVelocity[posX] * (currentTime - escapeTime);
		ballPosition[posY] = escapePosition[posY] + escapeVelocity[posY] * (currentTime - escapeTime);
		ballPosition[posZ] = escapePosition[posZ] + escapeVelocity[posZ] * (currentTime - escapeTime) - gravity / 2. * pow(currentTime - escapeTime, 2);		
	}
	glTranslatef(0., ballPosition[posY], ballPosition[posZ]); // The X (depth position) is set to 0 (should be cup[posX]+ball[posX] to be more general) otherwise ball disappears is the EE of the HM is not exactly on 0 on the X axis (i.e. if you push on the HM) (despite the spring to constrain the motion on the Y axis you can slighlty move in the other directions)
	glutSolidSphere(ballRadius, ballNbSlices, ballNbSlices);
	glPopMatrix();
}

void Display::DrawCup(GLfloat color[3])
{
	double *cup = pHaptic->GetCurrentPosition();

	glLineWidth(2*blockLineWidth);
	glColor3f(color[0], color[1], color[2]);
	glBegin(GL_LINE_STRIP);
	for (unsigned int i=0; i<cupShapePoints.size(); i++)
	{
		glVertex3f(0., cupShapePoints[i].first + cup[posY], cupShapePoints[i].second + cup[posZ]);  // The X (depth position) is set to 0 (should be cup[posX] to be more general) otherwise cup disappears is the EE of the HM is not exactly on 0 on the X axis (i.e. if you push on the HM) (despite the spring to constrain the motion on the Y axis you can slighlty move in the other directions)
	}
	glEnd();
}

void Display::DrawTargetBlock()
{
	glLineWidth(blockLineWidth);
	glColor3f(targetBlockColor[0], targetBlockColor[1], targetBlockColor[2]);
	glBegin(GL_QUADS); 
	glVertex3f(targetPosition[posX], targetPosition[posY] - targetWidth / 2, targetPosition[posZ] - ballRadius);
	glVertex3f(targetPosition[posX], targetPosition[posY] - targetWidth / 2, targetPosition[posZ] + 1.1 * cupHeight);
	glVertex3f(targetPosition[posX], targetPosition[posY] + targetWidth / 2, targetPosition[posZ] + 1.1 * cupHeight);
	glVertex3f(targetPosition[posX], targetPosition[posY] + targetWidth / 2, targetPosition[posZ] - ballRadius);
	glEnd();
}


void Display::DrawStartBlock()
{
	glLineWidth(blockLineWidth);
	glColor3f(startBlockColor[0], startBlockColor[1], startBlockColor[2]);
	glBegin(GL_QUADS); 
	glVertex3f(startPosition[posX], startPosition[posY] - targetWidth / 2, startPosition[posZ] - ballRadius);
	glVertex3f(startPosition[posX], startPosition[posY] - targetWidth / 2, startPosition[posZ] + 1.1 * cupHeight);
	glVertex3f(startPosition[posX], startPosition[posY] + targetWidth / 2, startPosition[posZ] + 1.1 * cupHeight);
	glVertex3f(startPosition[posX], startPosition[posY] + targetWidth / 2, startPosition[posZ] - ballRadius);
	glEnd();
}


void Display::DrawStatus(std::string text_str, GLfloat color[3], double position)
{
	// position parameter must be 0 < position < 1 and is the percentage of screen height where the text is located
	const char *text = text_str.c_str();
	double ViewportWidth = ((GLsizei)glutGet(GLUT_WINDOW_WIDTH));
	double ViewportHeight = ((GLsizei)glutGet(GLUT_WINDOW_HEIGHT));

	glPushMatrix(); 
	glLoadIdentity();
	gluOrtho2D(0.0, (GLfloat)ViewportWidth, 0.0, (GLfloat)ViewportHeight);
	glColor3f(color[0], color[1], color[2]);
	glRasterPos2i(ViewportWidth / 2 - text_str.length()*4., ViewportHeight*position);
	for (unsigned int i = 0; i < text_str.length(); i++)
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, text[i]);
	glPopMatrix(); 
}


void Display::DrawWindow(GLfloat currentStateColor[3])
{
	// Object last drawn is on the upper layer
	DrawFloor();
	DrawStartBlock();
	DrawTargetBlock();
	DrawBall(currentStateColor);
	DrawCup(cupColor);

}

void Display::Reshape(int iWidth, int iHeight)
{
	float fAspect = (float)iWidth / iHeight;

	double ViewportWidth = ((GLsizei)glutGet(GLUT_WINDOW_WIDTH));
	double ViewportHeight = ((GLsizei)glutGet(GLUT_WINDOW_HEIGHT));

	// Set viewport
	glViewport(0, 0, (int)ViewportWidth, (int)ViewportHeight);
}


void Display::InternalReshape(int iWidth, int iHeight)
{
	if (pDisplay != NULL)
		pDisplay->Reshape(iWidth, iHeight);
}


void Display::UpdateDisplay(void)
{
	char msg[1024]; 

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Set perspective
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// 2D view
	gluOrtho2D(gOrtLeft, gOrtRight, gOrtBottom, gOrtTop);

	// Define eyepoint in such a way that drawing can be done as in lab-frame rather than sgi-frame (so X towards user, Z is up)
	gluLookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);	

	// Draw what need be depending on the status
	switch (status)
	{
	case INITIALIZING:
		DrawStatus("Initializing", textColor, 0.75);
		break;

	case GOTONEXT:
		// Do not display anything
		break;

	case WAITFORSTART:
		sprintf_s(msg, "Trial Nb:  %i ", trialNb);
		DrawStatus(msg, textColor, 0.75);
		DrawWindow(waitingColor);
		break;

	case STARTMOTION:
		sprintf_s(msg, "Go");
		DrawStatus(msg, textColor, 0.75);
		DrawWindow(activeColor);
		break;

	case INITIATEMOTION:
		sprintf_s(msg, "Go");
		DrawStatus(msg, textColor, 0.75);
		DrawWindow(activeColor);
		break;

	case INMOTION:		
		DrawWindow(activeColor);
		if(!selfPaced && speedHint && averageUserFrequency!=0)
		{
			if (averageUserFrequency * goalOscillationPeriod > (1. + frequencyTolerance))
				sprintf_s(msg, "Slow down");
			else if (averageUserFrequency * goalOscillationPeriod < (1. - frequencyTolerance))
				sprintf_s(msg, "Go faster");
			else
				sprintf_s(msg, "Good job");
			DrawStatus(msg, textColor, 0.75);	
		}	
		break;

	case TERMINATEMOTION:
		if (ballEscape)
			DrawWindow(waitingColor);
		else
			DrawWindow(successColor);
		break;

	case ENDOFTRIAL:
		if (ballEscape)
			DrawWindow(waitingColor);
		else
			DrawWindow(successColor);
		break;

	case END:
		break;
	}
	glutSwapBuffers();
	glutPostRedisplay();
}


void Display::InternalUpdateDisplay(void)
{
	if (pDisplay != NULL)
		pDisplay->UpdateDisplay();
}


int Display::Initialize(int argc, char** argv)
{
	// Starts with HM initialization
	if(pHaptic->InitHapticMaster() != 0) // HM initialization failed
		return -1;

	// Create OpenGL Window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(windowSizeX, windowSizeY);
	glutInitWindowPosition(windowPosX, windowPosY);
	glutCreateWindow(windowName);

	// Set background color
	glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2], backgroundColor[3]);

	// OpenGL Initialization Calls
	glutReshapeFunc(InternalReshape);
	glutDisplayFunc(InternalUpdateDisplay);
	glutKeyboardFunc(InternalKeyboard);
	glutTimerFunc(loopPeriod, InternalTimer, loopTimerID);

	return 0;
}


void Display::LaunchLoop()
{
	glutMainLoop();
}


// Data are recorded as a .csv file, but can be converted into a .mat file using the csv2mat.m script provided
void Display::WriteDataInFile()
{
	char nbTrialChar[4]; // should be enough, less that 1000 trials + end character
	_itoa_s(trialNb, nbTrialChar, 10);
	std::string filename = "Output/" + blockName + "_trial_" + (std::string)nbTrialChar + ".csv";	
	const char * filenameChar = filename.c_str();
	double nb_lines_header = 21; // Does not include names and units of variables
	std::ofstream data_file(filenameChar);
	if (data_file)
	{
		// First write parameters used for the trial
		data_file << "RythmicTask"  << ";" << nb_lines_header << std::endl;
		data_file << "TrialNumber" << ";" << trialNb  << ";" << "N/A" << std::endl;
		data_file << "Success" << ";" << !ballEscape  << ";" << "(bool)" << std::endl;
		data_file << "TrialDuration" << ";" << durationOneTrial  << ";" << "(s)" << std::endl; // can be shorter than the actual recorded data in cas autoStart is off and user does not start motion exactly at the starting cue
		data_file << "IsSelfPaced" << ";" << selfPaced  << ";" << "(bool" << std::endl;
		data_file << "IsSpeedHint" << ";" << speedHint << ";" << "(bool)" << std::endl;
		if (selfPaced)
			data_file << "GoalOscillationFrequency" << ";" << "N/A"  << ";" << "(Hz)" << std::endl; 
		else
			data_file << "GoalOscillationFrequency" << ";" << 1. / goalOscillationPeriod  << ";" << "(Hz)" << std::endl; 
		data_file << "StartToTargetDistance" << ";" << targetPosition[axisOfMotion] - startPosition[axisOfMotion]  << ";" << "(m)" << std::endl;
		data_file << "HMInertia" << ";" << inertiaHM  << ";" << "(kg)" << std::endl;
		data_file << "PendulumMass" << ";" << pendulumMass  << ";" << "(kg)" << std::endl;
		data_file << "PendulumDamping" << ";" << pendulumDamping  << ";" << "(N.m.s)" << std::endl;
		data_file << "PendulumLength" << ";" << pendulumLength  << ";" << "(m)" << std::endl;
		data_file << "ArcOfCup" << ";" << arcOfCup  << ";" << "(rad)" << std::endl;
		data_file << "PendulumInitialAngle" << ";" << pendulumInitialAngle  << ";" << "(rad)" << std::endl;
		data_file << "PendulumInitialVelocity" << ";" << pendulumInitialVelocity  << ";" << "(rad/s)" << std::endl;
		data_file << "AmplitudeAccuracyFactor" << ";" << amplitudeFactorAccuracy  << ";" << "(block width=factor*cup width)" << std::endl;
		data_file << "CartAccelerationAmplificationFactor" << ";" << accelerationAmplificationFactor  << ";" << "N/A" << std::endl;
		data_file << "VisualScalingFactor" << ";" << visualScalingFactor  << ";" << "(visual=factor*real)" << std::endl;
		data_file << "CupAdditionalVisualScalingFactor" << ";" << cupAdditionalVisualScalingFactor << ";" << "(visual_cup=additionalFactor*visualFactor*real)" << std::endl; // also affect the tolerance to reach the target
		data_file << "AutoStartMode" << ";" << autoStartMode  << ";" << "(bool)" << std::endl;
		data_file << "CanBallEscape" << ";" << canBallEscape  << ";" << "(bool)" << std::endl;

		// Then write the actual data (we only care about the Y motion of teh cart)
		data_file <<	"Time" << ";" << "Pendulum_Angle"  << ";" << "Pendulum_AngularVel"  << ";" << "Pendulum_AngularAcc"  << ";" << 
						"Cart_Pos_X"  << ";" << "Cart_Vel_X"  << ";" << "Cart_Acc_X"  << ";" << 
						"Ball_Force"  << ";" << "User_Force_X" << ";" << "User_Force_Y" << ";" << "User_Force_Z" << std::endl;
		data_file <<	"(s)" << ";" << "(rad)"  << ";" << "(rad/s)"  << ";" << "(rad/s/s)"  << ";" << 
						"(m)"  << ";" << "(m/s)"  << ";" << "(m/s/s)"  << ";" << 
						"(N)"  << ";" << "(N)" << ";" << "(N)" << ";" << "(N)" << std::endl;

		// Evene if all vectorsa are supposed to be the same length, still check and take the minimum length, just in case
		unsigned int nbLinesPosP = pendulumAngleData.size();
		unsigned int nbLinesVelP = pendulumAngularVelocityData.size();
		unsigned int nbLinesAccP = pendulumAngularAccelerationData.size();
		unsigned int nbLinesPosC = cartPositionData.size();
		unsigned int nbLinesVelC = cartVelocityData.size();
		unsigned int nbLinesAccC = cartAccelerationData.size();
		unsigned int nbLinesForce = pendulumForceData.size(); 
		unsigned int nbLinesUserForce = min(userForce[posX].size(), min(userForce[posY].size(), userForce[posZ].size()));
		unsigned int nbLines = min(timeData.size(), min(nbLinesPosP, min(nbLinesVelP, min(nbLinesAccP, min(nbLinesPosC, min(nbLinesVelC, min(nbLinesAccC, min(nbLinesForce, nbLinesUserForce))))))));
		while (nbLines > 0)
		{
			// Drop data in file
			data_file << timeData.front() << ";" << 
				pendulumAngleData.front() << ";" << pendulumAngularVelocityData.front() << ";" << pendulumAngularAccelerationData.front() << ";" <<
				cartPositionData.front() << ";" << cartVelocityData.front() << ";" << cartAccelerationData.front() << ";" <<
				pendulumForceData.front() << ";" <<
				userForce[posX].front() << ";" << userForce[posY].front() << ";" << userForce[posZ].front() << std::endl; 
			// Remove first element (oldest) in queues
			timeData.pop();
			pendulumAngleData.pop();
			pendulumAngularVelocityData.pop();
			pendulumAngularAccelerationData.pop();
			cartPositionData.pop();
			cartVelocityData.pop();
			cartAccelerationData.pop();
			pendulumForceData.pop();
			for(int i=0; i<3; i++)
				userForce[i].pop();
			nbLines--;
		}
	}
	else
		std::cout << "Error on file opening" << std::endl;
}


void Display::StartRecording()
{
	isRecording = true;
}


void Display::StopRecording()
{
	isRecording = false;
}


void Display::RecordMotionData() // most functions called here are already called in Timer, so it is not very efficient in term of computation time. If computation time is an issue, modify here to prevent calling the same function twice in one controller loop
{
	if (pHaptic != NULL)
	{
		// Get time
		unsigned __int64 currentTimeStamp;
		QueryPerformanceCounter((LARGE_INTEGER *)&currentTimeStamp);
		double currentTime = (1. * currentTimeStamp) / timerFrequency; 	
		double pendulumAngle = pModel->GetPendulumAngle();
		double pendulumAngularVelocity = pModel->GetPendulumAngularVelocity();
		double pendulumAngularAcceleration = pModel->GetPendulumAngularAcceleration();
		double cartPosition = pHaptic->GetCurrentPosition()[axisOfMotion];
		double cartVelocity = pHaptic->GetCurrentVelocity()[axisOfMotion];
		double cartAcceleration = pHaptic->GetCurrentAcceleration()[axisOfMotion]; // since the data to record are read after the ball force on the cup is applied, the acceleration here already contains the acceleration amplification gain (however the velocity does not include the gain)
		double pendulumForce = pModel->ComputePendulumForceOnCart(cartAcceleration);
		double *userExtForce = pHaptic->GetCurrentForce();

		timeData.push(currentTime - startTime);
		pendulumAngleData.push(pendulumAngle);
		pendulumAngularVelocityData.push(pendulumAngularVelocity);
		pendulumAngularAccelerationData.push(pendulumAngularAcceleration);
		cartPositionData.push(cartPosition);
		cartVelocityData.push(cartVelocity);
		cartAccelerationData.push(cartAcceleration);  
		pendulumForceData.push(pendulumForce);
		for(int i=0; i<3; i++)
			userForce[i].push(userExtForce[i]);
	}

}

void Display::ClearDataBuffer()
{
	// Clear queues 
	while (!timeData.empty())
		timeData.pop();
	while (!pendulumAngleData.empty())
		pendulumAngleData.pop();
	while (!pendulumAngularVelocityData.empty())
		pendulumAngularVelocityData.pop();
	while (!pendulumAngularAccelerationData.empty())
		pendulumAngularAccelerationData.pop();
	while (!cartPositionData.empty())
		cartPositionData.pop();
	while (!cartVelocityData.empty())
		cartVelocityData.pop();
	while (!cartAccelerationData.empty())
		cartAccelerationData.pop();
	while (!pendulumForceData.empty())
		pendulumForceData.pop();
	for(int i=0; i<3; i++)
		while(!userForce[i].empty())
			userForce[i].pop();

}
