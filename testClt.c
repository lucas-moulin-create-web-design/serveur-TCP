#include <stdio.h>
#include <stdlib.h> /* pour exit */
#include <unistd.h> /* pour close */
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h> /* pour memset */
#include <netinet/in.h> /* pour struct sockaddr_in */
#include <arpa/inet.h>

#define LG_MESSAGE 256

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

int commande(char *tab);
void afficherPanel(Matrix mat[][H]);
void receveMatrix(Matrix mat[][H], int descripteurSocket, char messageRecu[]);

int main(){
    int descripteurSocket;
    struct sockaddr_in pointDeRencontreDistant;
    socklen_t longueurAdresse;
    char messageEnvoi[LG_MESSAGE];
    char messageRecu[LG_MESSAGE];
    int ecrits, lus;
    int retour;

    descripteurSocket = socket(PF_INET, SOCK_STREAM, 0);

    if(descripteurSocket<0){
        perror("socket");
        exit(-1);
    }
    printf("Socket créée avec succès ! (%d)\n", descripteurSocket);

    longueurAdresse=sizeof(pointDeRencontreDistant);
    memset(&pointDeRencontreDistant, 0x00, longueurAdresse);
    pointDeRencontreDistant.sin_family=PF_INET;
    pointDeRencontreDistant.sin_port=htons(IPPORT_USERRESERVED);
    inet_aton("127.0.0.1", &pointDeRencontreDistant.sin_addr);

    if((connect(descripteurSocket,(struct sockaddr *)&pointDeRencontreDistant,longueurAdresse))==-1){
        perror("connect");
        close(descripteurSocket);
        exit(-2);
    }
    printf("Connextion au serveur réussie avec succès !\n");

    //Etape 4
    memset(messageEnvoi, 0x00, LG_MESSAGE*sizeof(char));
    memset(messageRecu, 0x00, LG_MESSAGE*sizeof(char));
    
    /*Code de préliminaires : initialisation du tableau*/
    sprintf(messageEnvoi, "/getSizes");
    write(descripteurSocket, messageEnvoi, strlen(messageEnvoi));
    lus = read(descripteurSocket, messageRecu, LG_MESSAGE*sizeof(char));
    sscanf(messageRecu,"%dx%d", &L, &H);
    sprintf(messageEnvoi,"/getMatrix");
    write(descripteurSocket, messageEnvoi, strlen(messageEnvoi));
    Matrix mat[L][H];
    receveMatrix(mat, descripteurSocket, messageRecu);
    afficherPanel(mat);

    /*Fin des préliminaires : passons aux choses sérieuses*/

    switch(lus){
        case -1:
            perror("read");
            close(descripteurSocket);
            exit(-4);
        case 0:
            fprintf(stderr, "La socket a été fermée par le serv \n\n");
            close(descripteurSocket);
            return 0;
        default:
            printf("Confirmation de réception.\n");
            printf("%s\n", messageRecu);
    }

    return 0;
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

void afficherPanel(Matrix mat[][H]){
    for(int i=0; i<H;++i){
        if(i<10){
            printf(" %d ",i);
        } else if(i<100){
            printf(" %d",i);
        } else{
            printf("%d", i);
        }
    }
    printf("\n");
    for(int l=0;l<L;++l){
        for(int c=0;c<H;++c){
            printf("\x1b[38;2;255;255;255;48;2;%d;%d;%dm   \x1b[0m", mat[l][c].R, mat[l][c].G, mat[l][c].B);
        }
        printf("  %d\n",l);
    }
}

void receveMatrix(Matrix mat[][H], int descripteurSocket, char messageRecu[]){
    for(int l=0;l<L;++l){
        for(int h=0;h<H;++h){
            read(descripteurSocket, messageRecu, 4*sizeof(char));
            printf("%s\n", messageRecu);
            mat[l][h].R=(messageRecu[0] << 2) | (messageRecu[1] >> 4);
            printf("R: %d\n",mat[l][h].R);
            mat[l][h].G=((messageRecu[1] & 0x0F) << 4) | (messageRecu[2] >> 2);
            printf("G: %d\n",mat[l][h].G);
            mat[l][h].B=((messageRecu[2] & 0x03) << 6) | messageRecu[3];
            printf("B: %d\n",mat[l][h].B);
        }
    }
}