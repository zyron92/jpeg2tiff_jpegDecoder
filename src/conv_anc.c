#include "conv.h"
#include <stdlib.h>
#include <stdio.h>
#include "debogage.h"

static uint8_t troncature(float x){
	if (x < 0){
		return 0;
	}else if(x > 255){
		return 255;
	}else{
		return x;
	}
}


static uint32_t calcul_ARGB (uint8_t y, uint8_t cb, uint8_t cr)
//Cette fonction effectue les calculs de R, G et B
//puis les range dans un uint32_t qu'elle renvoie
{

        uint8_t R,G,B;
        R = troncature (y + 1.402 * (cr-128)) ;
        G = troncature (y - 0.34414 * (cb-128) - 0.71414 * (cr-128));
        B = troncature (y + 1.772 * (cb-128));

        return (R<<16) + (G<<8) + B;

}

extern void YCbCr_to_ARGB(uint8_t  *mcu_YCbCr[3], uint32_t *mcu_RGB,
		uint32_t nb_blocks_h, uint32_t nb_blocks_v)
{
       
        if (mcu_YCbCr==NULL || mcu_RGB==NULL) {
                ERREUR1("Erreur : Pointeur null\n");
                exit(EXIT_FAILURE);
        }

        const uint32_t nb_pixels_par_block=64;
        uint32_t nb_blocks = nb_blocks_h * nb_blocks_v;
        uint32_t premier_pixel, pixel_cour;

        
        for (uint32_t i=0; i<nb_blocks;i++) {
                //Pour chaque bloc
                //calcul du numero du premier pixel du bloc
                premier_pixel= i * nb_pixels_par_block;

                for (uint32_t j=0; j<nb_pixels_par_block;j++) {
                        //Pour chaque pixel du block
                        pixel_cour=premier_pixel+j;

                        //on calcule l'entier ARGB correspondant
                        //et on le range dans mcu_RGB
                        mcu_RGB[pixel_cour]=calcul_ARGB(
                                *(mcu_YCbCr[0]+pixel_cour),
                                *(mcu_YCbCr[1]+pixel_cour),
                                *(mcu_YCbCr[2]+pixel_cour));
                }
        }
    
}

