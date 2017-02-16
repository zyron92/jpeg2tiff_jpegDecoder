#include "unpack.h"
#include <stdlib.h>
#include <stdio.h>
#include "debogage.h"

static uint32_t lire (struct bitstream *stream, uint8_t nb_bits)
{
        uint32_t dest;
        uint8_t verif = read_bitstream(stream,nb_bits,&dest,true);
        
        if (verif!=nb_bits){
                ERREUR2("Erreur : On attendait %u bits, %u ont été lu\n"
                        ,nb_bits, verif);
                exit(EXIT_FAILURE);
        }
        return(dest);
}

static uint32_t extraire_coefficient (int8_t magnitude, uint32_t sequence) 
{ 
        //Cette fonction prend en paramètre une classe de magnitude 
        //et une séquence de bits
        //Elle renvoie le coefficient correspondant

        if (magnitude > 0) {
                //on extraie le bit de poids fort de la séquence
                uint8_t bit_pds_fort = sequence >> (magnitude-1);

                if (bit_pds_fort==1)
                        return sequence;
                else
                        return - (1  << magnitude) + sequence +1;
        }
        else
                return 0;
}


static uint32_t remplir_de_zeros (int32_t tab[64], uint32_t indice_dep, 
                                  uint32_t nombre) 
{
        //Cette fonction place nombre 0 dans tab, à partir de l'indice
        //indice_dep. Elle renvoie le premier indice qu'elle n'a pas
        //initialisé à 0

        uint32_t indice_fin = indice_dep + nombre;
        if (indice_fin> 64) {
                ERREUR1("ERREUR : plus de 64 coefficients dans un bloc\n");
                exit(EXIT_FAILURE);
        }
        
        for (uint32_t i = indice_dep; i < indice_fin ; i++)
                tab[i]=0;

        return indice_fin;
}



extern void unpack_block(struct bitstream *stream,
                         struct huff_table *table_DC, int32_t *pred_DC,
                         struct huff_table *table_AC,
                         int32_t bloc[64]) 
{

        uint8_t symbole;
        uint8_t magnitude, nb_zeros;
        uint32_t sequence; //sequence de bits                

        magnitude = next_huffman_value(table_DC,stream);
        sequence = lire(stream,magnitude);    
        bloc[0] = extraire_coefficient(magnitude,sequence) + *pred_DC;
        *pred_DC = bloc[0];    

        uint32_t case_courante=1;//indice de la case courante de bloc
        while (case_courante < 64) {                
                                        
                symbole = next_huffman_value(table_AC,stream);
                
                switch (symbole) {
                case 0x00 : //cas End Of Block
                        case_courante=remplir_de_zeros(bloc,case_courante,
                                                     64-case_courante);
                        break;

                case 0xF0: //cas 16 comosantes nulles
                        case_courante=remplir_de_zeros(bloc,case_courante,16);
                        break;
                                
                default:
                        nb_zeros=symbole>>4;                        
                        magnitude=0xf & symbole;

                        if (magnitude==0 || magnitude==11 ) { 
                                //ces cas sont invalides
                                ERREUR1("Erreur dans l'extraction des blocs\n");
                                exit(EXIT_FAILURE);
                        }

                        //On place nb_zeros 0 dans le tableau
                        case_courante=remplir_de_zeros(bloc,
                                                       case_courante,
                                                       nb_zeros);

                        //On place le coefficient dans bloc
                        sequence=lire(stream,magnitude);
                        bloc[case_courante] = extraire_coefficient(magnitude,
                                                                   sequence);

                        case_courante++;
                }
               
        }
       
}
