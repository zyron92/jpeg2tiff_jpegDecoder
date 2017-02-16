#include "huffman.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "debogage.h"

#define TAILLE_CODE_MAX 16 

//Définition des status de fonction
typedef enum statut statut;
enum statut{
	ERREUR,
	OK,
	SENTINELLE1 //##MODIF 
};

//Définition des types de noeud
typedef enum type_noeud type_noeud;
enum type_noeud{
        FEUILLE,
        INTERN,
        SENTINELLE2 //##MODIF 
};

//Définition d'une structure arbre de huffman
struct huff_table{
	type_noeud type;
        union{
            	//s'il s'agit d'un noeud intern:-
                //fils[0] -> fils gauche
                //fils[1] -> fils droite
                struct huff_table *fils[2];

                //s'il s'agit d'une feuille de l'arbre
                uint32_t valeur_feuille;    
        }element;
};

//Renvoie d'un nouveau noeud initialisé selon le type
//ou NULL si l'erreur
static struct huff_table *creer_noeud(type_noeud type, uint32_t valeur_feuille)
{
	struct huff_table *nouv_noeud = calloc(1,sizeof(struct huff_table));

	//Si on a une erreur d'allocation
	if(nouv_noeud == NULL){
		ERREUR1("Erreur : Erreur d'allocation.\n");
		return NULL;
	}

	//Le test de type du noeud & son initialisation
	switch(type){
	case FEUILLE:
		nouv_noeud->type=FEUILLE;
		(nouv_noeud->element).valeur_feuille=valeur_feuille;
		break;
	case INTERN:
		nouv_noeud->type=INTERN;
		(nouv_noeud->element).fils[0]=NULL;
		(nouv_noeud->element).fils[1]=NULL;
		break;
	default:
		ERREUR1("Erreur : Erreur de programme.\n");
		free(nouv_noeud);
		return NULL;
		break;
	}
	return nouv_noeud;
}

//Création des noeuds et ajout d'une feuille recursivement 
//dans l'arbre Huffman pour la profondeur donnée
//Requis un noeud intern initialisé au départ
static statut ajouter_feuille(uint32_t profondeur, uint32_t valeur_feuille, 
			      struct huff_table *arbre)
{
        //Si le noeud de départ est vide, on renvoie l'erreur
	if (arbre==NULL){
		return ERREUR;
	}

        //On arrive à une feuille où on ne peut plus insérer un fils
	//donc, on arrète la récursivité depuis le noeud courant
        if (arbre->type != INTERN){
                return ERREUR;
        }

	//On atteint la profondeur souhaitée pour insérer un fils
	if(profondeur==1){
                //On cherche un fils libre
                if((arbre->element).fils[0]==NULL){
                        (arbre->element).fils[0]=creer_noeud(FEUILLE, 
                                                             valeur_feuille);
                        return OK;
                }else if((arbre->element).fils[1]==NULL){
                        (arbre->element).fils[1]=creer_noeud(FEUILLE, 
                                                             valeur_feuille);
                        return OK;
                }else{
                        //ce noeud courant n'a pas de fils libres
                        return ERREUR;
                }
			 
		//On n'atteint pas encore la profondeur souhaitée	
	}else{
                //Si le fils de gauche n'existe pas encore, on le cree
		if((arbre->element).fils[0]==NULL){
			(arbre->element).fils[0]=creer_noeud(INTERN, 0);
		}
			
		//(a)On essaie d'insérer l'élément dans le fils gauche fils[0]
		//du noeud courant
		//s'il est de type INTERN (ayant des fils)
		//sinon on arrète la recursivité de côté gauche
		//avec un statut test_rempli erreur
		//(b)renvoie un statut test_rempli pour savoir 
                //si on a bien inséré la feuille
		statut test_rempli = ERREUR;
                test_rempli=ajouter_feuille(profondeur-1, valeur_feuille, 
                                            (arbre->element).fils[0]);

		//si on n'a pas encore inséré l'élément
		//(indiqué par le statut de test_rempli)
		//on refait la même chose que précédent mais,
		//cette fois-ci on regard celui de droite
		if (test_rempli==ERREUR){
			if((arbre->element).fils[1]==NULL){
				(arbre->element).fils[1]=creer_noeud(INTERN, 0);
			}
                        test_rempli=ajouter_feuille(profondeur-1, valeur_feuille, 
                                                    (arbre->element).fils[1]);
		}

		//Renvoie le statut test_rempli pour ce noeud courant
		return test_rempli;
	}
}

//Lecture des nombres des codes depuis le flux
//et les mettre dans le tableau
static statut lecture_tab_nombre_code(struct bitstream *stream, 
				      uint32_t tab_nombre_code[TAILLE_CODE_MAX], 
				      uint16_t *nb_byte_read){
	for(uint32_t i=0; i<TAILLE_CODE_MAX; i++){
		//Lecture depuis le flux pour les nombres des codes 
                //pour chaque longueur i+1
		uint8_t nb_bits_lu = 0;
		nb_bits_lu = read_bitstream(stream, 8, &(tab_nombre_code[i]), false);
		(*nb_byte_read)++;

                //Test s'il y a une erreur de lecture
		if (nb_bits_lu!=8){
			return ERREUR;
		}
	}
	return OK;
}

//Créer une table Huffman (un arbre de Huffman)
extern struct huff_table *load_huffman_table(struct bitstream *stream, 
					     uint16_t *nb_byte_read)
{
        //Initialisation & test préalable
        *nb_byte_read=0;
        if(stream==NULL){
		ERREUR1("Erreur : Erreur de lecture, pas de flux!\n");
		*nb_byte_read=-1;
		return NULL;
        }

	//## 1°) Création du tableau contenant les nombres des codes
	//pour chaque longueur

	statut statut_fonction = ERREUR;
	uint32_t tab_nombre_code[TAILLE_CODE_MAX];
	statut_fonction = lecture_tab_nombre_code(stream, tab_nombre_code, 
						  nb_byte_read);
        
	//Si on a une erreur pendant la lecture des nombres des codes
	if (statut_fonction == ERREUR){
		ERREUR1("Erreur : Erreur de lecture.\n");
		*nb_byte_read=-1;
		return NULL;
	}
        	
	//## 2°) Lecture des valeurs de feuille et 
	//création d'un arbre de Huffman

	//L'allocation de mémoire pour le noeud de départ
        struct huff_table *arbre_huff = creer_noeud(INTERN, 0);
        
        //Si on a une erreur d'allocation
	if(arbre_huff == NULL){
		ERREUR1("Erreur : Erreur de lecture.\n");
		*nb_byte_read=-1;
		return NULL;
	}
	
	//Lecture octet par octet décrivant un symbole ou un magnitude
	for(uint32_t i=0; i<TAILLE_CODE_MAX; i++){ 
		for(uint32_t j=0; j<tab_nombre_code[i]; j++){
			uint8_t nb_bits_lu=0;
			uint32_t octet_lu;
			nb_bits_lu = read_bitstream(stream, 8, 
						    &octet_lu, false);
			(*nb_byte_read)++;
		
			//S'il y a une erreur de lecture
			if (nb_bits_lu != 8){
				ERREUR1( "Erreur : Erreur de lecture.\n");
				free_huffman_table(arbre_huff);
				*nb_byte_read=-1;
				return NULL;
			}

			//On ajout une feuille à l'(i+1)-ième profondeur
			//dans l'arbre
                        statut statut_ajout = ERREUR;
			statut_ajout = ajouter_feuille(i+1, octet_lu, 
                                                       arbre_huff);

			//S'il y a une erreur d'ajout
			if(statut_ajout == ERREUR){
				ERREUR1("Erreur: Erreur d'ajout\
                                          d'une feuille dans l'arbre.\n");
				free_huffman_table(arbre_huff);
				*nb_byte_read=-1;
				return NULL;	
			}
		}
	}
	return arbre_huff;
}

//Renvoie la valeur décompréssé en utilisant la table Huffman (arbre Huffman) 
extern int8_t next_huffman_value(struct huff_table *table, 
                                 struct bitstream *stream)
{
        //Initialisation & test préalable
        if(stream==NULL){
		ERREUR1( "Erreur : Erreur de lecture, pas de flux!\n");
		exit(EXIT_FAILURE);
        }

        struct huff_table *position_courante = table;

	if (position_courante == NULL){
		ERREUR1( "Erreur : Erreur de lecture de l'arbre.\n");
		exit(EXIT_FAILURE);
	}

	//Tant qu'on n'arrive pas à une feuille
	//on lit le flux bit par bit pour s'avancer dans l'arbre
	while (position_courante->type == INTERN){
		uint8_t nb_bits_lu=0;
		uint32_t bit_lu;
		nb_bits_lu = read_bitstream(stream, 1, &bit_lu, true);
		
                //S'il y a une erreur de lecture
		if (nb_bits_lu != 1){
			ERREUR1( "Erreur : Erreur de lecture.\n");
			exit(EXIT_FAILURE);
		}

                //S'avancer dans l'arbre d'un bit lu
                struct huff_table *nouv_position=NULL;
                nouv_position = (position_courante->element).fils[bit_lu];
                if (nouv_position == NULL){
                        ERREUR1( "Erreur : Erreur de décodage.\n");
			exit(EXIT_FAILURE);
                }
		position_courante = nouv_position;
	}
	
	if (position_courante->type != FEUILLE){
		ERREUR1( "Erreur : Erreur de lecture de l'arbre.\n");
		exit(EXIT_FAILURE);
	}

	//Ici, on arrive à une feuille qui contient l'octet souhaité
	return (int8_t)(position_courante->element).valeur_feuille;
}

//Libérer récursivement chaque élément de la table (l'arbre)
extern void free_huffman_table(struct huff_table *table)
{
	if (table != NULL){
		if (table->type==INTERN){
			free_huffman_table((table->element).fils[0]);
			free_huffman_table((table->element).fils[1]);
		}
		//quelque soit le type, on libère l'arbre courant
		free(table);
	}
}
