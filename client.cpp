#include <iostream>
#include <unistd.h>
#include "Socket.hpp"

int main()
{
	try {
		//std::cout << "0 do things!\n";
		InetSocketAddress adr("127.0.0.1", 1234);
		//std::cout << "1 do things!\n";
		ClientStreamSocket s(&adr);
		//std::cout << "2 do things!\n";
		s.Connect(&adr);
		std::string response = "Hello, World!!!";
		s.Write(response);
		s.Read(response);
		std::cout << response << std::endl;
		int pid = getpid();
		s.Write("Going to send own PID to server...\n");
		std::cout << "Клиент: my PID is " << pid << std::endl; 
		s.Read(response);
		std::cout << response;
		s.Write(&pid, sizeof(int));
		//std::cout << "PID WRITTEN!!\n";
		s.Read(response);
		std::cout << response << std::endl;
	}
	catch (std::exception& err) {
		std::cerr << err.what() << std::endl;
	}
}
