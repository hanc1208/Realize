#pragma once
#include "stdafx.h"
#include "Macro.h"
#include "Function.h"
#include "Packet.h"

NS_REALIZE_BEGIN

#ifdef WIN32
#define CROSS_FLATFORM_GETLASTERROR GetLastError()
#else
#define CROSS_FLATFORM_GETLASTERROR errno
#endif

class Client
{
private:
	mutex m_mutex;

	SOCKET m_socket;

	thread m_thread_async_recv;
	map<int, function<void(const Packet& packet)>> m_map_async_recv_listener;

	bool m_connected;
public:
	Client(void);
	virtual ~Client(void);

	static void Thread_Async_Recv(Client* client);

	bool sync_send(const Packet& packet);
	bool sync_recv(Packet& packet);

	void async_recv_register(int id, function<void(const Packet& packet)> listener);
	void async_recv_unregister(int id);

	void connect(const char* ip, const int port);
	void disconnect();
	bool is_connected();

	virtual void onError(const char* message) = 0;
	virtual void onWarning(const char* message) = 0;
	virtual void onLog(const char* message) = 0;
};

NS_REALIZE_END