#ifdef SERVER
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#pragma comment(lib, "Ws2_32.lib")

#include <iostream>
#include <string>

std::string serverPort = "27015";
bool verboseOutput = false;

void parseCommandLineArguments(const uint32_t argc, char** argv)
{
    for (uint32_t i = 0; i < argc; ++i)
    {
        if (strcmp("port", argv[i] + 1) == NULL)
            serverPort = argv[++i];

        if (strcmp("verbose", argv[i] + 1) == NULL)
            verboseOutput = true;
    }
}

int main(int argc, char** argv)
{
    if (argc > 1)
        parseCommandLineArguments(argc - 1, argv + 1);

    //  Server application.
    WSADATA wsaData;
    int32_t wsaInitResult;
    addrinfo* resultAddressInfo = nullptr, hintsAddressInfo;
    SOCKET listenSocket = INVALID_SOCKET;

    wsaInitResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaInitResult != NULL)
    {
        std::cout << "WSAStartup returned " << wsaInitResult << std::endl;
        return 1;
    }

    if (verboseOutput)
        std::cout << "WSAStartup success!" << std::endl;

    ZeroMemory(&hintsAddressInfo, sizeof(hintsAddressInfo));
    hintsAddressInfo.ai_family = AF_INET;
    hintsAddressInfo.ai_socktype = SOCK_STREAM;
    hintsAddressInfo.ai_protocol = IPPROTO_TCP;
    hintsAddressInfo.ai_flags = AI_PASSIVE;

    int32_t addrInfoResult = getaddrinfo(NULL, serverPort.c_str(), &hintsAddressInfo, &resultAddressInfo);
    if (addrInfoResult != NULL)
    {
        std::cout << "getaddrinfo returned " << addrInfoResult << std::endl;
        WSACleanup();
        return 1;
    }

    if (verboseOutput)
        std::cout << "GetAddrInfo success!" << std::endl;

    listenSocket = socket(resultAddressInfo->ai_family, resultAddressInfo->ai_socktype, resultAddressInfo->ai_protocol);
    if (listenSocket == INVALID_SOCKET)
    {
        std::cout << "An invalid socket was retrieved! " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    if (verboseOutput)
        std::cout << "Listen socket retrieved!" << std::endl;

    int32_t bindSocketResult = bind(listenSocket, resultAddressInfo->ai_addr, (int)resultAddressInfo->ai_addrlen);
    if (bindSocketResult == SOCKET_ERROR)
    {
        std::cout << "Cannot bind the socket! " << WSAGetLastError() << std::endl;
        freeaddrinfo(resultAddressInfo);
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    if (verboseOutput)
        std::cout << "List socket bind successfully!" << std::endl;

    freeaddrinfo(resultAddressInfo);

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cout << "Cannot listen for incoming connections on a socket! " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Accepting connections on port " << serverPort.c_str() << std::endl;

    //  Wait for and accept client connections.
    SOCKET clientSocket = INVALID_SOCKET;

    clientSocket = accept(listenSocket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET)
    {
        std::cout << "Cannot accept incoming connection! " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    if (verboseOutput)
        std::cout << "Incoming connection!" << std::endl;

    //  Handle incoming connection.
    char recieveBuffer[1024] = {};
    int32_t recieveResult;

    do
    {
        recieveResult = recv(clientSocket, recieveBuffer, 1024, NULL);
        if (recieveResult > 0)
        {
            //  Handle received data here!
            std::cout << "Client has sent " << recieveResult << " bytes of data!" << std::endl;
            std::cout << "Message: " << recieveBuffer << std::endl;
            memset(recieveBuffer, NULL, 1024);
        }
        else
        {
            if (recieveResult == NULL)
            {
                std::cout << "Connection closing!" << std::endl;
            }
            else
            {
                //  Client may have forcefully closed a connection. Shutdown gracefully.
                std::cout << "Client connection shutdown forcefully! " << WSAGetLastError() << std::endl;
                closesocket(clientSocket);
                WSACleanup();
                std::cout << "Server stopped!" << std::endl;
                return 1;
            }
        }
    } while (recieveResult > 0);

    if (verboseOutput)
        std::cout << "Closing socket..." << std::endl;

    //  Cleanup.
    int32_t shutdownResult = shutdown(clientSocket, SD_SEND);
    if (shutdownResult == SOCKET_ERROR)
    {
        std::cout << "Cannot shutdown! " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    closesocket(clientSocket);
    WSACleanup();

    if (verboseOutput)
        std::cout << "Server clean success!" << std::endl;

    return 0;
}
#endif