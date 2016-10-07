/******************************\
* Sakuma Hiroki               *
* 2016/10/07                  *
* Modular Synthesizer Project *
\******************************/

#include "ofApp.h"

void ofApp::setup()
{
	ofBackground(0);

	ofSetWindowPosition(0, 0);

	sender.setup(HOST, PORT);

	camera.setPosition(ofVec3f(0, 0, 0));
	camera.lookAt(ofVec3f(0, 1, 0), ofVec3f(0, 0, -1));

	capture.open(1);

	imageWidth = capture.get(CAP_PROP_FRAME_WIDTH);
	imageHeight = capture.get(CAP_PROP_FRAME_HEIGHT);

	windowWidth = ofGetWidth();
	windowHeight = ofGetHeight();

	writer.open("Modular Synthesizer.avi", -1, capture.get(CAP_PROP_FPS), Size(imageWidth, imageHeight));

	dictionary = aruco::getPredefinedDictionary(aruco::DICT_4X4_50);
	detectorParams = aruco::DetectorParameters::create();

	ofxBaseGui::loadFont("Stereo.ttf", 12);
	ofxBaseGui::setDefaultBackgroundColor(ofColor(0, 0));
	ofxBaseGui::setDefaultBorderColor(ofColor::white);
	ofxBaseGui::setDefaultFillColor(ofColor::gray);
	ofxBaseGui::setDefaultHeaderBackgroundColor(ofColor::gray);
	ofxBaseGui::setDefaultTextColor(ofColor::white);
	ofxBaseGui::setDefaultWidth(300);
	ofxBaseGui::setDefaultHeight(40);

	gui = (new ofxPanel())->setup("SETTINGS");
	bWriteImage = ((new ofxToggle())->setup("WRITE", false));
	bDrawImage = (new ofxToggle())->setup("IMAGE", false);
	bDrawHand = (new ofxToggle())->setup("HAND", false);
	bCalibrateCamera = (new ofxToggle())->setup("CALIBRATE CAMERA", false);
	bCalibrateSensor = (new ofxToggle())->setup("CALIBRATE SENSOR", false);
	minPerimeterRate = (new ofxFloatSlider())->setup("MIN PERIMETER RATE", 0.05, 0, 1);
	maxPerimeterRate = (new ofxFloatSlider())->setup("MAX PERIMETER RATE", 0.5, 0, 1);
	detectInterval = (new ofxFloatSlider())->setup("DETECT INTERVAL", 2, 0, 10);
	detectDistance = (new ofxFloatSlider())->setup("DETECT DISTANCE", 8, 0, 10);
	detectAngle = (new ofxFloatSlider())->setup("DETECT ANGLE", 6, 0, 10);
	connectDistance = (new ofxFloatSlider())->setup("CONNECT DISTANCE", 400, 0, 1000);
	screenDistance = (new ofxFloatSlider())->setup("SCREEN DISTANCE", 30, 0, 50);

	bCalibrateCamera->addListener(this, &ofApp::calibrateCamera);
	bCalibrateSensor->addListener(this, &ofApp::calibrateSensor);
	minPerimeterRate->addListener(this, &ofApp::minPerimeterRateChanged);
	maxPerimeterRate->addListener(this, &ofApp::maxPerimeterRateChanged);

	gui->add(bWriteImage);
	gui->add(bDrawImage);
	gui->add(bDrawHand);
	gui->add(bCalibrateCamera);
	gui->add(bCalibrateSensor);
	gui->add(minPerimeterRate);
	gui->add(maxPerimeterRate);
	gui->add(detectInterval);
	gui->add(detectDistance);
	gui->add(detectAngle);
	gui->add(connectDistance);
	gui->add(screenDistance);
	gui->setPosition(windowWidth - gui->getWidth() - 10, 10);

	modules.resize(8);
	for (auto begin(modules.begin()), end(modules.end()), itr(begin); itr != end; ++itr)
	{
		*itr = new Module();
		initModule(*itr);
	}

	//circleImage
	{
		int lineWidth(2);
		int feedout(100);
		int mediumRadius(80);
		int smallRadius(mediumRadius - lineWidth);
		int largeRadius(mediumRadius + lineWidth);
		int maxRadius(largeRadius + feedout);
		int minRadius(smallRadius - feedout);
		int width(maxRadius * 2);
		int height(maxRadius * 2);

		ofFloatPixels pixels;
		pixels.allocate(width, height, OF_PIXELS_RGBA);

		ofVec2f center(maxRadius, maxRadius);
		for (int x(0); x < width; x++)
		{
			for (int y(0); y < height; y++)
			{
				ofVec2f position(x, y);
				float radius(position.distance(center));
				radius = ofClamp(radius, minRadius, maxRadius);
				float distance(0);

				if (radius < smallRadius)
					distance = smallRadius - radius;
				else if (radius > largeRadius)
					distance = radius - largeRadius;

				float rawValue(ofMap(distance, 0, feedout, 0, 1));
				// alpha = 1.0 * (1 - x^0.5)
				ofFloatColor color(1.0, 1.0 * (1 - pow(rawValue, 0.5)));
				pixels.setColor(x, y, color);
			}
		}

		Module::circle.allocate(width, height, GL_RGBA32F_ARB);
		Module::circle.begin();
		{
			ofClear(0, 0);
			ofSetColor(ofColor::white);
			ofFloatImage(pixels).draw(0, 0);
		}
		Module::circle.end();
	}

	//lineImage
	{
		int lineWidth(2);
		int feedout(100);
		int mediumHeight(feedout + lineWidth);
		int smallHeight(mediumHeight - lineWidth);
		int largeHeight(mediumHeight + lineWidth);
		int width(mediumHeight * 2);
		int height(mediumHeight * 2);

		ofFloatPixels pixels;
		pixels.allocate(width, height, OF_IMAGE_COLOR_ALPHA);

		for (int y(0); y < height; y++)
		{
			float distance(0);

			if (y < smallHeight)
				distance = smallHeight - y;
			else if (y > largeHeight)
				distance = y - largeHeight;

			float rawValue(ofMap(distance, 0, feedout, 0, 1));
			// alpha = 1.0 * (1 - x^0.5)
			ofFloatColor color(1.0, 1.0 * (1 - pow(rawValue, 0.5)));
			for (int x(0); x < width; x++)
			{
				pixels.setColor(x, y, color);
			}
		}

		Module::line.allocate(width, height, GL_RGBA32F_ARB);
		Module::line.begin();
		{
			ofClear(0, 0);
			ofSetColor(ofColor::white);
			ofFloatImage(pixels).draw(0, 0);
		}
		Module::line.end();
	}

	//moduleImage
	struct figureInfo
	{
		figureInfo(const string& _type, const vector<string>& _inputNames, const vector<string>& _outputNames)
			: type(_type), inputNames(_inputNames), outputNames(_outputNames){}
		string type;
		vector<string> inputNames;
		vector<string> outputNames;
	};

	vector<figureInfo> figureInfos
	{
		figureInfo("EMPTY", {}, {}),
		figureInfo("OSCILLATOR", {"V/OCT", "EXP", "LIN", "PWM", "SYNC"}, {"OUT", "PHASOR"}),
		figureInfo("WAVETABLE", {"V/OCT", "EXP", "LIN", "WAVE", "BANK"}, {"OUT"}),
		figureInfo("MMF", {"IN", "V/OCT", "EXP", "RES"}, {"OUT"}),
		figureInfo("COMB FILTER", {"IN", "V/OCT", "EXP", "FEEDBACK"}, {"OUT"}),
		figureInfo("LADDER FILTER", {"IN", "V/OCT", "EXP", "RES"}, {"OUT"}),
		figureInfo("DIOBE LADDER FILTER", {"IN", "V/OCT", "EXP", "RES"}, {"OUT"}),
		figureInfo("VCA", {"IN", "GAIN"}, {"OUT"}),
		figureInfo("LFO", {}, {"OUT"}),
		figureInfo("EG", {"GATE"}, {"OUT"}),
		figureInfo("SPEAKER", {"IN"}, {}),
		figureInfo("SEQUENCER", {}, {"CV", "GATE"}),
	};

	for (auto begin(figureInfos.begin()), end(figureInfos.end()), itr(begin); itr != end; ++itr)
	{
		ofFbo& figure(figures[itr->type]);
		figure.allocate(300, 250, GL_RGBA);

		figure.begin();
		{
			ofClear(0, 0);

			ofSetColor(ofColor::white);
			ofSetLineWidth(2);
			ofNoFill();

			ofDrawRectangle(0, 0, 300, 250);
			ofDrawRectangle(25, 75, 250, 100);

			ofTrueTypeFont smallFont;
			smallFont.load("Stereo.ttf", 10);

			ofTrueTypeFont bigFont;
			bigFont.load("Stereo.ttf", 14);

			float x;
			float interval;

			x = 25;
			interval = 250.0 / (itr->inputNames.size() + 1);
			for (auto _begin(itr->inputNames.begin()), _end(itr->inputNames.end()), _itr(_begin); _itr != _end; ++_itr)
			{
				x += interval;
				ofDrawLine(x, 40, x, 75);
				ofDrawLine(x, 75, x - 5, 66);
				ofDrawLine(x, 75, x + 5, 66);
				smallFont.drawString(*_itr, x - (_itr->size() * 4), 25);
			}

			x = 25;
			interval = 250.0 / (itr->outputNames.size() + 1);
			for (auto _begin(itr->outputNames.begin()), _end(itr->outputNames.end()), _itr(_begin); _itr != _end; ++_itr)
			{
				x += interval;
				ofDrawLine(x, 175, x, 210);
				ofDrawLine(x, 210, x - 5, 201);
				ofDrawLine(x, 210, x + 5, 201);
				smallFont.drawString(*_itr, x - (_itr->size() * 4), 230);
			}

			bigFont.drawString(itr->type, 150 - (itr->type.size() * 6), 135);
		}
		figure.end();
	}

	imageVertices.resize(4);
	screenVertices.resize(4);

	select = nullptr;
}

void ofApp::update()
{
	//とりあえずカメラ、センサーからデータ習得
	capture >> image;

	if(bWriteImage)
		writer << image;

	frame = controller.frame();
	hands = frame.hands();
	
	for (int i(0); i < hands.count(); i++)
	{
		Hand hand(hands[i]);

		if (!hand.isValid())
			continue;

		if (!hand.isRight())
			continue;
		
		FingerList fingers(hand.fingers().fingerType(Finger::Type::TYPE_INDEX));
		
		for (int j(0); j < fingers.count(); j++)
		{
			Finger finger(fingers[j]);

			if (!finger.isValid())
				continue;

			index = finger;
		}
	}
	
	//センサーキャリブレーションしたらタッチ検出
	if (*bCalibrateSensor)
	{
		Vector tipPosition(index.stabilizedTipPosition());
		screenPosition.set(tipPosition.x, tipPosition.y, tipPosition.z);
		screenPosition -= screenVertices[0];

		screenPosition.set
		(
			matrix.a * screenPosition.x + matrix.b * screenPosition.y + matrix.c * screenPosition.z,
			matrix.d * screenPosition.x + matrix.e * screenPosition.y + matrix.f * screenPosition.z,
			matrix.g * screenPosition.x + matrix.h * screenPosition.y + matrix.i * screenPosition.z
		);

		ofVec3f scale(windowWidth, windowHeight, 1);
		screenPosition *= scale;

		if (screenPosition.x < windowWidth && screenPosition.y < windowHeight)
		{
			moveMouse(screenPosition);

			//gui内をpressしたときのみguiは反応し、releaseするまで効果は持続
			if (screenPosition.z < *screenDistance)
			{
				if (!bMousePressed)
					pressMouse();
			}
			else
			{
				if (bMousePressed)
					releaseMouse();
			}

			bMousePressed = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
		}
	}

	//カメラキャリブレーションしたらマーカ検出
	if (*bCalibrateCamera)
	{
		vector<int> ids;
		vector<vector<Point2f>> corners;

		aruco::detectMarkers(image, dictionary, corners, ids, detectorParams);
		aruco::drawDetectedMarkers(image, corners, ids);

		float currentTime(ofGetElapsedTimef());

		for (int i(0); i < corners.size(); i++)
		{
			if (ids[i] >= modules.size())
				continue;

			Module* module(modules[ids[i]]);

			module->setDetectTime(currentTime);

			//射影変換
			perspectiveTransform(corners[i], corners[i], perspectiveMatrix);

			//ポジション確定
			ofVec2f lastPosition(module->getPosition());
			ofVec2f currentPosition;
			for (int j(0); j < corners[i].size(); j++)
			{
				currentPosition += ofVec2f(corners[i][j].x, corners[i][j].y);
			}
			currentPosition /= corners[i].size();

			module->setPosition(currentPosition);

			//ディレクション確定
			ofVec2f lastDirection(module->getDirection());
			ofVec2f currentDirection(corners[i][0].x, corners[i][0].y);

			currentDirection -= currentPosition;

			module->setDirection(currentDirection);

			//新参
			if (!module->getVisible())
			{
				module->setAxis(currentDirection);
				module->setVisible(true);
			}
			//古参
			else
			{
				float angle(currentDirection.angle(lastDirection));
				if (fabs(angle) > *detectAngle)
				{
					select = module;

					if (module->getType() != "EMPTY")
						module->update();
				}

				float distance(currentPosition.distance(lastPosition));
				if (distance > *detectDistance)
					moveModule(module);
			}
		}

		for (auto begin(modules.begin()), end(modules.end()), itr(begin); itr != end; ++itr)
		{
			if (!(*itr)->getVisible())
				continue;

			if (currentTime - (*itr)->getDetectTime() > *detectInterval)
				eraseModule(*itr);
		}
	}
}

void ofApp::draw()
{
	gui->draw();

	if (*bDrawImage)
	{
		ofSetColor(ofColor::white);

		ofImage _image;
		_image.setFromPixels(image.ptr(), image.cols, image.rows, OF_IMAGE_COLOR, false);
		_image.draw(0, 0);

		ofSetColor(ofColor::yellow);
		ofNoFill();
		ofSetLineWidth(2);

		ofBeginShape();
		for (auto begin(imageVertices.begin()), end(imageVertices.end()), itr(begin); itr != end; ++itr)
		{
			ofVertex(ofVec3f(*itr));
		}
		ofEndShape(true);
	}

	if (*bDrawHand)
	{
		ofSetColor(ofColor::white);
		ofNoFill();

		camera.begin();
		for (int i(0); i < hands.count(); i++)
		{
			Hand hand(hands[i]);

			if (hand.isValid())
				drawHand(hand);
		}
		camera.end();
	}


	if (*bCalibrateCamera)
	{
		ofSetColor(ofColor::white);

		for (auto begin(modules.begin()), end(modules.end()), itr(begin); itr != end; ++itr)
		{
			(*itr)->draw();
		}

		if (select != nullptr)
		{
			select->getGui()->draw();
			figures[select->getType()].draw(10, select->getGui()->getHeight() + 10);
		}
	}

	if (*bCalibrateSensor)
	{
		if (bMousePressed)
			ofSetColor(ofColor::darkCyan);
		else
			ofSetColor(ofColor::white);

		ofFill();
		ofDrawCircle(ofVec2f(screenPosition), 8);
	}


	ofSetColor(ofColor::white);
	ofFill();
	for (int i(0); i < 4; i++)
	{
		ofDrawCircle((i == 0 || i == 3) ? 0 : windowWidth, (i == 0 || i == 1) ? 0 : windowHeight, 10);
	}
	
	ofDrawBitmapString("FPS: " + to_string((int)ofGetFrameRate()), windowWidth - 200, windowHeight - 10);
}

void ofApp::calibrateCamera(bool& value)
{
	if (value)
	{
		if (imageVertices.size() == 4)
		{
			vector<Point2f> src;
			src.reserve(4);
			for (int i(0); i < 4; i++)
			{
				src.emplace_back(imageVertices[i].x, imageVertices[i].y);
			}

			vector<Point2f> dst;
			dst.reserve(4);
			for (int i(0); i < 4; i++)
			{
				dst.emplace_back((i == 1 || i == 2) ? windowWidth : 0, (i == 2 || i == 3) ? windowHeight : 0);
			}

			perspectiveMatrix = getPerspectiveTransform(src, dst);
		}
	}
}

void ofApp::calibrateSensor(bool& value)
{
	if (value)
	{
		if (screenVertices.size() == 4)
		{
			ofVec3f sx(screenVertices[1] - screenVertices[0]);
			ofVec3f sy(screenVertices[3] - screenVertices[0]);
			ofVec3f sz(sy.getPerpendicular(sx));

			matrix.set
			(
				sx.x, sy.x, sz.x,
				sx.y, sy.y, sz.y,
				sx.z, sy.z, sz.z
			);

			matrix.invert();
		}

		ofHideCursor();
	}
	else
	{
		ofShowCursor();
	}
}

void ofApp::minPerimeterRateChanged(float& value)
{
	detectorParams->minMarkerPerimeterRate = value;
}

void ofApp::maxPerimeterRateChanged(float& value)
{
	detectorParams->maxMarkerPerimeterRate = value;
}

void ofApp::mousePressed(ofMouseEventArgs& mouse)
{
	ofVec2f& vertex
	(
		(mouse.x < imageWidth / 2) ?
		((mouse.y < imageHeight / 2) ? imageVertices[0] : imageVertices[3]) :
		((mouse.y < imageHeight / 2) ? imageVertices[1] : imageVertices[2])
	);

	vertex.set(mouse);
}

void ofApp::keyPressed(ofKeyEventArgs& key)
{
	Vector tipPosition(index.stabilizedTipPosition());

	ofVec3f& vertex
	(
		(tipPosition.x > 0) ?
		((tipPosition.z < 0) ? screenVertices[0] : screenVertices[3]) :
		((tipPosition.z < 0) ? screenVertices[1] : screenVertices[2])
	);

	vertex.set(tipPosition.x, tipPosition.y, tipPosition.z);
}

void ofApp::pressMouse()
{
	INPUT inputs[] =
	{
		{ INPUT_MOUSE,{ 0, 0, 0, MOUSEEVENTF_LEFTDOWN, 0, 0 } }
	};

	SendInput(1, inputs, sizeof(INPUT));
}

void ofApp::releaseMouse()
{
	INPUT inputs[] =
	{
		{ INPUT_MOUSE,{ 0, 0, 0, MOUSEEVENTF_LEFTUP  , 0, 0 } }
	};

	SendInput(1, inputs, sizeof(INPUT));
}

void ofApp::moveMouse(const ofVec2f& position)
{
	int x(ofMap(position.x, 0, GetSystemMetrics(SM_CXSCREEN), 0, 65535));
	int y(ofMap(position.y, 0, GetSystemMetrics(SM_CYSCREEN), 0, 65535));

	INPUT inputs[] =
	{
		{ INPUT_MOUSE,{ x, y, 0, MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, 0, 0 } }
	};

	SendInput(1, inputs, sizeof(INPUT));
}

void ofApp::initModule(Module* module)
{	
	ofxPanel* gui(module->getGui());

	gui->setName("EMPTY");

	ofxButton* oscillator((new ofxButton())->setup("OSCILLATOR"));
	ofxButton* wavetable((new ofxButton())->setup("WAVETABLE"));
	ofxButton* multiFilter((new ofxButton())->setup("MULTI FILTER"));
	ofxButton* combFilter((new ofxButton())->setup("COMB FILTER"));
	ofxButton* ladderFilter((new ofxButton)->setup("LADDER FILTER"));
	ofxButton* diodeLadderFilter((new ofxButton())->setup("DIOBE LADDER FILTER"));
	ofxButton* tripleMorphingFilter((new ofxButton())->setup("TRIPLE MORPHING FILTER"));
	ofxButton* amplifier((new ofxButton())->setup("AMPLIFIER"));
	ofxButton* lfo((new ofxButton())->setup("LFO"));
	ofxButton* eg((new ofxButton())->setup("EG"));
	ofxButton* sequencer((new ofxButton())->setup("SEQUENCER"));
	ofxButton* speaker((new ofxButton())->setup("SPEAKER"));

	oscillator->addListener(this, &ofApp::oscillatorSelected);
	wavetable->addListener(this, &ofApp::wavetableSelected);
	multiFilter->addListener(this, &ofApp::multiFilterSelected);
	combFilter->addListener(this, &ofApp::combFilterSelected);
	ladderFilter->addListener(this, &ofApp::ladderFilterSelected);
	diodeLadderFilter->addListener(this, &ofApp::diodeLadderFilterSelected);
	tripleMorphingFilter->addListener(this, &ofApp::tripleMorphingFilterSelected);
	amplifier->addListener(this, &ofApp::amplifierSelected);
	lfo->addListener(this, &ofApp::lfoSelected);
	eg->addListener(this, &ofApp::egSelected);
	sequencer->addListener(this, &ofApp::sequencerSelected);
	speaker->addListener(this, &ofApp::speakerSelected);

	gui->add(oscillator);
	gui->add(wavetable);
	gui->add(multiFilter);
	gui->add(combFilter);
	gui->add(ladderFilter);
	gui->add(diodeLadderFilter);
	gui->add(tripleMorphingFilter);
	gui->add(amplifier);
	gui->add(lfo);
	gui->add(eg);
	gui->add(sequencer);
	gui->add(speaker);

	gui->getControl(0)->setFillColor(ofColor::darkCyan);
}

void ofApp::eraseModule(Module* module)
{
	vector<vector<Connection>> inputs(module->getInputs());
	vector<vector<Connection>> outputs(module->getOutputs());

	for (auto begin(inputs.begin()), end(inputs.end()), itr(begin); itr != end; ++itr)
	{
		if (itr->empty())
			continue;

		Connection connection(itr->back());
		disconnectModule(connection.getModule(), connection.getIndex(), module, itr - begin);
	}

	for (auto begin(outputs.begin()), end(outputs.end()), itr(begin); itr != end; ++itr)
	{
		if (itr->empty())
			continue;

		Connection connection(itr->back());
		disconnectModule(module, itr - begin, connection.getModule(), connection.getIndex());
	}

	sendDeleteMessage("module" + to_string(module->getId()));

	module->getGui()->clear();
	module->setPosition(ofVec2f::zero());
	module->setDirection(ofVec2f::one());
	module->inputsClear();
	module->outputsClear();
	module->setVisible(false);

	if (select == module)
		select = nullptr;

	initModule(module);
}

void ofApp::moveModule(Module* module)
{
	if (module->outputsEmpty())
		return;

	int outputIndex(module->getGui()->getIntSlider("OUTPUT"));

	if (module->outputEmpty(outputIndex))
	{
		Module* targetModule(nullptr);
		int targetIndex;

		float minDistance(*connectDistance);

		for (auto begin(modules.begin()), end(modules.end()), itr(begin); itr != end; ++itr)
		{
			if ((*itr) == module)
				continue;

			if ((*itr)->inputsEmpty())
				continue;

			int inputIndex((*itr)->getGui()->getIntSlider("INPUT"));

			if (!(*itr)->inputEmpty(inputIndex))
				continue;

			bool feedback(false);
			vector<vector<Connection>> outputs((*itr)->getOutputs());
			for (auto _begin(outputs.begin()), _end(outputs.end()), _itr(_begin); _itr != _end; ++_itr)
			{
				if (_itr->empty())
					continue;

				Connection connection(_itr->back());
				if (connection.getModule() != module)
					continue;

				feedback = true;
				break;
			}
			if (feedback)
				continue;

			float distance(module->getPosition().distance((*itr)->getPosition()));

			if (distance > minDistance)
				continue;

			targetModule = (*itr);
			targetIndex = inputIndex;
			minDistance = distance;
		}

		if (targetModule != nullptr)
			connectModule(module, outputIndex, targetModule, targetIndex);
	}
	else
	{
		Connection connection(module->outputBack(outputIndex));
		if (module->getPosition().distance(connection.getModule()->getPosition()) > *connectDistance)
			disconnectModule(module, outputIndex, connection.getModule(), connection.getIndex());
	}
}

void ofApp::connectModule(Module* inputModule, int outputIndex, Module* outputModule, int inputIndex)
{
	sendConnectMessage("module" + to_string(inputModule->getId()), outputIndex, "module" + to_string(outputModule->getId()), inputIndex);

	inputModule->outputEmplaceBack(outputIndex, outputModule, inputIndex);
	outputModule->inputEmplaceBack(inputIndex, inputModule, outputIndex);
}

void ofApp::disconnectModule(Module* inputModule, int outputIndex, Module* outputModule, int inputIndex)
{
	sendDisconnectMessage("module" + to_string(inputModule->getId()), outputIndex, "module" + to_string(outputModule->getId()), inputIndex);

	inputModule->outputPopBack(outputIndex);
	outputModule->inputPopBack(inputIndex);
}

void ofApp::oscillatorSelected()
{
	ofxPanel* gui(select->getGui());

	gui->clear();
	gui->setName("OSCILLATOR");

	ofxFloatSlider* param0((new ofxFloatSlider())->setup("WAVEFORM", 0, 0, 3));
	ofxFloatSlider* param1((new ofxFloatSlider())->setup("OFFSET", 0, -64, 64));
	ofxFloatSlider* param2((new ofxFloatSlider())->setup("FREQ MOD EXP", 0, -100, 100));
	ofxFloatSlider* param3((new ofxFloatSlider())->setup("FREQ MOD LIN", 0, -100, 100));
	ofxFloatSlider* param4((new ofxFloatSlider())->setup("PW", 0, 0, 100));
	ofxFloatSlider* param5((new ofxFloatSlider())->setup("PWM", 0, -100, 100));
	ofxIntSlider* input((new ofxIntSlider())->setup("INPUT", 0, 0, 4));
	ofxIntSlider* output((new ofxIntSlider())->setup("OUTPUT", 0, 0, 1));
	ofxIntSlider* interactionParam((new ofxIntSlider())->setup("INTERACTION PARAMETER", 0, 0, 5));

	param0->addListener(this, &ofApp::param0Changed);
	param1->addListener(this, &ofApp::param1Changed);
	param2->addListener(this, &ofApp::param2Changed);
	param3->addListener(this, &ofApp::param3Changed);
	param4->addListener(this, &ofApp::param4Changed);
	param5->addListener(this, &ofApp::param5Changed);
	interactionParam->addListener(this, &ofApp::interactionParamChanged);

	gui->add(param0);
	gui->add(param1);
	gui->add(param2);
	gui->add(param3);
	gui->add(param4);
	gui->add(param5);
	gui->add(input);
	gui->add(output);
	gui->add(interactionParam);

	select->setInputsSize(5);
	select->setOutputsSize(2);
	gui->getControl(0)->setFillColor(ofColor::darkCyan);
	
	sendCreateMessage("module" + to_string(select->getId()), ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "bpatcher", "Oscillator");

	for (int i(0); i < 6; i++)
	{
		string paramName("param" + to_string(select->getId()) + to_string(i));

		sendCreateMessage(paramName, ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "flonum");
		sendConnectMessage(paramName, 0, "module" + to_string(select->getId()), i + 5);
	}
}

void ofApp::wavetableSelected()
{
	ofxPanel* gui(select->getGui());

	gui->clear();
	gui->setName("WAVETABLE");

	ofxFloatSlider* param0((new ofxFloatSlider())->setup("OFFSET", 0, -64, 64));
	ofxFloatSlider* param1((new ofxFloatSlider())->setup("FREQ MOD EXP", 0, -100, 100));
	ofxFloatSlider* param2((new ofxFloatSlider())->setup("FREQ MOD LIN", 0, -100, 100));
	ofxFloatSlider* param3((new ofxFloatSlider())->setup("WAVEFORM", 0, 0, 3));
	ofxFloatSlider* param4((new ofxFloatSlider())->setup("WAVE", 0, 0, 15));
	ofxFloatSlider* param5((new ofxFloatSlider())->setup("WAVE MOD", 0, -100, 100));
	ofxFloatSlider* param6((new ofxFloatSlider())->setup("BANK", 0, 0, 15));
	ofxFloatSlider* param7((new ofxFloatSlider())->setup("BANK MOD", 0, -100, 100));
	ofxIntSlider* input((new ofxIntSlider())->setup("INPUT", 0, 0, 4));
	ofxIntSlider* output((new ofxIntSlider())->setup("OUTPUT", 0, 0, 0));
	ofxIntSlider* interactionParam((new ofxIntSlider())->setup("INTERACTION PARAMETER", 0, 0, 7));

	param0->addListener(this, &ofApp::param0Changed);
	param1->addListener(this, &ofApp::param1Changed);
	param2->addListener(this, &ofApp::param2Changed);
	param3->addListener(this, &ofApp::param3Changed);
	param4->addListener(this, &ofApp::param4Changed);
	param5->addListener(this, &ofApp::param5Changed);
	param6->addListener(this, &ofApp::param6Changed);
	param7->addListener(this, &ofApp::param7Changed);
	interactionParam->addListener(this, &ofApp::interactionParamChanged);

	gui->add(param0);
	gui->add(param1);
	gui->add(param2);
	gui->add(param3);
	gui->add(param4);
	gui->add(param5);
	gui->add(param6);
	gui->add(param7);
	gui->add(input);
	gui->add(output);
	gui->add(interactionParam);

	select->setInputsSize(5);
	select->setOutputsSize(1);
	gui->getControl(0)->setFillColor(ofColor::darkCyan);

	sendCreateMessage("module" + to_string(select->getId()), ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "bpatcher", "Wavetable");

	for (int i(0); i < 8; i++)
	{
		string paramName("param" + to_string(select->getId()) + to_string(i));

		sendCreateMessage(paramName, ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "flonum");
		sendConnectMessage(paramName, 0, "module" + to_string(select->getId()), i + 5);
	}
}

void ofApp::multiFilterSelected()
{
	ofxPanel* gui(select->getGui());

	gui->clear();
	gui->setName("MULTI FILTER");

	ofxFloatSlider* param0((new ofxFloatSlider())->setup("FITER MODE", 0, 0, 3));
	ofxFloatSlider* param1((new ofxFloatSlider())->setup("OFFSET", 0, -64, 64));
	ofxFloatSlider* param2((new ofxFloatSlider())->setup("FREQ MOD EXP", 0, -100, 100));
	ofxFloatSlider* param3((new ofxFloatSlider())->setup("RES", 0, 0, 100));
	ofxFloatSlider* param4((new ofxFloatSlider())->setup("RES MOD", 0, -100, 100));
	ofxIntSlider* input((new ofxIntSlider())->setup("INPUT", 0, 0, 3));
	ofxIntSlider* output((new ofxIntSlider())->setup("OUTPUT", 0, 0, 0));
	ofxIntSlider* interactionParam((new ofxIntSlider())->setup("INTERACTION PARAMETER", 0, 0, 4));

	param0->addListener(this, &ofApp::param0Changed);
	param1->addListener(this, &ofApp::param1Changed);
	param2->addListener(this, &ofApp::param2Changed);
	param3->addListener(this, &ofApp::param3Changed);
	param4->addListener(this, &ofApp::param4Changed);
	interactionParam->addListener(this, &ofApp::interactionParamChanged);

	gui->add(param0);
	gui->add(param1);
	gui->add(param2);
	gui->add(param3);
	gui->add(param4);
	gui->add(input);
	gui->add(output);
	gui->add(interactionParam);

	select->setInputsSize(4);
	select->setOutputsSize(1);
	gui->getControl(0)->setFillColor(ofColor::darkCyan);

	sendCreateMessage("module" + to_string(select->getId()), ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "bpatcher", "MultiFilter");

	for (int i(0); i < 5; i++)
	{
		string paramName("param" + to_string(select->getId()) + to_string(i));

		sendCreateMessage(paramName, ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "flonum");
		sendConnectMessage(paramName, 0, "module" + to_string(select->getId()), i + 4);
	}
}

void ofApp::combFilterSelected()
{
	ofxPanel* gui(select->getGui());

	gui->clear();
	gui->setName("COMB FILTER");

	ofxFloatSlider* param0((new ofxFloatSlider())->setup("OFFSET", 0, -64, 64));
	ofxFloatSlider* param1((new ofxFloatSlider())->setup("FREQ MOD EXP", 0, -100, 100));
	ofxFloatSlider* param2((new ofxFloatSlider())->setup("FEEDBACK", 0, 0, 100));
	ofxFloatSlider* param3((new ofxFloatSlider())->setup("INVERT", 0, 0, 1));
	ofxFloatSlider* param4((new ofxFloatSlider())->setup("MIX", 0, 0, 100));
	ofxIntSlider* input((new ofxIntSlider())->setup("INPUT", 0, 0, 3));
	ofxIntSlider* output((new ofxIntSlider())->setup("OUTPUT", 0, 0, 0));
	ofxIntSlider* interactionParam((new ofxIntSlider())->setup("INTERACTION PARAMETER", 0, 0, 4));

	param0->addListener(this, &ofApp::param0Changed);
	param1->addListener(this, &ofApp::param1Changed);
	param2->addListener(this, &ofApp::param2Changed);
	param3->addListener(this, &ofApp::param3Changed);
	param4->addListener(this, &ofApp::param4Changed);
	interactionParam->addListener(this, &ofApp::interactionParamChanged);

	gui->add(param0);
	gui->add(param1);
	gui->add(param2);
	gui->add(param3);
	gui->add(param4);
	gui->add(input);
	gui->add(output);
	gui->add(interactionParam);

	select->setInputsSize(4);
	select->setOutputsSize(1);
	gui->getControl(0)->setFillColor(ofColor::darkCyan);

	sendCreateMessage("module" + to_string(select->getId()), ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "bpatcher", "CombFilter");

	for (int i(0); i < 5; i++)
	{
		string paramName("param" + to_string(select->getId()) + to_string(i));

		sendCreateMessage(paramName, ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "flonum");
		sendConnectMessage(paramName, 0, "module" + to_string(select->getId()), i + 4);
	}
}

void ofApp::ladderFilterSelected()
{
	ofxPanel* gui(select->getGui());

	gui->clear();
	gui->setName("LADDER FILTER");

	ofxFloatSlider* param0((new ofxFloatSlider())->setup("OFFSET", 0, -64, 64));
	ofxFloatSlider* param1((new ofxFloatSlider())->setup("FREQ MOD EXP", 0, -100, 100));
	ofxFloatSlider* param2((new ofxFloatSlider())->setup("RES", 0, 0, 100));
	ofxFloatSlider* param3((new ofxFloatSlider())->setup("RES MOD", 0, -100, 100));
	ofxIntSlider* input((new ofxIntSlider())->setup("INPUT", 0, 0, 3));
	ofxIntSlider* output((new ofxIntSlider())->setup("OUTPUT", 0, 0, 0));
	ofxIntSlider* interactionParam((new ofxIntSlider())->setup("INTERACTION PARAMETER", 0, 0, 3));
	
	param0->addListener(this, &ofApp::param0Changed);
	param1->addListener(this, &ofApp::param1Changed);
	param2->addListener(this, &ofApp::param2Changed);
	param3->addListener(this, &ofApp::param3Changed);
	interactionParam->addListener(this, &ofApp::interactionParamChanged);

	gui->add(param0);
	gui->add(param1);
	gui->add(param2);
	gui->add(param3);
	gui->add(input);
	gui->add(output);
	gui->add(interactionParam);

	select->setInputsSize(4);
	select->setOutputsSize(1);
	gui->getControl(0)->setFillColor(ofColor::darkCyan);
	
	sendCreateMessage("module" + to_string(select->getId()), ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "bpatcher", "LadderFilter");
	
	for (int i(0); i < 4; i++)
	{
		string paramName("param" + to_string(select->getId()) + to_string(i));

		sendCreateMessage(paramName, ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "flonum");
		sendConnectMessage(paramName, 0, "module" + to_string(select->getId()), i + 4);
	}
}

void ofApp::diodeLadderFilterSelected()
{
	ofxPanel* gui(select->getGui());

	gui->clear();
	gui->setName("DIODE LADDER FILTER");

	ofxFloatSlider* param0((new ofxFloatSlider())->setup("OFFSET", 0, -64, 64));
	ofxFloatSlider* param1((new ofxFloatSlider())->setup("FREQ MOD EXP", 0, -100, 100));
	ofxFloatSlider* param2((new ofxFloatSlider())->setup("RES", 0, 0, 100));
	ofxFloatSlider* param3((new ofxFloatSlider())->setup("RES MOD", 0, -100, 100));
	ofxIntSlider* input((new ofxIntSlider())->setup("INPUT", 0, 0, 3));
	ofxIntSlider* output((new ofxIntSlider())->setup("OUTPUT", 0, 0, 0));
	ofxIntSlider* interactionParam((new ofxIntSlider())->setup("INTERACTION PARAMETER", 0, 0, 3));

	param0->addListener(this, &ofApp::param0Changed);
	param1->addListener(this, &ofApp::param1Changed);
	param2->addListener(this, &ofApp::param2Changed);
	param3->addListener(this, &ofApp::param3Changed);
	interactionParam->addListener(this, &ofApp::interactionParamChanged);

	gui->add(param0);
	gui->add(param1);
	gui->add(param2);
	gui->add(param3);
	gui->add(input);
	gui->add(output);
	gui->add(interactionParam);

	select->setInputsSize(4);
	select->setOutputsSize(1);
	gui->getControl(0)->setFillColor(ofColor::darkCyan);

	sendCreateMessage("module" + to_string(select->getId()), ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "bpatcher", "DiodeLadderFilter");

	for (int i(0); i < 4; i++)
	{
		string paramName("param" + to_string(select->getId()) + to_string(i));

		sendCreateMessage(paramName, ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "flonum");
		sendConnectMessage(paramName, 0, "module" + to_string(select->getId()), i + 4);
	}
}

void ofApp::tripleMorphingFilterSelected()
{
	ofxPanel* gui(select->getGui());

	gui->clear();
	gui->setName("TRIPLE MORPHING FILTER");

	ofxFloatSlider* param0((new ofxFloatSlider())->setup("PRESET A", 0, 0, 10));
	ofxFloatSlider* param1((new ofxFloatSlider())->setup("PRESET B", 0, 0, 10));
	ofxFloatSlider* param2((new ofxFloatSlider())->setup("MORPH", 0, 0, 100));
	ofxFloatSlider* param3((new ofxFloatSlider())->setup("MORPH MOD", 0, -100, 100));
	ofxFloatSlider* param4((new ofxFloatSlider())->setup("Q", 0, 0, 100));
	ofxFloatSlider* param5((new ofxFloatSlider())->setup("Q MOD", 0, -100, 100));
	ofxIntSlider* input((new ofxIntSlider())->setup("INPUT", 0, 0, 4));
	ofxIntSlider* output((new ofxIntSlider())->setup("OUTPUT", 0, 0, 0));
	ofxIntSlider* interactionParam((new ofxIntSlider())->setup("INTERACTION PARAMETER", 0, 0, 5));

	param0->addListener(this, &ofApp::param0Changed);
	param1->addListener(this, &ofApp::param1Changed);
	param2->addListener(this, &ofApp::param2Changed);
	param3->addListener(this, &ofApp::param3Changed);
	param4->addListener(this, &ofApp::param4Changed);
	param5->addListener(this, &ofApp::param5Changed);
	interactionParam->addListener(this, &ofApp::interactionParamChanged);

	gui->add(param0);
	gui->add(param1);
	gui->add(param2);
	gui->add(param3);
	gui->add(param4);
	gui->add(param5);
	gui->add(input);
	gui->add(output);
	gui->add(interactionParam);

	select->setInputsSize(5);
	select->setOutputsSize(1);
	gui->getControl(0)->setFillColor(ofColor::darkCyan);

	sendCreateMessage("module" + to_string(select->getId()), ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "bpatcher", "TripleMorphingFilter");

	for (int i(0); i < 6; i++)
	{
		string paramName("param" + to_string(select->getId()) + to_string(i));

		sendCreateMessage(paramName, ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "flonum");
		sendConnectMessage(paramName, 0, "module" + to_string(select->getId()), i + 5);
	}
}

void ofApp::amplifierSelected()
{
	ofxPanel* gui(select->getGui());

	gui->clear();
	gui->setName("AMPLIFIER");

	ofxFloatSlider* param0((new ofxFloatSlider())->setup("RESPONSE", 0, 0, 1));
	ofxIntSlider* input((new ofxIntSlider())->setup("INPUT", 0, 0, 1));
	ofxIntSlider* output((new ofxIntSlider())->setup("OUTPUT", 0, 0, 0));
	ofxIntSlider* interactionParam((new ofxIntSlider())->setup("INTERACTION PARAMETER", 0, 0, 0));

	param0->addListener(this, &ofApp::param0Changed);
	
	gui->add(param0);
	gui->add(input);
	gui->add(output);
	gui->add(interactionParam);
	interactionParam->addListener(this, &ofApp::interactionParamChanged);

	select->setInputsSize(2);
	select->setOutputsSize(1);
	gui->getControl(0)->setFillColor(ofColor::darkCyan);

	sendCreateMessage("module" + to_string(select->getId()), ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "bpatcher", "Amplifier");

	for (int i(0); i < 1; i++)
	{
		string paramName("param" + to_string(select->getId()) + to_string(i));

		sendCreateMessage(paramName, ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "flonum");
		sendConnectMessage(paramName, 0, "module" + to_string(select->getId()), i + 2);
	}
}

void ofApp::lfoSelected()
{
	ofxPanel* gui(select->getGui());

	gui->clear();
	gui->setName("LFO");

	ofxFloatSlider* param0((new ofxFloatSlider())->setup("WAVEFORM", 0, 0, 5));
	ofxFloatSlider* param1((new ofxFloatSlider())->setup("FREQ", 0, 0, 100));
	ofxIntSlider* output((new ofxIntSlider())->setup("OUTPUT", 0, 0, 0));
	ofxIntSlider* interactionParam((new ofxIntSlider())->setup("INTERACTION PARAMETER", 0, 0, 1));

	param0->addListener(this, &ofApp::param0Changed);
	param1->addListener(this, &ofApp::param1Changed);
	interactionParam->addListener(this, &ofApp::interactionParamChanged);

	gui->add(param0);
	gui->add(param1);
	gui->add(output);
	gui->add(interactionParam);

	select->setInputsSize(0);
	select->setOutputsSize(1);
	gui->getControl(0)->setFillColor(ofColor::darkCyan);
	
	sendCreateMessage("module" + to_string(select->getId()), ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "bpatcher", "LFO");
	
	for (int i(0); i < 2; i++)
	{
		string paramName("param" + to_string(select->getId()) + to_string(i));

		sendCreateMessage(paramName, ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "flonum");
		sendConnectMessage(paramName, 0, "module" + to_string(select->getId()), i);
	}
}

void ofApp::egSelected()
{
	ofxPanel* gui(select->getGui());

	gui->clear();
	gui->setName("EG");

	ofxFloatSlider* param0((new ofxFloatSlider())->setup("ATTACK", 0, 0, 4000));
	ofxFloatSlider* param1((new ofxFloatSlider())->setup("DECAY", 0, 0, 4000));
	ofxFloatSlider* param2((new ofxFloatSlider())->setup("SUSTEIN", 0, 0, 100));
	ofxFloatSlider* param3((new ofxFloatSlider())->setup("RELEASE", 0, 0, 4000));
	ofxFloatSlider* param4((new ofxFloatSlider())->setup("LAGATO", 0, 0, 1));
	ofxIntSlider* input((new ofxIntSlider())->setup("INPUT", 0, 0, 0));
	ofxIntSlider* output((new ofxIntSlider())->setup("OUTPUT", 0, 0, 0));
	ofxIntSlider* interactionParam((new ofxIntSlider())->setup("INTERACTION PARAMETER", 0, 0, 4));

	param0->addListener(this, &ofApp::param0Changed);
	param1->addListener(this, &ofApp::param1Changed);
	param2->addListener(this, &ofApp::param2Changed);
	param3->addListener(this, &ofApp::param3Changed);
	param4->addListener(this, &ofApp::param4Changed);
	interactionParam->addListener(this, &ofApp::interactionParamChanged);

	gui->add(param0);
	gui->add(param1);
	gui->add(param2);
	gui->add(param3);
	gui->add(param4);
	gui->add(input);
	gui->add(output);
	gui->add(interactionParam);

	select->setInputsSize(1);
	select->setOutputsSize(1);
	gui->getControl(0)->setFillColor(ofColor::darkCyan);
	
	sendCreateMessage("module" + to_string(select->getId()), ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "bpatcher", "EG");
	
	for (int i(0); i < 5; i++)
	{
		string paramName("param" + to_string(select->getId()) + to_string(i));

		sendCreateMessage(paramName, ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "flonum");
		sendConnectMessage(paramName, 0, "module" + to_string(select->getId()), i + 1);
	}
}

void ofApp::sequencerSelected()
{
	ofxPanel* gui(select->getGui());

	gui->clear();
	gui->setName("SEQUENCER");

	ofxFloatSlider* param0((new ofxFloatSlider())->setup("SWING", 0, 0, 100));
	ofxFloatSlider* param1((new ofxFloatSlider())->setup("GATE TIME", 0, 0, 100));
	vector<ofxFloatSlider*> seqParam(16);
	for (auto begin(seqParam.begin()), end(seqParam.end()), itr(begin); itr != end; ++itr)
	{
		*itr = (new ofxFloatSlider())->setup("STEP " + to_string(itr - begin + 1), 0, 0, 127);
	}
	ofxIntSlider* output((new ofxIntSlider())->setup("OUTPUT", 0, 0, 1));
	ofxIntSlider* interactionParam((new ofxIntSlider())->setup("INTERACTION PARAMETER", 0, 0, 17));

	param0->addListener(this, &ofApp::param0Changed);
	param1->addListener(this, &ofApp::param1Changed);
	seqParam[0]->addListener(this, &ofApp::seqParam0Changed);
	seqParam[1]->addListener(this, &ofApp::seqParam1Changed);
	seqParam[2]->addListener(this, &ofApp::seqParam2Changed);
	seqParam[3]->addListener(this, &ofApp::seqParam3Changed);
	seqParam[4]->addListener(this, &ofApp::seqParam4Changed);
	seqParam[5]->addListener(this, &ofApp::seqParam5Changed);
	seqParam[6]->addListener(this, &ofApp::seqParam6Changed);
	seqParam[7]->addListener(this, &ofApp::seqParam7Changed);
	seqParam[8]->addListener(this, &ofApp::seqParam8Changed);
	seqParam[9]->addListener(this, &ofApp::seqParam9Changed);
	seqParam[10]->addListener(this, &ofApp::seqParam10Changed);
	seqParam[11]->addListener(this, &ofApp::seqParam11Changed);
	seqParam[12]->addListener(this, &ofApp::seqParam12Changed);
	seqParam[13]->addListener(this, &ofApp::seqParam13Changed);
	seqParam[14]->addListener(this, &ofApp::seqParam14Changed);
	seqParam[15]->addListener(this, &ofApp::seqParam15Changed);
	interactionParam->addListener(this, &ofApp::interactionParamChanged);
	
	gui->add(param0);
	gui->add(param1);
	for (auto begin(seqParam.begin()), end(seqParam.end()), itr(begin); itr != end; ++itr)
	{
		gui->add(*itr);
	}
	gui->add(output);
	gui->add(interactionParam);

	select->setInputsSize(0);
	select->setOutputsSize(2);
	gui->getControl(0)->setFillColor(ofColor::darkCyan);
	
	sendCreateMessage("module" + to_string(select->getId()), ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "bpatcher", "Sequencer");

	for (int i(0); i < 2; i++)
	{
		string paramName("param" + to_string(select->getId()) + to_string(i));

		sendCreateMessage(paramName, ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "flonum");
		sendConnectMessage(paramName, 0, "module" + to_string(select->getId()), i);
	}

	for (int i(0); i < 2; i++)
	{
		string paramName("seqParam" + to_string(select->getId()) + to_string(i));

		sendCreateMessage(paramName, ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "flonum");
		sendConnectMessage(paramName, 0, "module" + to_string(select->getId()), i + 2);
	}
}

void ofApp::speakerSelected()
{
	ofxPanel* gui(select->getGui());

	gui->clear();
	gui->setName("SPEAKER");

	ofxFloatSlider* param0((new ofxFloatSlider())->setup("LEVEL", -30, -50, 0));
	ofxIntSlider* input((new ofxIntSlider())->setup("INPUT", 0, 0, 1));
	ofxIntSlider* interactionParam((new ofxIntSlider())->setup("INTERACTION PARAMETER", 0, 0, 0));

	param0->addListener(this, &ofApp::param0Changed);
	interactionParam->addListener(this, &ofApp::interactionParamChanged);

	gui->add(param0);
	gui->add(input);
	gui->add(interactionParam);

	select->setInputsSize(1);
	select->setOutputsSize(0);
	gui->getControl(0)->setFillColor(ofColor::darkCyan);

	sendCreateMessage("module" + to_string(select->getId()), ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "bpatcher", "Speaker");

	for (int i(0); i < 1; i++)
	{
		string paramName("param" + to_string(select->getId()) + to_string(i));

		sendCreateMessage(paramName, ofVec2f((select->getId() % 9) * 128, (select->getId() / 9) * 128), "flonum");
		sendConnectMessage(paramName, 0, "module" + to_string(select->getId()), i + 1);
	}
}

void ofApp::param0Changed(float& value)
{
	sendNumberMessage("param" + to_string(select->getId()) + "0", value);
}

void ofApp::param1Changed(float& value)
{
	sendNumberMessage("param" + to_string(select->getId()) + "1", value);
}

void ofApp::param2Changed(float& value)
{
	sendNumberMessage("param" + to_string(select->getId()) + "2", value);
}

void ofApp::param3Changed(float& value)
{
	sendNumberMessage("param" + to_string(select->getId()) + "3", value);
}

void ofApp::param4Changed(float& value)
{
	sendNumberMessage("param" + to_string(select->getId()) + "4", value);
}

void ofApp::param5Changed(float& value)
{
	sendNumberMessage("param" + to_string(select->getId()) + "5", value);
}

void ofApp::param6Changed(float& value)
{
	sendNumberMessage("param" + to_string(select->getId()) + "6", value);
}

void ofApp::param7Changed(float& value)
{
	sendNumberMessage("param" + to_string(select->getId()) + "7", value);
}

void ofApp::param8Changed(float& value)
{
	sendNumberMessage("param" + to_string(select->getId()) + "8", value);
}

void ofApp::param9Changed(float& value)
{
	sendNumberMessage("param" + to_string(select->getId()) + "9", value);
}

void ofApp::param10Changed(float& value)
{
	sendNumberMessage("param" + to_string(select->getId()) + "10", value);
}

void ofApp::param11Changed(float& value)
{
	sendNumberMessage("param" + to_string(select->getId()) + "11", value);
}

void ofApp::param12Changed(float& value)
{
	sendNumberMessage("param" + to_string(select->getId()) + "12", value);
}

void ofApp::param13Changed(float& value)
{
	sendNumberMessage("param" + to_string(select->getId()) + "13", value);
}

void ofApp::param14Changed(float& value)
{
	sendNumberMessage("param" + to_string(select->getId()) + "14", value);
}

void ofApp::param15Changed(float& value)
{
	sendNumberMessage("param" + to_string(select->getId()) + "15", value);
}

void ofApp::seqParam0Changed(float& value)
{
	sendNumberMessage("seqParam" + to_string(select->getId()) + "0", value);
	sendNumberMessage("seqParam" + to_string(select->getId()) + "1", 1);
}

void ofApp::seqParam1Changed(float& value)
{
	sendNumberMessage("seqParam" + to_string(select->getId()) + "0", value);
	sendNumberMessage("seqParam" + to_string(select->getId()) + "1", 2);
}

void ofApp::seqParam2Changed(float& value)
{
	sendNumberMessage("seqParam" + to_string(select->getId()) + "0", value);
	sendNumberMessage("seqParam" + to_string(select->getId()) + "1", 3);
}

void ofApp::seqParam3Changed(float& value)
{
	sendNumberMessage("seqParam" + to_string(select->getId()) + "0", value);
	sendNumberMessage("seqParam" + to_string(select->getId()) + "1", 4);
}

void ofApp::seqParam4Changed(float& value)
{
	sendNumberMessage("seqParam" + to_string(select->getId()) + "0", value);
	sendNumberMessage("seqParam" + to_string(select->getId()) + "1", 5);
}

void ofApp::seqParam5Changed(float& value)
{
	sendNumberMessage("seqParam" + to_string(select->getId()) + "0", value);
	sendNumberMessage("seqParam" + to_string(select->getId()) + "1", 6);
}

void ofApp::seqParam6Changed(float& value)
{
	sendNumberMessage("seqParam" + to_string(select->getId()) + "0", value);
	sendNumberMessage("seqParam" + to_string(select->getId()) + "1", 7);
}

void ofApp::seqParam7Changed(float& value)
{
	sendNumberMessage("seqParam" + to_string(select->getId()) + "0", value);
	sendNumberMessage("seqParam" + to_string(select->getId()) + "1", 8);
}

void ofApp::seqParam8Changed(float& value)
{
	sendNumberMessage("seqParam" + to_string(select->getId()) + "0", value);
	sendNumberMessage("seqParam" + to_string(select->getId()) + "1", 9);
}

void ofApp::seqParam9Changed(float& value)
{
	sendNumberMessage("seqParam" + to_string(select->getId()) + "0", value);
	sendNumberMessage("seqParam" + to_string(select->getId()) + "1", 10);
}

void ofApp::seqParam10Changed(float& value)
{
	sendNumberMessage("seqParam" + to_string(select->getId()) + "0", value);
	sendNumberMessage("seqParam" + to_string(select->getId()) + "1", 11);
}

void ofApp::seqParam11Changed(float& value)
{
	sendNumberMessage("seqParam" + to_string(select->getId()) + "0", value);
	sendNumberMessage("seqParam" + to_string(select->getId()) + "1", 12);
}

void ofApp::seqParam12Changed(float& value)
{
	sendNumberMessage("seqParam" + to_string(select->getId()) + "0", value);
	sendNumberMessage("seqParam" + to_string(select->getId()) + "1", 13);
}

void ofApp::seqParam13Changed(float& value)
{
	sendNumberMessage("seqParam" + to_string(select->getId()) + "0", value);
	sendNumberMessage("seqParam" + to_string(select->getId()) + "1", 14);
}

void ofApp::seqParam14Changed(float& value)
{
	sendNumberMessage("seqParam" + to_string(select->getId()) + "0", value);
	sendNumberMessage("seqParam" + to_string(select->getId()) + "1", 15);
}

void ofApp::seqParam15Changed(float& value)
{
	sendNumberMessage("seqParam" + to_string(select->getId()) + "0", value);
	sendNumberMessage("seqParam" + to_string(select->getId()) + "1", 16);
}

void ofApp::interactionParamChanged(int& value)
{
	ofxPanel* gui(select->getGui());
	int maxParamIndex(gui->getIntSlider("INTERACTION PARAMETER").getMax());
	for (int i(0); i <= maxParamIndex; i++)
	{
		gui->getControl(i)->setFillColor((i == value) ? ofColor::darkCyan : ofColor::gray);
	}
}

void ofApp::sendNumberMessage(const string& name, float number)
{
	ofxOscMessage message;
	message.setAddress("script");
	message.addStringArg("send");
	message.addStringArg(name);
	message.addFloatArg(number);
	sender.sendMessage(message);
}

void ofApp::sendCreateMessage(const string& name, const ofVec2f& position, const string& type)
{
	ofxOscMessage message;
	message.setAddress("script");
	message.addStringArg("newdefault");
	message.addStringArg(name);
	message.addFloatArg(position.x);
	message.addFloatArg(position.y);
	message.addStringArg(type);
	sender.sendMessage(message);
}

void ofApp::sendCreateMessage(const string& name, const ofVec2f& position, const string& type1, const string& type2)
{
	ofxOscMessage message;
	message.setAddress("script");
	message.addStringArg("newdefault");
	message.addStringArg(name);
	message.addFloatArg(position.x);
	message.addFloatArg(position.y);
	message.addStringArg(type1);
	message.addStringArg(type2);
	sender.sendMessage(message);
}

void ofApp::sendDeleteMessage(const string& name)
{
	ofxOscMessage message;
	message.setAddress("script");
	message.addStringArg("delete");
	message.addStringArg(name);
	sender.sendMessage(message);
}

void ofApp::sendConnectMessage(const string& inputName, int outputIndex, const string& outputName, int inputIndex)
{
	ofxOscMessage message;
	message.setAddress("script");
	message.addStringArg("connect");
	message.addStringArg(inputName);
	message.addIntArg(outputIndex);
	message.addStringArg(outputName);
	message.addIntArg(inputIndex);
	sender.sendMessage(message);
}

void ofApp::sendDisconnectMessage(const string& inputName, int outputIndex, const string& outputName, int inputIndex)
{
	ofxOscMessage message;
	message.setAddress("script");
	message.addStringArg("disconnect");
	message.addStringArg(inputName);
	message.addIntArg(outputIndex);
	message.addStringArg(outputName);
	message.addIntArg(inputIndex);
	sender.sendMessage(message);
}

void ofApp::drawHand(const Hand& hand) const
{
	drawPalm(hand);

	FingerList fingers(hand.fingers());

	for (int i(0); i < fingers.count(); i++)
	{
		Finger finger(fingers[i]);
		drawFinger(finger);
	}
}

void ofApp::drawPalm(const Hand& hand) const
{
	FingerList fingers(hand.fingers());

	for (int i(0); i < 5; i++)
	{
		Finger finger1(fingers.fingerType(static_cast<Finger::Type>((i) % 5))[0]);
		Finger finger2(fingers.fingerType(static_cast<Finger::Type>((i + 1) % 5))[0]);

		Bone::Type boneType1;

		if (finger1.type() == Finger::Type::TYPE_THUMB)
			boneType1 = Bone::Type::TYPE_INTERMEDIATE;

		else if (finger1.type() == Finger::Type::TYPE_PINKY)
			boneType1 = Bone::Type::TYPE_METACARPAL;

		else
			boneType1 = Bone::Type::TYPE_PROXIMAL;

		Bone::Type boneType2(Bone::Type::TYPE_PROXIMAL);

		Vector _prevJoint1(finger1.bone(boneType1).prevJoint());
		ofVec3f prevJoint1(_prevJoint1.x, _prevJoint1.y, _prevJoint1.z);

		Vector _prevJoint2(finger2.bone(boneType2).prevJoint());
		ofVec3f prevJoint2(_prevJoint2.x, _prevJoint2.y, _prevJoint2.z);

		ofVec3f position(prevJoint1.getMiddle(prevJoint2));
		ofVec3f direction(prevJoint2 - prevJoint1);

		ofPushMatrix();
		{
			ofTranslate(position);

			ofQuaternion rot;
			ofVec3f axis;
			float angle;

			ofVec3f yAxis(0, 1, 0);
			rot.makeRotate(yAxis, direction);
			rot.getRotate(angle, axis);
			ofRotate(angle, axis.x, axis.y, axis.z);

			ofDrawCylinder(3, direction.length());
		}
		ofPopMatrix();
	}
}

void ofApp::drawFinger(const Finger& finger) const
{
	if (finger.type() == Finger::Type::TYPE_THUMB || finger.type() == Finger::Type::TYPE_PINKY)
	{
		for (int i(0); i < 4; i++)
		{
			Bone bone(finger.bone(static_cast<Bone::Type>(i)));
			drawBone(bone);
		}
	}
	else
	{
		for (int i(1); i < 4; i++)
		{
			Bone bone(finger.bone(static_cast<Bone::Type>(i)));
			drawBone(bone);
		}
	}
}

void ofApp::drawBone(const Bone& bone) const
{
	Vector _center(bone.center());
	ofVec3f center(_center.x, _center.y, _center.z);

	Vector _nextJoint(bone.nextJoint());
	ofVec3f nextJoint(_nextJoint.x, _nextJoint.y, _nextJoint.z);

	Vector _prevJoint(bone.prevJoint());
	ofVec3f prevJoint(_prevJoint.x, _prevJoint.y, _prevJoint.z);

	Vector _direction(bone.direction());
	ofVec3f direction(_direction.x, _direction.y, _direction.z);

	ofDrawSphere(nextJoint, 4);
	ofDrawSphere(prevJoint, 4);

	ofPushMatrix();
	{
		ofTranslate(center);

		ofQuaternion rot;
		ofVec3f axis;
		float angle;

		ofVec3f yAxis(0, 1, 0);
		rot.makeRotate(yAxis, direction);
		rot.getRotate(angle, axis);

		ofRotate(angle, axis.x, axis.y, axis.z);

		ofDrawCylinder(3, bone.length());
	}
	ofPopMatrix();
}