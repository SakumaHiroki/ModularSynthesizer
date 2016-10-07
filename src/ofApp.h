#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "ofxOsc.h"
#include "Module.h"
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/imgproc.hpp>
#include <Leap.h>

#define HOST "localhost"
#define PORT 8000

using namespace cv;
using namespace Leap;

class ofApp : public ofBaseApp
{

private:

	int imageWidth;
	int imageHeight;
	int windowWidth;
	int windowHeight;
	bool bMousePressed;

	//opencv
	VideoCapture capture;
	VideoWriter writer;
	Mat image;
	Mat perspectiveMatrix;
	vector<ofVec2f> imageVertices;
	Ptr<aruco::Dictionary> dictionary;
	Ptr<aruco::DetectorParameters> detectorParams;

	//leapmotion
	ofCamera camera;
	Controller controller;
	Frame frame;
	HandList hands;
	Finger index;
	ofMatrix3x3 matrix;
	vector<ofVec3f> screenVertices;
	ofVec3f screenPosition;

	//setting gui
	ofxPanel* gui;
	ofxToggle* bWriteImage;
	ofxToggle* bDrawImage;
	ofxToggle* bDrawHand;
	ofxToggle* bCalibrateCamera;
	ofxToggle* bCalibrateSensor;
	ofxFloatSlider* minPerimeterRate;
	ofxFloatSlider* maxPerimeterRate;
	ofxFloatSlider* detectInterval;
	ofxFloatSlider* detectDistance;
	ofxFloatSlider* detectAngle;
	ofxFloatSlider* connectDistance;
	ofxFloatSlider* screenDistance;

	//max
	ofxOscSender sender;
	vector<Module*> modules;
	unordered_map<string, ofFbo> figures;
	Module* select;

public:

	void setup();
	void update();
	void draw();

	void calibrateCamera(bool& value);
	void calibrateSensor(bool& value);
	void minPerimeterRateChanged(float& value);
	void maxPerimeterRateChanged(float& value);
	
	void mousePressed(ofMouseEventArgs& mouse);
	void keyPressed(ofKeyEventArgs& key);

	//windows api
	void pressMouse();
	void releaseMouse();
	void moveMouse(const ofVec2f& position);
	
	void initModule(Module* module);
	void eraseModule(Module* module);
	void moveModule(Module* module);
	void connectModule(Module* inputModule, int outputIndex, Module* outputModule, int inputIndex);
	void disconnectModule(Module* inputModule, int outputIndex, Module* outputModule, int inputIndex);

	void oscillatorSelected();
	void wavetableSelected();
	void multiFilterSelected();
	void combFilterSelected();
	void ladderFilterSelected();
	void diodeLadderFilterSelected();
	void tripleMorphingFilterSelected();
	void amplifierSelected();
	void lfoSelected();
	void egSelected();
	void speakerSelected();
	void sequencerSelected();

	void param0Changed(float& value);
	void param1Changed(float& value);
	void param2Changed(float& value);
	void param3Changed(float& value);
	void param4Changed(float& value);
	void param5Changed(float& value);
	void param6Changed(float& value);
	void param7Changed(float& value);
	void param8Changed(float& value);
	void param9Changed(float& value);
	void param10Changed(float& value);
	void param11Changed(float& value);
	void param12Changed(float& value);
	void param13Changed(float& value);
	void param14Changed(float& value);
	void param15Changed(float& value);

	void seqParam0Changed(float& value);
	void seqParam1Changed(float& value);
	void seqParam2Changed(float& value);
	void seqParam3Changed(float& value);
	void seqParam4Changed(float& value);
	void seqParam5Changed(float& value);
	void seqParam6Changed(float& value);
	void seqParam7Changed(float& value);
	void seqParam8Changed(float& value);
	void seqParam9Changed(float& value);
	void seqParam10Changed(float& value);
	void seqParam11Changed(float& value);
	void seqParam12Changed(float& value);
	void seqParam13Changed(float& value);
	void seqParam14Changed(float& value);
	void seqParam15Changed(float& value);

	void interactionParamChanged(int& value);

	void sendNumberMessage(const string& name, float number);
	void sendCreateMessage(const string& name, const ofVec2f& position, const string& type);
	void sendCreateMessage(const string& name, const ofVec2f& position, const string& type1, const string& type2);
	void sendDeleteMessage(const string& name);
	void sendConnectMessage(const string& inputName, int outputIndex, const string& outputName, int inputIndex);
	void sendDisconnectMessage(const string& inputName, int outputIndex, const string& outputName, int inputIndex);

	void drawHand(const Hand& hand) const;
	void drawPalm(const Hand& hand) const;
	void drawFinger(const Finger& finger) const;
	void drawBone(const Bone& bone) const;
};