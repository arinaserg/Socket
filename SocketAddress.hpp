#pragma once
#include <string.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

class SocketAddress {
public:
    virtual ~SocketAddress() {}
    virtual int getLength() const = 0;
    virtual short getDomain() const = 0; // AF_INET, AF_UNIX, ....
    virtual const sockaddr* getAddr() const = 0;
    virtual  sockaddr* getAddr () = 0;
};

// UnixSocketAddress - представление адреса семейства AF_UNIX
class UnixSocketAddress : public SocketAddress {
    sockaddr_un address_;
public:
    UnixSocketAddress(const char* SockName) {
    	address_.sun_family = AF_UNIX;
    	strcpy (address_.sun_path, SockName);
    }
    int getLength() const override {
    	return sizeof(address_.sun_family) + strlen(address_.sun_path);
    }
    short getDomain() const override {
    	return AF_UNIX;
    }
    const sockaddr* getAddr () const override {
    	return (const sockaddr*)&address_;
    }
    sockaddr* getAddr () override {
    	return (sockaddr*)&address_;
    }
    ~UnixSocketAddress() {}
};

// InetSocketAddress - представление адреса семейства AF_INET
class InetSocketAddress : public SocketAddress {
    sockaddr_in address_;
    short portNum_;
public:
    InetSocketAddress(const char* HostName, short PortNum) {
    	address_.sin_family = AF_INET;
    	address_.sin_port = htons(PortNum);
    	address_.sin_addr.s_addr = inet_addr(HostName);
    }
    int getLength() const override {
    	return sizeof(address_);
    }
    short getDomain() const override {
    	return AF_INET;
    }
    const sockaddr* getAddr() const override {
    	return (const sockaddr*)&address_;
    }
    ~InetSocketAddress() {}
};
