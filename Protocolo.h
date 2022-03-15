#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#include <sstream>
#include <string.h>

#define VERSIONLEN 1
#define GROUPLEN   30
#define TYPELEN    4
#define ALIASLEN   15
#define SIZELEN    4
#define DATALEN    1400
#define BUFFERLEN  VERSIONLEN + GROUPLEN + TYPELEN + ALIASLEN + SIZELEN + DATALEN

#define INC  "INC "     // Comando de iniciativa de comunicação
#define TRAN "TRAN"     // Comando de transporte de mensagem
#define EXIT "EXIT"     // Comando para sair do grupo e aplicacao
#define TOUT "TOUT"     // Comando de notificação de ban do usuario

// Pacote de campos do protocolo já fragmentados
typedef struct {
    // É adicionado um caratectere a mais por
    // causa do caractere '\0', necessário em
    // variavéis do tipo char.
	char version[VERSIONLEN + 1];
	char group  [GROUPLEN   + 1];
	char type   [TYPELEN    + 1];
	char alias  [ALIASLEN   + 1];
	char size   [SIZELEN    + 1];
	char data   [DATALEN    + 1];
} PACKAGE;

// Buffer é um pacote de campos concatenados para envio
typedef struct {
	char data[BUFFERLEN];
	size_t length;
} BUFFER;

// Cria um pacote com configuração padrao, preenchendo campos com espaços
PACKAGE default_package() {
	PACKAGE pk;

    // Completa os campos do pacote com espaço branco
	memset(pk.version, ' ', VERSIONLEN);
	memset(pk.group, ' ', GROUPLEN);
	memset(pk.type, ' ', TYPELEN);
	memset(pk.alias, ' ', ALIASLEN);
	memset(pk.size, ' ', SIZELEN);
	memset(pk.data, ' ', DATALEN);

    // Adiciona o '\0' na ultima posição de cada campo do pacote
	pk.version[VERSIONLEN] = '\0';
	pk.group  [GROUPLEN]   = '\0';
	pk.type   [TYPELEN]    = '\0';
	pk.alias  [ALIASLEN]   = '\0';
	pk.size   [SIZELEN]    = '\0';
	pk.data   [DATALEN]    = '\0';

	return pk;
}

// Insere em algum campo do pacote uma string e preenche a sobra com espaços
void insert_field(char *dest, char const *source, size_t max) {
	for (int i = 0; i < max; i++) {
        if (i < strlen(source)) {
            dest[i] = source[i];
        }
        else {
            dest[i] = ' ';
        }
	}
}

// Converte numero para string
std::string size_to_string(int a) {
	std::ostringstream oss;
	oss << a;
	return oss.str();
}

// Converte string para numero
int string_to_size(std::string a) {
	std::istringstream iss;
	int valor;

	iss.str(a);
	iss >> valor;

	return valor;
}

// Converte um pacote para um buffer concatenando todos os seus campos
BUFFER package_to_buffer(PACKAGE pk) {
	BUFFER bf;

	strcpy(bf.data, "");
	strcat(bf.data, pk.version);
	strcat(bf.data, pk.group);
	strcat(bf.data, pk.type);
	strcat(bf.data, pk.alias);
	strcat(bf.data, pk.size);
	strcat(bf.data, pk.data);

    bf.data[BUFFERLEN-1] = '\0'; // Coloca o '\0' no final do buffer

	bf.length = strlen(bf.data);

	return bf;
}

// Converte um buffer para um pacote fragmentando o buffer
PACKAGE buffer_to_package(BUFFER bf) {
	PACKAGE pk;
	int data_size;

	strncpy(pk.version, &bf.data[0], VERSIONLEN);
	strncpy(pk.group, &bf.data[VERSIONLEN], GROUPLEN);
	strncpy(pk.type, &bf.data[VERSIONLEN + GROUPLEN], TYPELEN);
	strncpy(pk.alias, &bf.data[VERSIONLEN + GROUPLEN + TYPELEN], ALIASLEN);
	strncpy(pk.size, &bf.data[VERSIONLEN + GROUPLEN + TYPELEN + ALIASLEN], SIZELEN);

    if(strcmp(pk.size, "    ")) {
        data_size = string_to_size(pk.size);
        strncpy(pk.data, &bf.data[VERSIONLEN + GROUPLEN + TYPELEN + ALIASLEN + SIZELEN], data_size);
        pk.data[data_size] = '\0';
    }

	pk.version[VERSIONLEN] = '\0';
	pk.group  [GROUPLEN]   = '\0';
	pk.type   [TYPELEN]    = '\0';
	pk.alias  [ALIASLEN]   = '\0';
	pk.size   [SIZELEN]    = '\0';
	pk.data   [DATALEN]    = '\0';

	return pk;
}

#endif
