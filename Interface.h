#ifndef INTERFACE_H
#define INTERFACE_H

#include <Windows.h>
#include <process.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#define CHAT_X 79
#define CHAT_Y 21

#define SCREEN_X 80
#define SCREEN_Y 25

#define USER_INPUT_LINE 23

// GLOBAIS
unsigned int start_index = 0;
unsigned int end_index   = 0;

std::vector<std::string> chat_buffer;

HANDLE g_output_semaphore;
HANDLE g_buffer_semaphore;


// Posiciona caracteres na tela, útil para impressão da interface;
void gotoxy(int x, int y) {
	COORD coord;
	coord.X = x;
	coord.Y = y;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// Função que obtem a posição corrente de onde esta o cursor, necessário
// para salvar onde o usuário estava digitando a mensagem no momento em que
// chega uma nova, assim não interferindo na interface;
void getCursorXY(int &x, int &y) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
		x = csbi.dwCursorPosition.X;
		y = csbi.dwCursorPosition.Y;
	}
}

// Cria dois semafáros para manipular threads da interface, sendo
// uma para imprimir as mensagens e outra para a entrada de mensagens
// presentes no "rodapé" da pagina;
void start_interface() {
	g_output_semaphore = CreateSemaphore(NULL, 1, 1, NULL);
	g_buffer_semaphore = CreateSemaphore(NULL, 1, 1, NULL);
}

// Armazena as strings "command" e "adj", sendo o comando solicitado
// e um complemento, respectivamente, em que este complemento é
// propriamente a mensagem quando o comando solicitado é "\enviar", mas
// não é utilizado para qualquer outro comando.
// Esta função também utiliza seção critica para a impressao, pela possibilidade
// de chegar uma mensagem no momento da impressão;
void get_input(std::string &command, std::string &adj) {
	if(WaitForSingleObject(g_output_semaphore, INFINITE) != WAIT_FAILED) {
		gotoxy(0, USER_INPUT_LINE);
	    std::cout <<  "-> ";
	} ReleaseSemaphore (g_output_semaphore, 1, 0);

	std::cin >> command;
	getline(std::cin, adj);

    //Apagando mensagem anterior
	if(WaitForSingleObject(g_output_semaphore, INFINITE) != WAIT_FAILED) {
		gotoxy(0, USER_INPUT_LINE);
		std::cout << std::string(DATALEN, ' ');
		gotoxy(0, USER_INPUT_LINE);
	} ReleaseSemaphore (g_output_semaphore, 1, 0);

    //Necessário apagar o espaço inicial da mensagem, mas somente quando houver mensagem
	if(adj.length() > 0) {
	    adj.erase(adj.begin());
	}
}

// Inclui 40 espaços para formatação das sub_strings de uma mensagem, sendo que é
// incluido antes de cada sub_string, quando a mensagem é enviada pela origem,
// ou depois quando a mensagem é recebida, e para efeito de entendimento cada sub_string
// faz referencia a uma linha que é impressa na tela, assim quando o remetente suas mensagens
// ficam identadas á direita e quando as mensagens são recebidas do destinatário ficam
// identadas á esquerda na tela;
std::string generate_new_message(std::string message, int compare){
    std::string new_message;
    int n_substrings = message.length() / (CHAT_X - 40);

    for(int i = 0; i <= n_substrings; i++) {
        if(compare == 0)
            new_message = new_message + "                                        " +  message.substr(i * (CHAT_X-40), (CHAT_X - 40));
        else
            new_message = new_message  +  message.substr(i * (CHAT_X-40), (CHAT_X - 40)) + "                                        ";
    }
    return new_message;
}

// Insere as mensagens devidamente formatadas no buffer, onde cada
// mensagem com mais de 80 caracteres é dividida em submensagens com
// 80 caracteres, além da da ultima subdivisão que é permitido ficar
// com um numero inferior de caracteres;
void insert_message(std::string message, std::string sender) {
	int n_substrings;
    std::string new_message;

    //Identa as mensagens na tela á esquerda ou direita, dependendo de quem envia;
    new_message = generate_new_message(message, sender.compare("source"));

    n_substrings = new_message.length() / CHAT_X;

    for(int i = 0; i <= n_substrings; i++) {
		if(WaitForSingleObject(g_buffer_semaphore, INFINITE) != WAIT_FAILED) {
        	chat_buffer.push_back(new_message.substr(i * (CHAT_X), CHAT_X));
		} ReleaseSemaphore (g_buffer_semaphore, 1, 0);
    }
    //Atualizando variáveis que limitam a impressao do buffer
	if(chat_buffer.size() > CHAT_Y ) {
		start_index += n_substrings + 1;
	}
	end_index += n_substrings + 1;
}

// Desenha borda superior e inferior do campo de mensagens da tela de impressao
void box_window(int width, int beginning_column, int top_line, int bottom_line){
    int j;
    for (j=0;j<width ;j++){//Imprimindo borda superior e inferior;
        if(j==0){
            gotoxy(beginning_column,bottom_line);       printf("%c", 192);
            gotoxy(beginning_column+width,bottom_line); printf("%c", 217);
            gotoxy(beginning_column,top_line);          printf("%c", 218);
            gotoxy(beginning_column+width,top_line);    printf("%c", 191);
        }else{
            gotoxy(beginning_column+j,top_line);    printf("%c", 196);
            gotoxy(beginning_column+j,bottom_line); printf("%c", 196);
        }
    }
}

// Imprime na tela o buffer dentro dos limites estabelecidos, através do
// variável "start_index" até "end_index", mas antes apagando a tela com
// espaços, em que todas essas operações são realizadas em uma seção critica;
void print_chat() {
	int old_x, old_y;
	getCursorXY(old_x, old_y);//Salvando estado atual do cursor
	if(WaitForSingleObject(g_output_semaphore, INFINITE) != WAIT_FAILED) {
	    gotoxy(1, 1);
	    std::cout << std::string(1680, ' ');
	    gotoxy(0, 0);
	    printf("\n");
	    //Imprimindo o buffer no limite estabelecido
	    for(unsigned int i = start_index; i < end_index; i++) {
	        std::cout << chat_buffer.at(i) << std::endl;
	    }
	    //Imprimindo borda superior e inferior da janela;
	    box_window(CHAT_X,0,0,CHAT_Y+1);
		gotoxy(old_x, old_y);//Retomando estado do cursor;
	} ReleaseSemaphore (g_output_semaphore, 1, 0);
}

// Altera as variáveis que  manipulam os limites de impressao do buffer
// "subindo" uma unica posição, assim decrementando um de cada;
void chat_up() {
    if(start_index > 0) {
        start_index--;
        end_index--;
    }
}

// Altera as variáveis que  manipulam os limites de impressao do buffer
// "descendo" uma unica posição, assim incrementando um de cada;
void chat_down() {
    if(end_index < chat_buffer.size()) {
        start_index++;
        end_index++;
    }
}

// Altera as variáveis que  manipulam os limites de impressao do buffer
// fazendo o limite  inferior receber a ultima posição do buffer, e o limite
// inferior receber a 21º antes da ultima posição do buffer;
void chat_end() {
    // Condição para impedir que seja manipulada as variáves do limite,
    // quando o buffer ainda nao ultrapassou o tamanho da tela;
    if(chat_buffer.size() > CHAT_Y) {
        end_index = chat_buffer.size();
        start_index = end_index - CHAT_Y;
    }
}

// Altera as variáveis que  manipulam os limites de impressao do buffer
// fazendo o limite superior receber a primeira posição do buffer, e o limite
// inferior receber a 21º posição do buffer;
void chat_top() {
	if(chat_buffer.size() > CHAT_Y) {
        start_index = 0;
        end_index = start_index + CHAT_Y;
    }
}

#endif
