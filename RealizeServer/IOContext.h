#pragma once
#include "stdafx.h"
#include "Macro.h"
#include "Packet.h"

enum IOOperation
{
	IO_READ,
	IO_SEND
};

NS_REALIZE_BEGIN

struct IOContext
{
	friend class Server;
private:
	WSAOVERLAPPED overlapped;
	WSABUF wsabuf;
	IOOperation operation;
	int number_of_bytes_sent;
};

NS_REALIZE_END
