#include "upsampler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debogage.h"

// permet de dupliquer une image (tableau de pixels)
// quand le nombre de blocks en sortie et plus grand 
// que le nombre de blocks en sortie (Composante Cr et Cb)  
static void duplication(uint8_t *entree, uint8_t *sortie, 
			uint8_t coeff_h, uint8_t coeff_v)
{
	uint32_t ind1 ;
	uint32_t ind2 ;
	for(uint32_t i = 0 ; i < 8*coeff_v ; i++){
		if (coeff_v > 1) {  // si duplication verticale
			ind1 = i/coeff_v ;
		}else {
			ind1 = i;
		}
		for(uint32_t j = 0 ; j < 8*coeff_h ; j++){
			if (coeff_h > 1) {  // si duplication horizontale
				ind2 = j/coeff_h ;
			}else {
				ind2 = j;
			}
			sortie[8*coeff_h*i+j] = entree[8*ind1+ind2];
		}
	}
}

// permet de réorganiser le tableau d'entrée quand 
// le nombre de blocks ne changent pas (composante Y)
static void unification(uint8_t *entree, uint8_t *sortie, uint8_t in_h, 
			uint8_t in_v)
{
	uint32_t ind1 ;
	uint32_t ind2 ;
	for(uint32_t i = 0 ; i < 8*in_v ; i++){
		// k : blocs horizontal en cours de traitement
		for(uint32_t k = 0 ; k < in_h ; k++){
			for(uint32_t j = 8*k; j < 8*(k+1); j++){
				ind1 = i + k*7;
				ind2 = j;
				sortie[8*in_h*i+j] = entree[64*(in_h-1)*(i/8)+
							    8*ind1+ind2];
			}
		}
	}
}


extern void upsampler(uint8_t *in,
		      uint8_t nb_blocks_in_h, uint8_t nb_blocks_in_v,
		      uint8_t *out,
		      uint8_t nb_blocks_out_h, uint8_t nb_blocks_out_v)
{
	if (nb_blocks_in_h > nb_blocks_out_h){
		ERREUR1("duplication impossible : horizontal\n");
		exit(EXIT_FAILURE);
	}else if(nb_blocks_in_h > nb_blocks_out_h){
		ERREUR1("duplication impossible : vertical\n");
		exit(EXIT_FAILURE);
	}
	if(nb_blocks_in_h < nb_blocks_out_h || 
	   nb_blocks_in_v < nb_blocks_out_v){  //cas d'une duplication
		uint8_t coeff1 = nb_blocks_out_h/nb_blocks_in_h;
		uint8_t coeff2 = nb_blocks_out_v/nb_blocks_in_v;
		duplication(in, out, coeff1, coeff2);
	}else{ // unification
		unification(in, out, nb_blocks_in_h, nb_blocks_in_v);
	}
}
