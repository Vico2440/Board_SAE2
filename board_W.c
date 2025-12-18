#include <stdio.h>
#include <stdlib.h>
#include "board.h"


// le plateau de jeu
struct board_s {
    
    // la grille de jeu
    size grid[DIMENSION][DIMENSION];

    // le joeur actuelle
    player current_player;

    // 3. État de la phase de mise en place (Setup)
    // Il faut savoir combien de pièces chaque joueur a déjà posées pour respecter la limite
    int pieces_placed[NB_PLAYERS + 1][NB_SIZE + 1]; // +1 pour gérer les indices facilement

    // 4. État du mouvement en cours (très important !)
    // Quand un joueur fait 'pick_piece', le jeu est dans un état intermédiaire.
    // Il faut stocker :
    bool is_piece_picked;       // Est-ce qu'une pièce est "en main" ?
    int picked_line;            // Coordonnée ligne de la pièce choisie
    int picked_column;          // Coordonnée colonne de la pièce choisie
    int moves_remaining;        // Combien de pas il reste à faire (pour les rebonds)
    player winner = NO_PLAYER;
};


player next_player(player current_player){
    if (current_player == SOUTH_P){
        board_s->current_player = NORTH_P;
    }
    elif (current_player == NORTH_P){
        board_s->current_player = SOUTH_P;
    } 
}

board new_game(){
    board game;

    board game = malloc(sizeof(struct board_s));
    
    for(int i = 0; i < DIMENSION; i++){
        for(int j = 0; j < DIMENSION; j++){
            game->grid[i][j] = NONE;
        }
    }

    for(int i = 0; i < NB_PLAYERS + 1; i++){
        for(int j = 0; j < NB_SIZE + 1; j++){
            game->pieces_placed[i][j] = 0;
        }
    }
    
    game->current_player = SOUTH_P;

    // est-ce qu'il faut les initialiser?
    game->is_piece_picked = false ;       // Est-ce qu'une pièce est "en main" ?
    game->picked_line;            // Coordonnée ligne de la pièce choisie
    game->picked_column;          // Coordonnée colonne de la pièce choisie
    game->moves_remaining = 0;        // Combien de pas il reste à faire (pour les rebonds)
    game->winner = NO_PLAYER;

}

void destroy_game(board game){
    free(game);
}


board copy_game(board original_game){
    board board_copy = new_game();

    for(int i = 0; i < DIMENSION; i++){
        for(int j = 0; j < DIMENSION; j++){
            board_copy->grid[i][j] = original_game->grid[i][j];
        }
    }

    // autre chose à faire je pense

    return board_copy;
}


size get_piece_size(board game, int line, int column){
    return game->grid[line][column];
}

player get_winner(board game){
    switch(game->current_player){
        case NO_PLAYER:
            return NO_PLAYER;
        case NORTH_P:
            return NORTH_P;
        case SOUTH_P:
            return SOUTH_P;        
    }
}


int southmost_occupied_line(board game){
    for(int i = DIMENSION; i > 0; i--){
        for(int j = 0; j < DIMENSION; j++){
            if (game->grid[i][j] != NONE){
                return i;
            }
        }
    }
    return -1;
}

int northmost_occupied_line(board game){
    for(int i = 0; i < DIMENSION; i++){
        for(int j = 0; j < DIMENSION; j++){
            if (game->grid[i][j] != NONE){
                return i;
            }
        }
    }
    return -1;
}

