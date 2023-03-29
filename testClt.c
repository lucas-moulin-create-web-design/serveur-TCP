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
void menu(Matrix mat[][H], char messageEnvoi[], int LL, int HH);
void setPixel(char messageEnvoi[], char messageRecu[], int descripteurSocket, int LL, int HH);
void setPixelMan(char messageEnvoi[], char messageRecu[], int descripteurSocket, int L, int H);

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
    int choix=0;
    /*Fin des préliminaires : passons aux choses sérieuses*/
    while(1){       //"\n1. Actualiser la matrice\n2. Changer un pixel\n3. Version du protocol\nAutre. Quitter\n"
        menu(mat, messageEnvoi,L,H);
        scanf(" %d", &choix);
        switch (choix){
        case 1:
            sprintf(messageEnvoi,"/getMatrix");
            write(descripteurSocket,messageEnvoi,strlen(messageEnvoi));
            receveMatrix(mat, descripteurSocket, messageRecu);
            break;
        case 2:
            setPixel(messageEnvoi, messageRecu, descripteurSocket,L,H);
            break;
        
        default:
            close(descripteurSocket);
            printf("Déconexion\n");
            return 0;
            break;
        }
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
    system("clear");
    for(int i=0; i<L;++i){
        if(i<10){
            printf(" %d ",i);
        } else if(i<100){
            printf(" %d",i);
        } else{
            printf("%d", i);
        }
    }
    printf("\n");
    for(int c=0;c<H;++c){
        for(int l=0;l<L;++l){
            printf("\x1b[38;2;255;255;255;48;2;%d;%d;%dm   \x1b[0m", mat[l][c].R, mat[l][c].G, mat[l][c].B);
        }
        printf("  %d\n",c);
    }
}

void receveMatrix(Matrix mat[][H], int descripteurSocket, char messageRecu[]){
    char stryng[4];
    for(int l=0;l<L;++l){
        for(int h=0;h<H;++h){
            read(descripteurSocket, messageRecu, 4*sizeof(char));
            sscanf(messageRecu, "%c%c%c%c",&stryng[0], &stryng[1], &stryng[2], &stryng[3]);
            printf("%s\n",stryng);

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
              printf("%d %c\n",i,c);
            }

            // Extraire les valeurs RGB de la base 64
            mat[l][h].R = (decoded[0] << 2) | (decoded[1] >> 4);
            mat[l][h].G = ((decoded[1] & 0x0F) << 4) | (decoded[2] >> 2);
            mat[l][h].B = ((decoded[2] & 0x03) << 6) | decoded[3];
            //mat[l][h].R=(messageRecu[0] << 2) | (messageRecu[1] >> 4);
            //mat[l][h].G=((messageRecu[1] & 0x0F) << 4) | (messageRecu[2] >> 2);
            //mat[l][h].B=((messageRecu[2] & 0x03) << 6) | messageRecu[3];
        }
    }
}

void menu(Matrix mat[][H], char messageEnvoi[], int LL, int HH){
    afficherPanel(mat);
    for(int i=0;i<LL;++i){printf("===");}
    printf("\nCommandé : %s\n", messageEnvoi);
    for(int i=0;i<LL/2-1;++i){printf("===");}
    printf(" MENU ");
    for(int i=0;i<LL/2-1;++i){printf("===");}
    printf("\n1. Actualiser la matrice\n2. Changer un pixel (guidé)\n3. Changer un Pixel : <ligne> <colonne> <rouge>;<vert>;<bleu>\n4. \n5. Version du protocol\nAutre. Quitter\n");
    for(int i=0;i<LL;++i){printf("===");}
    printf("\nCommande : ");
}

void setPixel(char messageEnvoi[], char messageRecu[], int descripteurSocket, int L, int H){
    int x,y,r,g,b;
    while(1){
    printf("Colonne [0;%d] : ", H-1);
    scanf(" %d",&x);
    if(x>=0&&x<=L-1){break;}
    }
    while(1){
    printf("Ligne [0;%d] : ", L-1);
    scanf(" %d", &y);
    if(y>=0&&y<=H-1){break;}
    }

    while(1){
    printf("Rouge [0;255] : ");
    scanf(" %d", &r);
    if(r>=0&&r<=255){break;}
    }
    while(1){
        printf("Vert [0;255] : ");
    scanf(" %d", &g);
    if(g>=0&&g<=255){break;}
    }
    while(1){
        printf("Bleu [0;255] : ");
    scanf(" %d", &b);
    if(b>=0&&b<=255){break;}
    }
    
        int color = (r << 16) + (g << 8) + b;
        int c1 = (color>>18)&0x3F;
        int c2 = (color>>12)&0x3F;
        int c3 = (color>>6)&0x3F;
        int c4 = (color)&0x3F;
        sprintf(messageEnvoi, "/setPixel %dx%d %c%c%c%c",x,y,base64[c1],base64[c2],base64[c3],base64[c4]);
        write(descripteurSocket, messageEnvoi, strlen(messageEnvoi));
        read(descripteurSocket,messageRecu,2*sizeof(int));
}

void setPixelMan(char messageEnvoi[], char messageRecu[], int descripteurSocket, int L, int H){
    char str[50];
    int x,y,r,g,b;
    while(1){
        printf("<ligne> <colonne> <rouge>;<vert>;<bleu>\n");
        scanf(" %s", str);
        sscanf(str, "%d %d %d;%d;%d\0", &y,&x,&r,&g,&b);
        if((x>=0&&x<=L-1)&&(y>=0&&y<=H-1)&&(r>=0&&r<=255)&&(g>=0&&g<=255)&&(b>=0&&b<=255)){break;}
    }
    int color = (r << 16) + (g << 8) + b;
    int c1 = (color>>18)&0x3F;
    int c2 = (color>>12)&0x3F;
    int c3 = (color>>6)&0x3F;
    int c4 = (color)&0x3F;
    sprintf(messageEnvoi, "/setPixel %dx%d %c%c%c%c",x,y,base64[c1],base64[c2],base64[c3],base64[c4]);
    write(descripteurSocket, messageEnvoi, strlen(messageEnvoi));
    read(descripteurSocket,messageRecu,2*sizeof(int));
}