#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "board.h"

// Structure pour l'historique de mouvement (Undo)
typedef struct {
    int x;
    int y;
} Position;

/// @brief Enumération des différents Etats du jeu
typedef enum {
    STATE_SETUP,        // phase de placement des pièces
    STATE_TURN_START,   // début de tour
    STATE_PLAYER_TURN,  // Tours des joueurs (déplacement)
    STATE_END_TURN,     // finir le tour
    STATE_GAME_OVER     // fin du jeu
} GameState;

/// @brief Fonction qui permet d'alterner entre Joueur NORD ET SUD 
player turn_manager(player lastPlayer, char *name_n, char *name_s)
{
    if (lastPlayer == SOUTH_P) return NORTH_P;
    else return SOUTH_P;
}

/// @brief Fonction d'affichage
void display_board(board game, player current_player, char *name_n, char *name_s, int sel_x, int sel_y, size held_piece_size)
{
    printf("\x1b[H\x1b[2J"); // Refresh écran

    size piece_on_board;
    char *ind_n = "";
    char *ind_s = "";

    if (current_player == NORTH_P) ind_n = "  \x1b[1m<-- A TOI\x1b[0m";
    else if (current_player == SOUTH_P) ind_s = "  \x1b[1m<-- A TOI\x1b[0m";

    printf("\n");
    printf("         NORD : \x1b[34m%s\x1b[0m%s       \n", name_n, ind_n);
    printf("  //  //  // \\\\  \\\\  \\\\\n");
    printf("\x1b[90m╔═══╦═══╦═══╦═══╦═══╦═══╗\x1b[0m\n");

    for (int lig = DIMENSION - 1; lig >= 0; lig--) { 
        for (int col = 0; col < DIMENSION; col++) {
            piece_on_board = get_piece_size(game, lig, col); 
            printf("\x1b[90m║\x1b[0m");

            bool isSelected = (lig == sel_x && col == sel_y);

            if (isSelected && held_piece_size != NONE) {
                switch (held_piece_size) {
                    case ONE:   printf("\x1b[33m\x1b[1m*1*\x1b[0m"); break; 
                    case TWO:   printf("\x1b[33m\x1b[1m*2*\x1b[0m"); break;
                    case THREE: printf("\x1b[33m\x1b[1m*3*\x1b[0m"); break;
                    default:    printf(" * "); break;
                }
            } 
            else {
                switch (piece_on_board) {
                    case ONE:   printf("\x1b[34m\x1b[1m 1 \x1b[0m"); break;
                    case TWO:   printf("\x1b[32m\x1b[1m 2 \x1b[0m"); break;
                    case THREE: printf("\x1b[31m\x1b[1m 3 \x1b[0m"); break;
                    default:    printf("   "); break;
                }
            }
        }
        printf("\x1b[90m║\x1b[0m %d", lig + 1); 

        printf("    "); 
        if (lig == DIMENSION - 1) printf("\x1b[1m--- COMMANDES ---\x1b[0m");
        else if (lig == DIMENSION - 2) printf("[N, S, E, O, G] : Directions");
        else if (lig == DIMENSION - 3) printf("[M] : Valider mouvement");
        else if (lig == DIMENSION - 4) printf("[U] : Undo / [C] : Cancel");
        else if (lig == DIMENSION - 6) printf("[s]wap / [r]ebond");
        printf("\n"); 

        if (lig > 0) printf("\x1b[90m╠═══╬═══╬═══╬═══╬═══╬═══╣\x1b[0m\n");
    }

    printf("\x1b[90m╚═══╩═══╩═══╩═══╩═══╩═══╝\x1b[0m\n");
    printf("  \\\\  \\\\  \\\\ //  //  //\n");
    printf("         SUD : \x1b[31m%s\x1b[0m%s       \n", name_s, ind_s);
	printf("__________________________________________________ \n");
}

int pile_ou_face() 
{
    srand(time(NULL));
    return rand() % 2; 
}

player first_player(int n)
{
	return (n == 0) ? SOUTH_P : NORTH_P;
}

bool reste_des_piece(player p, board game)
{
	if(nb_pieces_available(game, ONE, p) != 0) return true;
	if(nb_pieces_available(game, TWO, p) != 0) return true;
	if(nb_pieces_available(game, THREE, p) != 0) return true;
	return false;
}

int saisir_piece(board game, player p)
{
    int nb = 0;
    bool isValide = true;
    size taille = NONE;
    
    while (isValide)
    {
        printf("Veuillez saisir une taille de Pièce (1, 2 ou 3) : ");
        if (scanf("%d", &nb) != 1) {
            int c; while ((c = getchar()) != '\n' && c != EOF);
            printf("Entrée invalide.\n");
            continue; 
        }

        if (nb >= 1 && nb <= 3) {
            switch (nb) {
                case 1: taille = ONE; break;
                case 2: taille = TWO; break;
                case 3: taille = THREE; break;
            }
            if (nb_pieces_available(game, taille, p) != 0) return nb; 
            else printf("Vous n'avez plus de pièce %d !\n", nb);
        } else {
            printf("NON VALIDE : Veuillez saisir 1, 2, ou 3.\n");
        }
    }
    return nb; 
}

// --- MODIFICATION POUR AUTO SETUP (67) ---
int saisir_coord(board game, const char *message, int min, int max, int nbVal, player p)
{
    int nb, colIndex;   
    int fixedLine = (p == SOUTH_P) ? 0 : DIMENSION - 1;  

    do {
        printf("%s (%d à %d) : ", message, min, max);
		if (scanf("%d", &nb) != 1) {
            int c; while ((c = getchar()) != '\n' && c != EOF);
            continue; 
        }

        // --- CHEAT CODE ---
        if (nb == 67) {
            return -99; // Code secret pour l'auto-placement
        }
        // ------------------

        if (nb < min || nb > max) {
            printf("Valeur invalide.\n");
            continue;  
        }

        colIndex = nb - 1;
        if (nbVal == 1) {
            if (get_piece_size(game, fixedLine, colIndex) != NONE) {
                printf("Case (%d,%d) occupée.\n", fixedLine + 1, colIndex + 1);
                continue;
            }
        }
        break;
    } while (1);
    return colIndex;
}

int saisir_nb(const char *message)
{
    int nb;
    do {
        printf("%s", message);  
        if (scanf("%d", &nb) != 1) {
            int c; while ((c = getchar()) != '\n' && c != EOF);
            continue;
        }
        if (nb < 1 || nb > 6) printf("Valeur invalide (1-6).\n");
    } while (nb < 1 || nb > 6);
    return nb - 1;  
}

// --- NOUVELLE FONCTION : PLACEMENT AUTOMATIQUE ---
void auto_place_remaining(board game, player p) {
    int row = (p == SOUTH_P) ? 0 : DIMENSION - 1;
    
    // On parcourt les tailles de 3 à 1
    for (int s = 3; s >= 1; s--) {
        size taille_enum = (s == 1) ? ONE : (s == 2) ? TWO : THREE;

        // Tant qu'il reste des pièces de cette taille
        while (nb_pieces_available(game, taille_enum, p) > 0) {
            // On cherche la première colonne vide
            for (int col = 0; col < DIMENSION; col++) {
                if (get_piece_size(game, row, col) == NONE) {
                    place_piece(game, s, p, col);
                    break; // Pièce posée, on passe à la suivante
                }
            }
        }
    }
}

void setup_pieces_game(board game, player p, char *name_n, char *name_s)
{
    bool canPlace = true;
    int column = 0;
    int piece = 0;
    
    while(canPlace)
    {
        display_board(game, p, name_n, name_s, -1, -1, NONE); 

        // Si personne n'a plus rien, on sort
        if(!reste_des_piece(SOUTH_P,game) && !reste_des_piece(NORTH_P,game)) {
            canPlace = false;
            break;
        }

        // On vérifie si le joueur actuel a encore des pièces
        // S'il n'en a plus (parce qu'il a déjà tout placé), on passe son tour
        if (!reste_des_piece(p, game)) {
            p = turn_manager(p, name_n, name_s);
            continue;
        }

        // --- SAISIE ---
        if(p == NORTH_P) printf("%s (NORD), numéro de la colonne", name_n);
        else printf("%s (SUD), numéro de la colonne", name_s);
            
        column = saisir_coord(game, " : ", 1, DIMENSION, 1, p);

        // --- GESTION DU CODE 67 ---
        if (column == -99) {
            printf(">> SETUP AUTOMATIQUE ACTIVÉ pour %s !\n", (p==NORTH_P)?name_n:name_s);
            auto_place_remaining(game, p);
            // On ne demande pas la pièce, on passe direct au joueur suivant
            p = turn_manager(p, name_n, name_s);
            continue; 
        }
        // --------------------------
        
        // Comportement normal si pas 67
        piece = saisir_piece(game, p);
        place_piece(game, piece, p, column);
        p = turn_manager(p, name_n, name_s);
    }   
}

GameState state_setup(board game, player current_player, char *name_n, char *name_s)
{
    setup_pieces_game(game, current_player, name_n, name_s);
    return STATE_TURN_START;
}

GameState state_turn_start(board game, player *current_player, char *name_n, char *name_s)
{
    *current_player = turn_manager(*current_player, name_n, name_s);
    return STATE_PLAYER_TURN;
}

GameState state_player_turn(board game, player *current_player, char *name_n, char *name_s)
{
    int x = 0, y = 0; 
    direction dir;
    return_code rc;

    display_board(game, *current_player, name_n, name_s, -1, -1, NONE);
    
    x = saisir_nb("Choisir la ligne de la pièce (1-6) : ");   
    y = saisir_nb("Choisir la colonne de la pièce (1-6) : ");   

    size picked_size = get_piece_size(game, x, y);

    rc = pick_piece(game, *current_player, x, y);
    if (rc != OK) {
        printf("Impossible de prendre cette pièce (code %d)\n", rc);
        printf("Appuyez sur Entrée...");
        getchar(); getchar();
        return STATE_PLAYER_TURN; 
    }

    // Init Tracking
    Position history[50]; 
    int hist_idx = 0;
    int cur_x = x; 
    int cur_y = y; 

    history[hist_idx].x = cur_x;
    history[hist_idx].y = cur_y;
    hist_idx++;

    display_board(game, *current_player, name_n, name_s, cur_x, cur_y, picked_size);

    while (picked_piece_owner(game) == *current_player) {

        int moves = movement_left(game);
        printf("Mouvements restants = %d\n", moves);

        if (moves == -1) break; 

        printf("\n[N, S, E, O] Deplacer | [U] Undo | [C] Cancel | [G] Goal\n");
        printf("Votre choix : ");
        char cmd;
        scanf(" %c", &cmd);

        if (cmd == 'U') {
            rc = cancel_step(game);
            if (rc == OK) {
                if (hist_idx > 1) {
                    hist_idx--; 
                    cur_x = history[hist_idx - 1].x;
                    cur_y = history[hist_idx - 1].y;
                }
                display_board(game, *current_player, name_n, name_s, cur_x, cur_y, picked_size); 
            } else {
                printf("Impossible d'annuler.\n");
            }
            continue;
        }

        if (cmd == 'C') {
            cancel_movement(game);
            display_board(game, *current_player, name_n, name_s, -1, -1, NONE); 
            return STATE_PLAYER_TURN;
        }

        if (cmd == 'G') {
            if (is_move_possible(game, GOAL)) {
                move_piece(game, GOAL);
                display_board(game, *current_player, name_n, name_s, -1, -1, NONE); 
                continue;
            } else {
                printf("Impossible d'aller au but.\n");
                continue;
            }
        }

        if (cmd == 'N' || cmd == 'S' || cmd == 'E' || cmd == 'O') {
            dir = (cmd == 'N') ? NORTH : (cmd == 'S') ? SOUTH : (cmd == 'E') ? EAST : WEST;

            if (moves == 0) {
                char choix;
                printf("Bloqué ! [r]ebond ou [s]wap ? : ");
                scanf(" %c", &choix);

                if (choix == 's') {
                    int tl = saisir_nb("Ligne swap : ");
                    int tc = saisir_nb("Colonne swap : ");
                    if (swap_piece(game, tl, tc) == OK) {
                        cur_x = tl; cur_y = tc;
                        history[hist_idx].x = cur_x; history[hist_idx].y = cur_y; hist_idx++;
                        display_board(game, *current_player, name_n, name_s, cur_x, cur_y, picked_size); 
                        break; 
                    }
                    continue; 
                }
            }

            if (is_move_possible(game, dir)) {
                rc = move_piece(game, dir);
                if (rc == OK) {
                    if (cmd == 'N') cur_x++;
                    if (cmd == 'S') cur_x--;
                    if (cmd == 'E') cur_y++;
                    if (cmd == 'O') cur_y--;
                    
                    if(hist_idx < 49) {
                        history[hist_idx].x = cur_x;
                        history[hist_idx].y = cur_y;
                        hist_idx++;
                    }
                    display_board(game, *current_player, name_n, name_s, cur_x, cur_y, picked_size); 
                }
            } else {
                printf("Mouvement impossible !\n");
                getchar(); getchar(); 
                display_board(game, *current_player, name_n, name_s, cur_x, cur_y, picked_size); 
            }
            continue;
        }
        printf("Commande inconnue.\n");
    }

    display_board(game, *current_player, name_n, name_s, -1, -1, NONE);

    if (movement_left(game) == -1) return STATE_END_TURN;  
    return STATE_PLAYER_TURN;
}

GameState state_end_turn(board game, player *current_player, char *name_n, char *name_s)
{
    player w = get_winner(game);
    if (w != NO_PLAYER) {
        printf("\x1b[H\x1b[2J"); 
        display_board(game, *current_player, name_n, name_s, -1, -1, NONE);
        printf("\n\n****************************************\n");
        printf(" VICTOIRE ! Bravo %s a gagné !\n", (w == NORTH_P) ? name_n : name_s);   
        printf("****************************************\n\n");
        return STATE_GAME_OVER;
    }
    return STATE_TURN_START;
}

int main(int args, char **argv)
{
    printf("\x1b[H\x1b[2J"); 

    board game = new_game();
    char name_n[50], name_s[50];

    printf("--- JEU DE PLATEAU ---\n\n");
    printf("Nom JOUEUR NORD : "); scanf("%s", name_n);
    printf("Nom JOUEUR SUD : "); scanf("%s", name_s);
    
    player p = first_player(pile_ou_face());
    GameState state = STATE_SETUP;

    printf("%s commence !\n", (p == NORTH_P) ? name_n : name_s);

    while (state != STATE_GAME_OVER) {
        switch (state) {
            case STATE_SETUP:      state = state_setup(game, p, name_n, name_s); break;
            case STATE_TURN_START: state = state_turn_start(game, &p, name_n, name_s); break;
            case STATE_PLAYER_TURN:state = state_player_turn(game, &p, name_n, name_s); break;
            case STATE_END_TURN:   state = state_end_turn(game, &p, name_n, name_s); break;
        }
    }
    destroy_game(game);
    return 0;
}