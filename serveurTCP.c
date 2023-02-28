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

#define PORT IPPORT_USERRESERVED // = 5000
#define LG_MESSAGE 256
#define MAX_CLIENTS 10

struct client {
    int socketDialogue;
    struct sockaddr_in pointDeRencontreDistant;
    struct client *suivant;
};

typedef struct client Client;

int ajouter_client(Client **liste_clients, int socketDialogue, struct sockaddr_in pointDeRencontreDistant) {
    Client *nouveau_client = (Client *) malloc(sizeof(Client));
    if (nouveau_client == NULL) {
        perror("malloc");
        exit(-1);
    }
    nouveau_client->socketDialogue = socketDialogue;
    nouveau_client->pointDeRencontreDistant = pointDeRencontreDistant;
    nouveau_client->suivant = *liste_clients;
    *liste_clients = nouveau_client;
    printf("Nouveau client connecté : %s:%d\n", inet_ntoa(pointDeRencontreDistant.sin_addr), ntohs(pointDeRencontreDistant.sin_port));
    return 0;
}

int supprimer_client(Client **liste_clients, int socketDialogue) {
    Client *client_courant = *liste_clients;
    Client *client_precedent = NULL;

    while (client_courant != NULL && client_courant->socketDialogue != socketDialogue) {
        client_precedent = client_courant;
        client_courant = client_courant->suivant;
    }

    if (client_courant == NULL) {
        return -1;
    }

    if (client_precedent == NULL) {
        *liste_clients = client_courant->suivant;
    } else {
        client_precedent->suivant = client_courant->suivant;
    }

    printf("Client déconnecté : %s:%d\n", inet_ntoa(client_courant->pointDeRencontreDistant.sin_addr), ntohs(client_courant->pointDeRencontreDistant.sin_port));
    close(client_courant->socketDialogue);
    free(client_courant);
    return 0;
}

int main(int argc, char *argv[]) {
    int socketEcoute;
    struct sockaddr_in pointDeRencontreLocal;
    socklen_t longueurAdresse;
    int socketDialogue;
    struct sockaddr_in pointDeRencontreDistant;
    char messageEnvoi[LG_MESSAGE]; /* le message de la couche Application ! */
    char messageRecu[LG_MESSAGE]; /* le message de la couche Application ! */
    int ecrits, lus; /* nb d’octets ecrits et lus */
    int retour;
    Client *liste_clients = NULL; // Liste chaînée de clients connectés
    int i, j;
    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 1; // le premier descripteur de fichier sera le socket d'écoute

    // Analyse des arguments en ligne de commande
    int port = PORT;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[i+1]);
            i++;
        }
    }

    // Crée un socket de communication
    socketEcoute = socket(PF_INET, SOCK_STREAM, 0); /* 0 indique que l’on utilisera le protocole par défaut associé à SOCK_STREAM soit TCP */

    // Teste la valeur renvoyée par l’appel système socket()
    if (socketEcoute < 0) { /* échec ? */
        perror("socket"); // Affiche le message d’erreur
        exit(-1); // On sort en indiquant un code erreur
    }

    printf("Socket créée avec succès ! (%d)\n", socketEcoute);

    // On prépare l’adresse d’attachement locale
    longueurAdresse = sizeof(struct sockaddr_in);
    memset(&pointDeRencontreLocal, 0x00, longueurAdresse);
    pointDeRencontreLocal.sin_family = PF_INET;
    pointDeRencontreLocal.sin_addr.s_addr = htonl(INADDR_ANY); // toutes les interfaces locales disponibles
    pointDeRencontreLocal.sin_port = htons(PORT);

    // On demande l’attachement local de la socket
    if((bind(socketEcoute, (struct sockaddr *)&pointDeRencontreLocal, longueurAdresse)) < 0)
    {
        perror("bind");
        exit(-2);
    }
    printf("Socket attachée avec succès !\n");

    // On fixe la taille de la file d’attente à 5 (pour les demandes de connexion non encore traitées)
    if(listen(socketEcoute, 5) < 0)
    {
        perror("listen");
        exit(-3);
    }
    printf("Socket placée en écoute passive sur le port %d ...\n", port);
    //--- Début de l’étape n°7 :
    // boucle d’attente de connexion : en théorie, un serveur attend indéfiniment !
    while(1)
    {
         // Attends qu'un événement se produise sur un des descripteurs de fichier surveillés
    if (poll(fds, nfds, -1) < 0) {
        if (errno == EINTR) {
            // L'appel a été interrompu par un signal, on recommence
            continue;
        } else {
            perror("poll");
            exit(-1);
        }
    }

    // Vérifie si le socket d'écoute a reçu une demande de connexion
    if (fds[0].revents & POLLIN) {
        // Accepte la connexion entrante
        longueurAdresse = sizeof(struct sockaddr_in);
        socketDialogue = accept(socketEcoute, (struct sockaddr *)&pointDeRencontreDistant, &longueurAdresse);

        if (socketDialogue < 0) {
            perror("accept");
            continue;
        }

        // Ajoute le nouveau client à la liste des clients connectés
        ajouter_client(&liste_clients, socketDialogue, pointDeRencontreDistant);

        // Ajoute le nouveau descripteur de fichier à la liste des descripteurs à surveiller
        fds[nfds].fd = socketDialogue;
        fds[nfds].events = POLLIN;
        nfds++;
    }

    // Vérifie si des données sont disponibles sur les descripteurs de fichier des clients connectés
    for (i = 1; i < nfds; i++) {
        if (fds[i].revents & POLLIN) {
            // Lit les données reçues
            lus = recv(fds[i].fd, messageRecu, LG_MESSAGE, 0);
            if (lus <= 0) {
                // Le client a fermé la connexion, on le supprime de la liste des clients connectés
                supprimer_client(&liste_clients, fds[i].fd);

                // Supprime le descripteur de fichier de la liste des descripteurs à surveiller
                fds[i] = fds[nfds-1];
                nfds--;
                i--;
                continue;
            }

            // Affiche le message reçu
            messageRecu[lus] = '\0';
            printf("Message reçu de %s:%d : %s", inet_ntoa(liste_clients->pointDeRencontreDistant.sin_addr), ntohs(liste_clients->pointDeRencontreDistant.sin_port), messageRecu);

            // Transmet le message à tous les autres clients connectés
            for (j = 1; j < nfds; j++) {
                if (j != i) {
                    ecrits = send(fds[j].fd, messageRecu, lus, 0);
                    if (ecrits < 0) {
                        perror("send");
                    }
                }
            }
        }
    }
}

// Ferme le socket d'écoute
close(socketEcoute);
    return 0;
    }
