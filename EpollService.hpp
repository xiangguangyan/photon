#ifdef __linux__

#ifndef _EPOLL_SERVICE_HPP_
#define _EPOLL_SERVICE_HPP_

#include <sys/epoll.h>
#include <iostream>

#include "IOService.hpp"
#include "IOComponent.hpp"
#include "Connection.hpp"
#include "Acceptor.hpp"

namespace photon
{
    class EpollService : public IOService
    {
    public:
        EpollService()
        {
            m_epoll = ::epoll_create(MAX_SOCKET_COUNT);
            if (m_epoll < 0)
            {
                std::cout << "Create epoll failed:" << errno << std::endl;
            }
        }

        ~EpollService()
        {
            if (m_epoll > 0)
            {
                ::close(m_epoll);
            }
        }

        virtual bool addConnection(Connection* connection) override
        {
            struct epoll_event event;
            event.data.ptr = connection;
            event.events = EPOLLET;
            if (::epoll_ctl(m_epoll, EPOLL_CTL_ADD, connection->getSocket().getSocket(), &event) != 0)
            {
                std::cout << "Add connection epoll ctl failed:" << errno << std::endl;
                return false;
            }

            connection->addRef();
            connection->getSocket().setBlock(false);
            return true;
        }

        virtual bool addAcceptor(Acceptor* acceptor) override
        {
            struct epoll_event event;
            event.data.ptr = acceptor;
            event.events = EPOLLET;
            if (::epoll_ctl(m_epoll, EPOLL_CTL_ADD, acceptor->getSocket().getSocket(), &event) != 0)
            {
                std::cout << "Add acceptor epoll ctl failed: " << errno << std::endl;
                return false;
            }

            acceptor->addRef();
            acceptor->getSocket().setBlock(false);
            return true;
        }

        virtual bool removeConnection(Connection* connection) override
        {
            struct epoll_event event;
            event.data.ptr = NULL;
            event.events = 0u;
            if (::epoll_ctl(m_epoll, EPOLL_CTL_DEL, connection->getSocket().getSocket(), &event) != 0)
            {
                std::cout << "Remove connection epoll ctl failed: " << errno << std::endl;
                return false;
            }
            connection->decRef();
            return true;
        }

        virtual bool removeAcceptor(Acceptor* acceptor) override
        {
            struct epoll_event event;
            event.data.ptr = NULL;
            event.events = 0u;
            if (::epoll_ctl(m_epoll, EPOLL_CTL_DEL, acceptor->getSocket().getSocket(), &event) != 0)
            {
                std::cout << "Remove acceptor epoll ctl failed: " << errno << std::endl;
                return false;
            }
            acceptor->decRef();
            return true;
        }

        virtual bool startRead(Connection* connection) override
        {
            if (connection->m_state.load() & IOComponent::IOComponentState::READING)
            {
                return true;
            }

            //if (!connection->read())
            //{
            //    return false;
            //}

            //if (connection->m_readQueue.full())
            //{
            //    return true;
            //}

            struct epoll_event event;
            event.data.ptr = connection;
            
            if (connection->m_state.load() & IOComponent::IOComponentState::WRITING)
            {
                event.events = EPOLLIN | EPOLLOUT | EPOLLET;
            }
            else
            {
                event.events = EPOLLIN | EPOLLET;
            }

            if (::epoll_ctl(m_epoll, EPOLL_CTL_MOD, connection->getSocket().getSocket(), &event) != 0)
            {
                std::cout << "Start read epoll ctl failed: " << errno << std::endl;
                return false;
            }

            connection->m_state |= IOComponent::IOComponentState::READING;
            return true;
        }

        bool stopRead(Connection* connection)
        {
            if (!(connection->m_state.load() & IOComponent::IOComponentState::READING))
            {
                return true;
            }

            struct epoll_event event;
            event.data.ptr = connection;

            if (connection->m_state.load() & IOComponent::IOComponentState::WRITING)
            {
                event.events = EPOLLOUT | EPOLLET;
            }
            else
            {
                event.events = EPOLLET;
            }

            if (::epoll_ctl(m_epoll, EPOLL_CTL_MOD, connection->getSocket().getSocket(), &event) != 0)
            {
                std::cout << "Stop read epoll ctl failed: " << errno << std::endl;
                return false;
            }

            connection->m_state &= ~IOComponent::IOComponentState::READING;
            return true;
        }

        virtual bool startWrite(Connection* connection) override
        {
            if (connection->m_state.load() & IOComponent::IOComponentState::WRITING)
            {
                return true;
            }

            //if (!connection->write())
            //{
            //    return false;
            //}

            //if (connection->m_writeQueue.empty())
            //{
            //    return true;
            //}

            struct epoll_event event;
            event.data.ptr = connection;

            if (connection->m_state.load() & IOComponent::IOComponentState::READING)
            {
                event.events = EPOLLIN | EPOLLOUT | EPOLLET;
            }
            else
            {
                event.events = EPOLLOUT | EPOLLET;
            }

            if (::epoll_ctl(m_epoll, EPOLL_CTL_MOD, connection->getSocket().getSocket(), &event) != 0)
            {
                std::cout << "Start write epoll ctl failed: " << errno << std::endl;
                return false;
            }

            connection->m_state |= IOComponent::IOComponentState::WRITING;
            return true;
        }

        bool stopWrite(Connection* connection)
        {
            if (!(connection->m_state.load() & IOComponent::IOComponentState::WRITING))
            {
                return true;
            }

            struct epoll_event event;
            event.data.ptr = connection;

            if (connection->m_state.load() & IOComponent::IOComponentState::READING)
            {
                event.events = EPOLLIN | EPOLLET;
            }
            else
            {
                event.events = EPOLLET;
            }

            if (::epoll_ctl(m_epoll, EPOLL_CTL_MOD, connection->getSocket().getSocket(), &event) != 0)
            {
                std::cout << "Stop write epoll ctl failed: " << errno << std::endl;
                return false;
            }

            connection->m_state &= ~IOComponent::IOComponentState::WRITING;
            return true;
        }

        virtual bool startAccept(Acceptor* acceptor) override
        {
            if (acceptor->m_state.load() & IOComponent::IOComponentState::ACCEPTING)
            {
                return true;
            }

            struct epoll_event event;
            event.data.ptr = acceptor;
            event.events = EPOLLET | EPOLLIN;

            if (::epoll_ctl(m_epoll, EPOLL_CTL_MOD, acceptor->getSocket().getSocket(), &event) != 0)
            {
                std::cout << "Start accept epoll ctl failed: " << errno << std::endl;
                return false;
            }

            acceptor->m_state |= IOComponent::IOComponentState::ACCEPTING;
            return true;
        }

        virtual bool startConnect(Connection* connection, const sockaddr* addr, socklen_t addrLen, const sockaddr* localAddr = nullptr) override
        {
            if (connection->m_state.load() & (IOComponent::IOComponentState::CONNECTING | IOComponent::IOComponentState::CONNECTED))
            {
                return false;
            }

            if (nullptr != localAddr)
            {
                if (!connection->getSocket().bind(localAddr, addrLen))
                {
                    return false;
                }
            }

            if (0 != connect(connection->getSocket().getSocket(), addr, addrLen))
            {
                if (EINPROGRESS != errno)
                {
                    std::cout << "Start connect connect failed: " << errno << std::endl;
                    return false;
                }

                struct epoll_event event;
                event.data.ptr = connection;
                event.events = EPOLLET | EPOLLOUT;

                if (::epoll_ctl(m_epoll, EPOLL_CTL_MOD, connection->getSocket().getSocket(), &event) != 0)
                {
                    std::cout << "Start connect epoll ctl failed: " << errno << std::endl;
                    return false;
                }

                connection->m_state |= IOComponent::IOComponentState::CONNECTING;
                return true;
            }

            connection->m_state |= IOComponent::IOComponentState::CONNECTED;
            return true;
        }

        virtual int getCompleteIOComponent(IOCompleteEvent(&completeEvents)[MAX_SOCKET_COUNT], int32_t timeOutMilliSeconds) override
        {
            struct epoll_event events[MAX_SOCKET_COUNT];
            int ret = ::epoll_wait(m_epoll, events, MAX_SOCKET_COUNT, timeOutMilliSeconds);
            if (ret == -1) 
            {
                std::cout << "Get complete io component epoll wait failed: " << errno << std::endl;
                return -1;
            }
           
			for (int i = 0; i < ret; ++i)
			{
				((IOComponent*)events[i].data.ptr)->addRef();
			}

            int completeCount = ret;
            for (int i = 0; i < ret; ++i)
            {
                IOComponent* component = (IOComponent*)events[i].data.ptr;
				/*if (component == NULL || component->m_ioService != this)
				{
					continue;
				}
                component->addRef();*/

                completeEvents[i].m_type = 0;
                completeEvents[i].m_component = component;

                Connection* connection = dynamic_cast<Connection*>(component);
                if (connection != nullptr)
                {
                    if (events[i].events & EPOLLIN)
                    {
                        completeEvents[i].m_type |= IOCompleteEvent::READ_COMPLETE;
                        connection->m_readQueue.lock();
                        if (!connection->read())
                        {
                            connection->m_state |= IOComponent::IOComponentState::IOERROR;
                            completeEvents[i].m_type |= IOCompleteEvent::IO_ERROR;
                        }
                        else if (connection->m_readQueue.full())
                        {
                            if (!stopRead(connection))
                            {
                                connection->m_state |= IOComponent::IOComponentState::IOERROR;
                                completeEvents[i].m_type |= IOCompleteEvent::IO_ERROR;
                            }
                        }
                        connection->m_readQueue.unlock();
                    }
                    if (events[i].events & EPOLLOUT)
                    {
                        if (connection->m_state.load() & IOComponent::IOComponentState::CONNECTING)
                        {
                            //检查是否连接成功
                            int optval = 0;
                            socklen_t optlen = sizeof(optval);
                            int ret = connection->getSocket().getOption(SOL_SOCKET, SO_ERROR, &optval, optlen);
                            if (ret < 0 || optval) 
                            {
                                connection->m_state |= IOComponent::IOComponentState::IOERROR;
                                completeEvents[i].m_type |= IOCompleteEvent::IO_ERROR;
                            }
                            else 
                            {
                                connection->m_state &= ~IOComponent::IOComponentState::CONNECTING;
                                connection->m_state |= IOComponent::IOComponentState::CONNECTED;
                                completeEvents[i].m_type |= IOCompleteEvent::CONNECT_COMPLETE;
                            }
                        }
                        else
                        {
                            completeEvents[i].m_type |= IOCompleteEvent::WRITE_COMPLETE;
                            connection->m_writeQueue.lock();
                            if (!connection->write())
                            {
                                connection->m_state |= IOComponent::IOComponentState::IOERROR;
                                completeEvents[i].m_type |= IOCompleteEvent::IO_ERROR;
                            }
                            else if (connection->m_writeQueue.empty())
                            {
                                if (!stopWrite(connection))
                                {
                                    connection->m_state |= IOComponent::IOComponentState::IOERROR;
                                    completeEvents[i].m_type |= IOCompleteEvent::IO_ERROR;
                                }
                            }
                            connection->m_writeQueue.unlock();
                        }
                    }
                    if (events[i].events & EPOLLERR)
                    {
                        connection->m_state |= IOComponent::IOERROR;
                        completeEvents[i].m_type |= IOCompleteEvent::IO_ERROR;
                    }
                }
                else
                {
                    Acceptor* acceptor = dynamic_cast<Acceptor*>(component);
                    if (acceptor != nullptr)
                    {
                        if (events[i].events & EPOLLIN)
                        {
                            completeEvents[i].m_type |= IOCompleteEvent::ACCEPT_COMPLETE;
                            Connection* conn = nullptr;
                            while((conn = acceptor->accept()) != nullptr)
                            {
                                conn->addRef();
                                conn->m_state |= IOComponent::IOComponentState::CONNECTED;
                                conn->m_acceptor = acceptor;
                                completeEvents[completeCount].m_type = IOCompleteEvent::ACCEPT_COMPLETE;
                                completeEvents[completeCount].m_component = conn;
                                ++completeCount;
                            }
                        }
                        if (events[i].events & EPOLLOUT)
                        {
                            std::cout << "Acceptor epoll out !\n";
                        }
                        if (events[i].events & EPOLLERR)
                        {
                            acceptor->m_state |= IOComponent::IOComponentState::IOERROR;
                            completeEvents[i].m_type |= IOCompleteEvent::IO_ERROR;
                        }
                    }
                }
            }

            return completeCount;
        }

    private:
        int m_epoll;
    };
}

#endif // _EPOLL_SERVICE_HPP_

#endif // __linux__