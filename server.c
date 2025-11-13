#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>

/*
INTEGRANTES:
- Gabriel Ken Kazama Geronazzo - 10418247
- Lucas Pires de Camargo Sarai - 10418013
*/

#define MAX_MESSAGE_SIZE 1024
#define PORT 9002
#define MAX_USERS_CONNECTED 20
#define MAX_OPTIONS 10
#define MAX_OPTION_LEN 100
#define MAX_VOTER_ID_LEN 50
#define LOG_FILE "eleicao.log"
#define RESULT_FILE "resultado_final.txt"

// Estrutura para armazenar opções de votação
typedef struct {
    char name[MAX_OPTION_LEN];
    int votes;
} VotingOption;

// Estrutura para armazenar eleitores que já votaram
typedef struct VoterNode {
    char voter_id[MAX_VOTER_ID_LEN];
    char voted_option[MAX_OPTION_LEN];
    struct VoterNode *next;
} VoterNode;

// Estrutura de cliente conectado
typedef struct Client {
    int client_socket;
    char voter_id[MAX_VOTER_ID_LEN];
    int authenticated;
    struct Client *next;
} Client;

// Variáveis globais
VotingOption options[MAX_OPTIONS];
int option_count = 0;
VoterNode *voters_head = NULL;
Client *clients = NULL;
int election_closed = 0;
FILE *log_file = NULL;

// Mutexes
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t voters_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t votes_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Funções de logging
void write_log(const char *message) {
    pthread_mutex_lock(&log_mutex);
    
    time_t now = time(NULL);
    char *time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0'; // Remove newline
    
    if (log_file) {
        fprintf(log_file, "[%s] %s\n", time_str, message);
        fflush(log_file);
    }
    
    pthread_mutex_unlock(&log_mutex);
}

// Inicializar opções de votação
void init_voting_options() {
    option_count = 5;
    
    strcpy(options[0].name, "Candidato_A");
    options[0].votes = 0;
    
    strcpy(options[1].name, "Candidato_B");
    options[1].votes = 0;
    
    strcpy(options[2].name, "Candidato_C");
    options[2].votes = 0;
    
    strcpy(options[3].name, "Candidato_D");
    options[3].votes = 0;
    
    strcpy(options[4].name, "Branco");
    options[4].votes = 0;
    
    write_log("Sistema de votação inicializado com 5 opções");
}

// Verificar se eleitor já votou
int has_voted(const char *voter_id) {
    pthread_mutex_lock(&voters_mutex);
    
    VoterNode *current = voters_head;
    while (current != NULL) {
        if (strcmp(current->voter_id, voter_id) == 0) {
            pthread_mutex_unlock(&voters_mutex);
            return 1;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&voters_mutex);
    return 0;
}

// Adicionar eleitor à lista de quem já votou
void add_voter(const char *voter_id, const char *option) {
    pthread_mutex_lock(&voters_mutex);
    
    VoterNode *new_voter = (VoterNode *)malloc(sizeof(VoterNode));
    strcpy(new_voter->voter_id, voter_id);
    strcpy(new_voter->voted_option, option);
    new_voter->next = voters_head;
    voters_head = new_voter;
    
    pthread_mutex_unlock(&voters_mutex);
}

// Validar opção de voto
int is_valid_option(const char *option_name) {
    for (int i = 0; i < option_count; i++) {
        if (strcmp(options[i].name, option_name) == 0) {
            return 1;
        }
    }
    return 0;
}

// Registrar voto
int register_vote(const char *voter_id, const char *option_name) {
    if (election_closed) {
        return -2; // Eleição encerrada
    }
    
    if (has_voted(voter_id)) {
        return -1; // Já votou
    }
    
    pthread_mutex_lock(&votes_mutex);
    
    int found = 0;
    for (int i = 0; i < option_count; i++) {
        if (strcmp(options[i].name, option_name) == 0) {
            options[i].votes++;
            found = 1;
            break;
        }
    }
    
    pthread_mutex_unlock(&votes_mutex);
    
    if (found) {
        add_voter(voter_id, option_name);
        
        char log_msg[MAX_MESSAGE_SIZE];
        snprintf(log_msg, sizeof(log_msg), "VOTO: %s votou em %s", voter_id, option_name);
        write_log(log_msg);
        
        return 0; // Sucesso
    }
    
    return -3; // Opção inválida
}

void add_user(int client_socket, const char *voter_id)
{
    pthread_mutex_lock(&clients_mutex);

    Client *new_user = (Client *)malloc(sizeof(Client));
    new_user->client_socket = client_socket;
    strcpy(new_user->voter_id, voter_id);
    new_user->authenticated = 0;

    if (clients == NULL)
    {
        clients = new_user;
        new_user->next = NULL;
    }
    else
    {
        Client *current = clients;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = new_user;
        new_user->next = NULL;
    }

    pthread_mutex_unlock(&clients_mutex);
}

void remove_user(int client_socket)
{
    pthread_mutex_lock(&clients_mutex);

    Client *current = clients;
    Client *previous = NULL;

    while (current != NULL)
    {
        if (current->client_socket == client_socket)
        {
            if (previous == NULL)
            {
                clients = current->next;
            }
            else
            {
                previous->next = current->next;
            }
            
            char log_msg[MAX_MESSAGE_SIZE];
            snprintf(log_msg, sizeof(log_msg), "Cliente desconectado: socket %d, voter_id: %s", 
                     client_socket, current->voter_id);
            write_log(log_msg);
            
            free(current);
            pthread_mutex_unlock(&clients_mutex);
            return;
        }
        previous = current;
        current = current->next;
    }

    pthread_mutex_unlock(&clients_mutex);
}

int count_connected_users()
{
    pthread_mutex_lock(&clients_mutex);

    int count = 0;
    Client *current = clients;

    while (current != NULL)
    {
        count++;
        current = current->next;
    }

    pthread_mutex_unlock(&clients_mutex);
    return count;
}

int create_server_socket()
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Permitir reutilização de endereço
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_USERS_CONNECTED) < 0)
    {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Servidor de votação iniciado na porta %d\n", PORT);
    printf("Capacidade: %d clientes simultâneos\n", MAX_USERS_CONNECTED);
    write_log("Servidor iniciado");

    return server_socket;
}

int accept_client_connection(int server_socket)
{
    int client_socket = accept(server_socket, NULL, NULL);

    if (client_socket < 0)
    {
        perror("Accept failed");
        return -1;
    }

    return client_socket;
}

void get_score_string(char *buffer, int buffer_size) {
    pthread_mutex_lock(&votes_mutex);
    
    int offset = 0;
    offset += snprintf(buffer + offset, buffer_size - offset, "SCORE %d", option_count);
    
    for (int i = 0; i < option_count; i++) {
        offset += snprintf(buffer + offset, buffer_size - offset, " %s:%d", 
                          options[i].name, options[i].votes);
    }
    offset += snprintf(buffer + offset, buffer_size - offset, "\n");
    
    pthread_mutex_unlock(&votes_mutex);
}

void save_final_result() {
    FILE *result_file = fopen(RESULT_FILE, "w");
    if (!result_file) {
        write_log("ERRO: Não foi possível criar arquivo de resultado");
        return;
    }
    
    fprintf(result_file, "=== RESULTADO FINAL DA ELEIÇÃO ===\n\n");
    
    time_t now = time(NULL);
    fprintf(result_file, "Data de encerramento: %s\n", ctime(&now));
    
    int total_votes = 0;
    for (int i = 0; i < option_count; i++) {
        total_votes += options[i].votes;
    }
    
    fprintf(result_file, "Total de votos: %d\n\n", total_votes);
    fprintf(result_file, "Detalhamento por opção:\n");
    fprintf(result_file, "%-20s | %-10s | %-10s\n", "Opção", "Votos", "Percentual");
    fprintf(result_file, "----------------------------------------------------\n");
    
    for (int i = 0; i < option_count; i++) {
        double percentage = (total_votes > 0) ? (options[i].votes * 100.0 / total_votes) : 0.0;
        fprintf(result_file, "%-20s | %-10d | %6.2f%%\n", 
                options[i].name, options[i].votes, percentage);
    }
    
    fclose(result_file);
    write_log("Resultado final salvo em resultado_final.txt");
}

void process_client_command(int client_socket, const char *message, char *voter_id, int *authenticated) {
    char response[MAX_MESSAGE_SIZE];
    char command[50], param1[MAX_OPTION_LEN], param2[MAX_OPTION_LEN];
    
    int parsed = sscanf(message, "%s %s %s", command, param1, param2);
    
    if (parsed < 1) {
        return;
    }
    
    // Comando HELLO
    if (strcmp(command, "HELLO") == 0 && parsed >= 2) {
        strcpy(voter_id, param1);
        *authenticated = 1;
        
        snprintf(response, sizeof(response), "WELCOME %s\n", voter_id);
        send(client_socket, response, strlen(response), 0);
        
        char log_msg[MAX_MESSAGE_SIZE];
        snprintf(log_msg, sizeof(log_msg), "Cliente autenticado: %s (socket %d)", voter_id, client_socket);
        write_log(log_msg);
        return;
    }
    
    // Verificar autenticação para outros comandos
    if (!(*authenticated)) {
        snprintf(response, sizeof(response), "ERR NOT_AUTHENTICATED\n");
        send(client_socket, response, strlen(response), 0);
        return;
    }
    
    // Comando ADMIN CLOSE
    if (strcmp(command, "ADMIN") == 0 && strcmp(param1, "CLOSE") == 0) {
        election_closed = 1;
        
        char score_buffer[MAX_MESSAGE_SIZE];
        get_score_string(score_buffer, sizeof(score_buffer));
        
        // Substituir SCORE por CLOSED FINAL
        char final_msg[MAX_MESSAGE_SIZE];
        snprintf(final_msg, sizeof(final_msg), "CLOSED FINAL %s", score_buffer + 6); // Skip "SCORE "
        
        send(client_socket, final_msg, strlen(final_msg), 0);
        
        save_final_result();
        write_log("ELEIÇÃO ENCERRADA por comando administrativo");
        
        printf("\n=== ELEIÇÃO ENCERRADA ===\n");
        return;
    }
    
    // Comando LIST
    if (strcmp(command, "LIST") == 0) {
        snprintf(response, sizeof(response), "OPTIONS %d", option_count);
        for (int i = 0; i < option_count; i++) {
            strcat(response, " ");
            strcat(response, options[i].name);
        }
        strcat(response, "\n");
        
        send(client_socket, response, strlen(response), 0);
        return;
    }
    
    // Comando VOTE
    if (strcmp(command, "VOTE") == 0 && parsed >= 2) {
        int result = register_vote(voter_id, param1);
        
        if (result == 0) {
            snprintf(response, sizeof(response), "OK VOTED %s\n", param1);
        } else if (result == -1) {
            snprintf(response, sizeof(response), "ERR DUPLICATE\n");
        } else if (result == -2) {
            snprintf(response, sizeof(response), "ERR CLOSED\n");
        } else {
            snprintf(response, sizeof(response), "ERR INVALID_OPTION\n");
        }
        
        send(client_socket, response, strlen(response), 0);
        return;
    }
    
    // Comando SCORE
    if (strcmp(command, "SCORE") == 0) {
        if (election_closed) {
            char score_buffer[MAX_MESSAGE_SIZE];
            get_score_string(score_buffer, sizeof(score_buffer));
            
            char final_msg[MAX_MESSAGE_SIZE];
            snprintf(final_msg, sizeof(final_msg), "CLOSED FINAL %s", score_buffer + 6);
            send(client_socket, final_msg, strlen(final_msg), 0);
        } else {
            char score_buffer[MAX_MESSAGE_SIZE];
            get_score_string(score_buffer, sizeof(score_buffer));
            send(client_socket, score_buffer, strlen(score_buffer), 0);
        }
        return;
    }
    
    // Comando BYE
    if (strcmp(command, "BYE") == 0) {
        snprintf(response, sizeof(response), "BYE\n");
        send(client_socket, response, strlen(response), 0);
        return;
    }
    
    // Comando desconhecido
    snprintf(response, sizeof(response), "ERR UNKNOWN_COMMAND\n");
    send(client_socket, response, strlen(response), 0);
}

void connect_user(int client_socket)
{
    char welcome_message[MAX_MESSAGE_SIZE];
    snprintf(welcome_message, sizeof(welcome_message), 
             "Bem-vindo ao Sistema de Votação!\nEnvie: HELLO <VOTER_ID> para começar\n");
    send(client_socket, welcome_message, strlen(welcome_message), 0);
}

void disconnect_user(int client_socket)
{
    remove_user(client_socket);
    close(client_socket);
}

typedef struct
{
    int client_socket;
} ClientData;

void *handle_client(void *arg)
{
    ClientData *client_data = (ClientData *)arg;
    int client_socket = client_data->client_socket;
    free(client_data);

    char message[MAX_MESSAGE_SIZE];
    char voter_id[MAX_VOTER_ID_LEN] = "";
    int authenticated = 0;

    for (;;)
    {
        memset(message, 0, sizeof(message));
        ssize_t bytes_received = recv(client_socket, message, sizeof(message) - 1, 0);

        if (bytes_received <= 0)
        {
            printf("Cliente %d desconectado.\n", client_socket);
            disconnect_user(client_socket);
            break;
        }

        // Remove quebra de linha
        message[strcspn(message, "\n")] = '\0';
        message[strcspn(message, "\r")] = '\0';

        if (strlen(message) > 0)
        {
            printf("Cliente %d: %s\n", client_socket, message);
            
            // Verificar comando BYE para desconectar
            if (strncmp(message, "BYE", 3) == 0) {
                process_client_command(client_socket, message, voter_id, &authenticated);
                disconnect_user(client_socket);
                break;
            }
            
            process_client_command(client_socket, message, voter_id, &authenticated);
        }
    }

    pthread_exit(NULL);
}

int main(void)
{
    // Abrir arquivo de log
    log_file = fopen(LOG_FILE, "w");
    if (!log_file) {
        perror("Erro ao criar arquivo de log");
        exit(EXIT_FAILURE);
    }
    
    init_voting_options();
    
    int server_socket = create_server_socket();

    printf("\n=== Sistema de Votação Distribuído ===\n");
    printf("Aguardando conexões...\n\n");

    for (;;)
    {
        int client_socket = accept_client_connection(server_socket);
        
        if (client_socket < 0) {
            continue;
        }

        if (count_connected_users() >= MAX_USERS_CONNECTED)
        {
            const char *error = "ERR SERVER_FULL Servidor cheio. Máximo de usuários atingido.\n";
            send(client_socket, error, strlen(error), 0);
            close(client_socket);
            write_log("Conexão recusada: servidor cheio");
            continue;
        }

        add_user(client_socket, "PENDING");
        connect_user(client_socket);

        printf("Novo cliente conectado: socket %d\n", client_socket);

        pthread_t client_thread;
        ClientData *client_data = malloc(sizeof(ClientData));
        client_data->client_socket = client_socket;

        if (pthread_create(&client_thread, NULL, handle_client, client_data) != 0)
        {
            perror("Failed to create client thread");
            disconnect_user(client_socket);
            free(client_data);
            continue;
        }

        // Detach thread para limpeza automática
        pthread_detach(client_thread);
    }

    close(server_socket);
    
    if (log_file) {
        fclose(log_file);
    }
    
    return EXIT_SUCCESS;
}