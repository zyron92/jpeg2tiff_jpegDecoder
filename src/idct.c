#include "idct.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "debogage.h"

#define PI 3.14159265358979323846264338327950288419716

// Les différentes couleurs appartiennent à l'intervalle [0,255]
// Cette fonction permet de ramener les valeurs à cet intervalle
static uint8_t troncature(float x)
{
	if (x < 0){
		return 0 ;
	}else if(x > 255){
		return 255 ;
	}else{
		return x ; 
	}
}

// pour l'implementation de l'algorithme de loeffler, on a utilisé
// une documentation qu'on va mettre dans le depôt

static void butterfly(float e0, float e1, float *s0, float *s1)
{
	*s0 = e0 + e1 ;
	*s1 = e0 - e1 ;
}

static void rotator(float e0, float e1, float *s0, float *s1, float k, uint8_t n)
{
	*s0 = k*e0*cos((n*PI)/(2*8)) - k*e1*sin((n*PI)/(2*8)) ;
	*s1 = k*e0*sin((n*PI)/(2*8)) + k*e1*cos((n*PI)/(2*8)) ;
}

static void loeffler_inverse(float *vecteur_e, float *vecteur_s)
{

//etape 1 :
 	float vecteur_i_1[8];
	vecteur_i_1[0] = vecteur_e[0];
	vecteur_i_1[1] = vecteur_e[4];
	vecteur_i_1[2] = vecteur_e[2];
	vecteur_i_1[3] = vecteur_e[6];
	vecteur_i_1[5] = sqrt(2) * vecteur_e[3];
	vecteur_i_1[6] = sqrt(2) * vecteur_e[5];
	butterfly(vecteur_e[1], vecteur_e[7], &vecteur_i_1[7], &vecteur_i_1[4]);

//etape 2 :
	float vecteur_i_2[8];
	butterfly(vecteur_i_1[0],vecteur_i_1[1],&vecteur_i_2[0],&vecteur_i_2[1]);
	rotator(vecteur_i_1[2], vecteur_i_1[3], 
		&vecteur_i_2[2],&vecteur_i_2[3], sqrt(2), 6);
	butterfly(vecteur_i_1[4],vecteur_i_1[6],&vecteur_i_2[4],&vecteur_i_2[6]);
	butterfly(vecteur_i_1[7],vecteur_i_1[5],&vecteur_i_2[7],&vecteur_i_2[5]);

//etape 3 :
	float vecteur_i_3[8];
	butterfly(vecteur_i_2[0],vecteur_i_2[3],&vecteur_i_3[0],&vecteur_i_3[3]);
	butterfly(vecteur_i_2[1],vecteur_i_2[2],&vecteur_i_3[1],&vecteur_i_3[2]);
	rotator(vecteur_i_2[4], vecteur_i_2[7], 
		&vecteur_i_3[4],&vecteur_i_3[7], 1, 3);
	rotator(vecteur_i_2[5], vecteur_i_2[6], 
		&vecteur_i_3[5],&vecteur_i_3[6], 1, 1);

//etape 4 :
	float vecteur_i_4[8];
	butterfly(vecteur_i_3[0],vecteur_i_3[7],&vecteur_i_4[0],&vecteur_i_4[7]);
	butterfly(vecteur_i_3[1],vecteur_i_3[6],&vecteur_i_4[1],&vecteur_i_4[6]);
	butterfly(vecteur_i_3[2],vecteur_i_3[5],&vecteur_i_4[2],&vecteur_i_4[5]);
	butterfly(vecteur_i_3[3],vecteur_i_3[4],&vecteur_i_4[3],&vecteur_i_4[4]);

//division par l'echelle (1/racine(8))
	for (int32_t k=0; k<8; k++){
	  	vecteur_s[k] = (1/sqrt(8))*vecteur_i_4[k];
	}
	
}


extern void idct_block(int32_t in[64], uint8_t out[64])
{
	if (in == NULL) {
		ERREUR1( "le tableau initial n'est pas alloué\n");
		exit(EXIT_FAILURE);
	}
  
	if (out == NULL){
		ERREUR1( "le tableau de sortie n'est pas alloué\n");
		exit(EXIT_FAILURE);
	}

//allocation de le memoire pour les vecteurs intermediares :
	float vecteur_i_e[8];
	float vecteur_i_s[8];
	float matrice_i[64];

//on applique l'algorithme de loeffler sur chaque colonne	
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			vecteur_i_e[j] = in[i+8*j];
		}
		loeffler_inverse(vecteur_i_e,vecteur_i_s);
		for (int j = 0; j < 8; j++) {
			matrice_i[i+8*j] = vecteur_i_s[j];
		}
	}
//on l'applique sur chaque ligne de la matrice intermediare
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			vecteur_i_e[j] = matrice_i[8*i+j];
		}
		loeffler_inverse(vecteur_i_e,vecteur_i_s);
		for (int j = 0; j < 8; j++) {
			out[8*i+j] = troncature(vecteur_i_s[j] +128);
		}
	}

}














