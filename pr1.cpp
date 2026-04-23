#include <iostream>
#include <unistd.h>
#include "Socket.hpp"

int main()
{
	try {
		//unlink("socket1");
		//unlink("socket2");
		//UnixSocketAddress myadr("/tmp/socket1");
		//UnixSocketAddress addrto("/tmp/socket2");
		UnixSocketAddress myaddr("socket1");
		UnixSocketAddress addrto("socket2");
		DatagramSocket s(&myaddr);
		s.Write("Hello socket!", &addrto);
		int pid = 0;
		s.Read(&pid, sizeof(int), &addrto);
		std::cout << pid << std::endl;
		//unlink("socket1");
		//unlink("socket2");
	}
	catch (std::exception& err) {
		unlink("socket1");
		unlink("socket2");
		std::cerr << err.what() << std::endl;
	}
	unlink("socket1");
	unlink("socket2");
}
