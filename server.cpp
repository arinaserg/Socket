#include <iostream>
#include "Socket.hpp"

int main()
{
	try {
		InetSocketAddress adr("127.0.0.1", 1234);
		ServerSocketAccept s(&adr);
		s.Accept();
	}
	catch (std::exception& err) {
		std::cerr << err.what();
	}
}
