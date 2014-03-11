#include "stdafx.h"
#include "Packet.h"

NS_REALIZE_BEGIN

Packet::Packet(void)
{
	Packet(0);
}

Packet::Packet(const int id) : m_id(id)
{
	m_data = nullptr;
	m_data_size = 0;
	m_data_pos = 0;
}

Packet::Packet(const Packet& packet)
{
	m_id = packet.m_id;
	m_data = new char[packet.m_data_size];
	memcpy(m_data, packet.m_data, packet.m_data_size);
	m_data_size = packet.m_data_size;
	m_data_pos = packet.m_data_pos;
}

Packet::~Packet(void)
{
	if(m_data != nullptr) {
		delete[] m_data;
	}
}

int Packet::getID() const
{
	return m_id;
}

void Packet::setID(const int id)
{
	m_id = id;
}

char* Packet::getData() const
{
	return m_data;
}

int Packet::getSize() const
{
	return m_data_size;
}

int Packet::readFromSocket(SOCKET socket)
{
	if(m_data != nullptr) {
		delete[] m_data;
	}

	m_data_pos = 0;

	int32_t bytes_to_receive;
	int bytes_received;

	bytes_received = recv(socket, reinterpret_cast<char*>(&bytes_to_receive), sizeof(bytes_to_receive), 0);
	if(bytes_received < sizeof(bytes_to_receive))
		return SOCKET_ERROR;

	bytes_received = 0;
	m_data_size = bytes_to_receive + sizeof(int32_t);
	m_data = new char[m_data_size];

	memcpy(m_data, &bytes_to_receive, sizeof(int32_t));

	while(bytes_to_receive > 0) {
		int receive_bytes = recv(socket, m_data + sizeof(int32_t) + bytes_received, bytes_to_receive, 0);

		if(receive_bytes <= 0)
			return receive_bytes;

		bytes_to_receive -= receive_bytes;
		bytes_received += receive_bytes;
	}
	memcpy(&m_id, m_data + sizeof(int32_t), sizeof(int32_t));

	m_data_pos = 0;

	return bytes_received;
}

string Packet::toString() const
{
	return stringf("id = %d, size = %d", m_id, m_data_size);
}

// string : s
// int32_t : 4
// int64_t : 8
// float : f
// double : d
string Packet::toString(const char* format)
{
	int original_data_pos = m_data_pos;
	string ret;

	ret += toString();

	m_data_pos = 0;

	for(int i=0; i<(int) strlen(format); i++) {
		switch(format[i]) {
			case 's': {
				string data;
				*const_cast<Packet*>(this) >> data;
				ret += ", " + data;
				break;
			}
			case '4': {
				int32_t data;
				*const_cast<Packet*>(this) >> data;
				ret += stringf(", %d", data);
				break;
			}
			case '8': {
				int64_t data;
				*const_cast<Packet*>(this) >> data;
				ret += stringf(", %l64d", data);
				break;
			}
			case 'f': {
				float data;
				*const_cast<Packet*>(this) >> data;
				ret += stringf(", %f", data);
				break;
			}
			case 'd': {
				double data;
				*const_cast<Packet*>(this) >> data;
				ret += stringf(", %lf", data);
				break;
			}
		}
	}

	m_data_pos = original_data_pos;

	return ret;
}

Packet& operator<<(Packet& packet, const string& data)
{
	packet.writeData(data.c_str(), data.size() + 1);
	return packet;
}

Packet& operator<<(Packet& packet, const int32_t data)
{
	char buffer[sizeof(int32_t)];
	memcpy(buffer, &data, sizeof(int32_t));

	packet.writeData(buffer, sizeof(int32_t));
	return packet;
}

Packet& operator<<(Packet& packet, const int64_t data)
{
	char buffer[sizeof(int64_t)];
	memcpy(buffer, &data, sizeof(int64_t));

	packet.writeData(buffer, sizeof(int64_t));
	return packet;
}

Packet& operator<<(Packet& packet, const float data)
{
	char buffer[sizeof(float)];
	memcpy(buffer, &data, sizeof(float));

	packet.writeData(buffer, sizeof(float));
	return packet;
}

Packet& operator<<(Packet& packet, const double data)
{
	char buffer[sizeof(double)];
	memcpy(buffer, &data, sizeof(double));

	packet.writeData(buffer, sizeof(double));
	return packet;
}

Packet& operator>>(Packet& packet, string& dest)
{
	int data_offset = sizeof(int32_t) + sizeof(int32_t);

	dest = "";
	for(; packet.m_data[data_offset + packet.m_data_pos] != '\0'; packet.m_data_pos++) {
		dest += packet.m_data[data_offset + packet.m_data_pos];
	}

	packet.m_data_pos++;

	return packet;
}

Packet& operator>>(Packet& packet, int32_t& dest)
{
	packet.readData(&dest, sizeof(int32_t));

	return packet;
}

Packet& operator>>(Packet& packet, int64_t& dest)
{
	packet.readData(&dest, sizeof(int64_t));

	return packet;
}

Packet& operator>>(Packet& packet, float& dest)
{
	packet.readData(&dest, sizeof(float));

	return packet;
}

Packet& operator>>(Packet& packet, double& dest)
{
	packet.readData(&dest, sizeof(double));

	return packet;
}

void Packet::writeData(const void* data, int size)
{
	int data_offset = sizeof(int32_t) + sizeof(int32_t);

	if(m_data == nullptr) {
		m_data = new char[m_data_size = size + data_offset];
		memcpy(m_data + data_offset, data, size);
	}
	else {
		char* buffer = new char[m_data_size];
		memcpy(buffer, m_data, m_data_size);

		delete[] m_data;

		m_data_size += size;
		m_data = new char[m_data_size];

		memcpy(m_data, buffer, m_data_size - size);
		memcpy(m_data + data_offset + m_data_pos, data, size);

		delete[] buffer;
	}

	m_data_pos += size;

	int real_size = m_data_size - 4;

	memcpy(m_data, &real_size, sizeof(int32_t));
	memcpy(m_data + sizeof(int32_t), &m_id, sizeof(int32_t));
}

void Packet::readData(void* data, int size)
{
	int data_offset = sizeof(int32_t) + sizeof(int32_t);

	memcpy(data, m_data + data_offset + m_data_pos, size);
	m_data_pos += size;
}

void Packet::clear()
{
	if(m_data != nullptr) {
		delete[] m_data;
		m_data = nullptr;

		m_data_size = 0;
		m_data_pos = 0;
	}
}

NS_REALIZE_END