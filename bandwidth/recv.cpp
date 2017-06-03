/*
  Maxwell Orth
  cs260
*/


#include "socket.h"
#include <cstring>
#include <cstdio>
#include <ctime>

union Packet
{
  unsigned packetnum;
  char buff[1400];
};

int main(int argc, const char **argv)
{
  if (argc < 3)
  {
    printf("Give me an ip and a port plz\n");
    return 0;
  }
  try
  {
    Socket::Init();
    
    int port = atoi(argv[2]);
    
    Socket sock(UDP_SOCKET);
    INetAddr address(argv[1], port);
    sock.Bind(address);
    printf("Counting data recieved...\n");
    
    time_t start = std::time(nullptr);
    unsigned packetsRecieved = 0;
    unsigned currentSeqNum = 0;
    Packet buffer;
    int size;
    
    do
    {
      size = sock.Recieve(&buffer, sizeof(buffer), address);
      packetsRecieved++;
      if (currentSeqNum < buffer.packetnum)
        currentSeqNum = buffer.packetnum;
    } while (size);
    
    time_t end = std::time(nullptr);
    
    double bytesPerSecond = static_cast<double>(sizeof(Packet) * packetsRecieved) / (end - start);
    
    printf("%i packets recieved\n", packetsRecieved);
    printf("%i packets sent\n", currentSeqNum);
    printf("%i packets dropped (at minimum)\n", currentSeqNum - packetsRecieved);
    printf("%f KB/s", bytesPerSecond / 1024.f);
    printf("%f MB/s", bytesPerSecond / (1024.f * 1024.f));
    printf("%f GB/s", bytesPerSecond / (1024.f * 1024.f * 1024.f));
    
    sock.Close();
    
    Socket::Cleanup();
  }
  catch (const SocketError &se)
  {
    printf("%i\n", se.Error());
  }
  
}


