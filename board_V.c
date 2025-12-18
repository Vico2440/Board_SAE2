#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"

/**
 * \file board.c
 * \brief Source code associated with \ref board.h
 * \author Gemini AI (based on spec by Paul Dorbec)
 */

// Structure interne pour sauvegarder l'historique d'un tour (pour cancel_step)
typedef struct {
    int line;
    int col;
    int moves_left;
} move_step;

/**
 * @brief The board of the game.
 */
struct board_s {
    size grid[DIMENSION][DIMENSION]; // Grille du jeu
    int placed_pieces[NB_PLAYERS + 1][NB_SIZE + 1]; // Compteur pour le setup

    // État du tour courant
    player current_player;      
    bool is_picked;             
    
    // Information sur la pièce "en main"
    size picked_size;           
    int picked_line;            
    int picked_col;             
    int moves_remaining;        
    
    // Pour cancel_movement
    int origin_line;            
    int origin_col;

    // Pour cancel_step (Undo)
    move_step history[50];      
    int history_count;          

    player winner;              
};

// --- Helpers ---

static bool is_valid_coord(int line, int col) {
    return (line >= 0 && line < DIMENSION && col >= 0 && col < DIMENSION);
}

// Vérifie si une pièce à (line, col) a au moins un mouvement valide
// Utile pour southmost/northmost
static bool piece_can_move(board game, int line, int col, player p) {
    size s = game->grid[line][col];
    if (s == NONE) return false;

    int dl[] = {1, -1, 0, 0};
    int dc[] = {0, 0, 1, -1};

    // Vérifie les 4 directions
    for (int i = 0; i < 4; i++) {
        int nl = line + dl[i];
        int nc = col + dc[i];

        if (is_valid_coord(nl, nc)) {
            // Libre ? OK.
            if (game->grid[nl][nc] == NONE) return true;
            // Occupé ? OK seulement si taille 1 (car rebond direct)
            if (game->grid[nl][nc] != NONE && s == ONE) return true;
        }
    }

    // Cas spécial GOAL
    if (p == SOUTH_P && line == DIMENSION - 1) return true;
    if (p == NORTH_P && line == 0) return true;

    return false;
}

// --- Création / Destruction ---

board new_game() {
    board new_board = malloc(sizeof(struct board_s));
    if (new_board == NULL) return NULL;

    for (int i = 0; i < DIMENSION; i++) {
        for (int j = 0; j < DIMENSION; j++) {
            new_board->grid[i][j] = NONE;
        }
    }
    memset(new_board->placed_pieces, 0, sizeof(new_board->placed_pieces));
    new_board->current_player = NO_PLAYER;
    new_board->is_picked = false;
    new_board->picked_size = NONE;
    new_board->moves_remaining = 0;
    new_board->winner = NO_PLAYER;
    new_board->history_count = 0;

    return new_board;
}

board copy_game(board original_game) {
    if (original_game == NULL) return NULL;
    board new_board = malloc(sizeof(struct board_s));
    if (new_board != NULL) {
        *new_board = *original_game;
    }
    return new_board;
}

void destroy_game(board game) {
    if (game != NULL) {
        free(game);
    }
}

// --- Getters ---

size get_piece_size(board game, int line, int column) {
    if (!is_valid_coord(line, column)) return NONE;
    // Si la case demandée est celle où se trouve virtuellement la pièce en main,
    // on renvoie NONE car la pièce est "en l'air", pas sur la grille.
    // (Sauf si c'est pour l'affichage, mais l'affichage gère ça).
    // Note: Dans cette implémentation, quand on pick, on met grid à NONE.
    return game->grid[line][column];
}

player get_winner(board game) {
    return game->winner;
}

player next_player(player current_player) {
    if (current_player == NORTH_P) return SOUTH_P;
    if (current_player == SOUTH_P) return NORTH_P;
    return NO_PLAYER;
}

int southmost_occupied_line(board game) {
    for (int l = 0; l < DIMENSION; l++) {
        for (int c = 0; c < DIMENSION; c++) {
            if (game->grid[l][c] != NONE && piece_can_move(game, l, c, SOUTH_P)) return l;
        }
    }
    return -1;
}

int northmost_occupied_line(board game) {
    for (int l = DIMENSION - 1; l >= 0; l--) {
        for (int c = 0; c < DIMENSION; c++) {
            if (game->grid[l][c] != NONE && piece_can_move(game, l, c, NORTH_P)) return l;
        }
    }
    return -1;
}

player picked_piece_owner(board game) {
    if (!game->is_picked) return NO_PLAYER; // Important pour arrêter la boucle du main
    return game->current_player;
}

size picked_piece_size(board game) {
    if (!game->is_picked) return NONE;
    return game->picked_size;
}

int picked_piece_line(board game) {
    if (!game->is_picked) return -1;
    return game->picked_line;
}

int picked_piece_column(board game) {
    if (!game->is_picked) return -1;
    return game->picked_col;
}

int movement_left(board game) {
    if (!game->is_picked) return -1;
    return game->moves_remaining;
}

// --- Setup ---

int nb_pieces_available(board game, size piece, player player) {
    if (piece < ONE || piece > THREE) return -1;
    if (player != NORTH_P && player != SOUTH_P) return -1;
    return NB_INITIAL_PIECES - game->placed_pieces[player][piece];
}

return_code place_piece(board game, size piece, player p, int column) {
    if (game == NULL) return PARAM;
    if (piece < ONE || piece > THREE) return PARAM;
    if (p != NORTH_P && p != SOUTH_P) return PARAM;
    if (!is_valid_coord(0, column)) return PARAM;

    if (nb_pieces_available(game, piece, p) <= 0) return FORBIDDEN;

    int line = (p == SOUTH_P) ? 0 : DIMENSION - 1;
    if (game->grid[line][column] != NONE) return EMPTY;

    game->grid[line][column] = piece;
    game->placed_pieces[p][piece]++;

    return OK;
}

// --- Gameplay ---

return_code pick_piece(board game, player current_player, int line, int column) {
    if (game->winner != NO_PLAYER) return FORBIDDEN;
    if (!is_valid_coord(line, column)) return PARAM;
    if (game->grid[line][column] == NONE) return EMPTY;

    // Vérification de la ligne valide (closest row rule)
    if (current_player == SOUTH_P) {
        if (line != southmost_occupied_line(game)) return FORBIDDEN;
    } else if (current_player == NORTH_P) {
        if (line != northmost_occupied_line(game)) return FORBIDDEN;
    } else {
        return PARAM;
    }

    game->current_player = current_player;
    game->is_picked = true;
    game->picked_size = game->grid[line][column];
    game->picked_line = line;
    game->picked_col = column;
    game->moves_remaining = (int)game->picked_size;
    
    game->origin_line = line;
    game->origin_col = column;
    game->history_count = 0;

    game->grid[line][column] = NONE; // On retire la pièce du plateau

    return OK;
}

bool is_move_possible(board game, direction dir) {
    if (!game->is_picked) return false;

    int next_l = game->picked_line;
    int next_c = game->picked_col;

    switch (dir) {
        case NORTH: next_l++; break;
        case SOUTH: next_l--; break;
        case EAST:  next_c++; break;
        case WEST:  next_c--; break;
        case GOAL:
            if (game->current_player == SOUTH_P && game->picked_line == DIMENSION - 1) return true;
            if (game->current_player == NORTH_P && game->picked_line == 0) return true;
            return false;
        default: return false;
    }

    // 1. Check Limites STRICTES
    if (!is_valid_coord(next_l, next_c)) return false;

    // 2. Check Cases Occupées
    // Si la case est vide : OK
    if (game->grid[next_l][next_c] == NONE) {
        return true;
    }
    
    // Si la case est occupée : 
    // Possible SEULEMENT si c'est le tout dernier pas (pour rebondir)
    if (game->moves_remaining == 1) {
        return true;
    }

    // Sinon bloqué
    return false;
}

return_code move_piece(board game, direction dir) {
    if (!game->is_picked) return EMPTY;
    
    // On vérifie d'abord si le mouvement est légal
    if (!is_move_possible(game, dir)) return FORBIDDEN;

    // Gestion du GOAL (Victoire)
    if (dir == GOAL) {
        game->winner = game->current_player;
        game->is_picked = false;
        game->current_player = NO_PLAYER; 
        return OK;
    }

    // Calcul coordonnées
    int next_l = game->picked_line;
    int next_c = game->picked_col;
    switch (dir) {
        case NORTH: next_l++; break;
        case SOUTH: next_l--; break;
        case EAST:  next_c++; break;
        case WEST:  next_c--; break;
        default: return PARAM;
    }

    // Sauvegarde Historique
    game->history[game->history_count].line = game->picked_line;
    game->history[game->history_count].col = game->picked_col;
    game->history[game->history_count].moves_left = game->moves_remaining;
    game->history_count++;

    // Mise à jour position
    game->picked_line = next_l;
    game->picked_col = next_c;
    game->moves_remaining--; // On décrémente

    // --- C'est ici que la magie opère (Correction des bugs) ---

    // Cas 1 : On atterrit sur une case VIDE
    if (game->grid[next_l][next_c] == NONE) {
        if (game->moves_remaining == 0) {
            // FIN DU MOUVEMENT !
            // On pose la pièce définitivement
            game->grid[next_l][next_c] = game->picked_size;
            
            // On n'a plus rien en main
            game->is_picked = false;
            game->current_player = NO_PLAYER;
            game->picked_size = NONE;
            game->history_count = 0; // Reset history pour tour suivant
        }
        // Sinon (moves > 0), on continue de "voler" (is_picked reste true)
    } 
    // Cas 2 : On atterrit sur une case OCCUPÉE (Rebond)
    else {
        // Normalement moves_remaining == 0 ici car is_move_possible l'a vérifié
        // On récupère la taille du dessous pour le rebond
        size size_under = game->grid[next_l][next_c];
        game->moves_remaining = (int)size_under;
        
        // IMPORTANT : is_picked reste TRUE.
        // Le main va détecter moves > 0 ou l'état de rebond et proposer le choix.
    }

    return OK;
}

return_code swap_piece(board game, int target_line, int target_column) {
    if (!game->is_picked) return EMPTY;
    
    // Swap possible seulement si on est "sur" une pièce (donc case occupée)
    if (game->grid[game->picked_line][game->picked_col] == NONE) return FORBIDDEN;
    
    // Cible valide et vide
    if (!is_valid_coord(target_line, target_column)) return PARAM;
    if (game->grid[target_line][target_column] != NONE) return FORBIDDEN;

    size piece_in_hand = game->picked_size;
    size piece_on_board = game->grid[game->picked_line][game->picked_col];

    // La pièce en main prend la place
    game->grid[game->picked_line][game->picked_col] = piece_in_hand;
    // La pièce du dessous va à la cible
    game->grid[target_line][target_column] = piece_on_board;

    // Fin du tour
    game->is_picked = false;
    game->current_player = NO_PLAYER;
    game->history_count = 0;

    return OK;
}

return_code cancel_movement(board game) {
    if (!game->is_picked) return EMPTY;

    game->grid[game->origin_line][game->origin_col] = game->picked_size;

    game->is_picked = false;
    game->current_player = NO_PLAYER;
    game->picked_size = NONE;
    game->history_count = 0;

    return OK;
}

return_code cancel_step(board game) {
    if (!game->is_picked) return EMPTY;

    if (game->history_count == 0) {
        return cancel_movement(game);
    }

    game->history_count--;
    move_step last_step = game->history[game->history_count];

    game->picked_line = last_step.line;
    game->picked_col = last_step.col;
    game->moves_remaining = last_step.moves_left;

    return OK;
}