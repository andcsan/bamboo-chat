#ifndef SERVIDOR_H
#define SERVIDOR_H

#define CLIENT_LIMIT 10		// Limite de clientes simultaneos
#define VERSION      "1"	// Versao do protocolo

#define RECV_PORT "15000"	// Envia dados na porta 15000
#define SEND_PORT "15010"	// Recebe dados da porta 15010

#define TOUT_TIME 600		// Timeout para o usuario ser banido

#define OFFLINE 1
#define ONLINE  0

// Estruta Client para agrupar informações de um usuário
typedef struct {
	char group[GROUPLEN + 1];
	char ip   [ALIASLEN + 1];
	time_t last_act;			// Ultima atividade do usuário
} CLIENT;

// Estrutura LINK agrupa informações de um cliente.
// Cada LINK conecta a thread que gerencia o cliente, o socket de envio e o socket
// para receber dados, além de uma estrutura CLIENT que armazena o ip e o grupo do
// cliente
typedef struct {
	HANDLE thrd;
	SOCKET server_sock;	// Socket para receber dados
	SOCKET client_sock;	// Socket para enviar dados
	CLIENT user;
} LINK;

// VARIAVEIS GLOBAIS
std::list<LINK> g_link;		// Lista de LINKs
HANDLE g_link_semaphore;	// Semaforo para operar a lista

// Repassa e mensagem enviada por um usuário para os outros usuários com o grupo
// em comum
void send_group(std::list<LINK>::iterator lk, PACKAGE pk, int from_server = 0) {
	std::list<LINK>::iterator it;	// Iterador para percorrer a lista de LINKs
	BUFFER buffer;
	int bytes;

	// Caso a mensagem a ser retransmitida seja de um usuário, e não do servidor
	if(!from_server) {
		insert_field(pk.alias, lk->user.ip, ALIASLEN);
	}

	buffer = package_to_buffer(pk);

	// Percore a lista de LINKs em busca de LINKs que sejam do mesmo grupo e que
	// não seja o proprio LINK que enviou
	for(it = g_link.begin(); it != g_link.end(); it++) {
		if (memcmp(&lk, &it, sizeof(lk))) {
			if(!strcmp(it->user.group, lk->user.group)) {
				bytes = send(it->client_sock, buffer.data, BUFFERLEN, 0);
			}
		}
	}
}

// Servidor envia o status de um usuario (se entrou ou saiu)
void send_status(std::list<LINK>::iterator lk, int status) {
	PACKAGE pacote = default_package();
	std::string message;
	int bytes;

	message = lk->user.ip;

	if(status == OFFLINE)
		message += " ficou offline.";
	else if(status == ONLINE)
		message += " entrou no grupo.";

	// Insere os campos do novo pacote
	insert_field(pacote.version, VERSION, VERSIONLEN);
	insert_field(pacote.group, lk->user.group, GROUPLEN);
	insert_field(pacote.type, TRAN, TYPELEN);
	insert_field(pacote.alias, "Servidor", ALIASLEN);
	insert_field(pacote.size, size_to_string(message.length()).data(), SIZELEN);
	insert_field(pacote.data, message.data(), DATALEN);

	send_group(lk, pacote, 1);
}

// Retorna um iterador para encontrar um determinado LINK na lista de LINKs.
// Essa função é necessária para alterar LINKs diretamente na lista
// (Iteradores são ponteiros que podem alterar os conteúdos por referencia)
std::list<LINK>::iterator find_link(LINK lk) {
	std::list<LINK>::iterator it;
	for (it = g_link.begin(); it != g_link.end(); it++) {
		if (lk.server_sock == it->server_sock && lk.thrd == it->thrd) {
			return it;
		}
	}
	return g_link.end();
}

// Thread timeout para banir usuários inativos
unsigned __stdcall timeout(void *) {
	std::list<LINK>::iterator it;	// Iterator para testar o tempo de inatividade de cada usuário
	PACKAGE pk = default_package();
	BUFFER bf;
	int bytes;

	insert_field(pk.type, TOUT, TYPELEN);	// Insere no pacote o comando de timeout

	bf = package_to_buffer(pk);

	// Loop principal para avaliação de inatividade
	while(1) {

		// Percorre a lista de links
		for(it = g_link.begin(); it != g_link.end(); it++) {

			// Checa se a ultima atividade do usuário excedeu o tempo limite
			if(time(NULL) - it->user.last_act >= TOUT_TIME) {
				// Caso tenha excedido, envia o comando de TOUT para o usuário
				bytes = send(it->client_sock, bf.data, BUFFERLEN, 0);
				send_status(it, OFFLINE);

				closesocket(it->client_sock);
				closesocket(it->server_sock);

				// A tarefa de remover o LINK da lista fica por parte da thread
				// client_handler, que recebera erros nos bytes pois os sockets
				// foram fechados
				break;
			}
		}
		Sleep(100);
	}
}

// Função de Thread para gerenciar cada cliente conectado individualmente
// Todo o gerenciamento é feito através dos LINKs respectivos a cada cliente
unsigned __stdcall client_handler(void *lk) {
	std::list<LINK>::iterator this_link;	// Iterador para o LINK desta thread

	PACKAGE pacote = default_package();
	BUFFER buffer;

	int bytes;

	this_link = find_link(*(LINK *)lk);		// Encontra o LINK desta thread na lista

	// Loop principal para receber dados do cliente
	while(1) {

		bytes = recv(this_link->server_sock, buffer.data, BUFFERLEN, 0);

		if(bytes == SOCKET_ERROR) {
			std::cout << "Conexao com " << this_link->user.ip << " perdida" << std::endl;
			send_status(this_link, OFFLINE);

			closesocket(this_link->client_sock);
			closesocket(this_link->server_sock);

			if(WaitForSingleObject(g_link_semaphore, INFINITE) != WAIT_FAILED) {
				g_link.erase(this_link);
			} ReleaseSemaphore (g_link_semaphore, 1, 0);

			// Quebra o loop para fechar a thread
			break;
		}

		pacote = buffer_to_package(buffer);

		this_link->user.last_act = time(NULL);

		if(!strcmp(pacote.type, INC)) {
			strcpy(this_link->user.group, pacote.group);
			std::cout << this_link->user.ip    << " entrou no grupo "
                      << this_link->user.group << std::endl;
			send_status(this_link, ONLINE);
		}

		else if(!strcmp(pacote.type, TRAN)) {
			std::cout << this_link->user.ip << " enviou mensagem." << std::endl;
			send_group(this_link, pacote);
		}

		else if(!strcmp(pacote.type, EXIT)) {
			std::cout << this_link->user.ip << " ficou offline" << std::endl;
			send_status(this_link, OFFLINE);

			closesocket(this_link->client_sock);
			closesocket(this_link->server_sock);

			if(WaitForSingleObject(g_link_semaphore, INFINITE) != WAIT_FAILED) {
				g_link.erase(this_link);
			} ReleaseSemaphore (g_link_semaphore, 1, 0);

			// Quebra o loop para fechar a thread
			break;
		}
	}
	_endthread();
	return 0;
}

#endif // !SERVIDOR_H
