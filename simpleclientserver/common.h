#pragma once
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <WinUser.h>

#pragma comment(lib, "Ws2_32.lib")

#include <iostream>
#include <string>
#include <thread>