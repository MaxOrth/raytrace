/*
  Maxwell Orth
  cs260
  HTTP client
*/

#include "socket.h"

#include <string>
#include <sstream>
#include <chrono>
#include <thread>

#include <cstdio>
#include <string.h>
#include <vector>
#include <iterator>
#include <map>
#include <algorithm>
#include <cctype>
#include <Ctime>

union Packet
{
  unsigned packetnum;
  char buff[1400];
};

int main(int argc, char **argv)
{
  if (argc < 4)
  {
    printf("Give me an ip and a port and a duration (seconds) plz\n");
    return 0;
  }
  
  
  int port = atoi(argv[2]);
  int duration = atoi(argv[3]); 
  time_t start = std::time(nullptr);
  
  Socket::Init();
  
  Packet buff;
  memset(&buff, 'A', sizeof(buff));
  buff.packetnum = 0;
  
  INetAddr address(argv[1], port);
  Socket sock(UDP_SOCKET);
  
  printf("Sending all the data\n");
  
  while (start + duration > std::time(nullptr))
  {
    sock.Send(&buff, sizeof(buff), address);
    buff.packetnum++;
  }
  
  sock.Send(&buff, 0, address);
  sock.Send(&buff, 0, address);
  sock.Send(&buff, 0, address);
  sock.Send(&buff, 0, address);
  sock.Send(&buff, 0, address);
  
  printf("%i packets sent\n", buff.packetnum);
  printf("%f KB sent\n", static_cast<float>(buff.packetnum * sizeof(Packet)) / 1024.f);
  printf("%f MB sent\n", static_cast<float>(buff.packetnum * sizeof(Packet)) / (1024.f * 1024.f));
  printf("%f MB sent\n", static_cast<float>(buff.packetnum * sizeof(Packet)) / (1024.f * 1024.f * 1024.f));
  
  sock.Close();
  
  Socket::Cleanup();
}


