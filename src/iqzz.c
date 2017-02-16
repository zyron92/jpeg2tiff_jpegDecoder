#include "iqzz.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "debogage.h"

#define TAILLE 8 //la largeur ou la hauteur du tableau carré

static uint32_t conversion(int32_t i, int32_t j);
static bool fin_tableau(int32_t i, int32_t j);
static void tester_depasser_tableau(int32_t *i, int32_t *j, bool *sens_bas);
static void parcourir_tableau_recursive(int32_t in[64], int32_t out[64], 
					uint8_t quantif[64], int32_t *i, int32_t *j, 
					uint32_t *ind_out, bool *sens_bas);
static void parcourir_diagonal_haut(int32_t *i, int32_t *j);
static void parcourir_diagonal_bas(int32_t *i, int32_t *j);
extern void iqzz_block (int32_t in[64], int32_t out[64], uint8_t quantif[64]);

//Conversion d'un indice de deux dimensions (matrice) 
//vers un indice d'une dimension (vecteur) 
static uint32_t conversion(int32_t i, int32_t j)
{
	//un parcours gauche à droite, ligne par ligne
	return TAILLE*i+j;
}

//Detecter si on arrive à la fin de tableau 
//à la case (TAILLE-1,TAILLE-1), ici (7,7)
static bool fin_tableau(int32_t i, int32_t j)
{
	if (i==TAILLE-1 && j==TAILLE-1){
		return true;
	}
	return false;
}

//Detecter si on dépasse le tableau pendant le parcours en diagonal
//Et changer de sens de parcours en diagonal
static void tester_depasser_tableau(int32_t *i, int32_t *j, bool *sens_bas)
{
	//On quitte le programme pour ce cas
	//Car on a assuré qu'on commence à la case(0,0) et 
	//finit à la case(TAILLE-1,TAILLE-1) 
	if ((*i<0 && *j<0) || (*i>TAILLE-1 && *j>TAILLE-1)){
		ERREUR1( "ERREUR : Erreur d'environment de programme.\n");
		exit(EXIT_FAILURE);
	}

	//Tester si i ou j négatif
	if (*i<0){
		(*i)++; //i revient à 0
		*sens_bas=true;

		//Aussi tester si j dépasse la hauteur-1 du tableau, ici 7
		if (*j>TAILLE-1){
			(*j)--; //j revient à TAILLE-1 ici 7
			(*i)++; //i avance qu'un pas car on s'est déjà avancé 
			        //un pas
		}
		
	}else if (*j<0){
		(*j)++; //j revient à 0
		*sens_bas=false;
		
		//Aussi tester si i dépasse la hauteur-1 du tableau, ici 7 
		if (*i>TAILLE-1){
			(*i)--; //i revient à TAILLE-1 ici 7
			(*j)++; //j avance qu'un pas car on s'est déjà avancé 
			        //un pas
		}

	//Tester si i ou j dépasse la hauteur-1 du tableau, ici 7
	}else if(*i>TAILLE-1){
		(*i)--; //i revient à TAILLE-1 ici 7
		(*j)+=2; //j avance 2 pas
		*sens_bas=false;

	}else if (*j>TAILLE-1){
		(*j)--; //j revient à TAILLE-1 ici 7
		(*i)+=2; //i avance 2 pas
		*sens_bas=true;
	}
}

//Parcourir le tableau en diagonal vers le haut du tableau
static void parcourir_diagonal_haut(int32_t *i, int32_t *j)
{
	*i = *i-1;
	*j = *j+1;
}

//Parcourir le tableau en diagonal vers le bas du tableau
static void parcourir_diagonal_bas(int32_t *i, int32_t *j)
{
	*i = *i+1;
	*j = *j-1;
}

//Parcourir le tableau de sortie en diagonal en récursive
//On modifie le sens (vers le haut ou le bas du tableau) au fur et à mesure
static void parcourir_tableau_recursive(int32_t in[64], int32_t out[64], 
					uint8_t quantif[64], 
					int32_t *i, int32_t *j, 
					uint32_t *ind_out, bool *sens_bas)
{
	if (!fin_tableau(*i,*j)){
		//On affecte la valeur dans le tableau de sortie
		//le parcours se fait en diagonal dans le tableau de sortie
		//car c'est l'opération zigzag inverse
		//On fait la multiplication par la table de quantification
                //en même temps pour gagner le temps d'éxécution
		out[conversion(*i,*j)]=in[*ind_out]*quantif[*ind_out];
		(*ind_out)++;
		
		if (*sens_bas){//sens de parcours en diagonal : vers le bas
			parcourir_diagonal_bas(i,j);
		}else{//sens de parcours en diagonal : vers le haut
			parcourir_diagonal_haut(i,j);
		}

		tester_depasser_tableau(i,j, sens_bas);
		parcourir_tableau_recursive(in, out, quantif, i,j, ind_out, 
					    sens_bas);
	}
}

//LA PROCEDURE PRINCIPALE
extern void iqzz_block (int32_t in[64], int32_t out[64], uint8_t quantif[64])
{
        //Erreur d'allocation de "out"
	if (out == NULL){
		ERREUR1("ERREUR : Erreur d'allocation du tableau.\n");
		exit(EXIT_FAILURE);
	}
	
	//"in" vide
	if (in == NULL){
		ERREUR1("ERREUR : Tableau d'entrée vide (NULL) , \
                          on va envoyer un tableu de sortie vide (NULL).\n");
		exit(EXIT_FAILURE);
	}

	//"quantif" vide
	if (quantif == NULL){
		ERREUR1("ERREUR : Tableau de quantification vide (NULL)\
                          , on va envoyer un tableu de sortie vide (NULL).\n");
		exit(EXIT_FAILURE);
	}

	/*####### Zig-zag inverse & Multiplication #######*/
	//IDEE : 
	//On parcourt le tableau de sortie en diagonale (zig-zag)
	//pour l'initialiser avec le tableau d'entrée (en lecture normale)
	//et fait la multiplication en même temps

	//Première case
	uint32_t ind_out=0;
	out[ind_out]=in[ind_out]*quantif[ind_out];
	ind_out++;
	
	//Les cases restantes
	int32_t i=0;
	int32_t j=1;
	bool sens_bas = true; //sens de parcours en diagonal
	parcourir_tableau_recursive(in, out, quantif, &i, &j, &ind_out, 
				    &sens_bas);

	//Dernière case
	out[ind_out]=in[ind_out]*quantif[ind_out];
	
	/*#### Fin Zig-zag inverse & Multiplication ####*/
}


