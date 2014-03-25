#pragma once
#include "stdafx.h"
#include "IOContext.h"
#include "Macro.h"

NS_REALIZE_BEGIN

struct ClientData
{
	friend class Server;
public:
	SOCKET m_socket;
protected:
	IOContext m_io_context;
};

NS_REALIZE_END
