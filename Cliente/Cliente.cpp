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
#include <cstdio>

#include <iostream>
#include <string>

#include "../Protocolo.h"
#include "../Interface.h"
#include "Cliente.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


int __cdecl main(int argc, char *argv[]){
    WSADATA wsaData;

    SOCKET listen_sock = INVALID_SOCKET;    // Socket para aguardar conexoes
    SOCKET client_sock = INVALID_SOCKET;    // Socket para enviar dados
    SOCKET server_sock = INVALID_SOCKET;    // Socket para receber dados

    HANDLE recv_thread;
    start_interface();
    system("cls");

    if(argc < 3) {
        std::cout << "Informe o IP do servidor e o grupo" << std::endl;
        return -1;
    }
    // WSA - Inicializa WinSock
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cout << "WSAStartup" << std::endl;
		return 1;
	}

    if(socket_establish(&client_sock, "client_socket", argv[1]) != 0 )
        return -1;
    if(socket_establish(&listen_sock, "listen_socket", argv[1]) != 0 )
        return -1;

    int bytes;

    PACKAGE pacote =  default_package();
    BUFFER buffer;

    insert_field(pacote.version, VERSION, VERSIONLEN);
    insert_field(pacote.group, argv[2], GROUPLEN);
    insert_field(pacote.type, INC, TYPELEN);
    buffer = package_to_buffer(pacote);
    bytes = send(client_sock, buffer.data, BUFFERLEN, 0);

    //Por ser o cliente só é necessário realizar um unico accept no servidor;
    server_sock = accept(listen_sock, NULL, NULL);

    //Imprimindo primeira janela;
    box_window(CHAT_X,0,0,CHAT_Y+1);

    //Iniciando thread que ouvira as mensagem vindas do servidor;
    recv_thread = (HANDLE)_beginthreadex(NULL, 0, &recv_handler, &server_sock, CREATE_SUSPENDED, NULL);
    ResumeThread(recv_thread);

    std::string command;
    std::string user_input;
    while(1) {
		get_input(command, user_input);

        if(command == "\\enviar"){
            // Formatando pacote com o tamanho padrão estabelecido, preenchendo com espaços e
            // o campo de cada segmento que sobra
            insert_field(pacote.type,  TRAN, TYPELEN);     //Encapsulando tipo;
            insert_field(pacote.alias, argv[1], ALIASLEN);//Encapsulando IP
            insert_field(pacote.size,  size_to_string(user_input.length()).data(), SIZELEN);//Encapsulando tamanho da mensagem
            insert_field(pacote.data,  user_input.data(), DATALEN);//Encapsulando a mensagem

            //Convertendo struct do pacote para string que será impressa no buffer;
            buffer = package_to_buffer(pacote);

            //Enviando string referente ao pacote já encapsulado;
            bytes = send(client_sock, buffer.data, BUFFERLEN, 0);

            //Inserindo no buffer de mensagens;
            insert_message("Voce disse: " + user_input, "source");
            chat_end();
        } else if(command == "\\quit") {
            insert_field(pacote.type, EXIT, TYPELEN);
            buffer = package_to_buffer(pacote);
            bytes = send(client_sock, buffer.data, BUFFERLEN, 0);
            system("cls");
            break;
        } else if(command == "\\up") {
            // Chama função que altera as variáveis que estabelecem o limite do buffer
            // "subindo" uma unica posição, assim decrementando um de cada;
            chat_up();
        } else if(command == "\\down") {
            // Chama função que altera as variáveis que estabelecem o limite do buffer
            // "descendo" uma unica posição, assim incrementando um de cada;
            chat_down();
        } else if(command == "\\end") {
            // Chama função que altera as variáveis que estabelecem o limite do buffer
            // fazendo o limite  inferior receber a ultima posição do buffer, e o limite
            // inferior receber a 21º antes da ultima posição do buffer;
            chat_end();
        } else if(command == "\\top") {
            // Chama função que altera as variáveis que estabelecem o limite do buffer
            // fazendo o limite superior receber a primeira posição do buffer, e o limite
            // inferior receber a 21º posição do buffer;
            chat_top();
        } else if(command == "\\ajuda") {
            // Imprime a lista de comandos disponiveis;
            system("cls");
            gotoxy(0, 0);
            std::cout << "Lista de comandos: " << std::endl << std::endl;
            std::cout << "\\enviar + mensagem   => Envia a mensagem para o grupo" << std::endl;
            std::cout << "\\quit                => Fecha a aplicacao" << std::endl << std::endl;

            std::cout << "Lista de comandos de interface: " << std::endl << std::endl;
            std::cout << "\\up   => Rola a lista de mensagens para cima" << std::endl;
            std::cout << "\\down => Rola a lista de mensagens para baixo" << std::endl;
            std::cout << "\\end  => Rola a lista de mensagens até o fim" << std::endl << std::endl;
            std::cout << "Pressione qualquer tecla para continuar";
            std::cin.ignore();
        } else {
            insert_message("! " + command + " => Comando Invalido !", "source");
        }
		print_chat();
    }

    return 0;
}
