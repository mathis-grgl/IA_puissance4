/*
	Canvas pour algorithmes de jeux à 2 joueurs
	
	joueur 0 : humain
	joueur 1 : ordinateur
			
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// Paramètres du jeu
#define LARGEUR_MAX 9 		// nb max de fils pour un noeud (= nb max de coups possibles)
#define TEMPS 5		// temps de calcul pour un coup avec MCTS (en secondes)
#define NB_LIGNES 6	// nb de lignes du plateau
#define NB_COLONNES 7		// nb de colonnes du plateau
#define NB_COUPS_GAGNANT 4	// nb de coups gagnants

// macros
#define AUTRE_JOUEUR(i) (1-(i))
#define min(a, b)       ((a) < (b) ? (a) : (b))
#define max(a, b)       ((a) < (b) ? (b) : (a))

// Critères de fin de partie
typedef enum {NON, MATCHNUL, ORDI_GAGNE, HUMAIN_GAGNE } FinDePartie;

// Definition du type Etat (état/position du jeu)
typedef struct EtatSt {
	int joueur; // à qui de jouer ? 
	char plateau[NB_LIGNES][NB_COLONNES]; // plateau de jeu avec des 'O' et des 'X'
} Etat;

// Definition du type Coup
typedef struct {
	int ligne; // numéro de ligne
	int colonne; // numéro de colonne
} Coup;

// Copier un état 
Etat * copieEtat( Etat * src ) {

	// allocation mémoire pour la etat
	Etat * etat = (Etat *)malloc(sizeof(Etat));

	// etat des champs
	etat->joueur = src->joueur;

	// Pour chaque case du plateau : on recopie la valeur
	for (int i = 0; i < NB_LIGNES; i++) {
		for (int j = 0; j < NB_COLONNES; j++) {
			etat->plateau[i][j] = src->plateau[i][j];
		}
	}
	

	// On retourne l'état copié
	return etat;
}

// Etat initial 
Etat * etat_initial( void ) {
	// allocation mémoire pour l'état initial
	Etat * etat = (Etat *)malloc(sizeof(Etat));
	
	// Pour chaque case du plateau : on met des espaces pour dire que c'est vide
	for (int i = 0; i < NB_LIGNES; i++) {
		for (int j = 0; j < NB_COLONNES; j++) {
			etat->plateau[i][j] = ' ';
		}
	}
	
	// On retourne l'état initial
	return etat;
}

// Affichage de l'état du jeu
void afficheJeu(Etat * etat) {

	// Affichage des numéros de colonnes
	printf("  0   1   2   3   4   5   6\n");

	// Affichage du plateau de jeu
	for (int i = 0; i < NB_LIGNES ; i++) {
		for (int j = 0; j < NB_COLONNES; j++) {
			printf("| %c ", etat->plateau[i][j]);
		}
		printf("|\n");
	}
}


// Nouveau coup 
Coup * nouveauCoup( int i, int j ) {
	Coup * coup = (Coup *)malloc(sizeof(Coup)); // allocation mémoire pour le coup
		
	coup->ligne = i; // on affecte la ligne
	coup->colonne = j; // on affecte la colonne
	
	// On retourne le coup créé
	return coup;
}

// Demander à l'humain quel coup jouer 
Coup * demanderCoup (Etat *etat) {

	// Choix de la colonne
	int colonne;
	printf("Quelle colonne jouer (0-%d) ? ", NB_COLONNES-1);
	scanf("%d", &colonne);

	// Choix de la colonne si elle n'est pas valide au départ
	while(colonne < 0 || colonne > NB_COLONNES-1) {
		printf("Quelle colonne jouer (0-%d) ? ", NB_COLONNES-1);
		scanf("%d", &colonne);
	}

	// Déterminer la ligne
	int ligne = NB_LIGNES - 1;
	while(ligne >= 0 && etat->plateau[ligne][colonne] != ' ') {
		ligne--;
	}

	// On retourne le coup choisi
	return nouveauCoup(ligne, colonne);
}

// Modifier l'état en jouant un coup 
// retourne 0 si le coup n'est pas possible
int jouerCoup( Etat * etat, Coup * coup ) {
	
	// Vérifier si le coup est possible
	if ( etat->plateau[coup->ligne][coup->colonne] != ' ' )
		return 0;

	// Sinon jouer le coup et passer la main à l'autre joueur
	else {
		etat->plateau[coup->ligne][coup->colonne] = etat->joueur ? 'O' : 'X';
		
		// à l'autre joueur de jouer
		etat->joueur = AUTRE_JOUEUR(etat->joueur); 	

		return 1;
	}	
}

// Retourne une liste de coups possibles à partir d'un etat 
// (tableau de pointeurs de coups se terminant par NULL)
Coup ** coups_possibles( Etat * etat ) {
	// allocation mémoire pour un tableau de pointeurs de coups
	Coup ** coups = (Coup **)malloc((NB_COLONNES+1) * sizeof(Coup *)); // +1 pour NULL à la fin
	
	// Initialiser k à 0, pour se souvenir où on en est dans le tableau
	int k = 0;
	
	// Pour chaque colonne, si on peut jouer dans cette colonne, on ajoute un pointeur vers un coup possible
	for (int j=0; j < NB_COLONNES; j++) {
		int i = NB_LIGNES - 1;
		while ( i >= 0 && etat->plateau[i][j] != ' ') 
			i--;
		if ( i >= 0 ) {
			coups[k] = nouveauCoup(i,j);
			k++;
		}
	}

	// On termine le tableau par NULL
	coups[k] = NULL;

	// On retourne le tableau de coups possibles
	return coups;
}


// Definition du type Noeud 
typedef struct NoeudSt {
		
	int joueur; // joueur qui a joué pour arriver ici
	Coup * coup;   // coup joué par ce joueur pour arriver ici
	
	Etat * etat; // etat du jeu
			
	struct NoeudSt * parent; 
	struct NoeudSt * enfants[LARGEUR_MAX]; // liste d'enfants : chaque enfant correspond à un coup possible
	int nb_enfants;	// nb d'enfants présents dans la liste
	
	// POUR MCTS:
	int nb_victoires; // nombre de victoires
	int nb_simus; // nombre de simulations
	
} Noeud;


// Créer un nouveau noeud en jouant un coup à partir d'un parent 
// utiliser nouveauNoeud(NULL, NULL) pour créer la racine
Noeud * nouveauNoeud (Noeud * parent, Coup * coup ) {

	// allocation mémoire pour le nouveau noeud
	Noeud * noeud = (Noeud *)malloc(sizeof(Noeud));
	
	// Initialiser les champs si ce n'est pas la racine
	if ( parent != NULL && coup != NULL ) {
		noeud->etat = copieEtat ( parent->etat );
		jouerCoup ( noeud->etat, coup );
		noeud->coup = coup;			
		noeud->joueur = AUTRE_JOUEUR(parent->joueur);		
	}

	// Sinon initialiser à NULL
	else {
		noeud->etat = NULL;
		noeud->coup = NULL;
		noeud->joueur = 0; 
	}

	// Initialiser les autres champs
	noeud->parent = parent; 
	noeud->nb_enfants = 0; 
	
	// POUR MCTS:
	noeud->nb_victoires = 0; // initialement 0 victoire
	noeud->nb_simus = 0; // initialement 0 simulation
	

	// On retourne le noeud créé
	return noeud; 	
}

// Ajouter un enfant à un parent en jouant un coup
// retourne le pointeur sur l'enfant ajouté
Noeud * ajouterEnfant(Noeud * parent, Coup * coup) {

	// Créer un nouveau noeud qui correspond à l'état obtenu en jouant "coup" à partir de l'état "parent->etat"
	Noeud * enfant = nouveauNoeud (parent, coup ) ;
	parent->enfants[parent->nb_enfants] = enfant;
	parent->nb_enfants++;

	// On retourne le pointeur sur le nouvel enfant
	return enfant;
}

// Libère la mémoire d'un noeud et de tous ses enfants
void freeNoeud ( Noeud * noeud) {

	// Si l'état n'est pas NULL, on le libère
	if ( noeud->etat != NULL)
		free (noeud->etat);
		
	// Tant qu'il y a des enfants, on les libère
	while ( noeud->nb_enfants > 0 ) {
		freeNoeud(noeud->enfants[noeud->nb_enfants-1]);
		noeud->nb_enfants --;
	}

	// Si le coup n'est pas NULL, on le libère
	if ( noeud->coup != NULL)
		free(noeud->coup); 

	// On libère le noeud lui-même
	free(noeud);
}
	

// Test si l'état est un état terminal 
// et retourne NON, MATCHNUL, ORDI_GAGNE ou HUMAIN_GAGNE
FinDePartie testFin( Etat * etat ) {

	// Initialiser le nombre de coups joués
	int k = 0;

	// Vérifier si un joueur a gagné
	for (int i = 0; i < NB_LIGNES; i++) {
		for (int j = 0; j < NB_COLONNES; j++) {
			if(etat->plateau[i][j] != ' ') {
				k++;	
				// Vérifier les lignes
				if(j <= NB_COLONNES - NB_COUPS_GAGNANT) {
					if(etat->plateau[i][j] == etat->plateau[i][j+1] && etat->plateau[i][j] == etat->plateau[i][j+2] && etat->plateau[i][j] == etat->plateau[i][j+3]) {
						if(etat->plateau[i][j] == 'O') {
							return ORDI_GAGNE;
						} else {
							return HUMAIN_GAGNE;
						}
					}
				}
				// Vérifier les colonnes
				if(i <= NB_LIGNES - NB_COUPS_GAGNANT) {
					if(etat->plateau[i][j] == etat->plateau[i+1][j] && etat->plateau[i][j] == etat->plateau[i+2][j] && etat->plateau[i][j] == etat->plateau[i+3][j]) {
						if(etat->plateau[i][j] == 'O') {
							return ORDI_GAGNE;
						} else {
							return HUMAIN_GAGNE;
						}
					}
				}
				// Vérifier les diagonales
				if(i <= NB_LIGNES - NB_COUPS_GAGNANT && j <= NB_COLONNES - NB_COUPS_GAGNANT) {
					if(etat->plateau[i][j] == etat->plateau[i+1][j+1] && etat->plateau[i][j] == etat->plateau[i+2][j+2] && etat->plateau[i][j] == etat->plateau[i+3][j+3]) {
						if(etat->plateau[i][j] == 'O') {
							return ORDI_GAGNE;
						} else {
							return HUMAIN_GAGNE;
						}
					}
				}
				if(i >= 3 && j <= NB_COLONNES - NB_COUPS_GAGNANT){
					if(etat->plateau[i][j] == etat->plateau[i-1][j+1] && etat->plateau[i][j] == etat->plateau[i-2][j+2] && etat->plateau[i][j] == etat->plateau[i-3][j+3]) {
						if(etat->plateau[i][j] == 'O') {
							return ORDI_GAGNE;
						} else {
							return HUMAIN_GAGNE;
						}
					}
				}
			}
		}
	}

	// Vérifier si c'est un match nul
	if(k == NB_LIGNES * NB_COLONNES) {
		return MATCHNUL;
	}

	return NON;
}

/**
 * Choisis le meilleur coup gagnant parmi les coups possibles
 * Si aucun coup gagnant n'est disponible, choisit un coup aléatoire parmi les autres
*/
Coup * choisirMeilleurCoupGagnant(Coup ** coups, Noeud * noeud) {
    Coup * meilleur_coup_gagnant = NULL;

    // Parcourir les coups possibles
    for (int i = 0; coups[i] != NULL; i++) {
        // Copier l'état actuel du jeu pour le simuler
        Etat * etat_simule = copieEtat(noeud->etat);
        // Jouer le coup
        jouerCoup(etat_simule, coups[i]);
        // Vérifier si ce coup mène à une victoire
        if (testFin(etat_simule) == ORDI_GAGNE) {
            meilleur_coup_gagnant = coups[i];
            // Libérer la mémoire de l'état simulé
            free(etat_simule);
            break;
        }
        // Libérer la mémoire de l'état simulé
        free(etat_simule);
    }

    // Si aucun coup gagnant n'est trouvé, choisir un coup aléatoire parmi les autres
    if (meilleur_coup_gagnant == NULL) {
        int k = 0;
        while (coups[k] != NULL) {
            k++;
        }
        meilleur_coup_gagnant = coups[rand() % k];
    }

    // Retourner le meilleur coup gagnant
    return meilleur_coup_gagnant;
}

/**
 * Crée les enfants du noeud
*/
void creation_enfants(Noeud * noeud) {
	// On récupère les coups possibles
	Coup ** coups = coups_possibles(noeud->etat);

	// Si le noeud n'a pas été visité et que la partie n'est pas finie
	if (noeud->nb_simus == 0 && testFin(noeud->etat) == NON) {

		// On choisit le meilleur coup gagnant grâce à la fonction choisirMeilleurCoupGagnant
		Coup * meilleur_coup_gagnant = choisirMeilleurCoupGagnant(coups, noeud);

		// On ajoute le meilleur coup gagnant comme enfant du noeud
		ajouterEnfant(noeud, meilleur_coup_gagnant);

		// On libère la mémoire
		free(meilleur_coup_gagnant);
	}

	// On libère la mémoire
	free(coups);
}

/**
 * Simule la partie
*/
void simulation(Noeud * noeud) {
	// On simule jusqu'à la fin de la partie
	while (testFin(noeud->etat) == NON) {

		// On récupère les coups possibles
		Coup ** coups = coups_possibles(noeud->etat);

		// Pour chaque coup possible, on joue le coup
		for (int k = 0; coups[k] != NULL; k++) {
			jouerCoup(noeud->etat, coups[k]);
		}

		// On libère la mémoire
		free(coups);
	}
}

/**
 * Finit la simulation et remonte dans l'arbre
*/
void fin_simulation(Noeud * noeud) {
	// On récupère la fin de la partie
	double num_vainc = (testFin(noeud->etat) == HUMAIN_GAGNE) ? 0 : 1;

	// Tant que le noeud n'est pas la racine
	while (noeud != NULL) {

		// On met à jour le nombre de simulations
		noeud->nb_simus++;

		// On met à jour le nombre de victoires en fonction de la fin de la partie
		noeud->nb_victoires += num_vainc;

		// On affecte le noeud parent à noeud
		noeud = noeud->parent;
	}	
}

// Calcule et joue un coup de l'ordinateur avec MCTS-UCT
// en tempsmax secondes
void ordijoue_mcts(Etat * etat, int tempsmax) {

	clock_t tic, toc;
	tic = clock();
	int temps;

	Coup ** coups;
	Coup * meilleur_coup ;
	
	// Créer l'arbre de recherche
	Noeud * racine = nouveauNoeud(NULL, NULL);	
	racine->etat = copieEtat(etat);
	
	// créer les premiers noeuds:
	coups = coups_possibles(racine->etat); 
	int k = 0;
	Noeud * enfant;
	while ( coups[k] != NULL) {
		enfant = ajouterEnfant(racine, coups[k]);
		k++;
	}
	
	
	//meilleur_coup = coups[ rand()%k ]; // choix aléatoire
	
	/*  TODO :
		- supprimer la sélection aléatoire du meilleur coup ci-dessus
		- implémenter l'algorithme MCTS-UCT pour déterminer le meilleur coup ci-dessous
*/
	int iter = 0;
	
	do {

		// On affecte la racine à un noeud
		Noeud * noeud = racine;
		
		// Tant que ce n'est pas une feuille
		while (noeud->nb_enfants > 0) {
			
			// On initialise les variables
			double max = -1;
			Noeud * meilleur_noeud;
			int i = 0;

			// On parcourt les enfants
			while(i < noeud->nb_enfants){

				// On initialise la B-valeur
				double b;

				// Si le noeud n'a pas été visité, on met la B-valeur à 1000000
				if(noeud->enfants[i]->nb_simus == 0) b = 1000000;

				// Sinon on calcule la B-valeur
				else b = (double)noeud->enfants[i]->nb_victoires / noeud->enfants[i]->nb_simus +sqrt(2) * sqrt(2 * log(noeud->nb_simus) / noeud->enfants[i]->nb_simus);

				// Si la B-valeur est supérieure au max, on la met à jour
				if (b > max) {
					max = b;
					meilleur_noeud = noeud->enfants[i];
				}
				i++;
			}
			// On affecte le meilleur noeud à noeud
			noeud = meilleur_noeud;
		}

		// On crée les enfants du noeud
		creation_enfants(noeud);
		
		// On simule
		simulation(noeud);

		// On finit la simulation et on remonte
		fin_simulation(noeud);	


		// Temps écoulé
		toc = clock(); 
		temps = (int)( ((double) (toc - tic)) / CLOCKS_PER_SEC );
	} while ( temps < tempsmax );

    // Choisit le meilleur coup
    double meilleur_score = -1;
	for (int i = 0; i < racine->nb_enfants; i++) {
		double score = (double)racine->enfants[i]->nb_victoires / racine->enfants[i]->nb_simus;
		if (score > meilleur_score) {
			meilleur_score = score;
			meilleur_coup = racine->enfants[i]->coup;
		}
	}

	// Afficher le nombre de simulations et la chance de victoire (Question 1)
	printf("\nNb de simulations : %d\n", racine->nb_simus);
	printf("Chance de victoire : %f\n", meilleur_score*100);
	
	// Jouer le meilleur premier coup
	jouerCoup(etat, meilleur_coup );
	
	// Libère la mémoire de l'arbre de recherche
	freeNoeud(racine);
}

int main(void) {

	Coup * coup;
	FinDePartie fin;
	
	// initialisation
	Etat * etat = etat_initial(); 
	
	// Choisir qui commence : 
	printf("Qui commence (0 : humain, 1 : ordinateur) ? ");
	scanf("%d", &(etat->joueur) );
	
	// boucle de jeu
	do {
		printf("\n");
		afficheJeu(etat);
		
		if ( etat->joueur == 0 ) {
			// tour de l'humain
			
			do {
				coup = demanderCoup(etat);
			} while ( !jouerCoup(etat, coup) );
									
		}
		else {
			// tour de l'Ordinateur
			
			ordijoue_mcts( etat, TEMPS );
			
		}
		
		fin = testFin( etat );
	}	while ( fin == NON ) ;

	printf("\n");
	afficheJeu(etat);
		
	if ( fin == ORDI_GAGNE )
		printf( "** L'ordinateur a gagné **\n");
	else if ( fin == MATCHNUL )
		printf(" Match nul !  \n");
	else
		printf( "** BRAVO, l'ordinateur a perdu  **\n");
	return 0;
}
