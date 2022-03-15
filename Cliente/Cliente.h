#ifndef CLIENTE_H
#define CLIENTE_H

// DEFINICOES
#define VERSION  "1"

#define SEND_PORT "15000"
#define RECV_PORT "15010"

#define INC  "INC "
#define TRAN "TRAN"
#define EXIT "EXIT"
#define TOUT "TOUT"

#define LOCALHOST "127.0.0.1"

// Quando ocorrer algum erro no estabelecimento de conexão
// imprime uma mensagem referente ao erro, e finaliza as variáveis
// de forma segura;
void print_error_message(char message[], SOCKET *selected_sock, struct addrinfo *hints , struct addrinfo *result, int *iResult_error){
    std::cout << message << std::endl;
    closesocket(*selected_sock);
    freeaddrinfo(hints);
    freeaddrinfo(result);
    WSACleanup();
    *iResult_error = -1;
}

// Estabelece conexão no socket selecionado, e realiza connect caso seja
// client_sock e bind, para aceitar a conexão do servidor, caso o socket
// selecionado seja o listen_socket
int socket_establish(SOCKET *selected_sock, std::string type, char argv[]){
    int iResult_error = 0;
    struct addrinfo *result = NULL;
	struct addrinfo  hints;

    ZeroMemory(&hints, sizeof(hints));
	hints.ai_family   = AF_INET;		// Define endere�amento IPv4
	hints.ai_socktype = SOCK_STREAM;	// Define socket Stream
	hints.ai_protocol = IPPROTO_TCP;	// Define o protocolo TCP
	hints.ai_flags    = AI_PASSIVE;

    //getaddrinfo
    if(type.compare("client_socket") == 0){//client_socket
        iResult_error = getaddrinfo(argv, SEND_PORT, &hints, &result);
    }else if (type.compare("listen_socket") == 0){//listen_socket
        iResult_error = getaddrinfo(NULL, RECV_PORT, &hints, &result);
    }
    if (iResult_error != 0)//Verifica erro no getaddrinfo
        print_error_message((char *)"Getaddrinfo", &*selected_sock, &hints, &*result, &iResult_error);


    *selected_sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (*selected_sock == INVALID_SOCKET)
        print_error_message((char *)"Socket", &*selected_sock, &hints, &*result, &iResult_error);


	if(type.compare("client_socket") == 0 && iResult_error == 0){// Client
        if (connect(*selected_sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR){
            print_error_message((char *)"Connect falhou", &*selected_sock, &hints, &*result, &iResult_error);
        }
	}else if(type.compare("listen_socket") == 0 && iResult_error == 0){//Listen
        //Configura TCP linten
        if (bind(*selected_sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR)
            print_error_message((char *)"Bind", &*selected_sock, &hints, &*result, &iResult_error);

        if (listen(*selected_sock, SOMAXCONN) == SOCKET_ERROR)//Verifica erro no socket ao re
            print_error_message((char *)"Listen", &*selected_sock, &hints, &*result, &iResult_error);

	}
	return iResult_error;
}

// Função passada para a thread que ira escutar as mensagens recebidas
// e irá armazenar no buffer de mensagens através da função insert();
unsigned __stdcall recv_handler(void *sk) {
	SOCKET sockfd = *(SOCKET *)sk;

	PACKAGE pacote;
	BUFFER buffer;

    //Iniciando pacote;
	pacote = default_package();

    // Nesta variavel sera formatada a mensagem para posteiormente armazenar no final do buffer;
	std::ostringstream oss;
	// String auxiliar que recebe a mensagem para concatenar a ostringstream oss;
	std::string txt_msg;

	int bytes;

	while(1) {
		bytes = recv(sockfd, buffer.data, BUFFERLEN, 0);
		if(bytes == SOCKET_ERROR) {//Erro no recebimento
			insert_message("!Conexao com o servidor perdida (SERVIDOR CAIU)!", "dest");
            insert_message("!Suas mensagens nao serao mais enviadas!", "dest");
            closesocket(sockfd);
            WSACleanup();
			break;
		}
        //Converte a string armazenada no recv para uma struct;
		pacote = buffer_to_package(buffer);

		if(!strcmp(pacote.type, TOUT)) {//Verifica timeout
            insert_message("!Conexao com o servidor perdida (BANIDO)!", "dest");
            insert_message("!Suas mensagens nao serao mais enviadas!", "dest");
            closesocket(sockfd);
            WSACleanup();
            break;
		} else if(!strcmp(pacote.type, TRAN)) {//Armezena no buffer a mensagem recebida;
	        txt_msg = pacote.data;
			oss << pacote.alias << " disse: " << txt_msg.substr(0, string_to_size(pacote.size));

			insert_message(oss.str(), "dest");
			oss.str("");
			oss.clear();
            //Notifica com um Beep o recebimento da mensagem;
			Beep(500, 150);
		}
        //Retorna para o fim do buffer quando recebe a mensagem, caso esteja vagando por mensagens antigas
		chat_end();
		print_chat();
	}
    chat_end();
    print_chat();

	_endthread();
}

#endif
