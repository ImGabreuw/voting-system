/*
INTEGRANTES:
- Gabriel Ken Kazama Geronazzo - 10418247
- Lucas Pires de Camargo Sarai - 10418013
*/

// Define _GNU_SOURCE para usleep e outras funções POSIX
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9002
#define MAX_MESSAGE_SIZE 1024

int connected = 1;
pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;

int connect_to_server()
{
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (client_socket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    int connection_status = connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    if (connection_status < 0)
    {
        perror("Connection to server failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    return client_socket;
}

void *handle_user_messages(void *arg)
{
    int client_socket = *(int *)arg;
    char server_response[MAX_MESSAGE_SIZE];

    while (connected)
    {
        memset(server_response, 0, sizeof(server_response));

        ssize_t bytes_received = recv(client_socket, server_response, sizeof(server_response) - 1, 0);

        if (bytes_received <= 0)
        {
            if (connected)
            {
                pthread_mutex_lock(&output_mutex);
                printf("\n[SISTEMA] Conexão com servidor perdida.\n");
                pthread_mutex_unlock(&output_mutex);
                connected = 0;
            }
            break;
        }

        pthread_mutex_lock(&output_mutex);
        printf("%s", server_response);
        fflush(stdout);
        pthread_mutex_unlock(&output_mutex);
    }

    pthread_exit(NULL);
}

void send_message(int client_socket, const char *message)
{
    if (send(client_socket, message, strlen(message), 0) < 0)
    {
        perror("Failed to send message");
        connected = 0;
    }
}

void print_menu() {
    printf("\n=== MENU DE VOTAÇÃO ===\n");
    printf("1. LIST         - Listar opções de votação\n");
    printf("2. VOTE <opção> - Registrar seu voto\n");
    printf("3. SCORE        - Ver placar parcial\n");
    printf("4. BYE          - Encerrar sessão\n");
    printf("5. ADMIN CLOSE  - Encerrar eleição (admin)\n");
    printf("========================\n");
}

int main(void)
{
    int client_socket = connect_to_server();

    printf("\n=== Cliente de Votação ===\n");
    printf("Conectado ao servidor de votação!\n\n");

    // Receber mensagem inicial
    char server_message[MAX_MESSAGE_SIZE];
    memset(server_message, 0, sizeof(server_message));
    recv(client_socket, server_message, sizeof(server_message) - 1, 0);
    printf("%s", server_message);

    // Thread para receber mensagens do servidor
    pthread_t receive_thread;
    if (pthread_create(&receive_thread, NULL, handle_user_messages, &client_socket) != 0)
    {
        perror("Failed to create receive thread");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Solicitar VOTER_ID
    printf("\nDigite seu VOTER_ID: ");
    fflush(stdout);
    
    char voter_id[100];
    if (fgets(voter_id, sizeof(voter_id), stdin) != NULL) {
        voter_id[strcspn(voter_id, "\n")] = '\0';
        
        char hello_cmd[MAX_MESSAGE_SIZE];
        snprintf(hello_cmd, sizeof(hello_cmd), "HELLO %s\n", voter_id);
        send_message(client_socket, hello_cmd);
        
        // Pequena pausa para receber a resposta
        sleep(1);
    }

    // Mostrar menu
    print_menu();

    // Loop para enviar comandos ao servidor
    char input[MAX_MESSAGE_SIZE];
    printf("\nComando> ");
    fflush(stdout);
    
    while (connected)
    {
        if (fgets(input, sizeof(input), stdin) != NULL)
        {
            input[strcspn(input, "\n")] = '\0';

            if (strlen(input) > 0)
            {
                // Adicionar newline para protocolo
                char command[MAX_MESSAGE_SIZE];
                snprintf(command, sizeof(command), "%s\n", input);
                
                send_message(client_socket, command);

                if (strncmp(input, "BYE", 3) == 0)
                {
                    sleep(1); // Aguardar resposta
                    connected = 0;
                    break;
                }
                
                // Pequena pausa para receber resposta
                usleep(100000); // 100ms
            }
            
            if (connected) {
                printf("\nComando> ");
                fflush(stdout);
            }
        }
    }

    // Aguardar thread de recebimento terminar
    pthread_join(receive_thread, NULL);

    close(client_socket);
    printf("\n[SISTEMA] Desconectado do servidor.\n");

    return EXIT_SUCCESS;
}