#pragma comment(lib, "ws2_32.lib")  // for MS VS
#include <winsock2.h>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <ws2tcpip.h>

// NOTE: as it is only experimental implementation, code is not separated into classes, files etc.
std::mutex mu;
const char * kPort = "8080";
const int kBufferSize = 256;
const int kMaxConnections = 30;
std::vector<SOCKET> Connections(kMaxConnections, INVALID_SOCKET);

int getEmptyConnectionId() {
  int i;
  for (i = 0; Connections[i] != INVALID_SOCKET; ++i);
  return i;
}

void clientHandler(int client_id) {
  char msg[kBufferSize] = "";
  while (recv(Connections[client_id], msg, sizeof(msg), 0) > 0) {
    std::lock_guard lock(mu);
    for(int i = 0; i < kMaxConnections; i++) {
      if(i != client_id && Connections[i] != INVALID_SOCKET) {
        int iSendResult = send(Connections[i], msg, sizeof(msg), 0);
        if (iSendResult == SOCKET_ERROR) {
          std::cerr << "Error: send to " << Connections[i] << " failed! Error code: " << WSAGetLastError() << std::endl;
        }
      }
    }
  }
  std::lock_guard lock(mu);
  int iShutdownResult = shutdown(Connections[client_id], SD_SEND);
  if (iShutdownResult == SOCKET_ERROR) {
    std::cerr << "Error: shutdown failed! Error code: " << WSAGetLastError() << std::endl;
    closesocket(Connections[client_id]);
  }
  std::cout << "Client " << Connections[client_id] << " disconnected!" << std::endl;
  Connections[client_id] = INVALID_SOCKET;
}

int main(int argc, char* argv[]) {
  //WSAStartup
  WSAData wsaData;
  WORD dllVersion = MAKEWORD(2, 2);
  int iResult = WSAStartup(dllVersion, &wsaData);
  if(iResult != 0) {
    std::cerr << "Error: library failed to load! Error code: " << iResult << std::endl;
    return 1;
  }

  addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;  // used to specify the IPv4 address family
  hints.ai_socktype = SOCK_STREAM;  // used to specify a stream socket
  hints.ai_protocol = IPPROTO_TCP;  // used to specify the TCP protocol
  hints.ai_flags = AI_PASSIVE;  // caller intends to use the returned socket address in a call to the bind function

  // Resolve the local address and port to be used by the server
  addrinfo *result = nullptr;
  iResult = getaddrinfo(nullptr, kPort, &hints, &result);
  if (iResult != 0) {
    std::cerr << "Error: getaddrinfo failed! Error code: " << iResult << std::endl;
    WSACleanup();
    return 1;
  }

  SOCKET listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (listenSocket == INVALID_SOCKET) {
    std::cerr << "Error: getting listen socket failed! Error code: " << WSAGetLastError() << std::endl;
    freeaddrinfo(result);
    WSACleanup();
    return 1;
  }

  // Setup the TCP listening socket
  iResult = bind( listenSocket, result->ai_addr, (int)result->ai_addrlen);
  freeaddrinfo(result);
  if (iResult == SOCKET_ERROR) {
    std::cerr << "Error: bind failed! Error code: " << WSAGetLastError() << std::endl;
    closesocket(listenSocket);
    WSACleanup();
    return 1;
  }

  iResult = listen(listenSocket, kMaxConnections);
  if ( iResult == SOCKET_ERROR ) {
    std::cerr << "Error: listen failed! Error code: " << WSAGetLastError() << std::endl;
    closesocket(listenSocket);
    WSACleanup();
    return 1;
  }

  int iSendResult = SOCKET_ERROR;
  SOCKET newConnection;
  do {
    newConnection = accept(listenSocket, nullptr, nullptr);
    if(newConnection == INVALID_SOCKET) {
      std::cerr << "Error: accept failed! Error code: " << WSAGetLastError() << std::endl;
    } else {
      int id = getEmptyConnectionId();
      if (id < kMaxConnections) {
        std::cout << "Client " << newConnection << " connected!" << std::endl;
        char msg[kBufferSize] = "You are free to send message!";
        iSendResult = send(newConnection, msg, sizeof(msg), 0);
        if (iSendResult == SOCKET_ERROR) {
          std::cerr << "Error: send failed! Error code: " << WSAGetLastError() << std::endl;
        }
        std::lock_guard lock(mu);
        Connections[id] = newConnection;
        std::thread(clientHandler, id).detach();
      } else {
        char msg[kBufferSize] = "Sorry, you are not allowed to send message, server clients limit is reached!";
        send(newConnection, msg, sizeof(msg), 0);
        std::cerr << "No free connections left!" << std::endl;
        closesocket(newConnection);
      }
    }
  } while (newConnection != INVALID_SOCKET);

  WSACleanup();

  return 0;
}
