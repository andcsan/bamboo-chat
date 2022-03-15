#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS         // Impede warnings e erros de compilação
#define _WINSOCK_DEPRECATED_NO_WARNINGS // Impede warnings e erros de compilação
#define _WIN32_WINNT  0x501             // Para compilar fora do Visual Studio

#include <Windows.h>
#include <process.h>
#include <ws2tcpip.h>
#include <WinSock2.h>
#include <cstdlib>

#include <iostream>
#include <sstream>
#include <ctime>
#include <list>

#include "../Protocolo.h"
#include "Servidor.h"

#pragma comment (lib, "Ws2_32.lib")

int __cdecl main()
{
	WSADATA wsaData;

	SOCKET listen_sock = INVALID_SOCKET;    // Socket para aguardar conexoes
    SOCKET client_sock = INVALID_SOCKET;    // Socket para enviar dados
    SOCKET server_sock = INVALID_SOCKET;    // Socket para receber dados

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	// Semaforo para operar a lista de LINKs
	g_link_semaphore = 	CreateSemaphore(NULL, 1, 1, NULL);

	// WSA
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cout << "WSAStartup" << std::endl;
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;			// Define endereçamento IPv4
	hints.ai_socktype = SOCK_STREAM;	// Define socket Stream
	hints.ai_protocol = IPPROTO_TCP;	// Define o protocolo TCP
	hints.ai_flags = AI_PASSIVE;

	// Getaddrinfo
	if (getaddrinfo(NULL, RECV_PORT, &hints, &result) != 0) {
		std::cout << "Getaddinfo" << std::endl;
		WSACleanup();
		return -1;
	}

	// Criando socket para listen
	listen_sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listen_sock == INVALID_SOCKET) {
		std::cout << "Socket" << std::endl;
		freeaddrinfo(result);
		WSACleanup();
		return -1;
	}

	// Bind
	if (bind(listen_sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
		std::cout << "Bind" << std::endl;
		freeaddrinfo(result);
		closesocket(listen_sock);
		WSACleanup();
		return -1;
	}

	// Definindo listen_sock como listen
	if (listen(listen_sock, SOMAXCONN) == SOCKET_ERROR) {
		std::cout << "Listen" << std::endl;
		closesocket(listen_sock);
		return -1;
	}

	freeaddrinfo(result);

	// Definindo horario de inicializacao do servidor
	time_t rawtime;
	time(&rawtime);
	std::cout << "Servidor iniciado em: " << ctime(&rawtime) << std::endl;
	std::cout << "Aguardando conexoes..." << std::endl;

	_beginthreadex(NULL, 0, &timeout, NULL, 0, NULL);

	// Loop principal para receber clientes e gerenciar
	while (true) {

		server_sock = accept(listen_sock, NULL, NULL);
		if (server_sock == INVALID_SOCKET) {
			std::cout << "Accept falhou" << std::endl;
			continue;
		}

		// Coleta o IP do cliente conectado
		SOCKADDR_IN addr;
		int addrlen = sizeof(addr);
		getpeername(server_sock, (struct sockaddr*)&addr, &addrlen);

	    // Getaddrinfo
		if (getaddrinfo(inet_ntoa(addr.sin_addr), SEND_PORT, &hints, &result) != 0) {
			std::cout << "Getaddrinfo" << std::endl;
			WSACleanup();
			return -1;
		}

		// Criando client_sock
		client_sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (client_sock == INVALID_SOCKET) {
			std::cout << "Socket falhou" << std::endl;
			WSACleanup();
			return -1;
		}

		if (connect(client_sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
			std::cout << "Connect falhou" << std::endl;
			continue;
		}

		if (g_link.size() >= CLIENT_LIMIT) {
			std::cout << "Servidor lotado. Tente novamente mais tarde" << std::endl;
			continue;
		}

		// Nova estrutura LINK para inserir na lista global
		LINK new_link;
		new_link.server_sock = server_sock;
		new_link.client_sock = client_sock;
		strcpy(new_link.user.ip, inet_ntoa(addr.sin_addr));

		new_link.thrd = (HANDLE)_beginthreadex(NULL, 0, &client_handler, &new_link, CREATE_SUSPENDED, NULL);

		if(WaitForSingleObject(g_link_semaphore, INFINITE) != WAIT_FAILED) {
			g_link.push_back(new_link);
		} ReleaseSemaphore(g_link_semaphore, 1, 0);

		std::cout << new_link.user.ip << " conectou-se!" << std::endl;

		ResumeThread(new_link.thrd);
	}

	system("pause");

	return 0;
}
