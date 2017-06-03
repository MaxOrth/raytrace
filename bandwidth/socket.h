/*
Maxwell Orth
cs260
*/

#ifdef _WIN32
#include <WinSock2.h>
#include <Ws2tcpip.h>
#else
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/cdefs.h>
#endif


#ifdef _WIN32

#define DNSERRORSTR(e) gai_strerrorA(e)

#else

#define DNSERRORSTR(e) gai_strerror(e)

#endif


#define UDP_SOCKET SOCK_DGRAM
#define TCP_SOCKET SOCK_STREAM
#define MAX_PACKET_SIZE (1400)


typedef struct sockaddr_in internet_address;
class Socket;

#ifndef WIN32
typedef int SOCKET;
#endif



class INetAddr
{
public:
  INetAddr(const char *address, unsigned short port);
  INetAddr();
  INetAddr(const INetAddr &);
  ~INetAddr();
  INetAddr(const internet_address *ia);
  INetAddr & operator=(INetAddr const &addr);

  static int DnsLookup(INetAddr &out, const char *host, const char *service = nullptr);

private:
  internet_address *addr;
  friend class Socket;
};

class Socket
{
  private:
    SOCKET socket_;
    bool dead;
    bool bound;
    bool connected;
    int type;
    bool recieving;
    bool sending;
    
    Socket(const Socket &s);
    Socket &operator=(const Socket &s);
  
  public:
    
    static void Init();
    static void Cleanup();
    static int GetLastError();
    
    
    Socket(int type);
    ~Socket();

    bool IsRecieving();

    void SetNonblocking();

    bool Close();
    void Kill();
    
    void Bind(const INetAddr &addr);
    bool Connect(const INetAddr &addr);
    
    template <typename T>
    void Send(const T *data, unsigned buffSize)
    {
      Send(static_cast<const void *>(data), buffSize);
    };
    
    void Send(const void *data, unsigned bytes);
    
    template <typename T>
    void Send(const T *data, unsigned buffSize, const INetAddr &dest)
    {
      Send(static_cast<const void *>(data), buffSize, dest);
    };
    
    void Send(const void *data, unsigned bytes, const INetAddr &dest);
    
    template <typename T>
    int Recieve(T *data, unsigned buffSize)
    {
      return Recieve(static_cast<void *>(data), buffSize);
    };
    
    template <typename T>
    int Recieve(T *data, unsigned buffSize, INetAddr &sender)
    {
      return Recieve(static_cast<void *>(data), buffSize, sender);
    };
    
    int Recieve(void *data, unsigned buffSize);
    int Recieve(void *data, unsigned buffSize, INetAddr &sender);
    
};

class SocketFailure
{
private:
  int errorCode;
public:
  SocketFailure(int sockerror) : errorCode(sockerror) {};
  SocketFailure() : errorCode(Socket::GetLastError()) {};
  int Error() const { return errorCode; };
};

class SocketError
{
private:
  int errorCode;
public:
  SocketError(int sockerror) : errorCode(sockerror) {};
  SocketError() : errorCode(Socket::GetLastError()) {};
  int Error() const { return errorCode; };
};
