#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "board.h"


/// @brief Enumération des différents Etats du jeu
typedef enum {
    STATE_SETUP,        // phase de placement des pièces
    STATE_TURN_START,   // début de tour
    STATE_PLAYER_TURN,  // Tours des joueurs (déplacement)
    STATE_END_TURN,     // finir le tour
    STATE_GAME_OVER     // fin du jeu
} GameState;

/// @brief Fonction qui permet d'alterner entre Joueur NORD ET SUD 
/// @param lastPlayer joueur du dernier tour
/// @return joueur du prochain tour
player turn_manager(player lastPlayer, char *name_n, char *name_s)
{
    if (lastPlayer == SOUTH_P)
    {
        printf("Tour de %s (\x1b[34m\x1b[1m NORD \x1b[0m)\n", name_n);
        return NORTH_P;
    }
    else
    {
        printf("Tour de %s (\x1b[31m\x1b[1m SUD \x1b[0m)\n", name_s);
        return SOUTH_P;
    }
}


/// @brief Fonction permettant d'afficher le plateau de jeu et autres information
/// @param game plateau à display
void display_board(board game, char *name_n, char *name_s)
{
    size piece;


    printf("\n");
    printf("         NORD : \x1b[34m%s\x1b[0m       \n", name_n);
    printf("  //  //  // \\\\  \\\\  \\\\\n");
    printf("\x1b[90m╔═══╦═══╦═══╦═══╦═══╦═══╗\x1b[0m\n");

    
    for (int lig = DIMENSION - 1; lig >= 0; lig--) { 
        
        for (int col = 0; col < DIMENSION; col++) {
            piece = get_piece_size(game, lig, col); 
            printf("\x1b[90m║\x1b[0m");

            switch (piece)
            {
                case ONE:
                    printf("\x1b[34m\x1b[1m 1 \x1b[0m");
                    break;
                case TWO:
                    printf("\x1b[32m\x1b[1m 2 \x1b[0m"); 
                    break;
                case THREE:
                    printf("\x1b[31m\x1b[1m 3 \x1b[0m"); 
                    break;
                default:
                    printf("   "); 
                    break;
            }
        }

        printf("\x1b[90m║\x1b[0m %d", lig + 1); 

        //affichage aide menu
        printf("    "); 

        if (lig == DIMENSION - 1) {
            printf("\x1b[1m--- COMMANDES ---\x1b[0m");
        }
        else if (lig == DIMENSION - 2) {
            printf("[N, S, E, O, G] : Directions");
        }
        else if (lig == DIMENSION - 3) {
            printf("[M] : Valider mouvement");
        }
        else if (lig == DIMENSION - 4) {
            printf("[U] : Undo / [C] : Cancel");
        }
        else if (lig == DIMENSION - 6) {
            printf("[s]wap / [r]ebond");
        }

        printf("\n"); 

        if (lig > 0) { 
            printf("\x1b[90m╠═══╬═══╬═══╬═══╬═══╬═══╣\x1b[0m\n");
        }
    }

    printf("\x1b[90m╚═══╩═══╩═══╩═══╩═══╩═══╝\x1b[0m\n");
    printf("  \\\\  \\\\  \\\\ //  //  //\n");
    
    printf("         SUD : \x1b[31m%s\x1b[0m       \n", name_s);
	printf("__________________________________________________ \n");
}

/// @brief reçoie le resultat de pile ou face
/// @param n pile ou face 
/// @return  renvoie le premier joueur de la partie à jouer
int pile_ou_face() 
{
    srand(time(NULL));

    int r = rand() % 2; 

    return r; 
}

player first_player(int n)
{
	if(n == 0)
	{
		return SOUTH_P;
	}

	return NORTH_P;
}

/// @brief Fonction qui permet de savoir si il reste de piece à placé pour un joueur
/// @param p joueur à tester
/// @param game plateau de jeu
/// @return si il reste de pièce à placé ou non
bool reste_des_piece(player p, board game)
{
	if(nb_pieces_available(game, ONE, p) != 0)
	{
		return true;
	}
	if(nb_pieces_available(game, TWO, p) != 0)
	{
		return true;
	}
	if(nb_pieces_available(game, THREE, p) != 0)
	{
		return true;
	}

	return false;
}

/// @brief Fonction qui renvoie une direction en fonction de la lettre tapé
/// @return la direction 
direction saisir_dir()
{
	char c;

	bool isValide = true;
	
	while(isValide)
	{
		printf("Veuillez saisir une direction : ");
		scanf(" %c",&c);

		if(c == 'N' || c == 'S' || c == 'E' || c == 'O' || c == 'G')
		{
			isValide = false;
		}
		else
		{
			printf("NON VALIDE : Veuillez saisir 'N', 'S', 'E' 'O' ou 'G'");
		}	
	}	
	direction dir;
	switch (c)
	{
	case 'N':
		dir = NORTH;
		break;
	case 'S':
		dir = SOUTH;
		break;	
	case 'E':
		dir = EAST;
		break;
	case 'O':
		dir = WEST;
		break;	

	case 'G':
		dir = GOAL;
		break;
	}

	return dir;
}

/// @brief Fonction qui dmemande la taille à saisir 
/// @param game plateau de jeu
/// @param p joueur qui joue 
/// @return taille de la pièce 
int saisir_piece(board game, player p)
{
    int nb = 0;
    bool isValide = true;
    size taille = NONE;
    
    while (isValide)
    {
        int result;

        printf("Veuillez saisir une taille de Pièce (1, 2 ou 3) : ");

        result = scanf("%d", &nb);


        if (result != 1)
        {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);

            printf("Entrée invalide : vous devez taper un nombre.\n");
            continue; 
        }

        if (nb >= 1 && nb <= 3)
        {
            switch (nb)
            {
                case 1: 
				taille = ONE;   
				break;
                case 2: 
				taille = TWO;  
				break;
                case 3: 
				taille = THREE; 
				break;
            }

            if (nb_pieces_available(game, taille, p) != 0)
            {
                isValide = false;
                return nb; 
            }
            else
            {
                printf("Vous n'avez plus de pièce %d !\n", nb);
            }
        }
        else
        {
            printf("NON VALIDE : Veuillez saisir 1, 2, ou 3.\n");
        }
    }

    return nb; 
}

/// @brief Fonction qui permet de saisir coordonnée de la mise en place des pièce en début de jeu
/// @param game plateau de jeu
/// @param message message personnalisé en fonction du joueur
/// @param min Coordonnée minimum accepter
/// @param max Coordonnée max Accepter
/// @param nbVal 
/// @param p Joueur qui joue 
/// @return retour de la colonne ou placé la piece 
int saisir_coord(board game, const char *message, int min, int max, int nbVal, player p)
{
    int nb;          
    int colIndex; 
	int result;   

    int fixedLine = (p == SOUTH_P) ? 0 : DIMENSION - 1;  

    do {
        printf("%s (%d à %d) : ", message, min, max);

		result = scanf("%d", &nb);
		

        if (result != 1)
        {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);

            printf("Entrée invalide : vous devez taper un nombre.\n");
            continue; 
        }
		

        if (nb < min || nb > max) {
            printf("Valeur invalide, recommencez.\n");
            continue;  
        }

        colIndex = nb - 1;

        if (nbVal == 1) {
            if (get_piece_size(game, fixedLine, colIndex) != NONE) {
                printf("Cette case (%d,%d) est déjà occupée, choisissez-en une autre.\n",
                       fixedLine + 1, colIndex + 1);
                nb = -1;
                continue;
            }
        }

        break;

    } while (1);

    return colIndex;
}

/// @brief fonction qui demande de saisir un nombre, utilisé pour saisir coordonnées lors de la phase de jeu
/// @param message message personnalisé pour afficher si colonne ou ligne
/// @return retourne la coordonnée de la ligne ou colonne 
int saisir_nb(const char *message)
{
    int nb;
    int result;

    do {
        printf("%s", message);  

        result = scanf("%d", &nb);

        if (result != 1)
        {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);

            printf("Entrée invalide : vous devez taper un nombre.\n");
            continue;
        }

        if (nb < 1 || nb > 6) {
            printf("Valeur invalide, recommencez.\n");
        }

    } while (nb < 1 || nb > 6);

    return nb - 1;  
}

/// @brief Fonction qui permet de setup les pièces en début de jeu
/// @param game plateau de jeu
/// @param p joueur qui place 
void setup_pieces_game(board game, player p, char *name_n, char *name_s)
{
    bool canPlace = true;
    int column = 0;
    int piece = 0;
    bool restePS = true;
    bool restePN = true;
    
    while(canPlace)
    {
        display_board(game, name_n, name_s); // On passe les noms ici

        restePS = reste_des_piece(SOUTH_P,game);
        restePN = reste_des_piece(NORTH_P,game);

        if(restePS == false && restePN == false)
        {
            canPlace = false;
            break;
        }

        if(p == NORTH_P)
        {
            printf("%s (NORD), numéro de la colonne", name_n); // Petit ajout perso
            column = saisir_coord(game, " : ", 1, DIMENSION, 1, p);
            piece = saisir_piece(game, p);
        }
        else
        {
            printf("%s (SUD), numéro de la colonne", name_s); // Petit ajout perso
            column = saisir_coord(game, " : ", 1, DIMENSION, 1, p);
            piece = saisir_piece(game, p);
        }
        
        place_piece(game, piece, p, column);

        p = turn_manager(p, name_n, name_s); // Et ici
    }   
}

/// @brief Etat de la FSM pour mettre en place le jeu et plateau de jeu
/// @param game plateau de jeu
/// @param current_player joueur qui joue
/// @return prochain etat du jeu
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

/// @brief Etat du tour du joueur pick de pièce, déplacement et tout action
/// @param game plateau de jeu
/// @param current_player joueur qui joue
/// @return prochain etat du jeu
GameState state_player_turn(board game, player *current_player, char *name_n, char *name_s)
{
    int x = 0;
    int y = 0; 
    direction dir;
    return_code rc;

    
    display_board(game, name_n, name_s);

    
    x = saisir_nb("Choisir la ligne de la pièce (1-6) : ");   
    y = saisir_nb("Choisir la colonne de la pièce (1-6) : ");   

    rc = pick_piece(game, *current_player, x, y);
    if (rc != OK) {
        printf("Impossible de prendre cette pièce (code %d)\n", rc);
        return STATE_PLAYER_TURN; 
    }

    
    while (picked_piece_owner(game) == *current_player) {

        int moves = movement_left(game);
        printf("Mouvements restants = %d\n", moves);

        if (moves == -1) {
            
            break;
        }

        
        printf("\nCommandes :\n");
        printf("N / S / O / E : déplacer la pièce\n");
        printf("U : annuler le dernier pas\n");
        printf("C : annuler tout le mouvement\n");
        if (is_move_possible(game, GOAL)) {
            printf("G : aller vers le but\n");
        }

        printf("Votre choix : ");
        char cmd;
        scanf(" %c", &cmd);

        
        if (cmd == 'U') {
            rc = cancel_step(game);
            if (rc != OK) {
                printf("Impossible d'annuler le dernier pas (code %d)\n", rc);
            } else {
                printf("Dernier pas annulé.\n");
                display_board(game, name_n, name_s); 
            }
            continue;
        }

        
        if (cmd == 'C') {
            rc = cancel_movement(game);
            if (rc != OK) {
                printf("Impossible d'annuler le mouvement (code %d)\n", rc);
                continue;
            }
            printf("Mouvement complet annulé. Vous pouvez choisir une autre pièce.\n");
            display_board(game, name_n, name_s); 
            return STATE_PLAYER_TURN;
        }

        if (cmd == 'G') {
            if (!is_move_possible(game, GOAL)) {
                printf("Impossible d'aller au but maintenant.\n");
                continue;
            }

            rc = move_piece(game, GOAL);
            if (rc != OK) {
                printf("Erreur move_piece (code %d)\n", rc);
            }
            display_board(game, name_n, name_s); 
            continue;
        }

        
        if (cmd == 'N' || cmd == 'S' || cmd == 'E' || cmd == 'O') {
            dir = (cmd == 'N') ? NORTH :
                  (cmd == 'S') ? SOUTH :
                  (cmd == 'E') ? EAST  : WEST;

            
            if (moves == 0) {
                char choix;
                printf("La piece est sur une autre piece.\n");
                printf("  r = rebondir (continuer le mouvement)\n");
                printf("  s = swap (échanger)\n");
                printf("Votre choix : ");
                scanf(" %c", &choix);

                // swap
                if (choix == 's') {
                    int tl, tc;
                    tl = saisir_nb("Ligne cible pour le swap : ");
                    tc = saisir_nb("Colonne cible pour le swap : ");

                    rc = swap_piece(game, tl, tc);
                    if (rc != OK) {
                        printf("Swap impossible (code %d), réessaie.\n", rc);
                        continue;
                    }

                    printf("Swap réussi.\n");
                    display_board(game, name_n, name_s); 
                    break; 
                }

                // rebond
                if (!is_move_possible(game, dir)) {
                    printf("Mouvement impossible dans cette direction.\n");
                    continue;
                }

                rc = move_piece(game, dir);
                if (rc != OK) {
                    printf("Erreur move_piece (code %d)\n", rc);
                    continue;
                }

                display_board(game, name_n, name_s); 
                continue;
            }

            
            if (!is_move_possible(game, dir)) {
                printf("Mouvement impossible.\n");
                continue;
            }

            rc = move_piece(game, dir);
            if (rc != OK) {
                printf("Erreur move_piece (code %d)\n", rc);
                continue;
            }

            display_board(game, name_n, name_s); 
            continue;
        }

        
        printf("Commande inconnue.\n");
    }

    
    if (movement_left(game) == -1) {
        return STATE_END_TURN;  
    }

    return STATE_PLAYER_TURN;
}


/// @brief Etat de fin de tour qui test si le jeu est fini ou non 
/// @param game plateau de jeu
/// @param current_player joueur qui joue
/// @return prochain etat du jeu
GameState state_end_turn(board game, player *current_player, char *name_n, char *name_s)
{
    player w = get_winner(game);

    if (w != NO_PLAYER) {
        if (w == NORTH_P)
            printf("VICTOIRE ! Bravo %s (NORD) a gagné !\n", name_n);
        else
            printf("VICTOIRE ! Bravo %s (SUD) a gagné !\n", name_s);
            
        return STATE_GAME_OVER;
    }

    return STATE_TURN_START;
}

int main(int args, char **argv)
{
    board game = new_game();

    
    char name_n[50];
    char name_s[50];

    printf("Entrez le nom du joueur NORD : ");
    scanf("%s", name_n);

    printf("Entrez le nom du joueur SUD : ");
    scanf("%s", name_s);
    

    player p = first_player(pile_ou_face());

    GameState state = STATE_SETUP;

    if(p == NORTH_P)
    {
        printf("%s (Nord) commence !\n", name_n);
    }
    else
    {
        printf("%s (Sud) commence !\n", name_s);
    }

    printf("Un plateau est créé.\n");

    while (state != STATE_GAME_OVER) {
        switch (state) {
            case STATE_SETUP:
                state = state_setup(game, p, name_n, name_s);
                break;
            case STATE_TURN_START:
                state = state_turn_start(game, &p, name_n, name_s);
                break;
            case STATE_PLAYER_TURN:
                state = state_player_turn(game, &p, name_n, name_s);
                break;
            case STATE_END_TURN:
                state = state_end_turn(game, &p, name_n, name_s);
                break;
        }
    }
    
    destroy_game(game);
    printf("suppression du plateau et sortie\n");

    return 0;
}


