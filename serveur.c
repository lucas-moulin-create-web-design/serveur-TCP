#include"serveur.h"

/*Fonction permettant l'ajout d'un nouveau client à la liste chainée lors de la demande de connexion */

void ajouter_client(Client **liste_clients, int socketDialogue, struct sockaddr_in pointDeRencontreDistant) {
    int numero_client=socketDialogue-3;
    Client *nouveau_client = (Client *) malloc(sizeof(Client));
    if (nouveau_client == NULL) {
        perror("malloc");
        exit(-1);
    }
    nouveau_client->socketDialogue = socketDialogue;
    nouveau_client->pointDeRencontreDistant = pointDeRencontreDistant;
    nouveau_client->suivant = *liste_clients;
    *liste_clients = nouveau_client;
    printf("\nNouveau client[%i] connecté : %s:%d\n", numero_client, inet_ntoa(pointDeRencontreDistant.sin_addr), ntohs(pointDeRencontreDistant.sin_port));
}

/*Fonction permettant la suppression d'un client de la liste chainée lors de la déconnexion */

void supprimer_client(Client **liste_clients, int socketDialogue) {
    Client *client_courant = *liste_clients;
    Client *client_precedent = NULL;
    int numero_client=socketDialogue-3;

    while (client_courant != NULL && client_courant->socketDialogue != socketDialogue) {
        client_precedent = client_courant;
        client_courant = client_courant->suivant;
    }

    if (client_courant == NULL) {
        printf("client_courant = NULL");
        exit(0);
    }

    if (client_precedent == NULL) {
        *liste_clients = client_courant->suivant;
    } else {
        client_precedent->suivant = client_courant->suivant;
    }

    printf("Client[%i] déconnecté : %s:%d\n", numero_client, inet_ntoa(client_courant->pointDeRencontreDistant.sin_addr), ntohs(client_courant->pointDeRencontreDistant.sin_port));
    close(client_courant->socketDialogue);
    free(client_courant);
}

/*Fonction permettant l'affichage de la liste chainée. Cette fonction est appelé à chaque nouvel ajout d'un client dans la liste chainée*/

void TEST_AFFICHAGE_LISTE(Client *liste_clients){
    printf("\n--------------------------------------------------------------------------------\n");
    while(liste_clients !=NULL){
        printf("socket dialogue: %d adresseIP: %s port source : %d ", liste_clients->socketDialogue, inet_ntoa(liste_clients->pointDeRencontreDistant.sin_addr), ntohs(liste_clients->pointDeRencontreDistant.sin_port));
        liste_clients=liste_clients->suivant;
        printf("\n--------------------------------------------------------------------------------\n");
    }
}

/*Fonction permettant l'affichage du message reçu par le serveur en précisant l'adresse et le port source du client*/

void print_message_reçu(Client **liste_clients, int socketDialogue, char *messageRecu, Matrix mat[][H]){
    Client *client_courant = *liste_clients;
    while(client_courant != NULL) {
        if(client_courant->socketDialogue == socketDialogue) {
            printf("\nMessage reçu de %s:%d : %s\n", inet_ntoa(client_courant->pointDeRencontreDistant.sin_addr), ntohs(client_courant->pointDeRencontreDistant.sin_port), messageRecu);
            reponse(client_courant, messageRecu);
            break;
        }
        client_courant = client_courant->suivant;
    }
}

void reponse(Client *clients, int socketDialogue, char *messageRecu, Matrix mat[][H]){
    int msg = commande(messageRecu);
    switch(msg){
        case 1:
            SendMatrix(messageEnvoi, socketDialogue, mat);
            break;
        case 2:
            sprintf(messageEnvoi, "%dx%d\n", L, H);
            ecrits = write(socketDialogue, messageEnvoi, strlen(messageEnvoi));
            break;
        case 3:
            sprintf(messageEnvoi, "%d\n", pxMin);
            ecrits = write(socketDialogue, messageEnvoi, strlen(messageEnvoi));
            break;
        case 4:
            sprintf(messageEnvoi, "\"/getVersion\" n'est pas encore développée...\n");
            ecrits = write(socketDialogue, messageEnvoi, strlen(messageEnvoi));
            break;
        case 5:
            sprintf(messageEnvoi, "\"/getWaitTime\" n'est pas encore développée...\n");
            ecrits = write(socketDialogue, messageEnvoi, strlen(messageEnvoi));
            break;
        case 0:
            sprintf(messageEnvoi,"Commande invalide !\n");
            ecrits = write(socketDialogue, messageEnvoi, strlen(messageEnvoi));
            break;
    }
}

void sendMatrix(char *messageEnvoi, int socketDialogue, Matrix mat[][H]){
    for(int h=0;h<H;++h){
        for(int l=0;l<L;++l){
            sprintf(messageEnvoi,"0x%x",mat[l][h].R);
            write(socketDialogue, messageEnvoi, strlen(messageEnvoi));
            sprintf(messageEnvoi,"0x%x",mat[l][h].G);
            write(socketDialogue, messageEnvoi, strlen(messageEnvoi));
            sprintf(messageEnvoi,"0x%x",mat[l][h].B);
            write(socketDialogue, messageEnvoi, strlen(messageEnvoi));
        }
    }
}

int commande(char *tab){
    if(strstr(tab, "/getMatrix")!=NULL){
        return 1;
    } else if(strstr(tab, "/getSize")!=NULL){
        return 2;
    } else if(strstr(tab, "/getLimits")!=NULL){
        return 3;
    } else if(strstr(tab, "/getVersion")!=NULL){
        return 4;
    } else if(strstr(tab, "/getWaitTime")!=NULL){
        return 5;
    } else{
        return 0;
    }
}

