#include "Module.h"

int Module::count(0);
ofFbo Module::circle;
ofFbo Module::line;

Module::Module()
	: gui((new ofxPanel())->setup()), position(0, 0), direction(1, 0), axis(1, 0), 
	id(count++), detectTime(0), visible(false)
{
}

Module::~Module()
{
	delete gui;
}

ofxPanel* Module::getGui() const
{
	return gui;
}

void Module::setGui(ofxPanel* _gui)
{
	gui = _gui;
}

ofVec2f Module::getPosition() const
{
	return position;
}

void Module::setPosition(const ofVec2f& _position)
{
	position = _position;
}

ofVec2f Module::getDirection() const
{
	return direction;
}

void Module::setDirection(const ofVec2f& _direction)
{
	direction = _direction;
}

ofVec2f Module::getAxis() const
{
	return axis;
}

void Module::setAxis(const ofVec2f& _axis)
{
	axis = _axis;
}

vector<vector<Connection>> Module::getInputs() const
{
	return inputs;
}

void Module::setInputs(const vector<vector<Connection>>& _inputs)
{
	inputs = _inputs;
}

void Module::inputsClear()
{
	inputs.clear();
}

bool Module::inputsEmpty() const
{
	return inputs.empty();
}

int Module::getInputsSize() const
{
	return inputs.size();
}

void Module::setInputsSize(int size)
{
	inputs.resize(size);
}

vector<vector<Connection>> Module::getOutputs() const
{
	return outputs;
}

void Module::setOutputs(const vector<vector<Connection>>& _outputs)
{
	outputs = _outputs;
}

void Module::outputsClear()
{
	outputs.clear();
}

bool Module::outputsEmpty() const
{
	return outputs.empty();
}

int Module::getOutputsSize() const
{
	return outputs.size();
}

void Module::setOutputsSize(int size)
{
	outputs.resize(size);
}

vector<Connection> Module::getInput(int index) const
{
	return inputs[index];
}

void Module::setInput(int index, const vector<Connection>& _input)
{
	inputs[index] = _input;
}

void Module::inputClear(int index)
{
	inputs[index].clear();
}

bool Module::inputEmpty(int index) const
{
	return inputs[index].empty();
}

Connection Module::inputBack(int index)
{
	return inputs[index].back();
}

void Module::inputPopBack(int index)
{
	inputs[index].pop_back();
}

void Module::inputPushBack(int index, const Connection& connection)
{
	inputs[index].push_back(connection);
}

void Module::inputEmplaceBack(int index, Module* inputModule, int outputindex)
{
	inputs[index].emplace_back(inputModule, outputindex);
}

vector<Connection> Module::getOutput(int index) const
{
	return outputs[index];
}

void Module::setOutput(int index, const vector<Connection>& _output)
{
	outputs[index] = _output;
}

void Module::outputClear(int index)
{
	outputs[index].clear();
}

bool Module::outputEmpty(int index) const
{
	return outputs[index].empty();
}

Connection Module::outputBack(int index)
{
	return outputs[index].back();
}

void Module::outputPopBack(int index)
{
	outputs[index].pop_back();
}

void Module::outputPushBack(int index, const Connection& connection)
{
	outputs[index].push_back(connection);
}

void Module::outputEmplaceBack(int index, Module* outputModule, int inputindex)
{
	outputs[index].emplace_back(outputModule, inputindex);
}

string Module::getType() const
{
	return gui->getName();
}

int Module::getId() const
{
	return id;
}

bool Module::getVisible() const
{
	return visible;
}

void Module::setVisible(bool _visible)
{
	visible = _visible;
}

float Module::getDetectTime() const
{
	return detectTime;
}

void Module::setDetectTime(float _detectTime)
{
	detectTime = _detectTime;
}

void Module::update()
{
	int paramIndex(gui->getIntSlider("INTERACTION PARAMETER"));
	string paramName(gui->getControl(paramIndex)->getName());
	ofxFloatSlider& param(gui->getFloatSlider(paramName));

	float angle(direction.angle(axis));
	param = ofMap(fabs(angle), 0, 180, param.getMin(), param.getMax());
}

void Module::draw()
{
	if (visible)
	{
		for (auto begin(outputs.begin()), end(outputs.end()), itr(begin); itr != end; ++itr)
		{
			if (itr->empty())
				continue;

			Connection output(itr->back());
			ofVec2f direction(output.getModule()->getPosition() - position);
			ofVec2f axis(1, 0);
			float distance(direction.length());
			float angle(axis.angle(direction));

			ofPushMatrix();
			{
				ofTranslate(position);
				ofRotate(angle);
				line.draw(80, -line.getHeight() / 2, distance - 160, line.getHeight());
			}
			ofPopMatrix();
		}

		circle.draw(position.x - (circle.getWidth() / 2), position.y - (circle.getHeight() / 2));
	}
}

