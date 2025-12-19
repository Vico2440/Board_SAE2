#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "board.h"


// Structure pour sauvegarder un pas (pour le Undo/Cancel)
typedef struct {
    int old_line;
    int old_col;
    int moves_at_step;
} step_history;

struct board_s {
    // La grille du jeu (contient les tailles des pièces posées)
    size grid[DIMENSION][DIMENSION];

    // Compteurs pour la phase de placement [JOUEUR][TAILLE]
    // On utilise NB_PLAYERS+1 et NB_SIZE+1 pour utiliser les indices 1,2,3 directement
    int setup_counts[NB_PLAYERS + 1][NB_SIZE + 1];

    // État de la partie
    player winner;

    // État de la pièce actuellement en main ("picked")
    player current_player;   // Qui tient la pièce ?
    size picked_piece;       // Quelle est la taille de la pièce en main ? (NONE si aucune)
    int p_line;              // Coordonnée Ligne actuelle de la pièce en main
    int p_col;               // Coordonnée Colonne actuelle de la pièce en main
    int moves_remaining;     // Combien de mouvements il reste
    
    // Sauvegarde pour remettre la pièce si on fait "Cancel Movement"
    int start_line;
    int start_col;

    // Historique pour "Cancel Step" (Annuler le dernier pas)
    step_history history[50]; // 50 est suffisant pour un tour
    int history_index;        // Combien de pas on a fait
};

// --- FONCTIONS BASIQUES & GESTION MÉMOIRE ---

board new_game() {
    // Allocation de la mémoire pour la structure
    board game = (board)malloc(sizeof(struct board_s));
    
    // Initialisation de la grille à vide
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

    // Initialisation des états
    game->winner = NO_PLAYER;
    game->current_player = NO_PLAYER;
    game->picked_piece = NONE;
    game->p_line = -1;
    game->p_col = -1;
    game->history_index = 0;

    return game;
}

board copy_game(board original_game) {
    board copy = new_game();
    // On copie tout le contenu de la mémoire de l'original vers la copie
    *copy = *original_game; 
    return copy;
}

void destroy_game(board game) {
    if (game != NULL) {
        free(game);
    }
}

// --- UTILITAIRES ---

player next_player(player current_player) {
    if (current_player == SOUTH_P) return NORTH_P;
    return SOUTH_P;
}

// Vérifie si une coordonnée est dans le plateau
bool is_inside(int line, int col) {
    return (line >= 0 && line < DIMENSION && col >= 0 && col < DIMENSION);
}

// --- GETTERS (Récupération d'infos) ---

size get_piece_size(board game, int line, int column) {
    if (!is_inside(line, column)) return NONE;
    
    // Si la pièce est "en main" et se trouve sur cette case, on la retourne
    // C'est important pour l'affichage pendant le déplacement
    if (game->picked_piece != NONE && game->p_line == line && game->p_col == column) {
        // Attention : s'il y a AUSSI une pièce en dessous (chevauchement),
        // on retourne celle en main car elle est au dessus.
        return game->picked_piece;
    }

    return game->grid[line][column];
}

player get_winner(board game) {
    return game->winner;
}

// Trouve la ligne la plus au sud (petit numéro) qui contient une pièce
int southmost_occupied_line(board game) {
    for (int l = 0; l < DIMENSION; l++) {
        for (int c = 0; c < DIMENSION; c++) {
            if (game->grid[l][c] != NONE) return l;
        }
    }
    return -1; // Plateau vide
}

// Trouve la ligne la plus au nord (grand numéro) qui contient une pièce
int northmost_occupied_line(board game) {
    for (int l = DIMENSION - 1; l >= 0; l--) {
        for (int c = 0; c < DIMENSION; c++) {
            if (game->grid[l][c] != NONE) return l;
        }
    }
    return -1; // Plateau vide
}

// --- INFOS SUR LA PIÈCE EN MAIN ---

player picked_piece_owner(board game) {
    if (game->picked_piece == NONE) return NO_PLAYER;
    return game->current_player;
}

size picked_piece_size(board game) {
    return game->picked_piece;
}

int picked_piece_line(board game) {
    return game->p_line;
}

int picked_piece_column(board game) {
    return game->p_col;
}

int movement_left(board game) {
    if (game->picked_piece == NONE) return -1;
    // Retourne le nombre de mouvements. 
    // Si c'est 0 et qu'on est sur une pièce, le main déclenchera le menu Swap/Rebond.
    return game->moves_remaining;
}

// --- SETUP (Mise en place) ---

int nb_pieces_available(board game, size piece, player player) {
    if (piece < ONE || piece > THREE) return -1;
    // On a le droit à NB_INITIAL_PIECES (2) moins ce qu'on a déjà posé
    return NB_INITIAL_PIECES - game->setup_counts[player][piece];
}

return_code place_piece(board game, size piece, player player, int column) {
    // 1. Vérif paramètres
    if (piece < ONE || piece > THREE || !is_inside(0, column)) return PARAM;
    
    // 2. Vérif disponibilité
    if (nb_pieces_available(game, piece, player) <= 0) return FORBIDDEN;

    // 3. Déterminer la ligne (0 pour SUD, 5 pour NORD)
    int line = (player == SOUTH_P) ? 0 : DIMENSION - 1;

    // 4. Vérif case vide
    if (game->grid[line][column] != NONE) return EMPTY; // EMPTY ici veut dire "Erreur, pas vide" selon l'enum

    // 5. Action
    game->grid[line][column] = piece;
    game->setup_counts[player][piece]++;
    
    return OK;
}

// --- JEU (Mouvement) ---

return_code pick_piece(board game, player current_player, int line, int column) {
    // Vérifications de base
    if (!is_inside(line, column)) return PARAM;
    if (game->winner != NO_PLAYER) return FORBIDDEN;
    if (game->grid[line][column] == NONE) return EMPTY; // Rien à ramasser
    
    // Règle : on doit prendre sur la ligne la plus proche de son camp
    // SUD commence ligne 0, donc il doit jouer sa ligne la plus basse (southmost)
    // NORD commence ligne 5, donc il doit jouer sa ligne la plus haute (northmost)
    if (current_player == SOUTH_P) {
        if (line != southmost_occupied_line(game)) return FORBIDDEN;
    } else {
        if (line != northmost_occupied_line(game)) return FORBIDDEN;
    }

    // Tout est bon, on ramasse la pièce
    game->current_player = current_player;
    game->picked_piece = game->grid[line][column]; // On note la taille
    game->p_line = line;
    game->p_col = column;
    game->moves_remaining = game->picked_piece; // Mouvements = taille
    
    // On retire la pièce de la grille "physique" (elle est dans la main maintenant)
    game->grid[line][column] = NONE;

    // On sauvegarde pour le Cancel
    game->start_line = line;
    game->start_col = column;
    game->history_index = 0;

    return OK;
}

bool is_move_possible(board game, direction direction) {
    if (game->picked_piece == NONE) return false;

    // Calcul de la case cible
    int target_l = game->p_line;
    int target_c = game->p_col;

    // Application de la direction
    switch(direction) {
        case NORTH: target_l++; break;
        case SOUTH: target_l--; break;
        case EAST:  target_c++; break;
        case WEST:  target_c--; break;
        case GOAL:
            if (game->current_player == SOUTH_P && game->p_line == DIMENSION - 1) return true;
            if (game->current_player == NORTH_P && game->p_line == 0) return true;
            return false;
    }

    // Vérif hors plateau
    if (!is_inside(target_l, target_c)) return false;

    // --- CORRECTION ICI ---
    // Cas spécial : REBOND
    // Si moves == 0 ET qu'on est sur une pièce, on a le droit de bouger (c'est le rebond)
    if (game->moves_remaining == 0 && game->grid[game->p_line][game->p_col] != NONE) {
        // Pour le rebond, la case cible DOIT être vide (sauf si c'est la fin du rebond, mais simplifions)
        // La règle dit "toward an empty square" pour les pas intermédiaires.
        if (game->grid[target_l][target_c] != NONE) return false;
        return true;
    }

    // Cas normal
    // On ne peut aller sur une case occupée QUE si c'est le DERNIER mouvement
    if (game->grid[target_l][target_c] != NONE) {
        if (game->moves_remaining != 1) return false;
    }

    return true;
}

return_code move_piece(board game, direction direction) {
    if (game->picked_piece == NONE) return EMPTY;
    if (!is_move_possible(game, direction)) return FORBIDDEN;

    // Gestion du BUT
    if (direction == GOAL) {
        game->winner = game->current_player;
        game->picked_piece = NONE;
        game->moves_remaining = 0;
        return OK;
    }

    // Calcul coordonnées futures
    int new_l = game->p_line;
    int new_c = game->p_col;
    if (direction == NORTH) new_l++;
    if (direction == SOUTH) new_l--;
    if (direction == EAST)  new_c++;
    if (direction == WEST)  new_c--;

    // Sauvegarde Historique (inchangé)
    game->history[game->history_index].old_line = game->p_line;
    game->history[game->history_index].old_col = game->p_col;
    game->history[game->history_index].moves_at_step = game->moves_remaining;
    game->history_index++;

    // --- LOGIQUE DE REBOND CORRIGÉE ---
    
    // CAS 1 : On démarre un rebond (On était à 0 sur une pièce et le joueur a choisi 'r')
    if (game->moves_remaining == 0 && game->grid[game->p_line][game->p_col] != NONE) {
        // On récupère la taille de la pièce sous nos pieds
        int bounce_size = game->grid[game->p_line][game->p_col];
        // On crédite les points (la taille de la pièce dessous)
        game->moves_remaining = bounce_size;
    }

    // On effectue le mouvement
    game->p_line = new_l;
    game->p_col = new_c;
    game->moves_remaining--; // On consomme 1 point pour ce pas

    // CAS 2 : On atterrit sur une pièce (fin de mouvement normal)
    if (game->grid[new_l][new_c] != NONE) {
        // La règle dit qu'on ne peut arriver sur une pièce qu'au dernier pas.
        // Donc ici moves_remaining vaut forcément 0.
        // On ne fait rien de spécial, on laisse moves_remaining à 0.
        // C'est ce 0 qui dira au main : "Hey, demande au joueur s'il veut Swap ou Rebondir".
    }
    else {
        // CAS 3 : On atterrit sur une case vide et on a fini (moves == 0)
        if (game->moves_remaining == 0) {
            // On pose la pièce définitivement
            game->grid[new_l][new_c] = game->picked_piece;
            game->picked_piece = NONE;
        }
    }

    return OK;
}

return_code swap_piece(board game, int target_line, int target_column) {
    if (game->picked_piece == NONE) return EMPTY;
    // Le swap n'est possible que si on est sur une autre pièce (overlap)
    // C'est à dire : case grille occupée ET on est dessus
    if (game->grid[game->p_line][game->p_col] == NONE) return EMPTY;

    // Vérif validité cible
    if (!is_inside(target_line, target_column)) return PARAM;
    if (game->grid[target_line][target_column] != NONE) return FORBIDDEN; // Cible doit être vide

    // Exécution du SWAP
    // 1. La pièce du dessous (actuellement dans grid) va vers la cible
    size piece_under = game->grid[game->p_line][game->p_col];
    game->grid[target_line][target_column] = piece_under;

    // 2. Notre pièce (picked) prend la place dans la grille
    game->grid[game->p_line][game->p_col] = game->picked_piece;

    // 3. Fin du tour
    game->picked_piece = NONE;
    game->moves_remaining = 0;

    return OK;
}

return_code cancel_movement(board game) {
    if (game->picked_piece == NONE) return EMPTY;

    // On remet la pièce au départ
    game->grid[game->start_line][game->start_col] = game->picked_piece;
    
    // On reset l'état "main"
    game->picked_piece = NONE;
    game->current_player = NO_PLAYER;
    game->p_line = -1;
    game->p_col = -1;
    game->moves_remaining = 0;
    
    return OK;
}

return_code cancel_step(board game) {
    if (game->picked_piece == NONE) return EMPTY;
    
    // S'il n'y a pas d'historique (on vient juste de pick), on annule tout
    if (game->history_index == 0) {
        return cancel_movement(game);
    }

    // On récupère le dernier état
    game->history_index--;
    step_history last = game->history[game->history_index];

    // On restaure la position
    game->p_line = last.old_line;
    game->p_col = last.old_col;
    game->moves_remaining = last.moves_at_step;

    return OK;
}