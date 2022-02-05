#include "common.h"
#ifdef SERVER

std::string serverPort = "27015";
bool verboseOutput = false;
SOCKET clientSocket = INVALID_SOCKET;
const char helpMessage[] =
    "@help - print this message\n@click [button] - send mouse click\n@mouse [X] [Y] - send mouse movement";

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
        {
            verboseOutput = true;
            validArguments++;
        }
    }

    return validArguments;
}

void handleClientMessage(const char* const msg, char* resp)
{
    char* msgCopy = _strdup(msg);
    char* nextToken = nullptr;
    char* token = strtok_s(msgCopy, " \n", &nextToken);

    if (strcmp(token + 1, "mouse") == NULL && nextToken != nullptr)
    {
        char* digitsDelimiter = strchr(nextToken, ' ');
        *digitsDelimiter = NULL;
        int32_t posX = atoi(nextToken);
        int32_t posY = atoi(digitsDelimiter + 1);

        std::cout << "Client requested to move mouse to " << posX << ", " << posY << std::endl;
        SetCursorPos(posX, posY);
    }

    if (strcmp(token + 1, "click") == NULL && nextToken != nullptr)
    {
        int32_t mouseButton = atoi(nextToken);
        INPUT clientInput;
        ZeroMemory(&clientInput, sizeof(clientInput));

        clientInput.type = INPUT_MOUSE;
        clientInput.mi.mouseData = 0;
        clientInput.mi.dwExtraInfo = NULL;
        clientInput.mi.time = 0;
        switch (mouseButton)
        {
            case 0:
                clientInput.mi.dwFlags = (MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP);
                break;
            case 1:
                clientInput.mi.dwFlags = (MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP);
                break;
            case 2:
                clientInput.mi.dwFlags = (MOUSEEVENTF_MIDDLEDOWN | MOUSEEVENTF_MIDDLEUP);
                break;
            default:
                break;
        }

        UINT inputResult = SendInput(1, &clientInput, sizeof(clientInput));

        std::cout << "Client requested to click a mouse button " << mouseButton << std::endl;
    }

    if (strcmp(token + 1, "help") == NULL)
        strcpy_s(resp, strlen(helpMessage) + 1, helpMessage);
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

    char recieveBuffer[128] = {};
    char sendBuffer[128] = {};
    int32_t recieveResult, sendResult;

    do
    {
        recieveResult = recv(clientSocket, recieveBuffer, sizeof(recieveBuffer), NULL);
        if (recieveResult > 0)
        {
            //  Handle received data here!
            if (verboseOutput)
                std::cout << "Client has sent " << recieveResult << " bytes of data!" << std::endl;
            std::cout << "[client]: " << recieveBuffer << std::endl;

            if (recieveBuffer[0] == '@')
                handleClientMessage(recieveBuffer, sendBuffer);

            memset(recieveBuffer, NULL, sizeof(recieveBuffer));

            sendResult = send(clientSocket, sendBuffer, sizeof(sendBuffer), 0);

            if (sendResult == SOCKET_ERROR)
            {
                std::cout << "Failed to send data to client! " << WSAGetLastError() << std::endl;
                closesocket(clientSocket);
                WSACleanup();
                return 1;
            }

            std::cout << "[server]: " << sendBuffer << std::endl;
            memset(sendBuffer, NULL, sizeof(sendBuffer));
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