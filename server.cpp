#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include "tinyxml2.cpp"
#include <time.h>
#include <pthread.h>
#include <signal.h>

#define size 5000

using namespace tinyxml2;

int msg(void *);
static void *treat(void *);

char fisier[30];

extern int errno;

typedef struct thData{
	int idThread;
	int cl;
}thData;

int main(int argc, char *argv[])
{
	struct sockaddr_in server;   //structura folosita de server
	struct sockaddr_in from;
	int sd, client, optval = 1;
	pthread_t th[100];   //identificatorii thread-urilor care se vor crea
	int i = 0;
	socklen_t length;
	char mesaj[300];

	if(argc != 2)
	{
		printf("Sintaxa: %s <port>\n", argv[0]);
		return -1;
	}

	int port = atoi(argv[1]);

	if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Server: Eroare la socket(). \n");
		return errno;
	}
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	bzero(&server, sizeof(server));
	bzero(&from, sizeof(from));

	//umplem structura folosita de server
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(port);

	//atasam socket-ul
	if(bind(sd, (struct sockaddr *) &server, sizeof(struct sockaddr)) == -1)
	{
		perror("Server: Eroare la bind(). \n");
		return errno;
	}

	//server-ul asculta daca vin clienti sa se conecteze
	if(listen(sd, 2) == -1)
	{
		perror("Server: Eroare la listen(). \n");
		return errno;
	}

	//copiez in fisierul azi.xml datele trenurilor care circula in aceasta zi (in functie de localtime)
	size_t t;
	time_t tm = time(NULL);
	struct tm * timeinfo;
	char buffer[32];
	timeinfo = localtime(&tm);
	t = strftime(buffer, 32, "%A", timeinfo);
	buffer[t] = '\0';
 	
 	strcpy(fisier, "xml/");
	if(strcmp(buffer, "Monday") == 0) strcat(fisier, "luni");
	else if(strcmp(buffer, "Tuesday") == 0) strcat(fisier, "marti");
	else if(strcmp(buffer, "Wednesday") == 0) strcat(fisier, "marti");
	else if(strcmp(buffer, "Thursday") == 0) strcat(fisier, "joi");
	else if(strcmp(buffer, "Friday") == 0) strcat(fisier, "vineri");
	else if(strcmp(buffer, "Saturday") == 0) strcat(fisier, "sambata");
	else if(strcmp(buffer, "Sunday") == 0) strcat(fisier, "duminica");
	strcat(fisier, ".xml");

	FILE *fd1, *fd2;
	fd1 = fopen(fisier,"r");
	fd2 = fopen("xml/azi.xml", "w");
	char c = fgetc(fd1);
	while(c != EOF)
	{
		fputc(c, fd2);
		c = fgetc(fd1);
	}
	fclose(fd1); fclose(fd2);


	printf("Server: Astept la portul %d...\n", port);
	fflush(stdout);

	while(1)
	{
		thData * td;
	  length = sizeof(from);

		client = accept(sd, (struct sockaddr *) &from, &length);

		if(client < 0)
		{
			perror("Server: Eroare la accept(). \n");
			continue;
		}

		td = (struct thData*)malloc(sizeof(struct thData));
		td->idThread=i++;
		td->cl = client;

		pthread_create(&th[i], NULL, &treat, td);
	}

}

char * lower(char str[300])
{
	for(int i = 0; str[i];i++)
		str[i] = tolower(str[i]);

	return str;
}

int numara_cuvinte(char str[300])
{
	char *p = strtok(str, " ");
	int cnt = 0;
	while(p)
	{
		cnt++;
		p = strtok(NULL, " ");
	}
	return cnt;
}

char * extrage_cuvant(char str[300], int index)
{
	static char rsp[50];
	int start = 0, final = 0, cnt = 0;
	if(index != 4) strcat(str, " -");
	rsp[0] = '\0';
	for(int i = 0; i < strlen(str); i++)
	{
		if(str[i] == ' ')
			{cnt++; start = final; final = i;}

		if(cnt == index)
		{
			int k = 0;
			for(int j = start + 1; j < final;j++)
			{
				rsp[k] = str[j];
				k++;
			}
			rsp[k] = '\0';
			break;
		}

		if(str[i] == '[' && index != 4)
		{
			int j = i + 1; int k = 0;
			while(str[j] != ']')
			{
				rsp[k] = str[j];
				j++;
				k++; 
			}
			rsp[k] = '\0';
			break;
		}
	}
	if(index == 4)
	{		
		int k = 0;
		for(int j = final; j <strlen(str);j++)
			{rsp[k] = str[j];
			k++;}
		rsp[k] = '\0';
	}

	return rsp;
}

char * adaugaIntarziere(char sosire[6], int minute)
{
  char *p;
  static char format[6];
  format[0] = '\0';
  int h = atoi(p = strtok(sosire,":"));
  int m = atoi(p = strtok(NULL,":"));
  int ore;

  //daca intarzierea depaseste ora curenta
  if(m + minute >= 60)
  {
    ore = (m + minute)/60;
    h = h + ore;
    m = m + minute - 60 * ore;
  }
  else m = m + minute;

  if(h > 23)
  	h = h - 24;

  char hh[3];
  char mm[3];
  
  sprintf(hh, "%d", h);
  sprintf(mm, "%d", m);

  if(h < 10)
  {
    strcat(format,"0");
    strcat(format,hh);
    strcat(format,":");
  }
  else {strcat(format,hh);strcat(format,":");}
  if(m < 10)
  {
    strcat(format,"0");
    strcat(format,mm);
  }
  else strcat(format,mm);

  return format;
}

char * intarziere(char mesaj[300])
{
	static char raspuns[300];

	char tren[30], gara[30];
	int minute;

	char copie_mesaj[300];
	strcpy(copie_mesaj, mesaj);

	strcpy(tren, lower(extrage_cuvant(copie_mesaj, 2)));
	strcpy(copie_mesaj, mesaj);
	strcpy(gara, lower(extrage_cuvant(copie_mesaj,3)));
	strcpy(copie_mesaj, mesaj);
	minute = atoi(extrage_cuvant(copie_mesaj,4));
	
	XMLDocument doc;
	doc.LoadFile("xml/azi.xml");

	XMLElement *el = doc.FirstChildElement("trenuri")->FirstChildElement("tren");

	int found = 0;

	//caut trenul specificat in comanda
	while(el)
	{
		char nume[300];
		strcpy(nume, el->FirstChildElement("nume")->GetText());

		if(strcmp(tren,lower(nume)) == 0)
			{
			 found = 1;
			 break;
			}
		el = el->NextSiblingElement("tren");
	}

	if(found == 0) 	//trenul nu a fost gasit
	{
		strcpy(raspuns, "Acest tren nu circula astazi.");
		return raspuns;
	}

	el = el->FirstChildElement("gari")->FirstChildElement("gara");
	found = 0;

	//caut gara specificata in comanda
	while(el)
	{
		char nume[300];
		strcpy(nume,el->FirstChildElement("numeGara")->GetText());

		if(strstr(gara, lower(nume)))
		{
			found = 1;
			break;
		}
		el = el->NextSiblingElement("gara");
	}

	if(found == 0)	//gara nu a fost gasita
	{
		strcpy(raspuns, "Aceasta gara nu exista. Asigura-te ca ai scris corect numele garii. Numele garilor formate din mai multe cuvinte trebuie scrise neaparat intre [].");
		return raspuns;
	}

	char str[10];

	//actualizez estimare plecare, estimare sosire si intarziere pentru toate garile incepand cu gara specificata in comanda
	while(el)
	{
		if(strstr(el->FirstChildElement("sosire")->GetText(),"-") == NULL)
		{
			strcpy(str,el->FirstChildElement("sosire")->GetText());
			el->FirstChildElement("estimareSosire")->SetText(adaugaIntarziere(str,minute));
		}

		if(strstr(el->FirstChildElement("plecare")->GetText(),"-") == NULL)
		{
			strcpy(str,el->FirstChildElement("plecare")->GetText());
			el->FirstChildElement("estimarePlecare")->SetText(adaugaIntarziere(str,minute));
		}
		
		el->FirstChildElement("intarziere")->SetText(minute);
		
		el = el->NextSiblingElement("gara");
	}

	strcpy(raspuns, "A fost adaugata cu succes intarzierea de ");
	str[0] = '\0';
	sprintf(str, "%d", minute);
	strcat(raspuns, str);
	strcat(raspuns, " minute pe trenul ");
	tren[0] = toupper(tren[0]);
	strcat(raspuns, tren);
	strcat(raspuns," la gara ");
	gara[0] = toupper(gara[0]);
	for(int i = 1; i < strlen(gara); i++)
		if(gara[i] == ' ')
			gara[i+1] = toupper(gara[i+1]);
	strcat(raspuns, gara);
	strcat(raspuns, " si la toate garile urmatoare. Estimarile pentru ora de sosire au fost actualizate.");

	doc.SaveFile("xml/azi.xml");

	return raspuns;
}

char * mersulTrenurilor(char mesaj[300])
{
	static char raspuns[size];
	char fis[50];
	strcpy(fis, "xml/");
	strcat(fis, lower(extrage_cuvant(mesaj, 3)));
	fis[strlen(fis) - 1] = '\0';
	strcat(fis, ".xml");

	XMLDocument doc;
	doc.LoadFile(fis);

	
	XMLElement *el = doc.FirstChildElement("trenuri")->FirstChildElement("tren");
	XMLElement *gara;

	raspuns[0] = '\0';

	while(el)
	{
		strcat(raspuns, "Trenul ");
		strcat(raspuns, el->FirstChildElement("nume")->GetText());
		strcat(raspuns, "\nRuta: ");
		strcat(raspuns, el->FirstChildElement("ruta")->GetText());
		strcat(raspuns,"\n");
		gara = el->FirstChildElement("gari")->FirstChildElement("gara");
		while(gara)
		{
			strcat(raspuns, "Gara: ");
			strcat(raspuns, gara->FirstChildElement("numeGara")->GetText());
			strcat(raspuns, " | Estimare sosire: ");
			strcat(raspuns, gara->FirstChildElement("estimareSosire")->GetText());
			if(strcmp(gara->FirstChildElement("estimareSosire")->GetText(),"-") != 0)
				{
					strcat(raspuns, " (");
					strcat(raspuns, gara->FirstChildElement("sosire")->GetText());
					strcat(raspuns, " + ");
					strcat(raspuns, gara->FirstChildElement("intarziere")->GetText());
					strcat(raspuns, " minute intarziere)");
				}
			strcat(raspuns, " | Estimare plecare: ");
			strcat(raspuns, gara->FirstChildElement("estimarePlecare")->GetText());
			if(strcmp(gara->FirstChildElement("estimarePlecare")->GetText(),"-") != 0)
				{
					strcat(raspuns, " (");
					strcat(raspuns, gara->FirstChildElement("plecare")->GetText());
					strcat(raspuns, " + ");
					strcat(raspuns, gara->FirstChildElement("intarziere")->GetText());
					strcat(raspuns, " minute intarziere)");
				}
			strcat(raspuns,"\n");
			gara = gara->NextSiblingElement("gara");
		}
		strcat(raspuns, "\n");
		el = el->NextSiblingElement("tren");
	}

	return raspuns;
}

char * plecari(char mesaj[300])
{
	static char raspuns[size];

	XMLDocument doc;
	doc.LoadFile("xml/azi.xml");

	XMLElement * el = doc.FirstChildElement("trenuri")->FirstChildElement("tren");
	XMLElement * gara;

	char numeGara[50];
	char copie_mesaj[300];
	strcpy(copie_mesaj, mesaj);
	strcpy(numeGara,lower(extrage_cuvant(copie_mesaj, 2)));
	for(int i = 0; i < strlen(numeGara); i++)
		if(numeGara[i] == ']')
			strcpy(numeGara + i, numeGara + i + 1);

	raspuns[0] = '\0';

	time_t current_time;
	struct tm * tinfo;
	char timeStr[6]; 

	time(&current_time);
	tinfo = localtime(&current_time);

	strftime(timeStr, sizeof(timeStr), "%H:%M", tinfo);
	
	char *p;
	int h = atoi(p = strtok(timeStr,":"));
  	int m = atoi(p = strtok(NULL,":"));
  	int hEstimare, mEstimare, hPlecare, mPlecare;

	numeGara[0] = toupper(numeGara[0]);
	for(int i = 1; i < strlen(numeGara); i++)
		if(numeGara[i] == ' ')
			numeGara[i+1] = toupper(numeGara[i+1]);

	strcpy(raspuns, "Plecari din aceasta gara, in urmatoarea ora: \n");

	while(el)
	{
		gara = el->FirstChildElement("gari")->FirstChildElement("gara");
		while(gara)
		{
			char copy[50];
			strcpy(copy, gara->FirstChildElement("numeGara")->GetText());
			if(strstr(lower(numeGara), lower(copy)))
			{
				char timp[30];
				strcpy(timp, gara->FirstChildElement("estimarePlecare")->GetText());
				if(strcmp(timp,"-") != 0 )
					{
						hEstimare = atoi(p = strtok(timp, ":"));
   						mEstimare = atoi(p = strtok(NULL,":"));
   					}
   				strcpy(timp, gara->FirstChildElement("plecare")->GetText());
   				if(strcmp(timp,"-") != 0 )
					{
						hPlecare = atoi(p = strtok(timp, ":"));
   						mPlecare = atoi(p = strtok(NULL,":"));
   					}
   				if(((h == hEstimare && m <= mEstimare) || (h + 1 == hEstimare && m >= mEstimare)) &&
   				    ((h == hPlecare && m <= mPlecare) || (h + 1 == hPlecare && m >= mPlecare)))  //daca si estimarea si ora initiala de plecare sunt in urm ora
   					{
   						strcat(raspuns, "Pleaca trenul ");
   						strcat(raspuns, el->FirstChildElement("nume")->GetText());
   						strcat(raspuns, " la ora: ");
   						strcat(raspuns, gara->FirstChildElement("estimarePlecare")->GetText());
   						strcat(raspuns, " (");
   						strcat(raspuns, gara->FirstChildElement("plecare")->GetText());
   						strcat(raspuns, " + ");
   						strcat(raspuns, gara->FirstChildElement("intarziere")->GetText());
   						strcat(raspuns, " minute intarziere)\n");
   					}
   				else if(((h == hPlecare && m <= mPlecare) || (h + 1 == hPlecare && m >= mPlecare)) && 
   					!((h == hEstimare && m <= mEstimare) || (h + 1 == hEstimare && m >= mEstimare))) //daca ora de plecare initiala este in urm ora dar estimarea nu
   					{
   						strcat(raspuns, "Ar trebui sa plece trenul ");
   						strcat(raspuns, el->FirstChildElement("nume")->GetText());
   						strcat(raspuns, " la ora ");
   						strcat(raspuns, gara->FirstChildElement("plecare")->GetText());
   						strcat(raspuns, ", dar acesta are o intarziere de ");
   						strcat(raspuns, gara->FirstChildElement("intarziere")->GetText());
   						strcat(raspuns, " minute si se estimeaza ca va pleca la ora ");
   						strcat(raspuns, gara->FirstChildElement("estimarePlecare")->GetText());
   						strcat(raspuns, "\n");
   					}
   					else if((!(h == hPlecare && m <= mPlecare) || (h + 1 == hPlecare && m >= mPlecare)) && 
   							((h == hEstimare && m <= mEstimare) || (h + 1 == hEstimare && m >= mEstimare)))  //daca estimarea este in urm ora dar ora initiala de plecare nu
   					{
   						strcat(raspuns, "Se estimeaza ca va pleca trenul ");
   						strcat(raspuns, el->FirstChildElement("nume")->GetText());
   						strcat(raspuns, " la ora ");
   						strcat(raspuns, gara->FirstChildElement("estimarePlecare")->GetText());
   						strcat(raspuns, ". Acesta avea plecarea programata la ora ");
   						strcat(raspuns, gara->FirstChildElement("plecare")->GetText());
   						strcat(raspuns, ", dar are o intarziere de ");
   						strcat(raspuns, gara->FirstChildElement("intarziere")->GetText());
   						strcat(raspuns, " minute\n");
   					}
			}

			gara = gara->NextSiblingElement("gara");
		}
		el = el->NextSiblingElement("tren");
	}	

	if(strcmp(raspuns, "Plecari din aceasta gara, in urmatoarea ora: \n") == 0)
		strcpy(raspuns, "Din aceasta gara nu pleaca niciun tren in urmatoarea ora");

	return raspuns;
}

char * sosiri(char mesaj[300])
{
	static char raspuns[size];

	XMLDocument doc;
	doc.LoadFile("xml/azi.xml");

	XMLElement * el = doc.FirstChildElement("trenuri")->FirstChildElement("tren");
	XMLElement * gara;

	char numeGara[50];
	char copie_mesaj[300];
	strcpy(copie_mesaj, mesaj);
	strcpy(numeGara,lower(extrage_cuvant(copie_mesaj, 2)));
	for(int i = 0; i < strlen(numeGara); i++)
		if(numeGara[i] == ']')
			strcpy(numeGara + i, numeGara + i + 1);

	raspuns[0] = '\0';

	time_t current_time;
	struct tm * tinfo;
	char timeStr[6]; 

	time(&current_time);
	tinfo = localtime(&current_time);

	strftime(timeStr, sizeof(timeStr), "%H:%M", tinfo);
	
	char *p;
	int h = atoi(p = strtok(timeStr,":"));
  	int m = atoi(p = strtok(NULL,":"));
  	int hEstimare, mEstimare, hSosire, mSosire;

	numeGara[0] = toupper(numeGara[0]);
	for(int i = 1; i < strlen(numeGara); i++)
		if(numeGara[i] == ' ')
			numeGara[i+1] = toupper(numeGara[i+1]);
	

	strcpy(raspuns, "Sosiri in aceasta gara, in urmatoarea ora: \n");

	while(el)
	{
		gara = el->FirstChildElement("gari")->FirstChildElement("gara");
		while(gara)
		{
			char copy[50];
			strcpy(copy, gara->FirstChildElement("numeGara")->GetText());
			if(strstr(lower(numeGara), lower(copy)))
			{
				char timp[30];
				strcpy(timp, gara->FirstChildElement("estimareSosire")->GetText());
				if(strcmp(timp,"-") != 0 )
					{
						hEstimare = atoi(p = strtok(timp, ":"));
   						mEstimare = atoi(p = strtok(NULL,":"));
   					}
   				strcpy(timp, gara->FirstChildElement("sosire")->GetText());
   				if(strcmp(timp,"-") != 0 )
					{
						hSosire = atoi(p = strtok(timp, ":"));
   						mSosire = atoi(p = strtok(NULL,":"));
   					}
   				if(((h == hEstimare && m <= mEstimare) || (h + 1 == hEstimare && m >= mEstimare)) &&
   				    ((h == hSosire && m <= mSosire) || (h + 1 == hSosire && m >= mSosire)))  //daca si estimarea si ora initiala de sosire sunt in urm ora
   					{
   						strcat(raspuns, "Ajunge trenul ");
   						strcat(raspuns, el->FirstChildElement("nume")->GetText());
   						strcat(raspuns, " la ora: ");
   						strcat(raspuns, gara->FirstChildElement("estimareSosire")->GetText());
   						strcat(raspuns, " (");
   						strcat(raspuns, gara->FirstChildElement("sosire")->GetText());
   						strcat(raspuns, " + ");
   						strcat(raspuns, gara->FirstChildElement("intarziere")->GetText());
   						strcat(raspuns, " minute intarziere)\n");
   					}
   				else if(((h == hSosire && m <= mSosire) || (h + 1 == hSosire && m >= mSosire)) && 
   					!((h == hEstimare && m <= mEstimare) || (h + 1 == hEstimare && m >= mEstimare))) //daca ora de sosire initiala este in urm ora dar estimarea nu
   					{
   						strcat(raspuns, "Ar trebui sa ajunga trenul ");
   						strcat(raspuns, el->FirstChildElement("nume")->GetText());
   						strcat(raspuns, " la ora ");
   						strcat(raspuns, gara->FirstChildElement("sosire")->GetText());
   						strcat(raspuns, ", dar acesta are o intarziere de ");
   						strcat(raspuns, gara->FirstChildElement("intarziere")->GetText());
   						strcat(raspuns, " minute si se estimeaza ca va ajunge la ora ");
   						strcat(raspuns, gara->FirstChildElement("estimareSosire")->GetText());
   						strcat(raspuns, "\n");
   					}
   					else if((!(h == hSosire && m <= mSosire) || (h + 1 == hSosire && m >= mSosire)) && 
   							((h == hEstimare && m <= mEstimare) || (h + 1 == hEstimare && m >= mEstimare))) //daca estimarea este in urm ora dar ora de sosire initiala nu
   					{
   						strcat(raspuns, "Se estimeaza ca va ajunge trenul ");
   						strcat(raspuns, el->FirstChildElement("nume")->GetText());
   						strcat(raspuns, " la ora ");
   						strcat(raspuns, gara->FirstChildElement("estimareSosire")->GetText());
   						strcat(raspuns, ". Acesta avea sosirea programata la ora ");
   						strcat(raspuns, gara->FirstChildElement("sosire")->GetText());
   						strcat(raspuns, ", dar are o intarziere de ");
   						strcat(raspuns, gara->FirstChildElement("intarziere")->GetText());
   						strcat(raspuns, " minute\n");
   					}
			}

			gara = gara->NextSiblingElement("gara");
		}
		el = el->NextSiblingElement("tren");
	}	

	if(strcmp(raspuns, "Sosiri in aceasta gara, in urmatoarea ora: \n") == 0)
		strcpy(raspuns, "In aceasta gara nu soseste niciun tren in urmatoarea ora");

	return raspuns;
}

static void *treat(void * arg)
{
	struct thData tdL;
	tdL = *((struct thData*)arg);

	char info[300];
	strcpy(info, "Lista comenzi: \n");
	strcat(info, "-> mersul trenurilor zi/azi (Ex1: mersul trenurilor azi | Ex2: mersul trenurilor joi) \n"); 
	strcat(info, "-> plecari [gara] (Ex: plecari [Bucuresti Nord]) \n-> sosiri [gara] (Ex: sosiri [Bucuresti Nord) \n");
	strcat(info, "-> intarziere tren [gara] minute (Ex: intarziere IR1862 [Constanta] 10)\n");
	strcat(info, "-> disconnect\n");

	if(write(tdL.cl, info, sizeof(info)) <= 0)
	{
		printf("Thread %d: ",tdL.idThread);
		perror("Thread: Eroare la write() catre client. \n");	
	}
	else printf("Thread %d: Mesajul a fost trimis cu succes.\n",tdL.idThread);

	while(1)
	{
		printf ("Thread %d: Asteptam mesajul...\n", tdL.idThread);
		fflush (stdout);		 
		pthread_detach(pthread_self());		
		if(msg((struct thData*)arg) == 1)
			break;
	}
	shutdown(tdL.cl, SHUT_RDWR);
	close ((intptr_t)arg);
	return(NULL);
}

int msg(void *arg)
{
	char mesaj[300];
	char raspuns[size] = " ";
	int i = 0;
	struct thData tdL;
	tdL= *((struct thData*)arg);
	if (read (tdL.cl, mesaj,sizeof(mesaj)) <= 0)
			{
			  printf("Thread %d: \n",tdL.idThread);
			  perror ("Eroare la read() de la client.\n");
			}

	printf("Thread %d: Mesajul a fost receptionat: %s\n",tdL.idThread, mesaj);

	char copie_mesaj[300]; strcpy(copie_mesaj, mesaj);
	int disconnect = 0;

	if(strstr(lower(mesaj), "intarziere"))
		if(numara_cuvinte(copie_mesaj) < 4)
		{
			bzero(raspuns, size);
			strcat(raspuns, "Sintaxa comenzii este: intarziere tren [gara] minute (Ex: intarziere IR1300 [Constanta] 10)");
		}
		else {
			bzero(raspuns, size);
			strcpy(raspuns,intarziere(mesaj));
		}
	else if(strstr(lower(mesaj),"mersul trenurilor"))
			if(numara_cuvinte(copie_mesaj) != 3)
			{
				bzero(raspuns, size);
				strcat(raspuns, "Sintaxa comenzii este: mersul trenurilor zi/azi (Ex1: mersul trenurilor duminica | Ex2: mersul trenurilor azi)");
			}
			else {
				bzero(raspuns, size);
				strcpy(raspuns, mersulTrenurilor(mesaj));
			}
		  else if (strstr(lower(mesaj), "plecari"))
		  		{
		  			int nr_cuv = numara_cuvinte(copie_mesaj);
		  			if(nr_cuv != 2 && nr_cuv != 3)
		  			{
		  				bzero(raspuns, size);
						strcat(raspuns, "Sintaxa comenzii este: plecari [gara] (Ex: plecari [Bucuresti Nord])");	
		  			}
		  			else{
		  				bzero(raspuns, size);
		  				strcpy(raspuns, plecari(mesaj));
		  			}
		  		}
		  		else if (strstr(lower(mesaj), "sosiri"))
		  			{
		  				int nr_cuv = numara_cuvinte(copie_mesaj);
		  				if(nr_cuv != 2 && nr_cuv != 3)
		  				{
		  					bzero(raspuns, size);
							strcat(raspuns, "Sintaxa comenzii este: sosiri [gara] (Ex: sosiri [Bucuresti Nord])");	
		  				}
		  				else{
		  					bzero(raspuns, size);
		  					strcpy(raspuns, sosiri(mesaj));
		  				}
		  			}
		  			else if(strstr(lower(mesaj),"disconnect"))
		  			{
		  				bzero(raspuns, size);
		  				strcat(raspuns, "Vei fi deconectat");
		  				disconnect = 1;		  			
		  			}
		  			else {
		  				bzero(raspuns, size);
		  				strcat(raspuns, "Aceasta comanda nu a fost recunoscuta de server.");
		  			}

	printf("Thread %d: Trimit inapoi mesajul: \n%s\n",tdL.idThread, raspuns);

	if(write(tdL.cl, raspuns, sizeof(raspuns)) <= 0)
	{
		printf("Thread %d: ",tdL.idThread);
		perror("Thread: Eroare la write() catre client. \n");	
	}
	else printf("Thread %d: Mesajul a fost trimis cu succes.\n",tdL.idThread);

	if(disconnect == 0) return 0;
	else return 1;
}