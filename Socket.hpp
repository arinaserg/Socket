#pragma once
#include <iostream>
#include <exception>
#include <errno.h>
#include <unistd.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdexcept>
#include "SocketAddress.hpp"
//#include "ClientStreamSocket.hpp"

class Socket {
	int sd_;
protected:
	Socket(int sd) : sd_(sd) {}
public:
	int getSocket() const { return sd_; }
	void Bind(SocketAddress* pAddr) 
	{
		if (bind(sd_, pAddr->getAddr(), pAddr->getLength()) < 0) {
			throw std::runtime_error(std::string("Bind error: ") + strerror(errno));
		}
	}
	virtual bool isStream() = 0;
	virtual bool isDatagram() = 0;
	virtual ~Socket() { close(sd_); }
};

class StreamSocket : public Socket {
protected:
	StreamSocket(SocketAddress* pAddr) : Socket(socket(pAddr->getDomain(), SOCK_STREAM, IPPROTO_TCP))
	{
		if (getSocket() < 0) {
			throw std::runtime_error(std::string("Create stream socket error: ") + strerror(errno));
		}
	}
	StreamSocket(int sd) : Socket(sd) {}
public:
	bool isStream() override { return true; }
	bool isDatagram() override { return false; }
};

class ClientStreamSocket : public StreamSocket {
	//friend void ServerSocket::Accept();
public:
	ClientStreamSocket(int sd) : StreamSocket(sd) {}
	ClientStreamSocket(SocketAddress* pAddr) : StreamSocket(pAddr) {}
	void Connect(SocketAddress* serverAddress) {
		//std::cout << "Connection...\n";
		//std::cout << getSocket() << std::endl << serverAddress->getAddr() << std::endl << serverAddress->getLength() << std::endl;
		if (connect(getSocket(), serverAddress->getAddr(), serverAddress->getLength()) < 0) {
			throw std::runtime_error(std::string("CLIENT:: Connection error: ") + strerror(errno));
		}
		//std::cout << "Connection successful!\n";
	}
	~ClientStreamSocket() override { shutdown(getSocket(), SHUT_RDWR); }
	
	void Write(const void* pBytes, int n)
	{
		std::cout << "Клиент пытается записать в сокет...\n";
		size_t send_size = n; 
		if (send(getSocket(), &send_size, sizeof(send_size), 0) < (ssize_t) sizeof(send_size)) {
			throw std::runtime_error(std::string("CLIENT:: Write stream socket error: ") + strerror(errno));
		}
		if (send(getSocket(), pBytes, n, 0) < n) {
			throw std::runtime_error(std::string("CLIENT:: Write stream socket error") + strerror(errno));
		}
	}
	void Write(const char* str_to_write)
	{
		std::cout << "Клиент пытается записать в сокет...\n";
		size_t send_size = strlen(str_to_write); 
		if (send(getSocket(), &send_size, sizeof(send_size), 0) < (ssize_t) sizeof(send_size)) {
			throw std::runtime_error(std::string("CLIENT:: Write stream socket error: ") + strerror(errno));
		}
		if (send(getSocket(), (const void*) str_to_write, strlen(str_to_write), 0) < (ssize_t) strlen(str_to_write)) {
			throw std::runtime_error(std::string("CLIENT:: Write stream socket error: ") + strerror(errno));
		}
	}	
	void Write(const std::string& str_to_write)
	{
		std::cout << "Клиент пытается записать в сокет...\n";
		size_t send_size = str_to_write.size(); 
		if (send(getSocket(), &send_size, sizeof(send_size), 0) < (ssize_t) sizeof(send_size)) {
			throw std::runtime_error(std::string("CLIENT:: Write stream socket error: ") + strerror(errno));
		}
		if (send(getSocket(), str_to_write.data(), str_to_write.size(), 0) < (ssize_t) str_to_write.size()) {
			throw std::runtime_error(std::string("CLIENT:: Write stream socket error: ") + strerror(errno));
		}
	}

	int Read(void* pBytes, int n)
	{
		int ret;
		std::cout << "Клиент пытается прочитать из сокета...\n";
		size_t expecting_size = 0;
		if ((ret = recv(getSocket(), &expecting_size, sizeof(expecting_size), 0)) < 0) {
			throw std::runtime_error(std::string("CLIENT:: Read stream socket error: ") + strerror(errno));
		}
		if ((ret = recv(getSocket(), pBytes, n, 0)) < 0) {
			throw std::runtime_error(std::string("CLIENT:: Read stream socket error: ") + strerror(errno));
		}
	}
	
	int Read(char* str_read)
	{
		int ret;
		std::cout << "Клиент пытается прочитать из сокета...\n";
		size_t expecting_size = 0;
		if ((ret = recv(getSocket(), &expecting_size, sizeof(expecting_size), 0)) < 0) {
			throw std::runtime_error(std::string("CLIENT:: Read stream socket error: ") + strerror(errno));
		}
		char * new_str = new char [expecting_size];
		if ((ret = recv(getSocket(), new_str, expecting_size, 0)) < 0) {
			throw std::runtime_error(std::string("CLIENT:: Read stream socket error: ") + strerror(errno));
		}
		new_str[ret] = '\0';
		strcpy(str_read, new_str);
		delete[] new_str;
		return ret;
	}
	
	int Read(std::string& str_read)
	{
		size_t data_size = 0;
		ssize_t received = recv(getSocket(), &data_size, sizeof(data_size), 0);
		if (received < (ssize_t)sizeof(data_size)) {
			throw std::runtime_error("Failed to read data size");
		}
		str_read.resize(data_size);
		size_t total_read = 0;
		while (total_read < data_size) {
			received = recv(getSocket(), 
							str_read.data() + total_read,
							data_size - total_read, 
							0);
			if (received <= 0) {
				throw std::runtime_error("Failed to read data");
			}
			total_read += received;
		}
		return total_read;
	}
	// здесь допишите полезные перегрузки метода Read для других параметров, например const char *, std::string
};

class ServerSocket : public StreamSocket {
	static const int BACKLOG = 5;
protected:
	SocketAddress* myAddr_;
	virtual void OnAccept(ClientStreamSocket* pClientSocket) = 0;
public:
	ServerSocket(SocketAddress* pAddr) : StreamSocket(pAddr), myAddr_(pAddr)
	{
		Bind(pAddr);
		std::cout << "Listening...\n";
		if (listen(getSocket(), BACKLOG) < 0) {
			throw std::runtime_error(std::string("SERVER:: Listen server socket error: ") + strerror(errno));
		}
		std::cout << "Listened!\n";
	}
	void Accept()
	{		
		std::cout << "Accepting...\n";
		int cd = accept(getSocket(), NULL, NULL);
		std::cout << "Accepted!\n";
		if (cd < 0) {
			throw std::runtime_error(std::string("SERVER:: Accept error: ") + strerror(errno));
		}
		ClientStreamSocket * pClientSocket = new ClientStreamSocket(cd);
		OnAccept(pClientSocket);
		delete pClientSocket;
	}
};

class ServerSocketAccept : public ServerSocket {
public:
	ServerSocketAccept (SocketAddress* pAddr) : ServerSocket(pAddr) {}
	void Write(const void* pBytes, const int n, const int cd) {
		std::cout << "\t\tСервер пытается записать в сокет...\n";
		size_t send_size = n; 
		if (send(cd, &send_size, sizeof(send_size), 0) < (ssize_t) sizeof(send_size)) {
			throw std::runtime_error(std::string("CLIENT:: Write stream socket error: ") + strerror(errno));
		}
		if (send(cd, pBytes, n, 0) < n) {
			throw std::runtime_error(std::string("SERVER:: Write stream socket error: ") + strerror(errno));
		}
	}
	void Write(const char* str_to_write, const int cd)
	{
		std::cout << "\t\tСервер пытается записать в сокет...\n";
		size_t send_size = strlen(str_to_write); 
		if (send(cd, &send_size, sizeof(send_size), 0) < (ssize_t) sizeof(send_size)) {
			throw std::runtime_error(std::string("SERVER:: Write stream socket error: ") + strerror(errno));
		}
		if (send(cd, (const void*) str_to_write, strlen(str_to_write), 0) < (ssize_t) strlen(str_to_write)) {
			throw std::runtime_error(std::string("SERVER:: Write stream socket error: ") + strerror(errno));
		}
	}
	void Write(std::string str_to_write, const int cd) {
		std::cout << "\t\tСервер пытается записать в сокет...\n";
		size_t send_size = str_to_write.size(); 
		if (send(cd, &send_size, sizeof(send_size), 0) < (ssize_t) sizeof(send_size)) {
			throw std::runtime_error(std::string("SERVER:: Write stream socket error: ") + strerror(errno));
		}
		if (send(cd, str_to_write.data(), str_to_write.size(), 0) < (ssize_t) str_to_write.size()) {
			throw std::runtime_error(std::string("SERVER:: Write stream socket error: ") + strerror(errno));
		}
	}
	int Read(void* pBytes, const int n, const int cd) {
		int ret;
		std::cout << "\t\tСервер пытается прочитать в сокет...\n";
		size_t expecting_size = 0;
		if ((ret = recv(cd, &expecting_size, sizeof(expecting_size), 0)) < 0) {
			throw std::runtime_error(std::string("SERVER:: Read stream socket error: ") + strerror(errno));
		}
		if ((ret = recv(cd, pBytes, n, 0)) < 0) {
			throw std::runtime_error(std::string("SERVER:: Read stream socket error: ") + strerror(errno));
		}
		//printf ("Check ---- %s\n", (char*)pBytes);
		return ret;
	}
	int Read(char* str_read, const int cd)
	{
		int ret;
		std::cout << "\t\tСервер пытается прочитать в сокет...\n";
		size_t expecting_size = 0;
		if ((ret = recv(cd, &expecting_size, sizeof(expecting_size), 0)) < 0) {
			throw std::runtime_error(std::string("SERVER:: Read stream socket error: ") + strerror(errno));
		}
		char * new_str = new char [expecting_size];
		if ((ret = recv(cd, new_str, expecting_size, 0)) < 0) {
			throw std::runtime_error(std::string("SERVER:: Read stream socket error: ") + strerror(errno));
		}
		new_str[ret] = '\0';
		strcpy (str_read, new_str);
		delete new_str;
		return ret;
	}
	
	int Read(std::string& str_read, const int cd)
	{
		int ret;
		size_t expecting_size = str_read.size();
		std::cout << "\t\tСервер пытается прочитать в сокет...\n";
		if ((ret = recv(cd, &expecting_size, sizeof(expecting_size), 0)) < 0) {
			throw std::runtime_error(std::string("SERVER:: Read stream socket error: ") + strerror(errno));
		}
		str_read.resize(expecting_size);
		if ((ret = recv(cd, str_read.data(), str_read.size(), 0)) < 0) {
			throw std::runtime_error(std::string("SERVER:: Read stream socket error: ") + strerror(errno));
		}
		str_read.resize(ret);
		return ret;
	}  
protected:
    void OnAccept(ClientStreamSocket* pClientSocket) override {
        if (fork() == 0) {
            try {
                std::unique_ptr<ClientStreamSocket> client(pClientSocket);
                std::string first_msg;
                client->Read(first_msg);
                std::cout << "\t\t" << first_msg << std::endl;
                client->Write("Connection installed!");
            
                std::string second_msg;
                client->Read(second_msg);
                std::cout << "\t\t" << second_msg;
                client->Write("Waiting for PID...\n");
                int pid = 0;
                client->Read(&pid, sizeof(pid));
                std::cout << "\t\tPID received: " << pid << std::endl;
                
                // 6. Подтверждение
                client->Write("Nice PID, Bro!\n");
                
            } catch (const std::exception& e) {
                std::cerr << "Error handling client: " << e.what() << std::endl;
            }
            exit(0);  
        }
        delete pClientSocket;
    }
};	
	
class DatagramSocket : public Socket {
protected:
	SocketAddress* myAddr_;
public:
	DatagramSocket(SocketAddress* pAddr) : Socket(socket(pAddr->getDomain(), SOCK_DGRAM, 0)), myAddr_(pAddr)
	{
		if (getSocket() < 0) {
			throw std::runtime_error(std::string("UDP:: Create datagram socket error: ") + strerror(errno));
		}
		Bind(myAddr_);
	}
	bool isStream() override { return false; }
	bool isDatagram() override { return true; }
	
	void Write(const void* pBytes, int n, SocketAddress* to) {
		size_t send_size = n;
		if (sendto(getSocket(), &send_size, sizeof(send_size), 0, to->getAddr(), to->getLength()) < (ssize_t) sizeof(send_size)) {
			throw std::runtime_error(std::string("UDP:: Datagram socket sendto error: ") + strerror(errno));
			//throw std::exception(perror("Datagram socket sendto error"));
		}
		if (sendto(getSocket(), pBytes, send_size, 0, to->getAddr(), to->getLength()) < n) {
			throw std::runtime_error(std::string("UDP:: Datagram socket sendto error: ") + strerror(errno));
			//throw std::exception(perror("Datagram socket sendto error"));
		}
	}
	
	void Write(const char* str_to_write, SocketAddress* to) {
		size_t send_size = strlen(str_to_write);
		if (sendto(getSocket(), &send_size, sizeof(send_size), 0, to->getAddr(), to->getLength()) < (ssize_t) sizeof(send_size)) {
			throw std::runtime_error(std::string("UDP:: Datagram socket sendto error: ") + strerror(errno));
			//throw std::exception(perror("Datagram socket sendto error"));
		}
		if (sendto(getSocket(), str_to_write, send_size, 0, to->getAddr(), to->getLength()) < (ssize_t) send_size) {
			throw std::runtime_error(std::string("UDP:: Datagram socket sendto error: ") + strerror(errno));
			//throw std::exception(perror("Datagram socket sendto error"));
		}
	}
	
	void Write(std::string str_to_write, SocketAddress* to) {
		size_t send_size = str_to_write.size();
		if (sendto(getSocket(), &send_size, sizeof(send_size), 0, to->getAddr(), to->getLength()) < (ssize_t) sizeof(send_size)) {
			throw std::runtime_error(std::string("UDP:: Datagram socket sendto error: ") + strerror(errno));
			//throw std::exception(perror("Datagram socket sendto error"));
		}
		if (sendto(getSocket(), str_to_write.data(), send_size, 0, to->getAddr(), to->getLength()) < (ssize_t) send_size) {
			throw std::runtime_error(std::string("UDP:: Datagram socket sendto error: ") + strerror(errno));
			//throw std::exception(perror("Datagram socket sendto error"));
		}
	}
	
/*	void Write(const void* pBytes, const int n, const int cd) {
		std::cout << "\t\tСЕРВЕР ПЫТАЕТСЯ ЗАПИСАТЬ В СОКЕТ...\n";
		size_t send_size = n; 
		if (send(cd, &send_size, sizeof(send_size), 0) < (ssize_t) sizeof(send_size)) {
			throw std::runtime_error(std::string("CLIENT:: Write stream socket error: ") + strerror(errno));
		}
		if (send(cd, pBytes, n, 0) < n) {
			throw std::runtime_error(std::string("SERVER:: Write stream socket error: ") + strerror(errno));
		}
	} */
	
	int Read(void* pBytes, int n, SocketAddress* from) {
		int ret;
		size_t expecting_size = 0;
		socklen_t fromlen = 0;
		if ((ret = recvfrom(getSocket(), &expecting_size, sizeof(expecting_size), 0, from->getAddr(), &fromlen)) < 0) {
			throw std::runtime_error(std::string("UDP:: Datagram socket recvfrom error: ") + strerror(errno));
		}
		if ((ret = recvfrom(getSocket(), pBytes, n, 0, from->getAddr(), &fromlen)) < 0) {
			throw std::runtime_error(std::string("UDP:: Datagram socket recvfrom error: ") + strerror(errno));
		}
		return ret;
	}
	
	int Read(char* str_read, SocketAddress* from) {
		int ret;
		size_t expecting_size = 0;
		socklen_t fromlen = 0;
		if ((ret = recvfrom(getSocket(), &expecting_size, sizeof(expecting_size), 0, from->getAddr(), &fromlen)) < 0) {
			throw std::runtime_error(std::string("UDP:: Datagram socket recvfrom error: ") + strerror(errno));
		}
		char * new_str = new char [expecting_size];
		if ((ret = recvfrom(getSocket(), new_str, expecting_size, 0, from->getAddr(), &fromlen)) < 0) {
			throw std::runtime_error(std::string("UDP:: Datagram socket recvfrom error: ") + strerror(errno));
		}
		new_str[ret] = '\0';
		strcpy(str_read, new_str);
		delete new_str;
		return ret;
	}
	
	int Read(std::string& str_read, SocketAddress* from) {
		size_t data_size = 0;
		socklen_t fromlen = from->getLength();
		if (recvfrom(getSocket(), &data_size, sizeof(data_size), 0, from->getAddr(), &fromlen) < 0) {
			throw std::runtime_error("Failed to read size");
		}
		str_read.resize(data_size);
		ssize_t received = recvfrom(getSocket(), str_read.data(), data_size, 0, from->getAddr(), &fromlen);
		if (received < 0) {
			throw std::runtime_error("Failed to read data");
		}
		str_read.resize(received);
		
		return received;
	}
	// здесь допишите полезные перегрузки методов Read и Write для других параметров, например const char *, std::string
	
	/*int Read(void* pBytes, const int n, const int cd) {
		int ret;
		std::cout << "\t\tСЕРВЕР ПЫТАЕТСЯ ПРОЧИТАТЬ ИЗ СОКЕТА...\n";
		size_t expecting_size = 0;
		if ((ret = recv(cd, &expecting_size, sizeof(expecting_size), 0)) < 0) {
			throw std::runtime_error(std::string("SERVER:: Read stream socket error: ") + strerror(errno));
		}
		if ((ret = recv(cd, pBytes, n, 0)) < 0) {
			throw std::runtime_error(std::string("SERVER:: Read stream socket error: ") + strerror(errno));
		}
		//printf ("Check ---- %s\n", (char*)pBytes);
		return ret;
	} */
};


