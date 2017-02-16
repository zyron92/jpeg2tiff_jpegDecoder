#Repertoires du projet
OBJ_DIR = ./bin
SRC_DIR = ./src
INC_DIR = ./include

# Options de compilation 
CC = gcc
LD = gcc
INC = -I$(INC_DIR)
CFLAGS = $(INC) -Wextra -Wall -std=c99 -O3 #-Wextra -g #-O3
LDFLAGS = -lm

# Liste des objets realises (nouveaux objets)
OBJ_FILES = $(OBJ_DIR)/conv.o $(OBJ_DIR)/idct.o $(OBJ_DIR)/iqzz.o
OBJ_FILES += $(OBJ_DIR)/bitstream.o $(OBJ_DIR)/unpack.o $(OBJ_DIR)/upsampler.o
OBJ_FILES += $(OBJ_DIR)/tiff.o $(OBJ_DIR)/huffman.o $(OBJ_DIR)/main.o

################################################################################

all : jpeg2tiff

# Génération d'un objet à partir d'un fichier source .c
$(OBJ_DIR)/main.o: $(SRC_DIR)/main.c
	$(CC) -c $(CFLAGS) $< -o $@ $(LDFLAGS)

# Génération d'un objet à partir d'un fichier specification .h 
# et d'un fichier source .c
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/%.h
	$(CC) -c $(CFLAGS) $< -o $@ $(LDFLAGS)

# Edition de lien des executables (PRINCIPALE)
jpeg2tiff: $(OBJ_FILES)
	$(LD) -o $@ $^ $(LDFLAGS)

################################################################################

# Génération d'un objet de test module
$(TEST_DIR)/%.o: $(TEST_DIR)/%.c
	$(CC) -c $(CFLAGS) $< -o $@ $(LDFLAGS)

################################################################################

# Nettoyage
clean: clean_temp
	rm -f jpeg2tiff $(OBJ_DIR)/*.o

clean_all: clean_temp clean

.PHONY: clean_temp
clean_temp:
	rm -f ./*/*~
	rm -f ./*~
