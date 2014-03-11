#pragma once
#include "stdafx.h"
#include "ClientData.h"
#include "Macro.h"
#include "Function.h"

NS_REALIZE_BEGIN

class Server
{
private:
	CRITICAL_SECTION m_critical_section;

	list<HANDLE> m_list_iocp_threads;
	HANDLE m_accept_thread;

	HANDLE m_iocp;
	SOCKET m_socket;

	list<ClientData*> m_list_completion_key;

	bool m_server_running;
public:
	Server(void);
	virtual ~Server(void);

	static unsigned int __stdcall Thread_IOCP(void* parameter);
	static unsigned int __stdcall Thread_Accept(void* parameter);

	int async_send(ClientData* client_data, Packet& packet);
	int async_recv(ClientData* client_data) ;

	void remove_client(const ClientData* client_data);

	void start(const int port);
	void end();
	bool is_running() const;


	virtual ClientData* onAccept(SOCKET socket) = 0;
	virtual void onAcceptFailed(const ClientData* client_data) = 0;
	virtual void onClose(const ClientData* client_data) = 0;
	virtual void onRead(const ClientData* client_data, Packet& packet) = 0;

	virtual void onError(const char* message) = 0;
	virtual void onWarning(const char* message) = 0;
	virtual void onLog(const char* message) = 0;
};

NS_REALIZE_END