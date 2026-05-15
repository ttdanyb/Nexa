#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DB_FILE "messages_db.txt"
#define ARCHIVE_FILE "messages_archive.txt"

// 1. Enregistre un message avec un horodatage (crucial pour UDP)
int save_messages_to_db(const char* message) {
    FILE *file = fopen(DB_FILE, "a");
    if (file == NULL) return -1;

    time_t now = time(NULL);
    char *date = ctime(&now);
    date[strlen(date) - 1] = '\0'; // Enlever le retour à la ligne de ctime

    fprintf(file, "[%s] %s\n", date, message);
    fclose(file);
    return 0;
}

// 2. Charge et affiche les messages stockés
int load_messages_from_db() {
    FILE *file = fopen(DB_FILE, "r");
    if (file == NULL) {
        printf("Aucun historique trouvé.\n");
        return -1;
    }

    char buffer[1024];
    printf("--- Historique des messages ---\n");
    while (fgets(buffer, sizeof(buffer), file)) {
        printf("%s", buffer);
    }
    fclose(file);
    return 0;
}

// 3. Déplace le contenu de la DB vers l'archive et vide la DB
void archive_old_messages() {
    FILE *db = fopen(DB_FILE, "r");
    FILE *archive = fopen(ARCHIVE_FILE, "a");

    if (db && archive) {
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), db)) {
            fputs(buffer, archive);
        }
        fclose(db);
        fclose(archive);
        
        // Vider le fichier original
        remove(DB_FILE);
        printf("Messages archivés avec succès.\n");
    }
}

// 4. Exporte l'historique dans un fichier spécifique
int export_chat_history(const char* filename) {
    FILE *db = fopen(DB_FILE, "r");
    FILE *export_file = fopen(filename, "w");

    if (db == NULL || export_file == NULL) return -1;

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), db)) {
        fputs(buffer, export_file);
    }

    fclose(db);
    fclose(export_file);
    return 0;
}

// --- Exemple d'utilisation dans un contexte UDP ---
int main() {
    // Simulation de réception de messages UDP
    save_messages_to_db("Utilisateur1: Salut, tu reçois mon paquet UDP ?");
    save_messages_to_db("Utilisateur2: Oui, le datagramme est bien arrivé.");

    // Chargement au démarrage
    load_messages_from_db();

    // Exportation
    if (export_chat_history("mon_export.txt") == 0) {
        printf("Historique exporté dans 'mon_export.txt'\n");
    }

    // Archivage pour libérer de la place
    archive_old_messages();

    return 0;
}
