#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "board.h"


// Structure qui sauvegarde les coordonnées d'un pas  utile pour annulé un move 
typedef struct {
    int old_line;
    int old_col;
    int moves_at_step;
} step_history;

// Structure principale du plateau de jeu
struct board_s {
    size grid[DIMENSION][DIMENSION]; 

    // Compteurs pour la phase de placement 
    int setup_counts[NB_PLAYERS + 1][NB_SIZE + 1];

    player winner;

    //Attributs de la pièce en main
    player current_player;
    size picked_piece;
    int p_line;
    int p_col;
    int moves_remaining;
    

    int start_line;
    int start_col;

    step_history history[50];
    int history_index;
};

// Fonction pour l'initialisation d'une nouvelle partie
board new_game() {

    // Allocation mémoire pour la structure du board
    board game = (board)malloc(sizeof(struct board_s));
    
    //Boucle d'initialisation du board à NONE
    for(int i=0; i<DIMENSION; i++){
        for(int j=0; j<DIMENSION; j++){
            game->grid[i][j] = NONE;
        }
    }

    // Initialisation des compteurs de setup à 0
    for(int p=0; p<=NB_PLAYERS; p++){
        for(int s=0; s<=NB_SIZE; s++){
            game->setup_counts[p][s] = 0;
        }
    }

    game->winner = NO_PLAYER;
    game->current_player = NO_PLAYER;
    game->picked_piece = NONE;
    game->p_line = -1;
    game->p_col = -1;
    game->history_index = 0;

    return game;
}

// Fonction pour copier l'état actuel du jeu
board copy_game(board original_game) {
    board copy = new_game();
    *copy = *original_game; 
    return copy;
}

// Fonction pour libérer la mémoire allouée au jeu
void destroy_game(board game) {
    if (game != NULL) {
        free(game);
    }
}

// Fonction pour la gestion de tours passage aux joueurs suivants
player next_player(player current_player) {
    if (current_player == SOUTH_P)
    {
        return NORTH_P;
    } 
    return SOUTH_P;
}

// fonction qui vérifie si une coordonnée rentrée est dans le plateau
bool is_inside(int line, int col) {
    return (line >= 0 && line < DIMENSION && col >= 0 && col < DIMENSION);
}

size get_piece_size(board game, int line, int column) {

    //Si les coordonnées rentrées ne sont pas dans le plateau on retourne NONE
    if (!is_inside(line, column)) 
    {
        return NONE;
    }
    
    // Si la pièce est en main, on la retourne
    if (game->picked_piece != NONE && game->p_line == line && game->p_col == column) {
        return game->picked_piece;
    }

    // Sinon, on retourne la pièce dans le board
    return game->grid[line][column];
}


// Fonction pour obtenir le gagnant
player get_winner(board game) {
    return game->winner;
}

// Trouve la ligne la plus au sud qui contient une pièce
int southmost_occupied_line(board game) {

    // Parcours du plateau de haut en bas
    for (int l = 0; l < DIMENSION; l++) {
        for (int c = 0; c < DIMENSION; c++) {
            if (game->grid[l][c] != NONE) 
            {
                return l;
            }
        }
    }
    return -1;
}

// Trouve la ligne la plus au nord qui contient une pièce
int northmost_occupied_line(board game) {

    // Parcours du plateau de bas en haut
    for (int l = DIMENSION - 1; l >= 0; l--) {
        for (int c = 0; c < DIMENSION; c++) {
            if (game->grid[l][c] != NONE) 
            {
                return l;
            }
        }
    }
    return -1;
}

// Fonction qui retourne le joueur propriétaire de la pièce en main
player picked_piece_owner(board game) {
    if (game->picked_piece == NONE) 
    {
        return NO_PLAYER;
    }
    return game->current_player;
}

// Fonction qui retourne la taille de la pièce en main
size picked_piece_size(board game) {
    return game->picked_piece;
}

// Fonction qui retourne la ligne de la pièce en main
int picked_piece_line(board game) {
    return game->p_line;
}

// Fonction qui retourne la colonne de la pièce en main
int picked_piece_column(board game) {
    return game->p_col;
}

// Fonction qui retourne le nombre de mouvements restants pour la pièce en main
int movement_left(board game) {
    if (game->picked_piece == NONE) 
    {
        return -1;
    }
    return game->moves_remaining;
}

//Fonction qui retourne le nombre de pièces disponibles pour un joueur et une taille donnée
int nb_pieces_available(board game, size piece, player player) {
    if (piece < ONE || piece > THREE) 
    {
        return -1;
    }
    return NB_INITIAL_PIECES - game->setup_counts[player][piece];
}

//Fonction pour placer une pièce sur le plateau
return_code place_piece(board game, size piece, player player, int column) {

    //Si les pièce sont inférieures à ONE ou supérieures à THREE ou si la colonne n'est pas dans le plateau on retourne PARAM
    if (piece < ONE || piece > THREE || !is_inside(0, column)) 
    {
        return PARAM;
    }
    
    //Si le joueur n'a plus de pièces de cette taille on retourne FORBIDDEN
    if (nb_pieces_available(game, piece, player) <= 0) 
    {
        return FORBIDDEN;
    }

    
    int line = (player == SOUTH_P) ? 0 : DIMENSION - 1;

    if (game->grid[line][column] != NONE) return EMPTY;

    game->grid[line][column] = piece;
    game->setup_counts[player][piece]++;
    
    return OK;
}

return_code pick_piece(board game, player current_player, int line, int column) {
    //si c'est pas dans la grille
    if (!is_inside(line, column))
    {
        return PARAM;
    } 
    //si il y a déjà un gagnant
    if (game->winner != NO_PLAYER)
    {
        return FORBIDDEN;
    } 
    //si la case est vide
    if (game->grid[line][column] == NONE)
    {
        return EMPTY;
    } 
    //si le joueur est SUD mais que la ligne n'est pas celle la plus au sud
    if (current_player == SOUTH_P && line != southmost_occupied_line(game)) 
    {
        return FORBIDDEN;     
    } 
    //si le joueur est NORD mais que la ligne n'est pas celle la plus au nord
    if (current_player == NORTH_P && line != northmost_occupied_line(game))
    {
        return FORBIDDEN;
    }

    //on actualise les informations du jeu
    game->current_player = current_player;
    game->picked_piece = game->grid[line][column];
    game->p_line = line;
    game->p_col = column;
    game->moves_remaining = game->picked_piece;
    
    //la case devient vide
    game->grid[line][column] = NONE;

    //pour annuler les mouvements
    game->start_line = line;
    game->start_col = column;
    game->history_index = 0;

    return OK;
}

bool is_move_possible(board game, direction direction) {
    if (game->picked_piece == NONE) return false;

    //les coordonnées
    int target_l = game->p_line;
    int target_c = game->p_col;

    switch(direction) {
        case NORTH:
            target_l++;
            break;
        case SOUTH:
            target_l--;
            break;
        case EAST:
            target_c++;
            break;
        case WEST:
            target_c--;
            break;
        case GOAL:
            if (game->current_player == SOUTH_P && game->p_line == DIMENSION - 1)
            {
                return true;
            }
            if (game->current_player == NORTH_P && game->p_line == 0)
            {
                return true;
            }    
            return false;
    }

    //si c'est pas dans la grille -> false
    if (!is_inside(target_l, target_c)) return false;

    //si on doit rebondir alors on doit rebondir sur une case vide
    if (game->moves_remaining == 0 && game->grid[game->p_line][game->p_col] != NONE) 
    {
        if (game->grid[target_l][target_c] != NONE) return false;
        return true;
    }

    //si on essaye de rebondir mais que ce n'est pas notre dernier déplacements -> false
    if (game->grid[target_l][target_c] != NONE) 
    {
        if (game->moves_remaining != 1) return false;
    }

    return true;
}

return_code move_piece(board game, direction direction) {
    if (game->picked_piece == NONE)
    {
        return EMPTY;
    } 

    if (!is_move_possible(game, direction))
    {
        return FORBIDDEN;
    } 

    if (direction == GOAL) 
    {
        game->winner = game->current_player;
        game->picked_piece = NONE;
        game->moves_remaining = 0;
        return OK;
    }

    int new_l = game->p_line;
    int new_c = game->p_col;
    if (direction == NORTH) new_l++;
    if (direction == SOUTH) new_l--;
    if (direction == EAST)  new_c++;
    if (direction == WEST)  new_c--;

    //on actualise les données de l'historique des coups
    game->history[game->history_index].old_line = game->p_line;
    game->history[game->history_index].old_col = game->p_col;
    game->history[game->history_index].moves_at_step = game->moves_remaining;
    game->history_index++;

    //si il y a un rebond on ajoute le nombre de coup en fonction de la valeur de la case
    if (game->moves_remaining == 0 && game->grid[game->p_line][game->p_col] != NONE) 
    {
        int bounce_size = game->grid[game->p_line][game->p_col];
        game->moves_remaining = bounce_size;
    }

    //on actualise les coordonnées de la pièce dans le jeu
    game->p_line = new_l;
    game->p_col = new_c;
    game->moves_remaining--;
    
    
    if (game->grid[new_l][new_c] != NONE)
    {
        //Ne se passe rien pour conserver la pièce en main et permettre le rebond ou swap
    }
    else 
    {
        if (game->moves_remaining == 0) 
        {
            game->grid[new_l][new_c] = game->picked_piece;
            game->picked_piece = NONE;
        }
    }

    return OK;
}

return_code swap_piece(board game, int target_line, int target_column) {
    if (game->picked_piece == NONE)
    {
        return EMPTY;
    } 

    if (game->grid[game->p_line][game->p_col] == NONE)
    {
        return EMPTY;
    } 

    if (!is_inside(target_line, target_column))
    {
        return PARAM;
    }

    if (game->grid[target_line][target_column] != NONE)
    {
        return FORBIDDEN;
    } 

    //on déplace la pièce aux coordonnées choisies
    size piece_under = game->grid[game->p_line][game->p_col];
    game->grid[target_line][target_column] = piece_under;

    //on pose la pièce aux coordonnées de la pièce qui a été déplacer
    game->grid[game->p_line][game->p_col] = game->picked_piece;

    game->picked_piece = NONE;
    game->moves_remaining = 0;

    return OK;
}

return_code cancel_movement(board game) {
    if (game->picked_piece == NONE)
    {
        return EMPTY;
    }   

    //on remet la pièce à sa place initial
    game->grid[game->start_line][game->start_col] = game->picked_piece;
    
    //on réinitialise les données du jeu
    game->picked_piece = NONE;
    game->current_player = NO_PLAYER;
    game->p_line = -1;
    game->p_col = -1;
    game->moves_remaining = 0;
    
    return OK;   
}

return_code cancel_step(board game) {
    if (game->picked_piece == NONE)
    {
        return EMPTY;
    }
    
    if (game->history_index == 0) 
    {
        return cancel_movement(game);
    }
    
    //on prend les informations du dernier mouvement
    game->history_index--;
    step_history last = game->history[game->history_index];

    //on modifie les données de la pièce
    game->p_line = last.old_line;
    game->p_col = last.old_col;
    game->moves_remaining = last.moves_at_step;

    return OK;
}