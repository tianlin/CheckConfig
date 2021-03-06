// CheckConfig.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <windows.h>
#include <msi.h>
#include <msiquery.h>
#include <regex>
#include <string>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(linker, "/EXPORT:CheckConfig=_CheckConfig@4")

const DWORD kMaxLength = 255;
const std::string kIPPattern = "^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?):([0-9]{1,4}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])$";
const std::string kTokenPattern = "[a-f0-9]{32}";

bool isValidListenAddr(std::string listenAddr) {
	WSADATA wsaData;
	int iResult = 0;

	SOCKET ListenSocket = INVALID_SOCKET;
	sockaddr_in service;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		return false;
	}
	//----------------------
	// Create a SOCKET for listening for incoming connection requests.
	ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET) {
		WSACleanup();
		return false;
	}

	size_t colonOffset = listenAddr.find_first_of(":", 0);
	std::string ip = listenAddr.substr(0, colonOffset);
	std::string portStr = listenAddr.substr(colonOffset + 1);

	short port = std::stoi(portStr);

	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port for the socket that is being bound.
	service.sin_family = AF_INET;
	
	service.sin_addr.s_addr = inet_addr(ip.c_str());
	service.sin_port = htons(port);

	iResult = bind(ListenSocket, (SOCKADDR *)& service, sizeof(service));
	if (iResult == SOCKET_ERROR) {
		iResult = closesocket(ListenSocket);
		WSACleanup();
		return false;
	}
	//----------------------
	// Listen for incoming connection requests 
	// on the created socket
	iResult = listen(ListenSocket, SOMAXCONN);

	closesocket(ListenSocket);
	WSACleanup();

	return iResult != SOCKET_ERROR;
}

extern "C" UINT __stdcall CheckConfig(MSIHANDLE hInstall) {
	char listenAddr[kMaxLength];
	char hbCollectorAddr[kMaxLength];
	char collectorAddr[kMaxLength];

	std::regex ipPattern(kIPPattern);

	DWORD listenAddrLength = kMaxLength;
	MsiGetProperty(hInstall, "LISTENADDR", listenAddr, &listenAddrLength);
	std::string listenAddrStr(listenAddr, listenAddrLength);
	if (!std::regex_match(listenAddrStr, ipPattern)) {
		MsiSetProperty(hInstall, "CONFIGACCEPTED", "0");
		MsiSetProperty(hInstall, "CONFIGERROR", "Listen Addr is invalid");
		return ERROR_SUCCESS;
	}

	// check listen addr can listen
	if (!isValidListenAddr(listenAddr)) {
		MsiSetProperty(hInstall, "CONFIGACCEPTED", "0");
		MsiSetProperty(hInstall, "CONFIGERROR", "Can not listen at listen addr");
		return ERROR_SUCCESS;
	}


	DWORD hbCollectorAddrLength = kMaxLength;
	MsiGetProperty(hInstall, "HBCOLLECTORADDR", hbCollectorAddr, &hbCollectorAddrLength);
	std::string hbCollectorAddrStr(hbCollectorAddr, hbCollectorAddrLength);
	if (!std::regex_match(hbCollectorAddrStr, ipPattern)) {
		MsiSetProperty(hInstall, "CONFIGACCEPTED", "0");
		MsiSetProperty(hInstall, "CONFIGERROR", "HB Collector Addr is invalid");
		return ERROR_SUCCESS;
	}

	DWORD collectorAddrLength = kMaxLength;
	MsiGetProperty(hInstall, "COLLECTORADDR", collectorAddr, &collectorAddrLength);
	std::string collectorAddrStr(collectorAddr, collectorAddrLength);
	if (!std::regex_match(collectorAddrStr, ipPattern)) {
		MsiSetProperty(hInstall, "CONFIGACCEPTED", "0");
		MsiSetProperty(hInstall, "CONFIGERROR", "Collector Addr is invalid");
		return ERROR_SUCCESS;
	}

	std::regex tokenPattern(kTokenPattern);
	DWORD tokenLength = kMaxLength;
	char token[kMaxLength];
	MsiGetProperty(hInstall, "TOKEN", token, &tokenLength);
	std::string tokenStr(token, tokenLength);
	if (!std::regex_match(tokenStr, tokenPattern)) {
		MsiSetProperty(hInstall, "CONFIGACCEPTED", "0");
		MsiSetProperty(hInstall, "CONFIGERROR", "Token is invalid");
		return ERROR_SUCCESS;
	}


	MsiSetProperty(hInstall, "CONFIGACCEPTED", "1");
	return ERROR_SUCCESS;
} // CheckConfig


