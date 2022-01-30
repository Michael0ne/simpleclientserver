#ifdef CLIENT
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#pragma comment(lib, "Ws2_32.lib")

#include <iostream>
#include <string>

std::string serverPort = "27015";
std::string serverIp = "";
bool verboseOutput = false;

void parseCommandLineArguments(const uint32_t argc, char** argv)
{
    for (uint32_t i = 0; i < argc; ++i)
    {
        if (strcmp("port", argv[i] + 1) == NULL)
            serverPort = argv[++i];

        if (strcmp("verbose", argv[i] + 1) == NULL)
            verboseOutput = true;

        if (strcmp("ip", argv[i] + 1) == NULL)
            serverIp = argv[++i];
    }
}

int main(int argc, char** argv)
{
    if (argc == 1)
    {
        std::cout << "Command line arguments:" << std::endl;
        std::cout << "-ip <server ip/hostname> - to specify server IP address OR host name." << std::endl;
        std::cout << "-port <port> - to specify server port." << std::endl;
        std::cout << "-verbose - to enable verbose output." << std::endl;

        return 1;
    }

    if (argc > 1)
        parseCommandLineArguments(argc - 1, argv + 1);

    //  Client application.
    WSADATA wsaData;
    int32_t wsaInitResult;
    addrinfo* resultAddressInfo = nullptr, hintsAddressInfo;
    SOCKET clientSocket = INVALID_SOCKET;

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

    int32_t serverAddrInfo = getaddrinfo(serverIp.c_str()[0] == NULL ? NULL : serverIp.c_str(), serverPort.c_str(), &hintsAddressInfo, &resultAddressInfo);
    if (serverAddrInfo)
    {
        std::cout << "Cannot get remote server address! " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    clientSocket = socket(resultAddressInfo->ai_family, resultAddressInfo->ai_socktype, resultAddressInfo->ai_protocol);
    if (clientSocket == INVALID_SOCKET)
    {
        std::cout << "Cannot create a new client socket! " << WSAGetLastError() << std::endl;
        freeaddrinfo(resultAddressInfo);
        WSACleanup();
        return 1;
    }

    int32_t connectionResult = connect(clientSocket, resultAddressInfo->ai_addr, (int)resultAddressInfo->ai_addrlen);
    if (connectionResult == SOCKET_ERROR)
    {
        std::cout << "Cannot connect to a specified socket! " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }

    freeaddrinfo(resultAddressInfo);

    if (clientSocket == INVALID_SOCKET)
    {
        std::cout << "Cannot connect to a specified socket! " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    std::cout << "Server connection estabilished!" << std::endl;

    //  Send and receive data.
    const char helloPacket[] = "hello";
    const int32_t helloPacketSize = (int32_t)strlen(helloPacket) + 1;

    int32_t helloRequestResult = send(clientSocket, helloPacket, helloPacketSize, 0);
    if (helloRequestResult == SOCKET_ERROR)
    {
        std::cout << "Hello request was not sent! " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    if (verboseOutput)
        std::cout << "Hello request was sent!" << std::endl;

    int32_t recievedSize;
    char recieveBuffer[1024] = {};

    do
    {
        recievedSize = recv(clientSocket, recieveBuffer, 1024, NULL);

        if (recievedSize > 0)
            std::cout << "Server says: " << recieveBuffer << std::endl;
        else
            if (recievedSize == NULL)
                std::cout << "Connection closed!" << std::endl;
            else
                std::cout << "Failed to recieve server messages! " << WSAGetLastError() << std::endl;
    } while (recievedSize > 0);

    //  Shutdown connection.
    int32_t shutdownResult = shutdown(clientSocket, SD_SEND);
    if (shutdownResult == SOCKET_ERROR)
    {
        std::cout << "Failed to close client socket! " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    closesocket(clientSocket);
    WSACleanup();

    std::cout << "Shutdown success!" << std::endl;

    return 0;
}
#endif