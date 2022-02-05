#include "common.h"
#ifdef CLIENT

std::string serverPort = "27015";
std::string serverIp = "127.0.0.1";
bool verboseOutput = false;
SOCKET clientSocket = INVALID_SOCKET;

int32_t parseCommandLineArguments(const uint32_t argc, char** argv)
{
    int32_t validArguments = 0;

    for (uint32_t i = 0; i < argc; ++i)
    {
        if (strcmp("port", argv[i] + 1) == NULL)
        {
            serverPort = argv[++i];
            validArguments++;
        }

        if (strcmp("verbose", argv[i] + 1) == NULL)
            verboseOutput = true;

        if (strcmp("ip", argv[i] + 1) == NULL)
        {
            serverIp = argv[++i];
            validArguments++;
        }
    }

    return validArguments;
}

void printUsage()
{
    std::cout << "Command line arguments:" << std::endl;
    std::cout << "-ip <server ip/hostname> - to specify server IP address OR host name." << std::endl;
    std::cout << "-port <port> - to specify server port." << std::endl;
    std::cout << "-verbose - to enable verbose output." << std::endl;
}

int main(int argc, char** argv)
{
    if (argc > 1)
    {
        if (!parseCommandLineArguments(argc - 1, argv + 1))
        {
            printUsage();
            return 1;
        }
    }
    else
    {
        printUsage();
        std::cout << "Starting with default parameters, since no arguments were specified." << std::endl;
    }

    //  Client application.
    WSADATA wsaData;
    int32_t wsaInitResult;
    addrinfo* resultAddressInfo = nullptr, hintsAddressInfo;

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

    int32_t sentSize, recievedSize;
    char sendBuffer[128] = {};
    char recieveBuffer[128] = {};
    char* sendBufferPtr = sendBuffer;
    int32_t lastChar = NULL;
    bool shouldShutdown = false;

    do
    {
        if (shouldShutdown)
            break;

        do
        {
            lastChar = getchar();
            *sendBufferPtr++ = lastChar;
        } while (lastChar != 10);

        std::cout << "[client]: " << sendBuffer << std::endl;

        sentSize = send(clientSocket, sendBuffer, sizeof(sendBuffer), NULL);

        memset(sendBuffer, NULL, sizeof(sendBuffer));
        sendBufferPtr = sendBuffer;

        if (sentSize > 0)
        {
            if (verboseOutput)
                std::cout << "Sent " << sentSize << " bytes of data to the server!" << std::endl;

            recievedSize = recv(clientSocket, recieveBuffer, sizeof(recieveBuffer), NULL);
            if (recievedSize > 0)
            {
                std::cout << "[server]: " << recieveBuffer << std::endl;
            }
            else
            {
                if (recievedSize == NULL)
                    std::cout << "Connection closed!" << std::endl;
                else
                    std::cout << "Failed to recieve server messages! " << WSAGetLastError() << std::endl;

                memset(recieveBuffer, NULL, sizeof(recieveBuffer));
                shouldShutdown = true;
            }

            memset(recieveBuffer, NULL, sizeof(recieveBuffer));
        }
        else
        {
            if (sentSize == NULL)
                std::cout << "Connection closed!" << std::endl;
            else
                std::cout << "Failed to send message to the server! " << WSAGetLastError() << std::endl;
        }
    } while (sentSize > 0);

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