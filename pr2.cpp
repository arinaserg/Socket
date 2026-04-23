#include <iostream>
#include <unistd.h>
#include "Socket.hpp"

int main()
{
	try {
		//unlink("socket1");
		//unlink("socket2");
		//UnixSocketAddress myadr("/tmp/socket2");
		//UnixSocketAddress addrto("/tmp/socket1");
		UnixSocketAddress myaddr("socket2");
		UnixSocketAddress addrto("socket1");
		DatagramSocket s(&myaddr);
		std::string response;
		s.Read(response, &addrto);
		std::cout << response << std::endl;
		int pid = getpid();
		s.Write(&pid, sizeof(int), &addrto);
		unlink("socket1");
		unlink("socket2");
	}
	catch (std::exception& err) {
		unlink("socket1");
		unlink("socket2");
		std::cerr << err.what() << std::endl;
	}
}
