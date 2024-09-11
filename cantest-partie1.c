#define _GNU_SOURCE 
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h> 
#include <stdlib.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>


int fdSocketCAN;
struct sockaddr_can addr;
struct ifreq ifr;
struct can_frame frame;


int canTransmit(const char *message, int longueur);
int canReceive(void);


int main(int argc, char **argv)
{
    

	pid_t pid;
    char buf;

    unsigned char toucheLue = 'N';


    /*  Parametre du CAN utilise  */

    if(argc == 2) // si un argument est passé au programme, on l'assigne au nom da l'interface CAN à utiliser
		strcpy(ifr.ifr_name, argv[1]);
	else strcpy(ifr.ifr_name, "vcan0" ); // par défaut l'interface can0

    /*
	La première étape est de créer un socket. 
	Cette fonction accepte trois paramètres : 
		domaine/famille de protocoles (PF_CAN), 
		type de socket (raw ou datagram) et 
		protocole de socket. 
	la fonction retourne un descripteur de fichier.
	*/
	if ((fdSocketCAN = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) { // Création du socket CAN, de type RAW
		perror("Socket");
		return -1;
	}

	/*
	Ensuite, récupérer l'index de l'interface pour le nom de l'interface (can0, can1, vcan0, etc.) 
	que nous souhaitons utiliser. Envoyer un appel de contrôle d'entrée/sortie et 
	passer une structure ifreq contenant le nom de l'interface 
	*/
	

	ioctl(fdSocketCAN, SIOCGIFINDEX, &ifr);
	/*	Alternativement, zéro comme index d'interface, permet de récupérer les paquets de toutes les interfaces CAN.
	Avec l'index de l'interface, maintenant lier le socket à l'interface CAN
	*/

	memset(&addr, 0, sizeof(addr));
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(fdSocketCAN, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Bind");
		return -1;
	}
    
    struct can_filter rfilter[1]; // filtres pour 2 ID

    rfilter[0].can_id   = 0x903; 
	rfilter[0].can_mask = 0xFFF;
	rfilter[1].can_id   = 0x480;
	rfilter[1].can_mask = 0xFF0;


	setsockopt(fdSocketCAN, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));



    /*  Creation de deux processus  */

    pid = fork();
    if (pid == -1) { // Une erreur s'est produite
        perror("fork");
        return -1;
    }

    if (pid == 0) { // Code exécuté par le processus enfant
        while(1)
        {
            canReceive();
        }
        printf("Fin du processus enfant\n");
    } 
    else 
    {  // Code exécuté par le processus parent
        printf("Tapez:\n\r");
        printf("1: Pour Envoyer \" Salut\"\n");
        printf("2: Pour Envoyer \" Bonjour\"\n");
        printf("3: Pour Envoyer \" Felix\"\n");
        printf("\n\r");

        while(1)
        {
            toucheLue = getchar();
            switch(toucheLue)
            {
                case '1':
                    canTransmit("Salut", 5);
                break;

                case '2':
                    canTransmit("Bonjour", 7);
                break;

                case '3':
                    canTransmit("Felix", 5);
                break;

                default:
                break;
            }
            toucheLue = 'N';
        }
        printf("Fin du processus pere\n");



        wait(NULL);        // Attendre la fin du processus enfant
    }


    if (close(fdSocketCAN) < 0) {
		perror("Close");
		return -1;
	}

	return 0;
}




int canTransmit(const char *message, int longueur)
{
	/*
	Envoyer une trame CAN, initialiser une structure can_frame et la remplir avec des données. 
	La structure can_frame de base est définie dans include/linux/can.h  
	*/
	frame.can_id = 0x903;  	// identifiant CAN, exemple: 247 = 0x0F7
	frame.can_dlc = longueur;		// nombre d'octets de données
	sprintf(frame.data, message);  // données 
    printf("Message CAN Transmis par le père: %C\n", message);

	if (write(fdSocketCAN, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
		perror("Write");
		return -1;
	}
}


int canReceive(void)
{
    int nbytes, i;

    nbytes = read(fdSocketCAN, &frame, sizeof(struct can_frame));

	if (nbytes < 0) {
		perror("Read");
		return -1;
	}

	printf("0x%03X [%d] ",frame.can_id, frame.can_dlc);

	for (i = 0; i < frame.can_dlc; i++)
		printf("%c ",frame.data[i]);
        //      %02X


	printf("\r\n");

}