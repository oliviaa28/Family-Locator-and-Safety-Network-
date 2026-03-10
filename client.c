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
#include<arpa/inet.h>
#include<stdbool.h>

#define BUF_SIZE 300
#define RESET "\033[0m" //culori ansi
#define BOLD "\033[1m"

#define GREEN "\033[32m"
#define MAGENTA "\033[35m"
#define BLUE "\033[34m"


#define ERROR(mesaj)\
        { perror(mesaj); return errno;}

extern int errno;
int port;

char *meniu=
MAGENTA BOLD 
"==============================================================================\n"
"||                   FAMILY LOCATOR AND SAFETY NETWORK                      ||\n"
"==============================================================================\n"
"||                 Bine ati venit! Meniul de comenzi este:                  ||\n"
"==============================================================================\n" 
RESET 
BLUE 
"|| 1. LOGIN <username> <parola>                                             ||\n"
"|| 2. UPDATE_LOCATION <username ><lat> <lon>                                ||\n"
"|| 3. SOS <id_familie>                                                      ||\n"
"|| 4. GET_LOCATION <family_id> <username of the person u want the location> ||\n"
"|| 5. GET_FAMILY_ID <username>                                              ||\n"
"|| 6. ADD FAMILY <username> <family_name>                                   ||\n"
"|| 7. ADD_LOCATION <ID_FAMILY><nume_locatie><latitudine><longitudine><raza> ||\n"
"|| 8. ADD_MEMBERS <id_family> <username>                                    ||\n"
"|| 9. JOIN_FAMILY <id_family>                                               ||\n"
"|| 10. TURN_LOCATION_ON <username>                                          ||\n"
"|| 11. TURN_LOCATION_OFF <username>                                         ||\n"
"|| 12. LEAVE_FAMILY <id_family>                                             ||\n"
"|| 23. LOGOUT<username>                                                     ||\n"
 "==============================================================================\n"
RESET 
MAGENTA BOLD
"==============================================================================\n"
"||  Alegeticomanda pe care doriti sa o executati.   ||(EXIT PENTRU IESIRE)  ||\n"
"==============================================================================\n"
"\n" RESET;

int citeste(int fd, char *buf, int bytes){
   int total=0, bytes_ramasi=bytes, bytes_cititi;

   while(total < bytes){
      bytes_cititi = read(fd, buf+total, bytes_ramasi);
      if (bytes_cititi <= 0 )
         return bytes_cititi;
       total+=bytes_cititi;
       bytes_ramasi-=bytes_cititi;
   }
   return total;
}

int main(int argc, char *argv[]){

       if(argc!=3){
        printf("err. Sintaxa este: adresa serverului, port.");
        return -1;
    }
    port = atoi (argv[2]);
    int sd;
    struct sockaddr_in server;
    char msg[BUF_SIZE];

    if((sd=socket(AF_INET, SOCK_STREAM, 0))== -1)
       ERROR("[CLIENT] EROARE la socket");
    
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(port);

    if (connect (sd, (struct sockaddr *)&server, sizeof(struct sockaddr ))== -1 )
       ERROR("[CLIENT] Eroare la conectare \n");

   /*int rasp_lg=0;
    if(citeste(sd, (char *)&rasp_lg, sizeof(int))<=0)
      ERROR("[CLIENT] EROARE la citirea lg. meniului");
    if(citeste(sd, msg, rasp_lg)<= 0)
       ERROR("[CLIENT] EROARE la citirea meniului");
   msg[rasp_lg]='\0';
   printf("%s\n" ,msg);*/
   printf("%s\n" ,meniu);
    
   bzero(msg, BUF_SIZE);

    while(1){
         int rasp_lg=0;
         bzero(msg, BUF_SIZE);
         printf("[COMANDA] \n");
         fflush (stdout);

      //   int bytes_read = read (0, msg, BUF_SIZE -1);
       //  if(bytes_read <= 0) break;

       if(fgets(msg, BUF_SIZE, stdin) ==NULL) break;

       int lg= strlen(msg);
       if(lg > 0 && msg[lg-1] == '\n'){  //sergem newline
           msg[lg-1] = '\0';
           lg--;
       }

       if(strcmp(msg, "EXIT") ==0) break;

        if( write(sd, &lg, sizeof(int))== -1)
           ERROR("[CLEINT] eroare la trimiterea lungimii comenzii");
        if(write(sd, msg, lg) == -1)
           ERROR("[Client] ERoare l atrimiterea comenzii");

         if( citeste(sd, (char *) &rasp_lg, sizeof(int))<= 0)
           ERROR("[CLIENT] Eroare la citirea mesajului de la server");

           if(rasp_lg >= BUF_SIZE) rasp_lg=BUF_SIZE -1;

          bzero(msg, BUF_SIZE);
        
         if (citeste (sd, msg, rasp_lg)<= 0 )
            ERROR("[CLIENT] eroare la citirea raspunsului");
         msg[rasp_lg]='\0';

    //  if( write(sd, msg, strlen(msg)) == -1 )
        //   ERROR("[client] Eroare la scriere");

      printf ( "[client]Mesajul primit este: %s\n \n" , msg);
      fflush (stdout);
    }

 
    close(sd);
    return 0;
}
