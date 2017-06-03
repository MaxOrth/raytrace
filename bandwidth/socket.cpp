/*
  Maxwell Orth
  cs260
*/



#include "socket.h"

#include <cstdlib>
#include <cstdio>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define SOCK_CLOSE_FUNC closesocket
#define SOCK_CTRL_FUNC  ioctlsocket

#define SOCK_SHUTDOWN_SEND SD_SEND
#define SOCK_SHUTDOWN_READ SD_READ
#define SOCK_SHUTDOWN_RDWR SD_RDWR

// *nix block error EAGAIN     :(
#define WSAEAGAIN WSAEWOULDBLOCK


#else
#define SOCK_CLOSE_FUNC close
#define SOCK_CTRL_FUNC  ioctl

#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define NO_ERROR (0)
#define SOCK_SHUTDOWN_SEND SHUT_WR
#define SOCK_SHUTDOWN_READ SHUT_RD
#define SOCK_SHUTDOWN_RDWR SHUT_RDWR


#endif


#ifdef _WIN32
#define MAKESOCKERR(a) (WSA ## a)
#else
#define MAKESOCKERR(a) (a)
#endif



namespace
{
  bool TestSocketBlockError(int error)
  {
    return error == MAKESOCKERR(EWOULDBLOCK) || error == MAKESOCKERR(EALREADY) || error == MAKESOCKERR(EINPROGRESS);
  }
}

int INetAddr::DnsLookup(INetAddr & out, const char *host, const char *service)
{
  addrinfo *addrList = nullptr;
  int error = getaddrinfo(host, service, nullptr, &addrList);
  if (error)
  {
    freeaddrinfo(addrList);
    return error;
  }
  
  out = INetAddr(reinterpret_cast<internet_address *>(addrList->ai_addr));
  freeaddrinfo(addrList);
  return 0;
}

INetAddr::INetAddr(const char *address, unsigned short port)
{
  addr = new internet_address;
  memset(addr, 0, sizeof(internet_address));
  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);

  if (address)
  {
    inet_pton(addr->sin_family, address, &addr->sin_addr);
  }
  else
  {
#ifdef _WIN32
    addr->sin_addr.S_un.S_addr = INADDR_ANY;
#else
    addr->sin_addr.s_addr = INADDR_ANY;
#endif
  }
}

INetAddr::INetAddr()
{
  addr = new internet_address;
  memset(addr, 0, sizeof(internet_address));
  addr->sin_family = AF_INET;
#ifdef _WIN32
  addr->sin_addr.S_un.S_addr = INADDR_ANY;
#else
  addr->sin_addr.s_addr = INADDR_ANY;
#endif
}

INetAddr::INetAddr(const internet_address *addr_in)
{
  addr = new internet_address;
  memset(addr, 0, sizeof(internet_address));
  *addr = *addr_in;
}

INetAddr & INetAddr::operator=(INetAddr const &rhs)
{
  delete addr;
  addr = new internet_address;
  *addr = *(rhs.addr);
  return *this;
}

INetAddr::~INetAddr()
{
  delete addr;
}

INetAddr::INetAddr(const INetAddr &rhs)
{
  addr = new internet_address;
  //memcpy(addr, rhs.addr, sizeof(internet_address));
  *addr = *(rhs.addr);
}

int Socket::GetLastError()
{
#ifdef _WIN32
  int error = WSAGetLastError();
#else
  int error = errno;
  errno = 0;
#endif
  return error;
}

void Socket::Init()
{
#ifdef _WIN32
  WSAData data;
  WSAStartup(MAKEWORD(2,2), &data);
#endif
}

void Socket::Cleanup()
{
#ifdef _WIN32
  WSACleanup();
#endif
}

Socket::Socket(int type) :
  dead(false),
  bound(false),
  connected(false),
  type(type),
  recieving(false),
  sending(false)
{
  socket_ = socket(AF_INET, type, 0);
  if (socket_ != INVALID_SOCKET)
  {
    if (type == UDP_SOCKET)
    {
      recieving = true;
      sending = true;
    }
  }
}

Socket::~Socket()
{
  //if (!dead)
  //{
    SOCK_CLOSE_FUNC(socket_);
    recieving = false;
    sending = false;
    dead = true;
  //}
}

bool Socket::IsRecieving()
{
  return !dead && recieving;
}

void Socket::Kill()
{
  SOCK_CLOSE_FUNC(socket_);
  recieving = false;
  sending = false;
  dead = true;
}

bool Socket::Close()
{
  int fail = shutdown(socket_, SOCK_SHUTDOWN_SEND);
  if (fail)
  {
    int error = Socket::GetLastError();
    if (error == MAKESOCKERR(EINPROGRESS) || error == MAKESOCKERR(EWOULDBLOCK))
    {
      return false;
    }
  }
  return true;
}

void Socket::Send(const void *data, unsigned bytes)
{
  if (!connected)
  {
    throw SocketError();
    return;
  }
  int result = send(socket_, static_cast<const char *>(data), bytes, 0);
  if (result == SOCKET_ERROR)
  {
    throw SocketError();
  }
}

void Socket::Send(const void *data, unsigned bytes, INetAddr const &dest)
{
  int result = sendto(socket_, static_cast<const char *>(data), bytes, 0, (sockaddr *)dest.addr, sizeof(*dest.addr));
  if (result == SOCKET_ERROR)
  {
    throw SocketError();
  }
}

int Socket::Recieve(void *data, unsigned buffSize)
{
  if (type == TCP_SOCKET && !connected)
  {
    throw SocketError();
  }
  int result = recv(socket_, static_cast<char *>(data), buffSize, 0);
  if (result == 0)
  {
    recieving = false;
    return 0;
  }
  else if (result == SOCKET_ERROR)
  {
    int error = Socket::GetLastError();
    if (TestSocketBlockError(error))
      return 0;
    else
      throw SocketError(error);
  }
  return result;
}

int Socket::Recieve(void *data, unsigned buffSize, INetAddr &sender)
{
  if (type == TCP_SOCKET && !connected)
  {
    throw SocketError();
  }
#ifdef _WIN32
  int size = sizeof(*sender.addr);
#else
  socklen_t size = sizeof(*sender.addr);
#endif
  int result = recvfrom(socket_, static_cast<char *>(data), buffSize, 0, (sockaddr *)sender.addr, &size );
  if (result == 0)
  {
    recieving = false;
    return 0;
  }
  else if (result == SOCKET_ERROR)
  {
    int error = Socket::GetLastError();
    if (TestSocketBlockError(error))
      return 0;
    else
      throw SocketError(error);
  }
  return result;
}


void Socket::Bind(INetAddr const &addr)
{
  int result = bind(socket_, (sockaddr *)addr.addr, sizeof(*addr.addr));
  if (result == SOCKET_ERROR)
  {
    // TODO throw error
  }
  else
  {
    bound = true;
  }
}

bool Socket::Connect(const INetAddr &addr)
{
  int fail = connect(socket_, (sockaddr *)addr.addr, sizeof(*addr.addr));
  if (fail)
  {
    int error = Socket::GetLastError();
    if (TestSocketBlockError(error))
    {
      return false;
    }
    else if (error == MAKESOCKERR(EINVAL) || error == MAKESOCKERR(EISCONN))
    {
      ; // good
    }
    else
    {
      throw SocketError(error);
    }
  }
  connected = true;
  recieving = true;
  sending = true;
  return true;
}

void Socket::SetNonblocking()
{
  unsigned long mode = 1;
  int err = SOCK_CTRL_FUNC(socket_, FIONBIO, &mode);
  if (err != NO_ERROR)
  {
    SOCK_CLOSE_FUNC(socket_);
    dead = true;
    recieving = false;
    sending = false;
    socket_ = INVALID_SOCKET;
    throw SocketFailure(err);
  }
}



