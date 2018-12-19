#include "Buffer.hpp"
#include "BufferQueue.hpp"
#include "SyncBufferQueue.hpp"
#include "WaitFreeBufferQueue.hpp"
#include "Socket.hpp"
#include "Acceptor.hpp"
#include "Connection.hpp"
#include "IOCPService.hpp"
#include "EpollService.hpp"

#include <thread>
#include <iostream>

using namespace photon;

int main1()
{
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
            std::string s1 = "We are good children !";
            std::string s2 = "The best children !";

            while (true)
            {
                sq.lock();
                sq << magicNum << s1 << s2;
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
                    ssize_t len = sendSocket.write(buffer.getData(), buffer.getSize());
                    sq.pop((uint32_t)len);
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
                    ssize_t len = recvSocket.read(buffer.getData(), buffer.getSize());
                    rq.push((uint32_t)len);
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
                std::string s1, s2;
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
	PacketResolver resolver;
	EventHandler eventHandler;
#ifdef _WIN32
	IOCPService service;
#elif defined __linux__
    EpollService service;
#endif

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

	PacketResolver resolver;
	EventHandler eventHandler;
#ifdef _WIN32
    IOCPService service;
#elif defined __linux__
    EpollService service;
#endif

    uint32_t threadCount = std::thread::hardware_concurrency() * 2;
	for (uint32_t i = 1; i < threadCount; ++i)
	{
		std::thread([&service]()
		{
			service.run();
		}).detach();
	}

	Acceptor acceptor(&service, &resolver, &eventHandler, AF_INET, SOCK_STREAM);
    acceptor.getSocket().setReuseAddr(true);
    acceptor.getSocket().setReusePort(true);

	if (!acceptor.listen(11111))
	{
		std::cout << "Listen failed" << std::endl;
	}

	Connection connection(&service, &resolver, &eventHandler, AF_INET, SOCK_STREAM);
	
	if (!connection.connect("127.0.0.1", 11111))
	{
		std::cout << "Connect failed" << std::endl;
	}

    //Connection *conn = new Connection(&service, &resolver, &eventHandler, AF_INET, SOCK_STREAM);
    //if (!conn->connect("127.0.0.1", 11111))
    //{
    //    std::cout << "Connect failed" << std::endl;
    //}

    //conn->close();

	DefaultPacket packet;
	packet.m_message = "我们都是好孩子，最最听话的孩子！";

	while (true)
	{
        if (connection.connected())
        {
            if (!connection.postPacket(&packet))
            {
                //std::cout << "post packet failed" << std::endl;
            }
        }
        //conn->postPacket(&packet);
		//Sleep(1000);
	}

    service.run();
	return 0;
}

int main_WaitFreeBufferQueue()
{
    WaitFreeBufferQueue<1024*128> q;

    for (int i = 0; i < 12; ++i)
    {
        std::thread([&]()
        {
            int magicNum = 123456789;

            while (true)
            {
                q << magicNum;
            }
        }).detach();

        std::cout << "Start push thread :" << i << std::endl;
    }


    for (int i = 0; i < 12; ++i)
    {
        std::thread([&]()
        {
            while (true)
            {
                int magicNum = 0;
                
                q >> magicNum ;
                std::cout << magicNum << std::endl;
            }
        }).detach();

        std::cout << "Start pop thread :" << i << std::endl;
    }
    getchar();
    return 0;
}