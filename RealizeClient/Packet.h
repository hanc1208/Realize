#pragma once
#include "stdafx.h"
#include "Macro.h"
#include "Function.h"

NS_REALIZE_BEGIN

class Packet
{
private:
	int m_id;
	char* m_data;
	int m_data_size;
	int m_data_pos;
public:
	Packet();
	Packet(const int32_t id);
	Packet(const Packet& packet);
	~Packet();

	int getID() const;
	void setID(const int32_t id);

	char* getData() const;

	int getSize() const;

	int readFromSocket(SOCKET socket);

	string toString() const;
	string toString(const char* format);

	friend Packet& operator<<(Packet& packet, const string& data);
	friend Packet& operator<<(Packet& packet, const int32_t data);
	friend Packet& operator<<(Packet& packet, const int64_t data);
	friend Packet& operator<<(Packet& packet, const float data);
	friend Packet& operator<<(Packet& packet, const double data);
	friend Packet& operator>>(Packet& packet, string& dest);
	friend Packet& operator>>(Packet& packet, int32_t& dest);
	friend Packet& operator>>(Packet& packet, int64_t& dest);
	friend Packet& operator>>(Packet& packet, float& dest);
	friend Packet& operator>>(Packet& packet, double& dest);

	void clear();
private:
	void writeData(const void* data, int size);
	void readData(void* data, int size);
};

Packet& operator<<(Packet& packet, const string& data);
Packet& operator<<(Packet& packet, const int32_t data);
Packet& operator<<(Packet& packet, const int64_t data);
Packet& operator<<(Packet& packet, const float data);
Packet& operator<<(Packet& packet, const double data);
Packet& operator>>(Packet& packet, string& dest);
Packet& operator>>(Packet& packet, int32_t& dest);
Packet& operator>>(Packet& packet, int64_t& dest);
Packet& operator>>(Packet& packet, float& dest);
Packet& operator>>(Packet& packet, double& dest);

NS_REALIZE_END