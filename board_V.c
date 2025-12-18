#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"

/**
 * \file board.c
 * \brief Source code associated with \ref board.h
 * \author Gemini AI (based on spec by Paul Dorbec)
 */

// Structure interne pour l'historique (Undo)
typedef struct {
    int line;
    int col;
    int moves_left;
} move_step;

/**
 * @brief The board of the game.
 */
struct board_s {
    size grid[DIMENSION][DIMENSION];
    int placed_pieces[NB_PLAYERS + 1][NB_SIZE + 1];

    player current_player;      
    bool is_picked;             
    
    size picked_size;           
    int picked_line;            
    int picked_col;             
    int moves_remaining;        
    
    int origin_line;            
    int origin_col;

    move_step history[50];      
    int history_count;          

    player winner;              
};

// --- Helpers ---

static bool is_valid_coord(int line, int col) {
    return (line >= 0 && line < DIMENSION && col >= 0 && col < DIMENSION);
}

static bool piece_can_move(board game, int line, int col, player p) {
    size s = game->grid[line][col];
    if (s == NONE) return false;

    int dl[] = {1, -1, 0, 0};
    int dc[] = {0, 0, 1, -1};

    for (int i = 0; i < 4; i++) {
        int nl = line + dl[i];
        int nc = col + dc[i];

        if (is_valid_coord(nl, nc)) {
            if (game->grid[nl][nc] == NONE) return true;
            if (game->grid[nl][nc] != NONE && s == ONE) return true;
        }
    }

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
    if (!game->is_picked) return NO_PLAYER;
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

    game->grid[line][column] = NONE; 

    return OK;
}

bool is_move_possible(board game, direction dir) {
    if (!game->is_picked) return false;

    // --- REBOND ---
    bool is_bouncing = (game->moves_remaining == 0 && 
                        game->grid[game->picked_line][game->picked_col] != NONE);

    if (game->moves_remaining == 0 && !is_bouncing) {
        return false; 
    }

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

    if (!is_valid_coord(next_l, next_c)) return false;

    if (game->grid[next_l][next_c] == NONE) {
        return true;
    }
    
    int moves_check = game->moves_remaining;
    if (is_bouncing) {
        moves_check = (int)game->grid[game->picked_line][game->picked_col];
    }

    if (moves_check == 1) {
        return true;
    }

    return false;
}

return_code move_piece(board game, direction dir) {
    if (!game->is_picked) return EMPTY;
    
    if (!is_move_possible(game, dir)) return FORBIDDEN;

    if (dir == GOAL) {
        game->winner = game->current_player;
        game->is_picked = false;
        game->current_player = NO_PLAYER; 
        return OK;
    }

    // --- GESTION DU REBOND (MODIFIÉ) ---
    // Si on a 0 mouvement et qu'on est sur une pièce :
    // On RECHARGE, mais on ne BOUGE PAS.
    // "La pièce reste et je récupère les mouvements"
    if (game->moves_remaining == 0 && game->grid[game->picked_line][game->picked_col] != NONE) {
        size size_under = game->grid[game->picked_line][game->picked_col];
        game->moves_remaining = (int)size_under;
        
        // On retourne OK tout de suite. Le mouvement n'est PAS effectué.
        // La boucle de jeu va se relancer, afficher le nouveau nombre de mouvements,
        // et le joueur pourra choisir sa direction.
        return OK; 
    }
    // -----------------------------------

    int next_l = game->picked_line;
    int next_c = game->picked_col;

    switch (dir) {
        case NORTH: next_l++; break;
        case SOUTH: next_l--; break;
        case EAST:  next_c++; break;
        case WEST:  next_c--; break;
        default: return PARAM;
    }

    game->history[game->history_count].line = game->picked_line;
    game->history[game->history_count].col = game->picked_col;
    game->history[game->history_count].moves_left = game->moves_remaining;
    game->history_count++;

    game->picked_line = next_l;
    game->picked_col = next_c;
    game->moves_remaining--; 

    // Atterrissage
    if (game->grid[next_l][next_c] == NONE) {
        if (game->moves_remaining == 0) {
            // Fin de mouvement normale
            game->grid[next_l][next_c] = game->picked_size;
            game->is_picked = false;
            game->current_player = NO_PLAYER;
            game->picked_size = NONE;
            game->history_count = 0;
        }
    } 
    // Sinon (atterrissage sur pièce) : on laisse is_picked à true, moves à 0.
    // Le main détectera le rebond au prochain tour.

    return OK;
}

return_code swap_piece(board game, int target_line, int target_column) {
    if (!game->is_picked) return EMPTY;
    
    if (game->grid[game->picked_line][game->picked_col] == NONE) return FORBIDDEN;
    
    if (!is_valid_coord(target_line, target_column)) return PARAM;
    if (game->grid[target_line][target_column] != NONE) return FORBIDDEN;

    size piece_in_hand = game->picked_size;
    size piece_on_board = game->grid[game->picked_line][game->picked_col];

    game->grid[game->picked_line][game->picked_col] = piece_in_hand;
    game->grid[target_line][target_column] = piece_on_board;

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