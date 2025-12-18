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
    // Grille du jeu : stocke la taille de la pièce (NONE, ONE, TWO, THREE)
    size grid[DIMENSION][DIMENSION];

    // -- Gestion de la configuration (Setup) --
    // Compte combien de pièces de chaque taille ont été placées
    // Index 0 inutilisé (NONE), index 1=ONE, 2=TWO, 3=THREE
    int placed_pieces[NB_PLAYERS + 1][NB_SIZE + 1];

    // -- État du tour courant --
    player current_player;      // Qui est en train de manipuler une pièce
    bool is_picked;             // Est-ce qu'une pièce est "en main" ?
    
    // Information sur la pièce "en main" (qui vole au-dessus du plateau)
    size picked_size;           // Taille de la pièce en main
    int picked_line;            // Position actuelle (ligne)
    int picked_col;             // Position actuelle (colonne)
    int moves_remaining;        // Mouvements restants
    
    // Information pour annuler tout le mouvement (cancel_movement)
    int origin_line;            // D'où la pièce vient-elle ce tour-ci ?
    int origin_col;

    // -- Historique pour cancel_step --
    move_step history[50];      // Pile pour stocker les étapes
    int history_count;          // Nombre d'étapes dans la pile

    // -- État global --
    player winner;              // Si quelqu'un a gagné
};

/**
 * @brief Helper: Vérifie si une coordonnée est dans le plateau
 */
static bool is_valid_coord(int line, int col) {
    return (line >= 0 && line < DIMENSION && col >= 0 && col < DIMENSION);
}

board new_game() {
    board new_board = malloc(sizeof(struct board_s));
    if (new_board == NULL) return NULL;

    // Initialisation de la grille à vide
    for (int i = 0; i < DIMENSION; i++) {
        for (int j = 0; j < DIMENSION; j++) {
            new_board->grid[i][j] = NONE;
        }
    }

    // Initialisation des compteurs de setup à 0
    memset(new_board->placed_pieces, 0, sizeof(new_board->placed_pieces));

    // Initialisation des états
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
        // Copie superficielle de la structure (suffisant car pas de pointeurs internes dynamiques)
        *new_board = *original_game;
    }
    return new_board;
}

void destroy_game(board game) {
    if (game != NULL) {
        free(game);
    }
}

// --- Getters et Infos ---

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
    // On cherche la ligne la plus petite (sud) contenant au moins une pièce
    for (int l = 0; l < DIMENSION; l++) {
        for (int c = 0; c < DIMENSION; c++) {
            if (game->grid[l][c] != NONE) return l;
        }
    }
    return -1; // Vide
}

int northmost_occupied_line(board game) {
    // On cherche la ligne la plus grande (nord) contenant au moins une pièce
    for (int l = DIMENSION - 1; l >= 0; l--) {
        for (int c = 0; c < DIMENSION; c++) {
            if (game->grid[l][c] != NONE) return l;
        }
    }
    return -1; // Vide
}

player picked_piece_owner(board game) {
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

// --- Setup Phase ---

int nb_pieces_available(board game, size piece, player player) {
    if (piece < ONE || piece > THREE) return -1;
    if (player != NORTH_P && player != SOUTH_P) return -1;

    // NB_INITIAL_PIECES est défini à 2 dans board.h
    return NB_INITIAL_PIECES - game->placed_pieces[player][piece];
}

return_code place_piece(board game, size piece, player p, int column) {
    if (game == NULL) return PARAM;
    if (piece < ONE || piece > THREE) return PARAM;
    if (p != NORTH_P && p != SOUTH_P) return PARAM;
    if (!is_valid_coord(0, column)) return PARAM;

    // Vérifier si le joueur a encore des pièces de ce type
    if (nb_pieces_available(game, piece, p) <= 0) return FORBIDDEN;

    // Déterminer la ligne : SUD joue en ligne 0, NORD en ligne DIMENSION-1
    int line = (p == SOUTH_P) ? 0 : DIMENSION - 1;

    // Vérifier si la case est vide
    if (game->grid[line][column] != NONE) return EMPTY; // EMPTY ici signifie "pas vide" selon la doc return_code... un peu contre-intuitif mais on suit la doc ("given space should... be empty")

    // Placer la pièce
    game->grid[line][column] = piece;
    game->placed_pieces[p][piece]++;

    return OK;
}

// --- Game Logic ---

return_code pick_piece(board game, player current_player, int line, int column) {
    if (game->winner != NO_PLAYER) return FORBIDDEN;
    if (!is_valid_coord(line, column)) return PARAM;
    
    // Vérifier s'il y a une pièce
    if (game->grid[line][column] == NONE) return EMPTY;

    // Vérifier si c'est la bonne ligne pour le joueur
    // Le joueur doit jouer la ligne la plus proche de son bord
    if (current_player == SOUTH_P) {
        if (line != southmost_occupied_line(game)) return FORBIDDEN;
    } else if (current_player == NORTH_P) {
        if (line != northmost_occupied_line(game)) return FORBIDDEN;
    } else {
        return PARAM;
    }

    // Tout est bon, on prend la pièce
    game->current_player = current_player;
    game->is_picked = true;
    game->picked_size = game->grid[line][column];
    game->picked_line = line;
    game->picked_col = column;
    game->moves_remaining = (int)game->picked_size; // Taille 1 = 1 mvmt, etc.
    
    // Sauvegarde pour cancel
    game->origin_line = line;
    game->origin_col = column;
    game->history_count = 0; // Reset history

    // On retire la pièce du plateau (elle est "dans la main")
    game->grid[line][column] = NONE;

    return OK;
}

bool is_move_possible(board game, direction dir) {
    if (!game->is_picked) return false;

    int next_l = game->picked_line;
    int next_c = game->picked_col;

    // Calcul de la case cible
    switch (dir) {
        case NORTH: next_l++; break;
        case SOUTH: next_l--; break;
        case EAST:  next_c++; break;
        case WEST:  next_c--; break;
        case GOAL:
            // GOAL valide si NORD est tout au sud (0) ou SUD est tout au nord (5)
            if (game->current_player == SOUTH_P && game->picked_line == DIMENSION - 1) return true;
            if (game->current_player == NORTH_P && game->picked_line == 0) return true;
            return false;
        default: return false;
    }

    // Vérification des limites du plateau (sauf GOAL déjà traité)
    if (!is_valid_coord(next_l, next_c)) return false;

    // Logique de mouvement
    // 1. Case vide : OK
    if (game->grid[next_l][next_c] == NONE) {
        return true;
    }
    
    // 2. Case occupée : Possible SEULEMENT si c'est le dernier mouvement
    // C'est un rebond ou un atterrissage pour swap
    if (game->moves_remaining == 1) {
        return true;
    }

    return false; // Case occupée et il reste trop de mouvements
}

return_code move_piece(board game, direction dir) {
    if (!game->is_picked) return EMPTY;
    
    if (!is_move_possible(game, dir)) return FORBIDDEN;

    // Si c'est GOAL
    if (dir == GOAL) {
        // VICTOIRE !
        game->winner = game->current_player;
        // La pièce quitte le plateau (plus pick, plus sur grid)
        game->is_picked = false;
        game->current_player = NO_PLAYER; 
        return OK;
    }

    int next_l = game->picked_line;
    int next_c = game->picked_col;

    switch (dir) {
        case NORTH: next_l++; break;
        case SOUTH: next_l--; break;
        case EAST:  next_c++; break;
        case WEST:  next_c--; break;
        default: return PARAM;
    }
    
    if (!is_valid_coord(next_l, next_c)) return PARAM;

    // Sauvegarder l'état actuel dans l'historique avant de modifier
    game->history[game->history_count].line = game->picked_line;
    game->history[game->history_count].col = game->picked_col;
    game->history[game->history_count].moves_left = game->moves_remaining;
    game->history_count++;

    // Effectuer le mouvement
    game->picked_line = next_l;
    game->picked_col = next_c;
    game->moves_remaining--;

    // Gestion du Rebond (Bounce)
    // Si on arrive sur une case occupée et que moves == 0 (car on vient de décrémenter)
    if (game->grid[next_l][next_c] != NONE && game->moves_remaining == 0) {
        // Rebond ! On récupère la taille de la pièce en dessous
        size size_under = game->grid[next_l][next_c];
        game->moves_remaining = (int)size_under;
        // La pièce reste "en l'air" au dessus de l'autre
    }

    return OK;
}

return_code swap_piece(board game, int target_line, int target_column) {
    if (!game->is_picked) return EMPTY;
    
    // Le swap n'est possible que si on est "au dessus" d'une pièce
    // C'est-à-dire : la case actuelle contient une pièce ET il nous reste des mouvements (le rebond vient d'être initié)
    // Ou techniquement : on vient de finir un mouvement sur une pièce.
    // D'après move_piece, si on atterrit sur une pièce, moves_remaining est reset à la taille du dessous.
    // Donc la condition est : grid[pos] != NONE.
    
    if (game->grid[game->picked_line][game->picked_col] == NONE) return FORBIDDEN;
    
    // Vérifier la cible du swap (doit être vide et valide)
    if (!is_valid_coord(target_line, target_column)) return PARAM;
    if (game->grid[target_line][target_column] != NONE) return FORBIDDEN;

    // 1. La pièce en main (moving piece) prend la place de la pièce en dessous
    size piece_in_hand = game->picked_size;
    size piece_on_board = game->grid[game->picked_line][game->picked_col];

    game->grid[game->picked_line][game->picked_col] = piece_in_hand;

    // 2. La pièce qui était sur le plateau va à la position cible
    game->grid[target_line][target_column] = piece_on_board;

    // 3. Fin du tour
    game->is_picked = false;
    game->current_player = NO_PLAYER;
    game->history_count = 0; // Reset history

    return OK;
}

return_code cancel_movement(board game) {
    if (!game->is_picked) return EMPTY;

    // Remettre la pièce à l'origine
    game->grid[game->origin_line][game->origin_col] = game->picked_size;

    // Reset état
    game->is_picked = false;
    game->current_player = NO_PLAYER;
    game->picked_size = NONE;
    game->history_count = 0;

    return OK;
}

return_code cancel_step(board game) {
    if (!game->is_picked) return EMPTY;

    // Si pas d'historique, c'est comme annuler tout le mouvement (retour au pick)
    if (game->history_count == 0) {
        return cancel_movement(game);
    }

    // Dépiler le dernier état
    game->history_count--;
    move_step last_step = game->history[game->history_count];

    // Restaurer l'état
    game->picked_line = last_step.line;
    game->picked_col = last_step.col;
    game->moves_remaining = last_step.moves_left;

    return OK;
}