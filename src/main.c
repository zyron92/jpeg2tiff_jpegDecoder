#include "bitstream.h"
#include "conv.h"
#include "huffman.h"
#include "idct.h"
#include "iqzz.h"
#include "tiff.h"
#include "unpack.h"
#include "upsampler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "debogage.h"

struct donnees_quanti {
        uint8_t indice;
        uint8_t table[64];
};

struct donnees_huff {
        uint8_t type, indice;
        struct huff_table *table;
};

struct donnees_composantes {
        //Donnees lues dans le section SOF
        uint8_t ic, sampling_h, sampling_v, iq;               
};
union donnees{
        struct donnees_huff h;
        struct donnees_quanti q;
        struct donnees_composantes c;
    
};
struct liste {
        union donnees d;
        struct liste *suiv;
};

static void traiter_arguments(int argc, char *argv[], 
                              char **nom_fic_entree, char **nom_fic_sortie)
{
	if ( argc > 3 || argc < 2)
	{
		ERREUR2("USAGE: %s nom_jpeg nom_tiff[optional]\n", 
			argv[0]);		
		exit(EXIT_FAILURE);
	}else{
		//## 1°) Nom de fichier d'entrée
		//on fait une recopie de pointeur seulement
		char *ptr_extension_entree = strrchr(argv[1], '.');
                if (ptr_extension_entree == NULL)
                {
                        ERREUR1("ERREUR : Extension de fichier d'entrée \
invalide, .jpg ou .jpeg attendu.\n");			
                        exit (EXIT_FAILURE);
                }

		if (strcmp(ptr_extension_entree,".jpg")!=0 && 
		    strcmp(ptr_extension_entree,".jpeg")!=0)
                {
                        ERREUR1("ERREUR : Extension de fichier d'entrée \
invalide, .jpg ou .jpeg attendu.\n");			
                        exit (EXIT_FAILURE);
                }
                *nom_fic_entree = argv[1];
	
		//## 2°) Nom de fichier de sortie
		//si le nom de fichier de sortie n'est pas précisé
		//on alloue un nom de fichier de sortie
		if ( argc == 3 ){
		  
                        *nom_fic_sortie = argv[2];
		}else{
                        char *nouv_nom;
                        if (strcmp(ptr_extension_entree,".jpg")==0){
                                nouv_nom = calloc(strlen(argv[1])+1+1, 
                                                  sizeof(char));
                        }else{//extension .jpeg
                                nouv_nom = calloc(strlen(argv[1])+1, 
                                                  sizeof(char));
                        }
                        //Une recopie du nom du fichier d'entrée
                        strncpy(nouv_nom, argv[1], strlen(argv[1]));
			
                        //Remplacement l'extension .jpg / .jpeg par .tiff
                        char *ptr_extension_sortie = strrchr(nouv_nom, '.');
                        memcpy(ptr_extension_sortie, ".tiff", 5);
                        nouv_nom[strlen(nouv_nom)]='\0';

                        *nom_fic_sortie = nouv_nom;
		}
		
	}
}

static uint32_t lire (struct bitstream *stream, 
                      uint8_t nb_bits, bool byte_stuffing){
        //Cette fonction lit nb_bits bits, vérifie le nombre de bits 
        //effectivement lus et renvoie les bits lus. 
        uint32_t dest=0;
        uint8_t verif=read_bitstream(stream,nb_bits,&dest,byte_stuffing);
                
        if (verif!=nb_bits){
                ERREUR2("On attendait %u bits, %u ont été lu\n", nb_bits, verif);		
        }
        return(dest);
}

static void verifier (uint32_t valeur, uint32_t valeur_attendue){
        if (valeur!=valeur_attendue){
                ERREUR2("On attendait %x, %x a été lue", valeur_attendue, valeur);
                exit(EXIT_FAILURE);
        }
}

static void verifier_alloc (void *ptr){
        if (ptr==NULL) {
                ERREUR1("Echec d'allocation");
                exit(EXIT_FAILURE);
        }
}

//les fonctions suivantes renvoient true si d contient l'indice souhaité
static bool trouv_h_dc (struct liste *l, uint8_t indice){
        return (l->d.h.type==0 && l->d.h.indice==indice);
}

static bool trouv_h_ac (struct liste *l, uint8_t indice){
        return (l->d.h.type==1 && l->d.h.indice==indice);
}

static bool trouv_q (struct liste *l, uint8_t indice){
        return (l->d.q.indice==indice);
}

static bool trouv_c (struct liste *l, uint8_t indice){
        return (l->d.c.ic==indice);
}

static struct liste *trouver_donnees (struct liste *l, uint8_t indice, 
                                    bool (*f) (struct liste *, uint8_t) ) 
//cette fonction retourne les donnees contenant 
//l'indice passé en paramètre
{
        while(l!=NULL && !f(l,indice))  
                l=l->suiv;

        if (l==NULL) {
                ERREUR1("Erreur : la table recherchée n'existe pas");
                exit(EXIT_FAILURE);
        }
        return l;
	
}

static struct liste *creer_liste() 
{
        struct liste *l=malloc(sizeof(struct liste));
        verifier_alloc(l);
        l->suiv=NULL;
        return l;
}


static void ajouter_liste(struct liste **l, union donnees d) 
{
        struct liste *l_suiv=creer_liste();
        l_suiv->d=d;
        l_suiv->suiv=*l;
        *l=l_suiv;        
}



static void liberer_liste (struct liste *liste, bool huff) 
//Cette fonction libère une liste, et les tables associées aux données
//contenues dans la liste pour les tables de Huffman
{        
        struct liste *cour=liste;
        struct liste *suiv;

        while (cour!=NULL) {
                if (huff)
                        free_huffman_table(cour->d.h.table);
                
                suiv=cour->suiv;
                free(cour);
                cour=suiv;
        }
}

                
static void traiter_APP0 (struct bitstream *stream) 
{
        uint16_t longueur=lire(stream,16,false) ;
        verifier(lire(stream,32,false),0x4a464946);
        for (int32_t i=0;i<(longueur-6) ;i++) {
                lire(stream,8,false);     
        }
}

static void traiter_COM (struct bitstream *stream) {
        uint16_t longueur=lire(stream,16,false) ;
        for (int32_t i=0;i<(longueur-2) ;i++) {
                lire(stream,8,false);    
        }
}

static struct liste *traiter_DQT (struct bitstream *stream, uint16_t *section)
//Cette fonction est appelée quand on a lu le marqueur DQT
//Elle retourne la liste des donnees_quanti associées à chaque composante
{ 
        struct liste *l=NULL;
        union donnees d;

        do {
                uint16_t longueur=lire(stream,16,false) ;
                for (uint32_t i=2; i<longueur;i++) {

                        uint8_t precision=lire(stream,4,false);
                        uint8_t indice=lire(stream,4,false);
			
			d.q.indice=indice;
			              
                        for (uint8_t j=0; j<64;j++) {
                                d.q.table[j]=lire(stream,8*(1+precision),false) ;   
			}
			i+=64*(1+precision);
                        ajouter_liste(&l,d);

                }
                *section=lire(stream,16,false);
		

        } while (*section==0xffdb);

        return l;
                  
}

static struct liste *traiter_SOF (struct bitstream *stream, 
                                  uint16_t *hauteur, uint16_t *largeur) 
//Cette fonction est appelée quand on a lu le marqueur SOF
//Elle retourne la liste des donnees associées à chaque composante
{
        uint16_t longueur=lire(stream,16,false) ;
        uint8_t precision=lire(stream,8,false);

        *hauteur= lire(stream,16,false);
        *largeur= lire(stream,16,false);

        uint8_t nb_composantes=lire(stream,precision,false);

        union donnees d;
	struct liste *l=NULL;
	
        for (uint32_t i=0; i<nb_composantes;i++) {
	  
	    
                d.c.ic=lire(stream,8,false);                
                d.c.sampling_h=lire(stream,4,false);
                d.c.sampling_v=lire(stream,4,false);                                                
                d.c.iq =lire(stream,8,false);
                ajouter_liste(&l,d);
	  
        }
	if (longueur!=8+3*nb_composantes)
                ERREUR1("erreur dans l'en-tête SOF");

	return l;
                  
}


static struct liste *traiter_Huff (struct bitstream *stream, uint16_t *section) 
//Cette fonction est appelée quand on a lu le marqueur DHT
//Elle retourne la liste des donnees associées à chaque table
{
	struct liste *l=NULL;
        union donnees d;
        uint16_t bytes_lus;

	do {
                uint16_t longueur=lire(stream,16,false) ;
                uint16_t total_bytes_lus=3;
		
                while(total_bytes_lus<longueur) {

                        lire(stream,3,false);
                        d.h.type=lire(stream,1,false);                        
                        d.h.indice=lire(stream,4,false);
			
                        d.h.table=load_huffman_table(stream, &bytes_lus);
                        verifier_alloc(d.h.table);
                        ajouter_liste(&l,d);

                        total_bytes_lus+=bytes_lus;
                }
              
                *section=lire(stream,16,false);

        } while (*section==0xffc4);
	return l;

}


static void lire_indices (struct bitstream *stream, uint8_t nb_comp,
                          uint8_t ic[], uint8_t ih_dc[], uint8_t ih_ac[]) 
//Cette fonction lit les indices dans le fichier (section SOS) et les range
//dans des tableaux
{        
	for (uint32_t i=0;i<nb_comp ; i++) { 

                ic[i]=lire(stream,8,true);
                ih_dc[i]=lire(stream,4,true);
                ih_ac[i]=lire(stream,4,true);
	}

}

static void calculer_tailles (uint8_t nb_composantes, 
                              struct liste *tables_c,uint8_t ic[],
                              uint8_t *taille_mcu_h, uint8_t *taille_mcu_v,
                              uint8_t * taille_mcu, uint8_t *nb_blocs,
                              uint8_t taille_comp_h[], uint8_t taille_comp_v[],
                              uint8_t taille_comp[]) 
{
        *taille_mcu_h =0;
        *taille_mcu_v =0;
        *nb_blocs=0;

        for (uint8_t i=0; i<nb_composantes;i++) {


                struct donnees_composantes c=trouver_donnees(
                        tables_c,ic[i], &trouv_c)->d.c;

                *taille_mcu_h = (*taille_mcu_h > c.sampling_h) ? 
                        *taille_mcu_h : c.sampling_h;
                *taille_mcu_v = (*taille_mcu_v > c.sampling_v) ? 
                        *taille_mcu_v : c.sampling_v;
                   
                taille_comp_h[i]=c.sampling_h;
                taille_comp_v[i]=c.sampling_v;
                taille_comp[i] = c.sampling_h*c.sampling_v;
                *nb_blocs+= c.sampling_h + c.sampling_v;
        }
        *taille_mcu=(*taille_mcu_v) * (*taille_mcu_h);
}

static void calculer_nb_mcu (uint32_t largeur, uint32_t hauteur, 
                             uint32_t taille_mcu_h, uint32_t taille_mcu_v,
                             uint32_t *nb_mcu_h, uint32_t *nb_mcu_v, 
                             uint32_t *nb_mcu)
//Cette fonction calcule le nombre de MCU dans l'image.
{
        *nb_mcu_h=(largeur%(8*taille_mcu_h)==0) ? largeur/(8*taille_mcu_h) 
                : largeur/(8*taille_mcu_h)+1;

        *nb_mcu_v=(hauteur%(8*taille_mcu_v)==0) ? hauteur/(8*taille_mcu_v) 
                : hauteur/(8*taille_mcu_v)+1;
        
        *nb_mcu= (*nb_mcu_h) * (*nb_mcu_v);
}

static uint8_t **concatener_blocs (uint8_t nb_comp, 
                                   uint8_t *taille, bool taille_fixe) 
//Cette foncation permet de réserver un tableau de nb_comp de taille taille
//taille est considéré comme une valeur si taille_fixe est vraie,
//comme  un tableau de valeurs sinon
{	
        uint8_t **tab=malloc(nb_comp*sizeof(uint8_t *));
        verifier_alloc(tab);
        for (uint32_t i=0;i<nb_comp ; i++) {     
                tab[i]=(taille_fixe) ? 
                        malloc(64*8**taille) : malloc(64*8*taille[i]);
                
                verifier_alloc(tab[i]);
        }
  
        return tab;
}

static void associer_tables(uint8_t nb_composantes, struct liste *tables_huff, 
                            struct liste *tables_composantes, 
                            struct liste *tables_quanti, uint8_t ih_dc[], 
                            uint8_t ih_ac[], uint8_t ic[], 
			    struct huff_table *huff_dc[], 
                            struct huff_table *huff_ac[], //uint8_t iq[], 
                            uint8_t *quanti [])
//Cette fonction range dans des tableaux les tables associées à chaque composante 
{ 
        uint8_t iq[nb_composantes];
        for (uint32_t i=0;i<nb_composantes ; i++) {
                huff_dc[i]= trouver_donnees(
                        tables_huff, ih_dc[i], &trouv_h_dc)->d.h.table;
                huff_ac[i]= trouver_donnees (
                        tables_huff, ih_ac[i], &trouv_h_ac)->d.h.table;
                iq[i]=trouver_donnees(
                        tables_composantes,ic[i], &trouv_c)->d.c.iq;
                quanti[i]=trouver_donnees(
                        tables_quanti, iq[i], &trouv_q)->d.q.table;
        }
}
static void liberer(struct liste *tables_composantes, 
                    struct liste *tables_quanti, struct liste *tables_huff, 
                    uint8_t nb_composantes, uint8_t **blocs_ech, 
                    uint8_t **blocs_concat, uint32_t *rgb,
                    struct bitstream *stream, int argc, 
                    char *fic_sortie) 
{
        if ( argc != 3 )
                free(fic_sortie);

        liberer_liste(tables_composantes,false);
        liberer_liste(tables_quanti,false);
        liberer_liste(tables_huff,true);
        for (uint16_t i=0; i<nb_composantes;i++) {
                free(blocs_ech[i]);
                free(blocs_concat[i]);
        }
        free (blocs_ech);
        free(blocs_concat);
        free(rgb);
  
        free_bitstream(stream);
}

int32_t main(int argc, char *argv[]) { 
        //Création du bitstream

	char *fic_entree;
	char *fic_sortie;
	traiter_arguments(argc, argv, &fic_entree, 
                          &fic_sortie);
	
	struct bitstream *flux;
        if ((flux=create_bitstream(fic_entree)) ==NULL)
                return 1;
	//On vérifie qu'on lit bien les bons marqueurs
        verifier(lire(flux,16,false),0xffd8);
        verifier(lire(flux,16,false),0xffe0);
        traiter_APP0(flux);
	uint16_t section=lire(flux,16,false);
	struct liste *tables_quanti;
	
	if (section==0xffdb)
                tables_quanti=traiter_DQT(flux,&section);	
	else {	
                verifier(section,0xfffe);
                traiter_COM(flux); 
                verifier(lire(flux,16,false),0xffdb);
                tables_quanti=traiter_DQT(flux,&section);
        }

        uint16_t hauteur,largeur;       
	verifier (section, 0xffc0);
        struct liste *tables_composantes=traiter_SOF(flux, &hauteur, &largeur);        
        verifier(lire(flux,16,false),0xffc4);    
	struct liste *tables_huff=traiter_Huff(flux,&section);
	verifier (section, 0xffda);

	uint16_t longueur=lire(flux,16,false) ;
	uint8_t nb_composantes=lire(flux,8,false) ;

	if (longueur!=2*nb_composantes+6 ||
            (nb_composantes != 3 && nb_composantes !=1))
                ERREUR1("erreur dans l'en-tête SOS\n");

        //Lecture des indices associés à chaque composante
        uint8_t ic[nb_composantes];
	uint8_t ih_dc[nb_composantes];
	uint8_t ih_ac[nb_composantes];
        lire_indices (flux, nb_composantes, ic, ih_dc, ih_ac);
	lire(flux,24,true); //On ignore les 3 octets suivants

        
        uint8_t taille_mcu_h, taille_mcu_v, taille_mcu;//tailles exprimées
        //en nombre de blocs
        uint8_t nb_blocs; //nombre de blocs total dans une MCU
        uint8_t taille_comp_h[nb_composantes];
        uint8_t taille_comp_v[nb_composantes];
        uint8_t taille_comp[nb_composantes]; //taille totale de chaque composante
        //en nombre de blocs

        calculer_tailles (nb_composantes, tables_composantes, ic, &taille_mcu_h, 
                          &taille_mcu_v, &taille_mcu, &nb_blocs, taille_comp_h,
                          taille_comp_v, taille_comp) ;         

        uint32_t nb_mcu_h, nb_mcu_v, nb_mcu;
        calculer_nb_mcu (largeur, hauteur,taille_mcu_h,taille_mcu_v,
                         &nb_mcu_h, &nb_mcu_v, &nb_mcu) ;

 
	int32_t bloc[nb_blocs][64]; //blocs bruts, tels qui sont extraits
	int32_t bloc_ordonne[nb_blocs][64]; //blocs réordonnés par iqzz
	uint8_t bloc_transform[nb_blocs][64]; //blocs apprès application de la 
        //transformée en cosinus discrète inverse
	
	uint8_t **blocs_concat=concatener_blocs(nb_composantes, taille_comp,false);
        //blocs concaténés par composante
	uint8_t **blocs_ech=concatener_blocs(nb_composantes, &taille_mcu,true);
        //blocs après l'étape upsampler

	uint32_t *rgb=malloc(taille_mcu_h*taille_mcu_v*64*32);
	verifier_alloc(rgb);

        int32_t pred_dc[nb_composantes];
        for (uint32_t i=0;i<nb_composantes;i++)
                pred_dc[i]=0;
        
        //Recherche des tables associées à chaque composante
        struct huff_table *huff_dc[nb_composantes];
        struct huff_table *huff_ac[nb_composantes];
        uint8_t *quanti[nb_composantes];
        associer_tables(nb_composantes, tables_huff, tables_composantes, 
                        tables_quanti, ih_dc, ih_ac, ic, huff_dc, huff_ac,
                        quanti ); 
       

        struct tiff_file_desc *tiff= init_tiff_file(fic_sortie, largeur,
                                                    hauteur,8*taille_mcu_v);

	for (uint32_t mcu=0; mcu<nb_mcu; mcu++) {
                //Pour chaque MCU
                int8_t num=0;
                for (uint32_t i=0;i<nb_composantes ; i++) {
                        //Pour chaque composante

                        for (uint8_t j=0;j< taille_comp[i];j++){
                                //Pour chaque bloc de la composante
                                unpack_block(flux,huff_dc[i],
                                             &(pred_dc[i]),huff_ac[i],bloc[num]);
                                iqzz_block(bloc[num],bloc_ordonne[num], quanti[i]);
                                idct_block(bloc_ordonne[num],bloc_transform[num]);                                        
                                num++;
                        }
                }
                
                //Remplissage de blocs_concat, avec les composantes 
                //dans le bon ordre
                uint32_t cour=0;
                for (uint32_t i=0;i<nb_composantes ; i++) {                        
                        for (uint32_t j=0; j<taille_comp[i];j++) {
                                for (uint32_t k=0; k<64;k++) {
                                        blocs_concat[ic[i]-1][j*64+k]=
                                                bloc_transform[cour][k];                                        
                                }
                                cour++;

                        }
                }
                for (uint32_t i=0;i<nb_composantes ; i++)
                        upsampler(blocs_concat[i],taille_comp_h[i],
                                  taille_comp_v[i],blocs_ech[i],
                                  taille_mcu_h,taille_mcu_v);
                 
		if (nb_composantes==3)
                        YCbCr_to_ARGB(blocs_ech,rgb,taille_mcu_h,taille_mcu_v);
		else
                        Y_to_ARGB(*blocs_ech,rgb,taille_mcu_h,taille_mcu_v);
               
                write_tiff_file(tiff, rgb,taille_mcu_h,taille_mcu_v);
	}

	//On avance jusqu'à la fin du fichier
        skip_bitstream_until(flux,0xff);
        verifier(lire(flux,16,false),0xffd9);

	if (!end_of_bitstream(flux))
                ERREUR1("Fin de l'image atteinte, pourtant le fichier \
 contient encore des octets\n");
	close_tiff_file(tiff);

	liberer(tables_composantes, tables_quanti, tables_huff, nb_composantes,
		blocs_ech, blocs_concat, rgb, flux, argc, fic_sortie);

        return 0;
}
