#include "idct.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "debogage.h"

#define PI 3.14159265358979323846264338327950288419716

// Les différentes couleurs appartiennent à l'intervalle [0,255]
// Cette fonction permet de ramener les valeurs à cet intervalle
static uint8_t troncature(float x)
{
	if (x < 0){
		return 0;
	}else if(x > 255){
		return 255;
	}else{
		return x;
	}
}

// renvoie (1/racine(2)) si l'entree et 0 , et 1 sinon
static double f_c(int32_t p)
{
	if (p == 0){
		return (1/sqrt(2));
	}else{
		return 1;
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

	int32_t n = 8 ;  // la taille des macroblocs
	
	for (int32_t x = 0 ; x < n ; x++) {
		for (int32_t y = 0 ; y < n ; y++) {
			float s = 0.0;   //s contient le macrobloc spatial
			for (int32_t lambda = 0 ; lambda < n ; lambda++) {
				for (int32_t mu = 0 ; mu < n ; mu++) {
					// s(x,y) signee
					// formule poly : 
					s += (1/sqrt(2*n))*f_c(lambda)*f_c(mu)*
					     cos(((2*x + 1)*lambda*PI)/(2*n))*
					     cos(((2*y + 1)*mu*PI)/(2*n))*
					     in[n*lambda + mu];
				}
			}
			//prendre en compte la representation non signee
			s += 128.0;
			out[n*x + y] = troncature(s) ;
		}
	}
}
  
      

//remarque :
// (i, j) => i*n + j
// cos(x) peut etre negative donc -
// - troncature doit tronquer les x negatifs 

