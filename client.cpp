#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

extern int errno;

int port;

int main(int argc, char *argv[])
{
	int sd;
	struct sockaddr_in server;
	char mesaj[5000];

	if(argc != 3)
	{
		printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
		return -1;
	}

	port = atoi(argv[2]);

	if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("Eroare la socket().\n");
      return errno;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(port);

    if(connect(sd, (struct sockaddr *) &server, sizeof(struct sockaddr)) == -1)
    {
    	perror("Client: Eroare la connect().");
    	return errno;
    }

    char info[300];
    if(read(sd, info, 300) < 0)
	    {
	    	perror("Client: Eroare la read() spre server.\n");
	    	return errno;
	    }
	  printf("%s\n",info);

    while(1)
    {
	    bzero(mesaj, 300);
	    printf("Client: Introduceti o comanda: ");
	    fflush(stdout);
	    read(0, mesaj, 300);

	    if(write(sd, mesaj, 300) <= 0)
	    {
	    	perror("Client: Eroare la write() spre server.\n");
	    	return errno;
	    }

	    if(read(sd, mesaj, 5000) < 0)
	    {
	    	perror("Client: Eroare la read() spre server.\n");
	    	return errno;
	    }

	    printf("Client: Mesajul primit este: \n");
	    printf("\033[0;31m%s\n", mesaj);
	    printf("\033[0m");

	    if(strcmp(mesaj, "Vei fi deconectat") == 0)
	    	break;

	}
    close(sd);
}