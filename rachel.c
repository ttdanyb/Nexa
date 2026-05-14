#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

/* ================= SDL UI ================= */

void ui_loop(); 
// Boucle principale de l'interface graphique

void render_main_window(); 
// Dessine toute l'interface (layout général)

void render_user_list(); 
// Affiche la liste des utilisateurs connectés (peers)

void render_chat_box(); 
// Affiche les messages du chat (lecture messages.txt)

void render_input_box(); 
// Zone où l'utilisateur écrit son message

void render_send_button(); 
// Bouton pour envoyer un message

void handle_ui_events(SDL_Event* event); 
// Gère les clics souris, clavier et interactions utilisateur

void send_current_message(); 
// Récupère le texte écrit et appelle add_unicast()

void cleanup_sdl(); 
// Libère la mémoire SDL et ferme la fenêtre

char current_message[512] = "";

/* ================= INITIALISATION SDL ================= */

int main(int argc, char **argv) {

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Erreur SDL: %s\n", SDL_GetError());
        return 0;
    }

    window = SDL_CreateWindow(
        "P2P Chat",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        900,
        600,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        printf("Erreur creation fenetre\n");
        return 0;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer) {
        printf("Erreur renderer\n");
        return 0;
    }
    ui_loop();
    cleanup_sdl();

    return 1;
}

/* ================= FENETRE PRINCIPALE ================= */

void render_main_window() {

    SDL_SetRenderDrawColor(renderer, 250,250,250,255);
    SDL_RenderClear(renderer);

    render_user_list();
    render_chat_box();
    render_input_box();
    render_send_button();

    SDL_RenderPresent(renderer);
}

/* ================= LISTE UTILISATEURS ================= */

void render_user_list() {

    SDL_Rect panel = {0,0,200,600};

    SDL_SetRenderDrawColor(renderer,255,220,230,255);
    SDL_RenderFillRect(renderer,&panel);
}

/* ================= CHAT BOX ================= */

void render_chat_box() {

    SDL_Rect chat = {200,0,700,500};

    SDL_SetRenderDrawColor(renderer,255,255,255,255);
    SDL_RenderFillRect(renderer,&chat);
}

/* ================= INPUT MESSAGE ================= */

void render_input_box() {

    SDL_Rect input = {200,500,550,100};

    SDL_SetRenderDrawColor(renderer,240,240,240,255);
    SDL_RenderFillRect(renderer,&input);
}

/* ================= BOUTON ENVOYER ================= */

void render_send_button() {

    SDL_Rect button = {750,500,150,100};

    SDL_SetRenderDrawColor(renderer,255,120,160,255);
    SDL_RenderFillRect(renderer,&button);
}

/* ================= ENVOYER MESSAGE ================= */

void send_current_message() {

    printf("Message envoye : %s\n", current_message);

    /* ici on pourra appeler add_unicast() */

    current_message[0] = '\0';
}

/* ================= EVENEMENTS ================= */

void handle_ui_events(SDL_Event* event) {

    if(event->type == SDL_QUIT)
        exit(0);

    if(event->type == SDL_MOUSEBUTTONDOWN){

        int x = event->button.x;
        int y = event->button.y;

        /* bouton envoyer */
        if(x > 750 && y > 500){
            send_current_message();
        }
    }

    if(event->type == SDL_TEXTINPUT){
        strcat(current_message,event->text.text);
    }

    if(event->type == SDL_KEYDOWN){

        if(event->key.keysym.sym == SDLK_BACKSPACE && strlen(current_message)>0){
            current_message[strlen(current_message)-1] = '\0';
        }

        if(event->key.keysym.sym == SDLK_RETURN){
            send_current_message();
        }
    }
}

/* ================= BOUCLE PRINCIPALE ================= */

void ui_loop(){

    SDL_Event event;

    SDL_StartTextInput();

    while(1){

        while(SDL_PollEvent(&event)){
            handle_ui_events(&event);
        }

        render_main_window();

        SDL_Delay(16);
    }
}

/* ================= NETTOYAGE ================= */

void cleanup_sdl(){

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
