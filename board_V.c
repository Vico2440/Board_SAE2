#include <stdio.h>
#include <stdlib.h>
#include "board.h"

/**
 * \file board.c
 *
 * \brief Source code associated with \ref board.h
 *
 * \author you
 */

/**
 * @brief The board of the game.
 */
struct board_s {
	// TODO: choisir une structure adaptee
};

board new_game(){
	board new_board = malloc(sizeof(struct board_s));
	// TODO preparer le plateau
	return new_board;
}

board copy_game(board original_game){
	board new_board = malloc(sizeof(struct board_s));
	// TODO copier la situation de l'ancien jeu dans le nouveau.
	return new_board;
}

void destroy_game(board game){
	free(game);
};

// TODO ajouter les entêtes des autres fonctions et completer.