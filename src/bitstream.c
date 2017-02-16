#include "bitstream.h"
#include <stdio.h>
#include <stdlib.h>
#include "debogage.h"

struct bitstream  {
        FILE *file;
        uint8_t byte_cour; //octet courant 
        uint8_t byte_prec; //octet précédent
        uint8_t bit_cour; //indice (de 0,bit de poids fort, à 7) du bit courant
        int8_t fin; 
};

static void octet_suivant (struct bitstream *stream)
{
        stream->bit_cour=0;
        stream->byte_prec=stream->byte_cour;
        
        //lecture de l'octet suivant dans le fichier
        //fscanf renvoie EOF si la fin du fichier a été atteinte
        stream->fin=fscanf(stream->file,"%c",&(stream->byte_cour));        
}

static void bit_suivant (struct bitstream *stream)
{
        stream->bit_cour = stream->bit_cour+1;
        if (stream->bit_cour == 8) {
                octet_suivant(stream);
        }
        
}


static uint8_t extraire_bit (struct bitstream *stream)
{
        uint8_t res= stream->byte_cour; //On place l'octet courant dans res
        res = (res >> (7-stream->bit_cour)) & 0x01; //On isole le bit courant

        //On met à jour le flux
        bit_suivant(stream);
        return res;
}


extern bool end_of_bitstream(struct bitstream *stream) 
{        
        return (stream->fin==EOF);
}

extern struct bitstream *create_bitstream(const char *filename) 
{        
        //création du bitstream
        struct bitstream *flux=malloc(sizeof(struct bitstream));
        FILE *fichier = fopen(filename,"r");

        if (flux==NULL || fichier==NULL) {
                ERREUR1("Erreur dans la création du flux\n");
                return NULL;
        }
                
        flux->file=fichier;
        octet_suivant(flux); //lecture du premier octet        
        return flux;
}

extern uint8_t read_bitstream(struct bitstream *stream, 
			      uint8_t nb_bits, uint32_t *dest,
                              bool byte_stuffing) {
        //Cas ou on demande de lire plus de 32 bits
        if (nb_bits > 32) {
                ERREUR1("Erreur : tentative de lecture de plus de 32 bits\n");
                exit(EXIT_FAILURE);
        }

        
        *dest=0;        
        for (uint8_t i=0; i<nb_bits; i++) {  
                
                if (end_of_bitstream(stream))
                        //cas où on arrive à la fin du flux
                        return i;                
                else {
                        
                        //traitement du byte_stuffing
                        if (byte_stuffing 
                            && stream->byte_prec==0xff 
                            && stream->byte_cour==0x00) {
                                
                                octet_suivant(stream);
                        }                                
                        
                        *dest = (*dest << 1) | extraire_bit(stream);
                }                        
        }        
        return nb_bits;  
}

extern void skip_bitstream_until(struct bitstream *stream,
				 uint8_t byte) {
 
        //Si on n'est pas au début d'un octet, on passe à l'octet suivant
        if (stream->bit_cour!=0)
                octet_suivant(stream);        

        while (! end_of_bitstream(stream) && (stream->byte_cour !=byte) ) {
                octet_suivant(stream);           
        }

}

extern void free_bitstream(struct bitstream *stream) {
        
        fclose(stream->file);
        free(stream);
}

