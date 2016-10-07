#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "Connection.h"

class Module
{
public:

	Module();
	~Module();

private:

	ofxPanel* gui;
	ofVec2f position;
	ofVec2f direction;
	ofVec2f axis;
	vector<vector<Connection>> inputs;
	vector<vector<Connection>> outputs;
	
	int id;
	float detectTime;
	bool visible;

public:

	static int count;
	static ofFbo circle;
	static ofFbo line;

	ofxPanel* getGui() const;
	void setGui(ofxPanel* _gui);

	ofVec2f getPosition() const;
	void setPosition(const ofVec2f& _position);

	ofVec2f getDirection() const;
	void setDirection(const ofVec2f& _direction);

	ofVec2f getAxis() const;
	void setAxis(const ofVec2f& _axis);

	vector<vector<Connection>> getInputs() const;
	void setInputs(const vector<vector<Connection>>& _inputs);
	void inputsClear();
	bool inputsEmpty() const;
	int getInputsSize() const;
	void setInputsSize(int size);

	vector<vector<Connection>> getOutputs() const;
	void setOutputs(const vector<vector<Connection>>& _outputs);
	void outputsClear();
	bool outputsEmpty() const;
	int getOutputsSize() const;
	void setOutputsSize(int size);

	vector<Connection> getInput(int index) const;
	void setInput(int index, const vector<Connection>& _input);
	void inputClear(int index);
	bool inputEmpty(int index) const;
	Connection inputBack(int index);
	void inputPopBack(int index);
	void inputPushBack(int index, const Connection& connection);
	void inputEmplaceBack(int index, Module* inputModule, int outputIndex);

	vector<Connection> getOutput(int index) const;
	void setOutput(int index, const vector<Connection>& _output);
	void outputClear(int index);
	bool outputEmpty(int index) const;
	Connection outputBack(int index);
	void outputPopBack(int index);
	void outputPushBack(int index, const Connection& connection);
	void outputEmplaceBack(int index, Module* outputModule, int inputIndex);

	int getId() const;
	string getType() const;

	bool getVisible() const;
	void setVisible(bool _visible);

	float getDetectTime() const;
	void setDetectTime(float _detectTime);

	void update();
	void draw();
};