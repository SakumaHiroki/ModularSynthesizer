#pragma once

class Module;

class Connection
{

public:
	Connection();
	Connection(Module* _module, int _index);
	Connection(const Connection& connection);
	~Connection();

	Connection& operator=(const Connection& connection);
	bool operator==(const Connection& connection) const;

private:
	Module* module;
	int index;

public:
	Module* getModule() const;
	void setModule(Module* _module);

	int getIndex() const;
	void setIndex(int _index);
};