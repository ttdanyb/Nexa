#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define NUM_CLIENTS 10
#define MESSAGE "Hello from client"

void *client_thread(void *arg) {
    int sock;
    struct sockaddr_in server;
    char buffer[1024];
    int client_id = *(int *)arg;

    // Création du socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Erreur socket");
        pthread_exit(NULL);
    }

    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);

    // Connexion au serveur
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Client %d : connexion échouée\n", client_id);
        close(sock);
        pthread_exit(NULL);
    }

    printf("Client %d connecté\n", client_id);

    // Envoi de message
    sprintf(buffer, "Client %d : %s", client_id, MESSAGE);
    send(sock, buffer, strlen(buffer), 0);

    // Réception (optionnel)
    int read_size = recv(sock, buffer, sizeof(buffer)-1, 0);
    if (read_size > 0) {
        buffer[read_size] = '\0';
        printf("Client %d reçu : %s\n", client_id, buffer);
    }

    close(sock);
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_CLIENTS];
    int client_ids[NUM_CLIENTS];

    for (int i = 0; i < NUM_CLIENTS; i++) {
        client_ids[i] = i;

        if (pthread_create(&threads[i], NULL, client_thread, &client_ids[i]) != 0) {
            perror("Erreur création thread");
        }
    }

    for (int i = 0; i < NUM_CLIENTS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Test terminé\n");
    return 0;
}
