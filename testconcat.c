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
#include <time.h>

int PORT = 5000;
#define LG_MESSAGE 256
#define MAX_CLIENTS 10

char base64[] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'};

int L =80;
int H =40;
int pxMin =10;

struct Matrix{
    // Il faudra peut-être remplacer les "int" par des "short char"
    int R;
    int G;
    int B;
};
typedef struct Matrix Matrix;

typedef struct client Client;

struct client {
    time_t temps;
    int socketDialogue;
    int pxmin;
    struct sockaddr_in pointDeRencontreDistant; //Pour avoir l'IP et le port source.
    struct client *suivant;
} client;

void ajouter_client(Client **liste_clients, int socketDialogue, struct sockaddr_in pointDeRencontreDistant);
void supprimer_client(Client **liste_clients, int socketDialogue);
void TEST_AFFICHAGE_LISTE(Client *liste_clients);
void print_message_recu(Client **liste_clients, int socketDialogue, char *messageEnvoi, char *messageRecu, Matrix mat[][H]);
int reponse(Client *clients, int socketDialogue, char *messageEnvoi, char *messageRecu, Matrix mat[][H]);
void sendMatrix(char *messageEnvoi, int socketDialogue, Matrix mat[][H]);
int commande(char *tab);

void testEnvoi(Matrix mat, char *messageEnvoi, int socketDialogue);

void rafraichissementTemps(Client *liste_clients){
    while(liste_clients !=NULL){
        liste_clients->pxmin = pxMin;
        liste_clients=liste_clients->suivant;
    }
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
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
    int stock = 5000;
	int largeur = 12;
    int hauteur = 10;
	int pxmin = 10;
	char str2[]="  ";
	for(int i=1; i<argc-1;++i){
		strcpy(str2, argv[i]);
		if(str2[0]=='-' && str2[1]=='p'){
			if(atoi(argv[i+1])>0){
				stock = atoi(argv[i+1]);
			} else{
				printf("Erreur de port. -p <port>\tport € N*\n");
			}
		} else if(str2[0]=='-' && str2[1]=='s'){
			if(atoi(argv[i+1]) == 0 || atoi(argv[i+2]) == 0){
				printf("Erreur de dimention. -s <Largeur> <Hauteur>\t(Largeur,Hauteur) € (N*)²\n");
			} else{
				largeur = atoi(argv[i+1]);
				hauteur = atoi(argv[i+2]);
			}
		} else if(str2[0]=='-' && str2[1]=='l'){
			if(atoi(argv[i+1])>0){
				pxmin = atoi(argv[i+1]);
			}else{
				printf("Erreur du nombre de pixel(s) par minute. -l <px/min>\t(Largeur,Hauteur) € (N*)²\n");
			}
		}
	}
	PORT = stock;
	L = largeur;
	H = hauteur;
	pxMin = pxmin;
	printf("Utilisation du port : %d\n", PORT);
	printf("Dimensions : %d par %d\n", L, H);
	printf("Pixels par minute : %d\n", pxMin);
    int port = PORT;
    Matrix mat[L][H];
    for(int l=0;l<L;l++){
        for(int h=0;h<H;h++){
            mat[l][h].R=10*h;//rand()%256;
            mat[l][h].G=10*h;//rand()%256;
            mat[l][h].B=10*h;//rand()%256;
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
        time_t start_time = time(NULL);
        time_t current_time;
        current_time = time(NULL);
        if (current_time - start_time >= 60) {
            rafraichissementTemps(liste_clients);
            start_time = current_time;
        }
        
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
            print_message_recu(&liste_clients, fds[i].fd, messageEnvoi, messageRecu, mat);
            //printf("Message reçu de %s:%d : %s", inet_ntoa(liste_clients->pointDeRencontreDistant.sin_addr), ntohs(liste_clients->pointDeRencontreDistant.sin_port), messageRecu);
        }
    }
}

// Ferme le socket d'écoute
close(socketEcoute);
    return 0;
}

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
    nouveau_client->pxmin= pxMin;
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
/**/
void print_message_recu(Client **liste_clients, int socketDialogue, char *messageEnvoi, char *messageRecu, Matrix mat[][H]){
    Client *client_courant = *liste_clients;
    while(client_courant != NULL) {
        if(client_courant->socketDialogue == socketDialogue) {
            printf("\nMessage reçu de %s:%d : %s\n", inet_ntoa(client_courant->pointDeRencontreDistant.sin_addr), ntohs(client_courant->pointDeRencontreDistant.sin_port), messageRecu);
            reponse(client_courant, socketDialogue, messageEnvoi, messageRecu, mat);
            break;
        }
        client_courant = client_courant->suivant;
    }
}

int reponse(Client *clients, int socketDialogue, char *messageEnvoi, char *messageRecu, Matrix mat[][H]){
    int msg = commande(messageRecu);
    int ecrits;
    int ixe=0,igrec=0;
    char stryng[4] = "AAAA";
    switch(msg){
        case 1:
            sendMatrix(messageEnvoi, socketDialogue, mat);
            break;
        case 2:
            sprintf(messageEnvoi, "%dx%d\n", L, H);
            ecrits = write(socketDialogue, messageEnvoi, strlen(messageEnvoi));
            break;
        case 3:
            sprintf(messageEnvoi, "%d et il vous en reste %d\n", pxMin, clients->pxmin);
            ecrits = write(socketDialogue, messageEnvoi, strlen(messageEnvoi));
            break;
        case 4:
            sprintf(messageEnvoi, "\"/getVersion\" n'est pas encore développée...\n");
            ecrits = write(socketDialogue, messageEnvoi, strlen(messageEnvoi));
            break;
        case 5:
            //sprintf(messageEnvoi, "Il reste %ld secondes\n", difftime(time()));
            ecrits = write(socketDialogue, messageEnvoi, strlen(messageEnvoi));
            break;
        case 6:
            sscanf(messageRecu, "/setPixel %dx%d %c%c%c%c", &ixe, &igrec, &stryng[0], &stryng[1], &stryng[2], &stryng[3]);
            if(clients->pxmin<=0){
                sprintf(messageEnvoi,"Vous ne pouvez plus placer de pixels !");
                write(socketDialogue,messageEnvoi,strlen(messageEnvoi));
                break;
            }
            clients->pxmin--;
            //printf("%s, %d, %d\n",stryng, ixe,igrec);

            // Convertir la base 64 en valeurs RGB
            unsigned char decoded[4];
            for (int i = 0; i < 4; i++) {
              unsigned char c = stryng[i];
              if (c >= 'A' && c <= 'Z') {c -= 'A';}
              else if (c >= 'a' && c <= 'z') {c -= 'a' - 26;}
              else if (c >= '0' && c <= '9') {c -= '0' - 52;}
              else if (c == '+') {c = 62;}
              else if (c == '/') {c = 63;}
              else {
                // Caractère invalide dans la base 64
                // Gérer l'erreur ici
              }
              decoded[i] = c;
              //printf("%d %c\n",i,c);
            }

            // Extraire les valeurs RGB de la base 64
            mat[ixe][igrec].R = (decoded[0] << 2) | (decoded[1] >> 4);
            mat[ixe][igrec].G = ((decoded[1] & 0x0F) << 4) | (decoded[2] >> 2);
            mat[ixe][igrec].B = ((decoded[2] & 0x03) << 6) | decoded[3];
            sprintf(messageEnvoi, "00\n");
            write(socketDialogue, messageEnvoi, strlen(messageEnvoi));

            // Afficher les valeurs RGB pour le débogage
            //printf("r %d\n", mat[ixe][igrec].R);
            //printf("g %d\n", mat[ixe][igrec].G);
            //printf("b %d\n", mat[ixe][igrec].B);

            break;
        case 0:
            sprintf(messageEnvoi,"10\n");
            ecrits = write(socketDialogue, messageEnvoi, strlen(messageEnvoi));
            break;
    }
    return ecrits;
}

void sendMatrix(char *messageEnvoi, int socketDialogue, Matrix mat[][H]){
    for(int l=0;l<L;++l){
        for(int h=0;h<H;++h){
            printf("[%d;%d;",l,h);
            testEnvoi(mat[l][h], messageEnvoi, socketDialogue);
            printf("]");
        }
        printf("\n");
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
    } else if(strstr(tab, "/setPixel")!=NULL){
        return 6;
    } else{
        return 0;
    }
}

void testEnvoi(Matrix mat, char *messageEnvoi, int socketDialogue){
    int color = (mat.R << 16) + (mat.G << 8) + mat.B;
    int c1 = (color>>18)&0x3F;
    int c2 = (color>>12)&0x3F;
    int c3 = (color>>6)&0x3F;
    int c4 = (color)&0x3F;
    sprintf(messageEnvoi, "%c%c%c%c",base64[c1],base64[c2],base64[c3],base64[c4]);
    printf("%s",messageEnvoi);
    write(socketDialogue, messageEnvoi, strlen(messageEnvoi));
}