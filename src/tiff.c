#include "tiff.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "debogage.h"

struct tiff_file_desc
{
	FILE *flux_tiff;            //pointeur vers le fichier tiff
	uint32_t identifiant;       //identifier les mcus
	uint32_t identifiant_bande; //identifier les bandes
	uint32_t largeur;
	uint32_t hauteur;
	uint32_t bande_par_image;
	uint16_t endianness;        //codé sur deux octects
	uint32_t *tableau_bande;    //tableau contenant les données d'une bande 
};

//ecrire deux octets dans le fichier tiff
static void ecrire_16_bits(uint16_t valeur_16, FILE *fichier)
{
	//extraction de l'octet de poids fort
	int8_t octet_fort = valeur_16 >> 8 ;
	fwrite(&octet_fort, sizeof(int8_t), 1, fichier);
	//extraction de l'octet de poids faible
	int8_t octet_faible = (uint8_t)(valeur_16 | 0x0000);
	fwrite(&octet_faible, sizeof(int8_t), 1, fichier);
}

//ecrire 4 octets dans le fichier tiff
static void ecrire_32_bits(uint32_t valeur_32, FILE *fichier)
{
	//extraction des deux octects de poids fort
	int16_t valeur_16_fort = valeur_32 >> 16 ;
	//extraction des deux octets forts
	ecrire_16_bits(valeur_16_fort, fichier);
	//extraction des deux octects de poids faible
	int16_t valeur_16_faible = (uint16_t)(valeur_32 | 0x00000000);
	//extraction des deux octets faibles
	ecrire_16_bits(valeur_16_faible, fichier);
}

//ecrire en une fois toutes les donnees d'un tag
static void entree_ifd(uint16_t code_tag, uint16_t type_tag, FILE *fichier,
		  uint32_t nb_valeur, uint32_t valeur)
{
	ecrire_16_bits(code_tag, fichier);
	ecrire_16_bits(type_tag, fichier);
	ecrire_32_bits(nb_valeur, fichier);
	ecrire_32_bits(valeur, fichier);	
}


//ecrire l'entete du fichier tiff, et initialiser la structure
extern struct tiff_file_desc *init_tiff_file (const char *file_name,
                                              uint32_t width,
                                              uint32_t height,
                                              uint32_t row_per_strip)
{
	struct tiff_file_desc *tfd_init = malloc(sizeof(struct tiff_file_desc));
	FILE *fichier = NULL;
	fichier = fopen(file_name, "wb");
        if (fichier == NULL){
                ERREUR1( "ERREUR: Fichier tiff non-ouvert\n"); 
                exit(EXIT_FAILURE);
        }

//header
	//endianness BE
	ecrire_16_bits(0x4d4d, fichier);
	// Identification du TIFF
	ecrire_16_bits(0x002a, fichier);
	// pointeur sur le premie IFD
	ecrire_32_bits(0x00000008, fichier);

//IFD
	//nombre d'entree N
	ecrire_16_bits(0x000c, fichier);
	//tag1 : image width(nombre de colonne de l'image)
	entree_ifd(0x0100, 0x0004, fichier, 0x00000001, width);
	//tag2 : image length(nombre de ligne de l'image)
	entree_ifd(0x0101, 0x0004, fichier, 0x00000001, height);
	//tag3 : bits per sample(taille d'une composante couleur)
	entree_ifd(0x0102, 0x0003, fichier, 0x00000003, 0x0000009e);
	//tag4 : compression
	entree_ifd(0x0103, 0x0003, fichier, 0x00000001, 0x00010000);
	//tag5 : photometricinterpretation
	entree_ifd(0x0106, 0x0003, fichier, 0x00000001, 0x00020000);
	//tag6 : stripoffsets
	//nombre de bande par image
	uint32_t strip_per_image = ceil((float)height / (float)row_per_strip);
	if (strip_per_image == 1){
		entree_ifd(0x0111, 0x0003, fichier, 0x00000001, 0x00b40000);	
	}else if (strip_per_image == 2){
		entree_ifd(0x0111, 0x0003, fichier, 0x00000002, 0x00b40174);
	}else{
		entree_ifd(0x0111, 0x0004, fichier, strip_per_image, 0x000000b4);
	}
	//tag7 : sampleserixels
	entree_ifd(0x0115, 0x0004, fichier, 0x00000001, 0x00000003);
	//tag8 : rowserstrip
	entree_ifd(0x0116, 0x0004, fichier, 0x00000001, row_per_strip);
	//tag9 : stribytecounts
	uint32_t samples_per_pixels = 3; // trois composante couleurs
	//strip size : taille d'une bande
	uint32_t strip_size = samples_per_pixels * width * row_per_strip ;
	if (strip_per_image == 1){
		entree_ifd(0x0117, 0x0004, fichier, 0x00000001, strip_size);	
	}else if (strip_per_image == 2){
		entree_ifd(0x0117, 0x0003, fichier, 0x00000002, 
			   strip_size + (strip_size << 16));
	}else{
		uint32_t first_free_adress = 4 * strip_per_image + 0x000000b4;
		entree_ifd(0x0117, 0x0004, fichier, strip_per_image, 
			   first_free_adress);
	}
	//tag10 : xresolution
	entree_ifd(0x011a, 0x0005, fichier, 0x00000001, 0x000000a4);
	//tag11 : yresolution
	entree_ifd(0x011b, 0x0005, fichier, 0x00000001, 0x000000ac);
	//tag12 : resolution unit
	entree_ifd(0x0128, 0x0003, fichier, 0x00000001, 0x00020000);
	//Offset suivant
	ecrire_32_bits(0x00000000, fichier);
	//BitsPersamles
	ecrire_16_bits(0x0008, fichier);
	ecrire_16_bits(0x0008, fichier);
	ecrire_16_bits(0x0008, fichier);
	//Xresolution
	ecrire_32_bits(0x00000064, fichier);
	ecrire_32_bits(0x00000001, fichier);
	//Yresolution
	ecrire_32_bits(0x00000064, fichier);
	ecrire_32_bits(0x00000001, fichier);
	if(strip_per_image > 2){
		//mettre les offsets des bandes
		uint32_t adress_strip_i =  0x000000b4 + 2*4*strip_per_image;
		for(uint32_t i=0; i<strip_per_image; i++){
			ecrire_32_bits(adress_strip_i, fichier);
			adress_strip_i += strip_size;
		};
		
		for(uint32_t i=0; i<strip_per_image-1; i++){
			ecrire_32_bits(strip_size, fichier);
		}
		//hauteur de la dernière bande
		uint32_t hauteur_bande_courante = height -
			((strip_per_image-1)*row_per_strip);
		strip_size = 3*hauteur_bande_courante*width;
		ecrire_32_bits(strip_size, fichier);
	}

//initialisation de notre structure
	tfd_init->flux_tiff = fichier;
	tfd_init->identifiant = 0;
	tfd_init->identifiant_bande = 0;
	tfd_init->largeur = width;
	tfd_init->hauteur = height;
	tfd_init->bande_par_image = row_per_strip; 
	tfd_init->endianness = 0x4d4d;
        tfd_init->tableau_bande = calloc(row_per_strip * width, sizeof(uint32_t));
	
	return tfd_init;
}


//extraire la couleur rouge de l'entier representant les couleurs
static uint8_t contribution_rouge(uint32_t couleur)
{
	uint16_t valeur_fort_couleur = couleur >> 16 ;
	uint8_t composante_rouge = (uint8_t)(valeur_fort_couleur | 0x0000);
	return composante_rouge;
}

//extraire la couleur verte
static uint8_t contribution_vert(uint32_t couleur)
{
	uint16_t valeur_faible_couleur = (uint16_t)(couleur | 0x00000000);
	uint8_t composante_verte = valeur_faible_couleur >> 8 ;
	return composante_verte;
}

//extraire la composante bleue
static uint8_t contribution_bleue(uint32_t couleur)
{
	uint16_t valeur_faible_couleur = (uint16_t)(couleur | 0x00000000);
	uint8_t composante_bleue = (uint8_t)(valeur_faible_couleur | 0x0000);
	return composante_bleue;
}

static uint32_t conversion(int32_t i, int32_t j, int32_t largeur)
{
	//un parcours gauche à droite, ligne par ligne
	return largeur*i+j;
}

static void decalage(uint32_t *tab_dest, uint32_t *tab_src, 
		     int32_t largeur_dest, int32_t largeur_src,
		     int32_t hauteur_dest, int32_t hauteur_src, 
		     int32_t decalage_horizontal)
{
	for (int32_t i=0; i<hauteur_src; i++ ){
		if(i>=hauteur_dest){
			break; //on sort de toute boucle 
		}
		for (int32_t j=0; j<largeur_src; j++){
			if ((j+decalage_horizontal)>=largeur_dest){
				break; //on sort de cette boucle imbriquee
			}
			uint32_t indice_dest = conversion(i,
							  j+decalage_horizontal,
							  largeur_dest);
			tab_dest[indice_dest] = 
				tab_src[conversion(i,j,largeur_src)];
		}
	}
}

//permet d'écrire une bande dans le fichier tiff
static void write_strip_tiff_file(struct tiff_file_desc *tfd, 
				  uint32_t hauteur_bande_courante)
{
	for (uint32_t i=0; i<hauteur_bande_courante; i++){
		for (uint32_t j=0; j<tfd->largeur; j++){
			uint8_t rouge = contribution_rouge
				((tfd->tableau_bande)
				 [conversion(i,j,tfd->largeur)]);
			fwrite(&rouge, sizeof(int8_t), 1, tfd->flux_tiff);
			uint8_t vert = contribution_vert
				((tfd->tableau_bande)
				 [conversion(i,j,tfd->largeur)]);
			fwrite(&vert, sizeof(int8_t), 1, tfd->flux_tiff);
			uint8_t bleue = contribution_bleue
				((tfd->tableau_bande)
				 [conversion(i,j,tfd->largeur)]);
			fwrite(&bleue, sizeof(int8_t), 1, tfd->flux_tiff);
		}
	}
}


//procedure principale du fichier tiff
/*permet de d'intgrer une mcu dans la structure
  pour que'elle soit ecrite apres dans le fichier tiff*/
extern int32_t write_tiff_file (struct tiff_file_desc *tfd,
                                uint32_t *mcu_rgb,
                                uint8_t nb_blocks_h,
                                uint8_t nb_blocks_v)
{
        if (mcu_rgb==NULL || tfd==NULL){
                ERREUR1( "ERREUR tiff.c L252 : erreur de(s) paramètre(s)\
.\n");
                exit(EXIT_FAILURE);
        }

	// nombre de mcu horizontales par ligne
	uint32_t nb_mcu_h_per_strip = ceil
		((float)(tfd->largeur)/(float)(8*nb_blocks_h)) ;
	
	// nombre de bandes par image
	uint32_t nb_strip = ceil
		((float)(tfd->hauteur)/(float)(8*nb_blocks_v));

	// decalage horizontal
	uint32_t decalage_h = 8*nb_blocks_h*(tfd->identifiant);

	// si on n'est pas sur la derniere bande 
	uint32_t hauteur_bande_courante = tfd->bande_par_image;
	
	// traitement de la derniere bande
	if (tfd->identifiant_bande == nb_strip-1){
		hauteur_bande_courante = 
			(tfd->hauteur)-((nb_strip-1)*8*nb_blocks_v);
	}

	decalage(tfd->tableau_bande, mcu_rgb, tfd->largeur, 8*nb_blocks_h, 
		 hauteur_bande_courante, 8*nb_blocks_v, decalage_h);
	
	(tfd->identifiant)++;

	if (tfd->identifiant == nb_mcu_h_per_strip){
		write_strip_tiff_file(tfd, hauteur_bande_courante);
		tfd->identifiant = 0;
		(tfd->identifiant_bande)++;
	}
	
	return 0;
	
}

//liberer l'espace memoire alloué par la structure
extern int32_t close_tiff_file(struct tiff_file_desc *tfd)
{
	if (tfd != NULL){
		if (tfd->tableau_bande != NULL){
			free(tfd->tableau_bande);
		}
		fclose(tfd->flux_tiff);
		free(tfd);
	}
	return 0;
}
