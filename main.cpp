#include "Buffer.hpp"
#include "BufferQueue.hpp"
#include "SyncBufferQueue.hpp"
#include "Socket.hpp"
#include "Acceptor.hpp"
#include "Connection.hpp"
#include "IOCPService.hpp"

#include <thread>
#include <iostream>

using namespace photon;

int main1()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	Socket listenSocket;
	listenSocket.init(AF_INET, SOCK_STREAM);

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = 9999;
	listenSocket.bind((sockaddr*)&addr, sizeof(addr));
	listenSocket.listen();

	Socket sendSocket;
	sendSocket.init(AF_INET, SOCK_STREAM);
	sendSocket.connect((sockaddr*)&addr, sizeof(addr));
	
	Socket recvSocket = listenSocket.accept();


	SyncBufferQueue<32> sq;
	SyncBufferQueue<32> rq;


	for (int i = 0; i < 100; ++i)
	{
		std::thread([&]()
		{
			int magicNum = 123456789;
			std::string s1 = "我们都是好孩子!";
			std::string s2 = "最最听话的孩子！";
	
			while (true)
			{
				sq.lock();
				sq <<magicNum << s1 << s2;
				sq.unlock();
			}
		}).detach();
	}

	for (int i = 0; i < 100; ++i)
	{
		std::thread([&]()
		{
			while (true)
			{
				sq.lock();
				Buffer buffer = sq.front();
				if (buffer)
				{
					int len = sendSocket.write(buffer.getData(), buffer.getSize());
					sq.pop(len);
					//std::cout << len << std::endl;
				}
				sq.unlock();
			}
		}).detach();
	}

	for (int i = 0; i < 100; ++i)
	{
		std::thread([&]()
		{
			while (true)
			{
				rq.lock();
				
				Buffer buffer = rq.reserve();
				if (buffer)
				{
					int len = recvSocket.read(buffer.getData(), buffer.getSize());
					rq.push(len);
				}
				
				rq.unlock();
			}
		}).detach();
	}

	for (int i = 0; i < 100; ++i)
	{
		std::thread([&]()
		{
			while (true)
			{
				int magicNum = 0;
				std::string s1,s2;
				rq.lock();
				rq >> magicNum >> s1 >> s2;
				std::cout << magicNum << std::endl << s1 << std::endl << s2 << std::endl;
				rq.unlock();
			}
		}).detach();
	}
	getchar();
	return 0;
}

int main2()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	PacketResolver resolver;
	EventHandler eventHandler;
	IOCPService service;

	for (int i = 0; i < 10; ++i)
	{
		std::thread([&service]()
		{
			service.run();
		}).detach();
	}

	Acceptor acceptor(&service, &resolver, &eventHandler, AF_INET, SOCK_STREAM);

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = 9999;
	if (!acceptor.listen((const sockaddr*)&addr, sizeof(addr)))
	{
		std::cout << "Listen failed" << std::endl;
	}

	/*Connection connection(&service, &resolver, &eventHandler, AF_INET, SOCK_STREAM);
	addr.sin_addr.s_addr = 16777343;	
	if (!connection.connect((const sockaddr*)&addr, sizeof(addr), true))
	{
		std::cout << "Connect failed" << std::endl;
	}

	DefaultPacket packet;
	packet.m_message = "我们都是好孩子，最最听话的孩子！";

	while (true)
	{
		if (!connection.postPacket(&packet))
		{
			std::cout << "post packet failed" << std::endl;
		}	
	}*/

	return getchar();
}

int main()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	PacketResolver resolver;
	EventHandler eventHandler;
	IOCPService service;

	for (int i = 0; i < 8; ++i)
	{
		std::thread([&service]()
		{
			service.run();
		}).detach();
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = 9999;

	Acceptor acceptor(&service, &resolver, &eventHandler, AF_INET, SOCK_STREAM);
	if (!acceptor.listen((const sockaddr*)&addr, sizeof(addr)))
	{
		std::cout << "Listen failed" << std::endl;
	}

	Connection connection(&service, &resolver, &eventHandler, AF_INET, SOCK_STREAM);
	addr.sin_addr.s_addr = 16777343;
	if (!connection.connect((const sockaddr*)&addr, sizeof(addr), true))
	{
		std::cout << "Connect failed" << std::endl;
	}

	DefaultPacket packet;
	packet.m_message = "我们都是好孩子，最最听话的孩子！";

	while (true)
	{
		if (!connection.postPacket(&packet))
		{
			//std::cout << "post packet failed" << std::endl;
		}

		//Sleep(1000);
	}

	return getchar();
}