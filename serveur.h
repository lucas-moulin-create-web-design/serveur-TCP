#ifndef SERVEUR_H
#define SERVEUR_H

#include <stdio.h>
#include <stdlib.h> /* pour exit */
#include <unistd.h> /* pour close et sleep */
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h> /* pour memset */
#include <netinet/in.h> /* pour struct sockaddr_in */
#include <arpa/inet.h> /* pour htons et inet_aton */
#include <sys/poll.h>
#include <errno.h>

//Define général :

#define PORT IPPORT_USERRESERVED // = 5000
#define LG_MESSAGE 256
#define MAX_CLIENTS 10

//Structure client :

struct client {
    int socketDialogue;
    struct sockaddr_in pointDeRencontreDistant; //Pour avoir l'IP et le port source.
    struct client *suivant;
};

typedef struct Matrix{
    int R;
    int G;
    int B;
};

typedef struct client Client;

//Prototype des fonctions :

void ajouter_client(Client **liste_clients, int socketDialogue, struct sockaddr_in pointDeRencontreDistant);
void supprimer_client(Client **liste_clients, int socketDialogue);
void TEST_AFFICHAGE_LISTE(Client *liste_clients);
void print_message_reçu(Client **liste_clients, int socketDialogue, char *messageRecu, Matrix mat[][H]);
void reponse(Client *clients, int socketDialogue, char *messageRecu, Matrix mat[][H]);
void sendMatrix(char *messageEnvoi, int socketDialogue, Matrix mat[][H]);
int commande(char *tab);

#endif
