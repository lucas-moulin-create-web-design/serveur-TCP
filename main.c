#include "serveur.h"

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

    // Analyse des arguments en ligne de commande pour spécifié le port.
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
    printf("Attente d’une demande de connexion (quitter avec Ctrl-C)\n\n");
    
    fds[0].fd = socketEcoute; //initialisation de la structure fds.
    fds[0].events = POLLIN | POLLPRI;

    // boucle d’attente de connexion : en théorie, un serveur attend indéfiniment !
    while(1)
    {
        memset(messageEnvoi, 0x00, LG_MESSAGE*sizeof(char));
        memset(messageRecu, 0x00, LG_MESSAGE*sizeof(char));
        
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
    if (fds[0].revents &POLLIN) {
        // Accepte la connexion entrante
        longueurAdresse = sizeof(struct sockaddr_in);
        socketDialogue = accept(socketEcoute, (struct sockaddr *)&pointDeRencontreDistant, &longueurAdresse);

        if (socketDialogue < 0) {
            perror("accept");
            continue;
        }

        // Ajoute le nouveau client à la liste des clients connectés
        ajouter_client(&liste_clients, socketDialogue, pointDeRencontreDistant);
        TEST_AFFICHAGE_LISTE(liste_clients);

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
            print_message_reçu(&liste_clients, fds[i].fd, messageRecu);
            //printf("Message reçu de %s:%d : %s", inet_ntoa(liste_clients->pointDeRencontreDistant.sin_addr), ntohs(liste_clients->pointDeRencontreDistant.sin_port), messageRecu);
        }
    }
}

// Ferme le socket d'écoute
close(socketEcoute);
    return 0;
}