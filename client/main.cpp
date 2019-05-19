#pragma comment(lib, "ws2_32.lib")   // for MS VS
#include <winsock2.h>
#include <iostream>
#include <thread>
#include <ws2tcpip.h>

SOCKET connectSocket = INVALID_SOCKET;
const int kBufferSize = 256;
const char * kPort = "8080";

void ClientHandler() {
  char msg[kBufferSize] = "";
  while(recv(connectSocket, msg, sizeof(msg), 0) > 0) {
    std::cout << msg << std::endl;
  }
  closesocket(connectSocket);
}

int main(int argc, char* argv[]) {
  // Validate the parameters
  if (argc < 2) {
    std::cerr << "Error: invalid argument! usage: " << argv[0] << " server-name(ip)" << std::endl;
    return 1;
  }

  //WSAStartup
  WSAData wsaData;
  WORD dllVersion = MAKEWORD(2, 2);
  int iResult = WSAStartup(dllVersion, &wsaData);
  if(iResult != 0) {
    std::cerr << "Error: library failed to load! Error code: " << iResult << std::endl;
    return 1;
  }

  addrinfo *result = nullptr;
  addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

// Resolve the server address and port
  iResult = getaddrinfo(argv[1], kPort, &hints, &result);
  if ( iResult != 0 ) {
    std::cerr << "Error: getaddrinfo failed! Error code: " << iResult << std::endl;
    WSACleanup();
    return 1;
  }

  iResult = SOCKET_ERROR;
// Attempt to connect to an address until one succeeds
  for(addrinfo *ptr = result; ptr != nullptr && iResult == SOCKET_ERROR ;ptr=ptr->ai_next) {
    // Create a SOCKET for connecting to server
    connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (connectSocket == INVALID_SOCKET) {
      std::cerr << "Error: socket failed! Error code: " << WSAGetLastError() << std::endl;
      WSACleanup();
      return 1;
    }
    // Connect to server.
    iResult = connect(connectSocket, ptr->ai_addr, (int) ptr->ai_addrlen);
    if (iResult != 0) {
      std::cerr << "Error: failed connect to server! Error code: " << WSAGetLastError() << std::endl;
      return 1;
    }
  }
  std::cout << "Connected!\n";
  freeaddrinfo(result);

  std::thread(ClientHandler).detach();

  char msg[kBufferSize];
  int sendResult = 0;
  do {
    std::cin.getline(msg, sizeof(msg));
    sendResult = send(connectSocket, msg, kBufferSize, 0);
  } while(sendResult > 0);

  WSACleanup();
  std::cout << "Disconnected!\n";
  return 0;
}
