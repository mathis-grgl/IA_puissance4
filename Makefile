# Makefile pour compiler du code C avec GCC

# Compilateur
CC = gcc

# Options du compilateur
CFLAGS = -Wall -Wextra -g

# Bibliothèques à lier
LDFLAGS = -lm

# Fichiers source
SRCS = jeu.c

# Fichiers objets
OBJS = $(SRCS:.c=.o)

# Exécutable
TARGET = jeu

# Cible par défaut
all:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

# Nettoyage des fichiers objets et de l'exécutable
clean:
	rm -f $(OBJS) $(TARGET)
