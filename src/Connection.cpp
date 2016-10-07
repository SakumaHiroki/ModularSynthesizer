#include "Connection.h"

Connection::Connection()
	: module(nullptr), index(0)
{
}

Connection::Connection(Module* _module, int _index)
	: module(_module), index(_index)
{
}

Connection::Connection(const Connection& connection)
	: module(connection.module), index(connection.index)
{
}

Connection::~Connection()
{
}

Connection& Connection::operator=(const Connection& connection)
{
	module = connection.module;
	index = connection.index;
	return *this;
}

bool Connection::operator==(const Connection& connection) const
{
	return (module == connection.module && index == connection.index);
}

Module* Connection::getModule() const
{
	return module;
}

void Connection::setModule(Module* _module)
{
	module = _module;
}

int Connection::getIndex() const
{
	return index;
}

void Connection::setIndex(int _index)
{
	index = _index;
}