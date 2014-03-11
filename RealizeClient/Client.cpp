#include "stdafx.h"
#include "Client.h"

NS_REALIZE_BEGIN

Client::Client(void)
{
	m_connected = false;
}

Client::~Client(void)
{
}

void Client::Thread_Async_Recv(Client* client)
{
	while(client->m_connected) {
		Packet packet;

		if(client->sync_recv(packet)) {
			for(auto it = client->m_map_async_recv_listener.begin(); it != client->m_map_async_recv_listener.end(); it++) {
				if(packet.getID() == it->first) {
					it->second(packet);
					break;
				}
			}
		}
	}
}

bool Client::sync_send(const Packet& packet)
{	
	int bytes_to_send = packet.getSize();
	int bytes_sent = 0;

	char* buf = packet.getData();

	while(bytes_to_send > 0) {
		int sent_bytes = send(m_socket, buf, bytes_to_send, 0);

		if(sent_bytes < 0) {
			this->onWarning(stringf("Client::sync_send > 패킷 전송 실패 (%d)", CROSS_FLATFORM_GETLASTERROR).c_str());
			return false;
		}

		bytes_to_send -= sent_bytes;
		bytes_sent += sent_bytes;

		buf += sent_bytes;
	}

	return true;
}
bool Client::sync_recv(Packet& packet)
{			
	int Error = packet.readFromSocket(m_socket);
	if(Error <= 0) {
		if(Error == 0)
			this->onLog("Client::sync_recv > 우아하게 연결 끊김");
		else
			this->onWarning(stringf(("Client::sync_recv > 패킷 받기 실패 (%d)"), CROSS_FLATFORM_GETLASTERROR).c_str());

		return false;
	}

	return true;
}

void Client::async_recv_register(int id, function<void(const Packet& packet)> listener)
{
	m_mutex.lock();
	m_map_async_recv_listener[id] = listener;
	m_mutex.unlock();
}
void Client::async_recv_unregister(int id)
{
	m_mutex.lock();
	m_map_async_recv_listener.erase(id);
	m_mutex.unlock();
}

void Client::connect(const char* ip, const int port)
{
#ifdef WIN32
	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		this->onError(stringf("Client::connect > WSAStartup 실패 (%d)", CROSS_FLATFORM_GETLASTERROR).c_str());
		return;
	}
#endif

	if((m_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		this->onError(stringf("Client::connect > 소켓 생성 실패 (%d)", CROSS_FLATFORM_GETLASTERROR).c_str());
		return;
	}

	struct sockaddr_in svraddr;
	svraddr.sin_family = AF_INET;
	svraddr.sin_addr.s_addr = inet_addr(ip);
	svraddr.sin_port = htons(port);

	if(::connect(m_socket, (struct sockaddr*)&svraddr, sizeof(svraddr)) == SOCKET_ERROR) {
		this->onError(stringf("Client::connect > 서버 연결 실패 (%d)", CROSS_FLATFORM_GETLASTERROR).c_str());
		return;
	}

	m_thread_async_recv = thread(Client::Thread_Async_Recv, this);

	m_connected = true;

	this->onLog(stringf("Client::connect > 서버 연결 성공 (%s:%d)", ip, port).c_str());
}

void Client::disconnect()
{
	if(m_socket != INVALID_SOCKET) {
#ifdef WIN32
		closesocket(m_socket);
#else
		close(m_socket);
#endif
		m_socket = INVALID_SOCKET;
	}

	if(m_thread_async_recv.joinable())
		m_thread_async_recv.join();

#ifdef WIN32
	WSACleanup();
#endif

	m_connected = false;
	
	this->onLog(stringf("Client::connect > 서버 연결 종료 성공").c_str());
}

bool Client::is_connected()
{
	return m_connected;
}

NS_REALIZE_END