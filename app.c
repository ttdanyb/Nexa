#define _WIN32_WINNT 0x0600

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

// Modif de Daniella
// Modif de Rachel

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define UDP_PORT 5000
#define TCP_PORT 6000
#define MAX_MSG 512
#define MAX_PEERS 64

/* ================= STRUCTURES ================= */

typedef struct {
    char uuid[40];
    char ip[16];
    char pseudo[36]; // pseudo + suffixe
} Peer;

typedef struct UniMsg {
    long id;
    char ip[16];
    char data[MAX_MSG];
    int confirmed;
    struct UniMsg* next;
} UniMsg;

/* ================= GLOBALS ================= */

Peer peers[MAX_PEERS];
int peer_count = 0;

UniMsg* uni_head = NULL;

CRITICAL_SECTION peersLock;
CRITICAL_SECTION uniLock;

char user_uuid[40];
char user_pseudo[36]; // pseudo + suffixe
long global_msg_id = 0;

/* ================= UTILS ================= */

void load_or_create_uuid(char* uuid) {
    FILE* f = fopen("uuid.txt", "r");
    if (f) {
        fgets(uuid, 40, f);
        fclose(f);
    } else {
        sprintf(uuid, "%08X-%08X", rand(), rand());
        f = fopen("uuid.txt", "w");
        fprintf(f, "%s", uuid);
        fclose(f);
    }
}

void create_pseudo_txt() {
    FILE* f = fopen("pseudo.txt", "a");
    fclose(f);
}

void save_pseudo(const char* pseudo) {
    FILE* f = fopen("pseudo.txt", "w");
    if (f) {
        fprintf(f, "%s", pseudo);
        fclose(f);
    }
}

/* ================= PSEUDO UNIQUE ================= */

// Génère un suffixe 4 hex basé sur l'UUID (déterministe)
void pseudo_from_uuid(const char* base, const char* uuid, char* final_pseudo) {
    unsigned int hash = 0;
    for (int i = 0; uuid[i]; i++) hash = hash * 31 + uuid[i];
    unsigned int suffix = hash & 0xFFFF; // 4 chiffres hex
    sprintf(final_pseudo, "%s#%04X", base, suffix);
}

void ask_pseudo() {
    char base[32] = {0};
    FILE *f = fopen("pseudo.txt", "r");
    char temp[36] = {0};
    fgets(temp, 32, f);
    if(strlen(temp) == 0){
        printf("Entrez votre pseudo : ");
        fgets(base, 32, stdin);
        base[strcspn(base, "\n")] = 0;
        if (strlen(base) == 0) strcpy(base, "Anonyme");
        pseudo_from_uuid(base, user_uuid, user_pseudo);
        save_pseudo(user_pseudo);
    }
    fclose(f);
    if(strlen(temp) > 0)
        strcpy(user_pseudo, temp);
    printf("Votre Pseudo est : %s\n", user_pseudo);
}

/* ================= PEERS ================= */

int peer_exists(const char* uuid) {
    for (int i = 0; i < peer_count; i++)
        if (strcmp(peers[i].uuid, uuid) == 0)
            return 1;
    return 0;
}

/* ================= UDP BROADCAST ================= */

void send_broadcast(const char* uuid) {
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == INVALID_SOCKET) return;

    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(UDP_PORT);
    addr.sin_addr.s_addr = INADDR_BROADCAST;

    char msg[MAX_MSG];
    sprintf(msg, "HELLO;%s;%s", uuid, user_pseudo);

    sendto(s, msg, (int)strlen(msg), 0, (SOCKADDR*)&addr, sizeof(addr));
    closesocket(s);
}

/* ================= UDP LISTENER ================= */

DWORD WINAPI udp_listener(LPVOID arg) {
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr, from;
    int fromlen = sizeof(from);
    char buf[MAX_MSG];

    addr.sin_family = AF_INET;
    addr.sin_port = htons(UDP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(s, (SOCKADDR*)&addr, sizeof(addr));

    while (1) {
        int len = recvfrom(s, buf, MAX_MSG - 1, 0, (SOCKADDR*)&from, &fromlen);
        if (len <= 0) continue;
        buf[len] = 0;

        if (strncmp(buf, "HELLO;", 6) == 0) {
            char* uuid = strtok(buf + 6, ";");
            char* pseudo = strtok(NULL, ";");

            char ip[16];
            inet_ntop(AF_INET, &from.sin_addr, ip, 16);

            EnterCriticalSection(&peersLock);
            if (!peer_exists(uuid) && peer_count < MAX_PEERS) {
                strcpy(peers[peer_count].uuid, uuid);
                strcpy(peers[peer_count].ip, ip);
                if (pseudo) strcpy(peers[peer_count].pseudo, pseudo);
                else strcpy(peers[peer_count].pseudo, "Anonyme");
                peer_count++;
            }
            LeaveCriticalSection(&peersLock);
        }
    }
}

/* ================= TCP SEND ================= */

int send_tcp(const char* ip, const char* msg) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) return 0;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TCP_PORT);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (connect(s, (SOCKADDR*)&addr, sizeof(addr)) != 0) {
        closesocket(s);
        return 0;
    }

    send(s, msg, (int)strlen(msg), 0);

    DWORD timeout = 3000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    char ack[8] = {0};
    int r = recv(s, ack, sizeof(ack)-1, 0);
    closesocket(s);

    if (r <= 0) return 0;
    return (strcmp(ack, "ACK") == 0);
}

/* ================= TCP LISTENER ================= */

DWORD WINAPI tcp_listener(LPVOID arg) {
    SOCKET srv = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(TCP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(srv, (SOCKADDR*)&addr, sizeof(addr));
    listen(srv, 5);

    while (1) {
        SOCKET c = accept(srv, NULL, NULL);
        if (c == INVALID_SOCKET) continue;

        char buf[MAX_MSG];
        int r = recv(c, buf, MAX_MSG - 1, 0);
        if (r > 0) {
            buf[r] = 0;

            // enregistrer le message
            FILE* f = fopen("messages.txt", "a");
            if (f) {
                fprintf(f, "[RECU] %s\n", buf);
                fclose(f);
            }

            send(c, "ACK", 3, 0);
        }
        closesocket(c);
    }
}

/* ================= UNICAST QUEUE ================= */

void add_unicast(const char* ip, const char* msg) {
    UniMsg* m = malloc(sizeof(UniMsg));
    m->id = InterlockedIncrement(&global_msg_id);
    strcpy(m->ip, ip);
    strcpy(m->data, msg);
    m->confirmed = 0;

    EnterCriticalSection(&uniLock);
    m->next = uni_head;
    uni_head = m;
    LeaveCriticalSection(&uniLock);

    FILE* f = fopen("messages.txt", "a");
    if (f) {
        fprintf(f, "[ENVOYE vers %s] %s\n", ip, msg);
        fclose(f);
    }
}

/* ================= CLEAN CONFIRMED MESSAGES ================= */

void cleanup_confirmed() {
    EnterCriticalSection(&uniLock);
    UniMsg** curr = &uni_head;
    while (*curr) {
        if ((*curr)->confirmed) {
            UniMsg* to_free = *curr;
            *curr = (*curr)->next;
            free(to_free);
        } else {
            curr = &(*curr)->next;
        }
    }
    LeaveCriticalSection(&uniLock);
}

/* ================= SENDER THREAD ================= */

DWORD WINAPI sender_thread(LPVOID arg) {
    char* uuid = (char*)arg;

    while (1) {
        send_broadcast(uuid);

        EnterCriticalSection(&uniLock);
        UniMsg* m = uni_head;
        while (m) {
            if (!m->confirmed) {
                if (send_tcp(m->ip, m->data))
                    m->confirmed = 1;
            }
            m = m->next;
        }
        LeaveCriticalSection(&uniLock);

        cleanup_confirmed();
        Sleep(1000 + rand() % 2000);
    }
}

/* ================= MENU ================= */

void print_menu() {
    printf("\n=== MENU ===\n");
    printf("1. Messages envoyes et recus\n");
    printf("2. Utilisateurs en ligne\n");
    printf("3. Changer pseudo\n");
    printf("4. Envoyer un message\n");
    printf("0. Quitter\n");
    printf("Choix: ");
}

void menu_loop() {
    int choix;
    char input[256];
    char new_pseudo[32];
    char dest_pseudo[36];
    char message[MAX_MSG];

    while (1) {
        print_menu();
        scanf("%d", &choix);
        getchar();

        if (choix == 0) break;

        else if (choix == 1) {
            FILE* f = fopen("messages.txt", "r");
            if (!f) { printf("Aucun message.\n"); continue; }
            char line[512];
            printf("\n=== MESSAGES ===\n");
            while (fgets(line, 512, f)) printf("%s", line);
            fclose(f);
            printf("\nAppuyez sur ENTER pour revenir au menu...");
            getchar();
        }

        else if (choix == 2) {
            EnterCriticalSection(&peersLock);
            printf("\n=== UTILISATEURS EN LIGNE ===\n");
            for (int i=0; i<peer_count; i++)
                printf("%s (%s)\n", peers[i].pseudo, peers[i].ip);
            LeaveCriticalSection(&peersLock);
            printf("\nAppuyez sur ENTER pour revenir au menu...");
            getchar();
        }

        else if (choix == 3) {
            printf("Nouveau pseudo: ");
            fgets(new_pseudo, 32, stdin);
            new_pseudo[strcspn(new_pseudo, "\n")] = 0;
            if (strlen(new_pseudo) == 0) strcpy(new_pseudo, "Anonyme");
            pseudo_from_uuid(new_pseudo, user_uuid, user_pseudo);
            save_pseudo(user_pseudo);
            printf("Pseudo modifie: %s\n", user_pseudo);
        }

        else if (choix == 4) {
            EnterCriticalSection(&peersLock);
            printf("\n=== UTILISATEURS DISPONIBLES ===\n");
            for (int i=0; i<peer_count; i++)
                printf("%s (%s)\n", peers[i].pseudo, peers[i].ip);
            LeaveCriticalSection(&peersLock);

            printf("\nPseudo du destinataire: ");
            fgets(dest_pseudo, 36, stdin);
            dest_pseudo[strcspn(dest_pseudo, "\n")] = 0;

            char dest_ip[16] = "";
            int found = 0;
            EnterCriticalSection(&peersLock);
            for (int i=0; i<peer_count; i++) {
                if (strcmp(peers[i].pseudo, dest_pseudo) == 0) {
                    strcpy(dest_ip, peers[i].ip);
                    found = 1;
                    break;
                }
            }
            LeaveCriticalSection(&peersLock);

            if (!found) { printf("Utilisateur non trouve.\n"); continue; }

            printf("Message: ");
            fgets(message, MAX_MSG, stdin);
            message[strcspn(message, "\n")] = 0;

            add_unicast(dest_ip, message);
            printf("Message ajoute a la file pour %s.\n", dest_pseudo);
        }
    }
}

/* ================= MAIN ================= */

int main() {
    WSADATA wsa;
    srand((unsigned)time(NULL));

    WSAStartup(MAKEWORD(2, 2), &wsa);

    InitializeCriticalSection(&peersLock);
    InitializeCriticalSection(&uniLock);

    load_or_create_uuid(user_uuid);
    create_pseudo_txt();
    ask_pseudo(); // pseudo demandé au démarrage et suffixe unique

    CreateThread(NULL, 0, udp_listener, NULL, 0, NULL);
    CreateThread(NULL, 0, tcp_listener, NULL, 0, NULL);
    CreateThread(NULL, 0, sender_thread, user_uuid, 0, NULL);

    menu_loop();

    DeleteCriticalSection(&peersLock);
    DeleteCriticalSection(&uniLock);
    WSACleanup();
    return 0;
}
