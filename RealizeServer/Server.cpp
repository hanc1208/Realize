#include "stdafx.h"
#include "Server.h"

NS_REALIZE_BEGIN

Server::Server()
{
	m_server_running = false;
}

Server::~Server(void)
{
}

unsigned int __stdcall Server::Thread_IOCP(void* parameter)
{
	Server* server = (Server*) parameter;

	int Error;

	DWORD NumberOfBytesTransferred = 0;
	while(server->m_server_running) {
		ClientData* completion_key = NULL;
		IOContext* io_context = NULL;

		Packet packet;

		Error = GetQueuedCompletionStatus(server->m_iocp, &NumberOfBytesTransferred, reinterpret_cast<PULONG_PTR>(&completion_key), reinterpret_cast<LPOVERLAPPED*>(&io_context), INFINITE);
		if(Error == 0) {
			if((Error = GetLastError()) != ERROR_NETNAME_DELETED) {
				server->onError(stringf(("Server::Thread_IOCP > IO Completion Port의 큐에서 데이터 얻어오기 실패 (%d)"), Error).c_str());
				server->end();
				break;
			}
			else {
				/* must be processed */
				server->onLog(stringf(("Server::Thread_IOCP > %s, 연결 끊김 (%d)"), getIPFromSocket(completion_key->m_socket), Error).c_str());
				server->remove_client(completion_key);
				continue;
			}
		}

		if(completion_key == NULL)
			continue;

		completion_key->m_io_context = *io_context;

		if(completion_key->m_io_context.operation == IO_READ) {
			Error = packet.readFromSocket(completion_key->m_socket);
			if(Error <= 0) {
				if(Error == 0)
					server->onLog(stringf(("Server::Thread_IOCP > %s, 우아하게 연결 끊김"), getIPFromSocket(completion_key->m_socket)).c_str());
				else
					server->onWarning(stringf(("Server::Thread_IOCP > %s, 패킷 받기 실패 (%d)"), getIPFromSocket(completion_key->m_socket), GetLastError()).c_str());
				server->remove_client(completion_key);
				continue;
			}

			server->onLog(stringf(("Server::Thread_IOCP > %s, 패킷 받기 성공 .. %s"), getIPFromSocket(completion_key->m_socket), packet.toString().c_str()).c_str());

			server->onRead(completion_key, packet);

			if(server->async_recv(completion_key) == SOCKET_ERROR && (Error = GetLastError()) != WSA_IO_PENDING ) {
				server->onWarning(stringf(("Server::Thread_IOCP > %s, 패킷 받기 요청 실패 (%d)"), Error).c_str());
				server->remove_client(completion_key);
				continue;
			}
		}
		else if(completion_key->m_io_context.operation == IO_SEND) {
			if(NumberOfBytesTransferred < completion_key->m_io_context.wsabuf.len) {
				server->onLog(stringf(("Server::Thread_IOCP > %s, IO_SEND FATAL ERROR"), getIPFromSocket(completion_key->m_socket)).c_str());
			}
		}
	}

	return 0;
}

unsigned int __stdcall Server::Thread_Accept(void* parameter)
{
	Server* server = (Server*) parameter;
	DWORD Error;

	while(server->m_server_running) {
		SOCKET socket;

		if((socket = WSAAccept(server->m_socket, NULL, 0, NULL, NULL)) == INVALID_SOCKET) {
			if((Error = GetLastError()) != WSANOTINITIALISED && Error != WSAEINTR)
				server->onWarning(stringf("Server::Thread_Accept > 클라이언트와 연결 실패 (%d)", GetLastError()).c_str());
			continue;
		}

		ClientData* completion_key = server->onAccept(socket);
		completion_key->m_socket = socket;

		if(!CreateIoCompletionPort((HANDLE) socket, server->m_iocp, (ULONG_PTR) completion_key, 0)) {
			server->onWarning(stringf(("Server::Thread_Accept > 클라이언트와 IOCP 연결 실패 (%d)"), GetLastError()).c_str());
			server->onAcceptFailed(completion_key);
			closesocket(socket);
			continue;
		}

		DWORD Error;
		if(server->async_recv(completion_key) == SOCKET_ERROR && (Error = GetLastError()) != WSA_IO_PENDING) {
			server->onWarning(stringf(("Server::Thread_Accept > 패킷 받기 요청 실패 (%d)"), Error).c_str());
			server->onAcceptFailed(completion_key);
			closesocket(socket);
			continue;
		}

		EnterCriticalSection(&server->m_critical_section);
		server->m_list_completion_key.push_back(completion_key);
		server->onLog(stringf(("Server::Thread_Accept > %s, 클라이언트 연결 성공"), getIPFromSocket(completion_key->m_socket)).c_str());
		LeaveCriticalSection(&server->m_critical_section);

	}

	return 0;
}



int Server::async_send(ClientData* client_data, Packet& packet)
{
	ZeroMemory(&client_data->m_io_context, sizeof(client_data->m_io_context));

	client_data->m_io_context.operation = IO_SEND;

	client_data->m_io_context.wsabuf.len = packet.getSize();
	client_data->m_io_context.wsabuf.buf = packet.getData();

	DWORD NumberOfBytesSent = 0;
	DWORD Error;

	if(WSASend(client_data->m_socket, &client_data->m_io_context.wsabuf, 1, &NumberOfBytesSent, NULL, reinterpret_cast<LPWSAOVERLAPPED>(&client_data->m_io_context), NULL) == SOCKET_ERROR && (Error = GetLastError()) != WSA_IO_PENDING) {
		this->onWarning(stringf(("Server::async_send > %s, 패킷 전송 실패 (%d)"), getIPFromSocket(client_data->m_socket), GetLastError()).c_str());
		return Error;
	}

	this->onLog(stringf(("Server::async_send > %s, 패킷 전송 성공 .. %s"), getIPFromSocket(client_data->m_socket), packet.toString().c_str()).c_str());

	return 0;
}

int Server::async_recv(ClientData* client_data)
{
	DWORD NumberOfBytesRecvd = 0, Flags = 0;

	ZeroMemory(&client_data->m_io_context, sizeof(client_data->m_io_context));
	client_data->m_io_context.operation = IO_READ;

	return WSARecv(client_data->m_socket, &client_data->m_io_context.wsabuf, 1, &NumberOfBytesRecvd, &Flags, reinterpret_cast<LPWSAOVERLAPPED>(&client_data->m_io_context), NULL);
}

void Server::remove_client(const ClientData* client_data)
{
	EnterCriticalSection(&m_critical_section);

	this->onClose(client_data);

	closesocket(client_data->m_socket);

	m_list_completion_key.remove(const_cast<ClientData*>(client_data));

	LeaveCriticalSection(&m_critical_section);
}

void Server::start(const int port)
{
	InitializeCriticalSection(&m_critical_section);

	m_server_running = true;

	WSADATA wsadata;

	if(WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
		this->onError(stringf("Server::start > WSAStartup 실패 (%d)", GetLastError()).c_str());
		m_server_running = false;
		return;
	}

	if((m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0)) == NULL) {
		this->onError(stringf("Server::start > IO Completion Port 생성 실패 (%d)", GetLastError()).c_str());
		m_server_running = false;
		return;
	}

	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);

	int n_iocp_threads = SystemInfo.dwNumberOfProcessors * 2 + 2;

	for(int i=0; i<n_iocp_threads; i++) {
		m_list_iocp_threads.push_back((HANDLE) _beginthreadex(NULL, 0, Server::Thread_IOCP, this, 0, NULL));
	}

	if((m_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
		this->onError(stringf("Server::start > 서버 소켓 생성 실패 (%d)", GetLastError()).c_str());
		end();
		return;
	}

	sockaddr_in socket_address = {0, };

	socket_address.sin_family = AF_INET;
	socket_address.sin_port = htons(port);
	socket_address.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(m_socket, (sockaddr*) &socket_address, sizeof(socket_address)) == SOCKET_ERROR) {
		this->onError(stringf("Server::start > 소켓 바인드 실패 (%d)", GetLastError()).c_str());
		end();
		return;
	}

	if(listen(m_socket, SOMAXCONN) == SOCKET_ERROR) {
		this->onError(stringf("Server::start > 소켓 listen 실패 (%d)", GetLastError()).c_str());
		m_server_running = false;
		end();
		return;
	}

	m_accept_thread = (HANDLE) _beginthreadex(NULL, 0, Server::Thread_Accept, this, 0, NULL);

	this->onLog("Server::start > 서버 시작");
}

void Server::end()
{
	int i = 0;

	m_server_running = false;

	int ret = WaitForSingleObject(m_accept_thread, 1000);
	if(m_accept_thread != INVALID_HANDLE_VALUE && ret != WAIT_OBJECT_0 && ret != WAIT_TIMEOUT) {
		this->onWarning(stringf("Server::end > Accept 스레드 종료 대기 실패 (%d)", GetLastError()).c_str());
		m_accept_thread = INVALID_HANDLE_VALUE;
	}

	if(CloseHandle(m_accept_thread) == FALSE) {
		this->onWarning(stringf("Server::end > Accept 스레드 종료 실패 (%d)", GetLastError()).c_str());
	}

	HANDLE* array_iocp_threads = new HANDLE[m_list_iocp_threads.size()];
	for(auto it = m_list_iocp_threads.begin(); it != m_list_iocp_threads.end(); it++, i++) {
		array_iocp_threads[i] = *it;
		PostQueuedCompletionStatus(m_iocp, 0, 0, NULL);
	}

	ret = WaitForMultipleObjects(m_list_iocp_threads.size(), array_iocp_threads, TRUE, 1000);
	if(ret != WAIT_OBJECT_0 && ret != WAIT_TIMEOUT) {
		this->onWarning(stringf("Server::end > IOCP 스레드 종료 대기 실패 (%d)", GetLastError()).c_str());
	}
	else {
		for(int i=0; i<(int) m_list_iocp_threads.size(); i++) {
			if(array_iocp_threads[i] != INVALID_HANDLE_VALUE) {
				if(CloseHandle(array_iocp_threads[i]) == FALSE) {
					this->onWarning(stringf("Server::end > IOCP 스레드 (%d) 종료 실패 (%d)", i, GetLastError()).c_str());
				}
			}
		}
		m_list_iocp_threads.clear();
	}
	
	delete[] array_iocp_threads;

	for(auto it = m_list_completion_key.begin(); it != m_list_completion_key.end(); it++) {
		this->remove_client(*it);
	}

	if(m_iocp) {
		CloseHandle(m_iocp);
		m_iocp = NULL;
	}

	if(m_socket != INVALID_SOCKET) {
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}

	WSACleanup();
	DeleteCriticalSection(&m_critical_section);

	this->onLog("Server::end > 서버 종료");
}

bool Server::is_running() const
{
	return m_server_running;
}

NS_REALIZE_END