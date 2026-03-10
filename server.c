#include<sys/types.h>
#include<sys/socket.h>
#include<sys/time.h>
#include<netinet/in.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<ctype.h>
#include<arpa/inet.h>
#include<stdbool.h>
#include<time.h>

#include<sqlite3.h>
#include "date.h"

#define MAP_W 645
#define MAP_H 393

#define PORT 2728
#define MAX_CLIENTS 20
#define MAX_FAMILII 3
#define MAX_LOC 20
#define BUF_SIZE 1000
#define ERROR(mesaj)\
        { perror(mesaj); return errno;}

extern int errno;

//pt sql
sqlite3 *db= NULL;

struct sockaddr_in server, from; 
struct timeval tv;
fd_set read_fds, active_fds;
int sd, client, optval=1, fd, max_fds, len;

typedef struct{
    int fd; 
    char username[50];//id, PK
    char parola[10];
    int id_family[3];
    double lat;  //latitudine 
    double lon;  //longitudine
    bool logat; //dupa ce s-a logat
    bool has_account; //dupa ce s-a inregistrat cu un cont+parola noua
    bool location; //on sau off 
    time_t last_update;
    char last_place_name[50];
    int current_fam_id;//din frontend

}client_info; 


client_info clt[MAX_CLIENTS];

typedef struct{
    char comanda[40];
    char parametri[100];
    int nr_parametrii; 
}mesaj_clt; 

mesaj_clt scrisoare;

//_______________________________________________________________________________

char* upper( char *cuvant){
    int i=0;
    char c;
    while( cuvant[i] ){
        c=cuvant[i];
        cuvant[i]=toupper((unsigned char)c);
        i++;
    }
    return cuvant; 
}

void eliminare_newline(char *cuvant ){
    char *pozitie= strchr(cuvant , '\n');
    if (pozitie != NULL )
        *pozitie= '\0';
}

void trim( char *cuvant){ //doar de la inceput sau final 
   char *f; // final
   int lg= strlen(cuvant) -1 ;
    
   while( isspace((unsigned char)*cuvant) ) // de la inceput
         cuvant++;

    if(*cuvant == 0 ) return; 
    
    f= cuvant+lg;
    while( cuvant < f && isspace((unsigned char)*f )) f--;
    f[1]= '\0';
}

//_______________________________________________________________________________

int extragere_comanda(char *mesaj, mesaj_clt *aux){ //separam mesajul transmis de la client 
     char m[BUF_SIZE], *p, parametrii[100]="";
     int n=BUF_SIZE;
    strncpy( m , mesaj, n-1);
    m[n-1] ='\0';

    eliminare_newline(m);
    trim(m);

    aux->comanda[0]= aux->parametri[0]= '\0';
   aux->nr_parametrii=0;

  p= strtok(m, " ");
  if( p== NULL) return -1;
 
  //extragem comanda
  strncpy(aux->comanda, p, 39); //comadta[40] , evitam \0
  aux->comanda[39]='\0';
  upper(aux->comanda);

   p=strtok(NULL, " ");
 
  while( p!=NULL){ //extragem parametrii
    if(aux->nr_parametrii>0) strcat(parametrii, " ");

    strcat(parametrii, p);
    aux->nr_parametrii++;
    p=strtok(NULL, " ");
  }

  trim(parametrii);
  strncpy(aux->parametri, parametrii, 99);
  aux->parametri[99] = '\0';
  return 0;
}

void initializare(){
    for(int i =0; i<MAX_CLIENTS; i++){
        clt[i].fd =-1;
        clt[i].logat = false;
        clt[i].has_account=false;
        clt[i].location=true;
        clt[i].current_fam_id=-1;
    }
}

int caut_clt(int fd){
    for(int i =0; i<MAX_CLIENTS; i++)
        if(clt[i].fd == fd)
        return i;
  return -1;
}

int caut_clt_nume(char *nume){
    for(int i =0; i<MAX_CLIENTS; i++)
        if(strcmp(clt[i].username, nume)==0)
           return i;
  return -1;
}

int add_clt( int fd){
    for(int i =0; i<MAX_CLIENTS; i++){
        if(clt[i].fd == -1){
           clt[i].fd = fd;
           clt[i].logat =false;
            return i; }
    }
return -1;
}

int remove_clt(int fd){
     for(int i =0; i<MAX_CLIENTS; i++)
        if(clt[i].fd == fd){
           clt[i].fd =-1;
           clt[i].logat =false;
          return 1;
        }
    return -1;
}

static int get_first_family_id_db(const char *username){
    sqlite3_stmt *var = NULL;
    int id = 0;
    char *sql = "SELECT f.id_family FROM family_members m Join families f on f.id_family = m.id_family where m.username = ? Order by f.id_family ASC LIMIT 1;";
    int a = sqlite3_prepare_v2(db, sql, -1, &var, NULL);
    if (a != SQLITE_OK) return 0;
    sqlite3_bind_text(var, 1, username, -1, SQLITE_TRANSIENT);
    a = sqlite3_step(var);
    if (a == SQLITE_ROW) id = sqlite3_column_int(var, 0);
    sqlite3_finalize(var);
    return id;
}

static int family_exists_db(int id_family){
    sqlite3_stmt *var = NULL;
    int ok = -1;
    char *sql = "SELECT 1 FROM families WHERE id_family = ? LIMIT 1;";
    int a = sqlite3_prepare_v2(db, sql, -1, &var, NULL);
    if (a != SQLITE_OK) return -1;
    sqlite3_bind_int(var, 1, id_family);
    a = sqlite3_step(var);
    if (a == SQLITE_ROW) ok = 1;
    else if (a == SQLITE_DONE) ok = 0;
    sqlite3_finalize(var);
    return ok;
}

int get_fam_id(int poz){
    if (clt[poz].current_fam_id > 0) return clt[poz].current_fam_id;
    int id = get_first_family_id_db(clt[poz].username);
    if (id > 0) clt[poz].current_fam_id = id;
    return id;
}

int trimite_raspuns(int fd, char *mesaj){
    int lungime;
    lungime=strlen(mesaj);
    if (write(fd, &lungime, sizeof(int))== -1){
        ERROR("[SERVER] EROARE l atroimiterea lungimii mesajului");
    }
    if(write(fd, mesaj, lungime) == -1)
       ERROR("[SERVER] ERoare l atrimiterea mesajului");

   // write(fd, mesaj, strlen(mesaj));
}

int citeste(int fd, char *buf, int bytes){
   int total=0, bytes_ramasi=bytes, bytes_cititi;

   while(total < bytes){
      bytes_cititi = read(fd, buf+total, bytes_ramasi);
      if (bytes_cititi <= 0 )
         return bytes_cititi; //eroare 
       total+=bytes_cititi;
       bytes_ramasi-=bytes_cititi;
   }
   return total;
}
//_______________________________

void get_families_list( int fd, int poz){
    char raspuns[10000]="FAMILIES_LIST";
    char buf[300];
    sqlite3_stmt *var;
    char *sql="SELECT f.id_family, f.family_name FROM families f JOIN family_members m ON f.id_family = m.id_family WHERE m.username = ?;";

     int a= sqlite3_prepare_v2(db, sql, -1, &var, NULL);
     if( a != SQLITE_OK){
        printf("[BD] Eroare la add_locations_bd\n");
        return ;
    }
    sqlite3_bind_text(var, 1, clt[poz].username, -1, SQLITE_TRANSIENT);
    int nr=0;

    while(sqlite3_step(var) == SQLITE_ROW){
        int id=sqlite3_column_int(var, 0);
        const char *nume=(const char*) sqlite3_column_text(var, 1);
        sprintf(buf, "\n%d %s", id, nume);
        strcat(raspuns, buf);
        nr++;
    }
    sqlite3_finalize(var);
    
    if(nr== 0) strcat(raspuns, "\nEMPTY");
    strcat(raspuns, "\nEND");
    
    trimite_raspuns(fd, raspuns);
}

void switch_family(int fd, int poz){
    int id_cerut;
    if(sscanf(scrisoare.parametri, "%d", &id_cerut)==1){
        if(is_member_bd(db, id_cerut, clt[poz].username)==1){
            clt[poz].current_fam_id=id_cerut;
            trimite_raspuns(fd, "OK_SWITCHED");
        }
        else trimite_raspuns(fd, "ERR_NOT_MEMBER");
    }
}
//________________________________________________________________________________________

void login(int fd, int poz){
 /* din bd: RETURNEZ:
    2- parola corecta 
    1- parola gresita
    0-nu exista user
    -1  - eroare
    */
    char nume[50], parola[10];
    char raspuns[BUF_SIZE];
        if(scrisoare.nr_parametrii !=2){
            trimite_raspuns(fd, "ERR_PARAM");
            return ;
        }
        int n= sscanf(scrisoare.parametri, "%s %s", nume, parola);

        if (n!=2){
            trimite_raspuns(fd,"ERR_PARAM");
            return;
       }
    
      
           int ok= login_bd(db, nume, parola);
           if (ok == 2){
            clt[poz].logat = true;
            strcpy( clt[poz].username, nume);
          //  trimite_raspuns(fd, "[SV] login SUCCES\n");
              trimite_raspuns(fd, "OK_LOGIN");
            return;
           }
           
           if(ok == 0){
             trimite_raspuns(fd, "ERR_USER_NOT_FOUND");
             return;
            }

          if(ok == 1){
            trimite_raspuns(fd, "ERR_WRONG_PASS");
            return;
          }
   trimite_raspuns(fd, "[SERVER] EROARE BD");
}

void register_clt(int fd, int poz){
    char nume[50], parola[10];
    char raspuns[BUF_SIZE];

    if(scrisoare.nr_parametrii !=2){
        trimite_raspuns(fd, "ERR_PARAM");
        return ;
    }

     int n= sscanf(scrisoare.parametri, "%s %s", nume, parola);
     if(n!=2){
        trimite_raspuns(fd, "ERR_PARAM");
        return;
     }

     int x=register_clt_bd(db, nume, parola);
     if(x == 0){
      //  trimite_raspuns(fd, "[SERVER] Acest nume de utilizator este luat de alt user. incercati din nou.");
      trimite_raspuns(fd, "ERR_USER_TAKEN");
        return;
     }

     if (x == 1){
      //  snprintf(raspuns, BUF_SIZE,"[SERVER] (OK) cont creeat cu succes. Username-ul dvs este %s. Va puteti conecta cu comanda LOGIN. \n", nume);
        trimite_raspuns(fd, "OK_REGISTER");
        strcpy(clt[poz].username, nume);
        strcpy(clt[poz].parola, parola);
        return;
    }

    trimite_raspuns(fd, "[SERVER] EROARE BD");
}

void notifica_familia_arrived(int poz, int id_fam, const char *nume_loc){
    char mesaj[BUF_SIZE];
    snprintf(mesaj, BUF_SIZE, "NOTIF_ARRIVED %s %s", clt[poz].username, nume_loc);

    for(int i=0;i<MAX_CLIENTS;i++){
        if(clt[i].logat==true && i!=poz){
            int k = is_member_bd(db, id_fam, clt[i].username);
            if(k==1) trimite_raspuns(clt[i].fd, mesaj);
        }
    }
}

void notifica_familia_left(int poz, int id_fam, const char *nume_loc){
    char mesaj[BUF_SIZE];
    snprintf(mesaj, BUF_SIZE, "NOTIF_LEFT %s %s", clt[poz].username, nume_loc);

    for(int i=0;i<MAX_CLIENTS;i++){
        if(clt[i].logat==true && i!=poz){
            int k = is_member_bd(db, id_fam, clt[i].username);
            if(k==1) trimite_raspuns(clt[i].fd, mesaj);
        }
    }
}

void update_location(int fd, int poz){

    char raspuns[BUF_SIZE];
    time_t timp= time(NULL);

        if(clt[poz].logat ==false){
            trimite_raspuns(fd, "[SERVER] eroare, clientul nu este conectat");
            return;
        }
      //  int n= sscanf(scrisoare.parametri, "%lf %lf", &lat, &lon);

       int x, y;
       x= rand() % MAP_W; y= rand() % MAP_H;
     

        int ok= update_location_bd(db, clt[poz].username, x , y , timp);

        if(ok == 0){
          //  trimite_raspuns(fd, "[SERVER] Nu exista nici un utilizator cu acest username;");
          trimite_raspuns(fd, "ERROR User inexistent");
          return;
        }

        if( ok== -1){
            trimite_raspuns(fd, "ERROR BD");
            return;
        }
        int id_fam=get_fam_id(poz);
        char nume_loc_curent[50];
        int zona= verifica_locatie_prestabilita_bd(db, id_fam, x, y, nume_loc_curent);

        if(id_fam > 0){
            if(zona==1){
                if(clt[poz].last_place_name[0] == '\0'){
                    notifica_familia_arrived(poz, id_fam, nume_loc_curent);
                    strcpy(clt[poz].last_place_name, nume_loc_curent);
                }else 
                if(strcmp(clt[poz].last_place_name, nume_loc_curent)!=0){
                    notifica_familia_left(poz, id_fam, clt[poz].last_place_name);
                    notifica_familia_arrived(poz, id_fam, nume_loc_curent);
                    strcpy(clt[poz].last_place_name, nume_loc_curent);
                }
            }
            else{
                if(clt[poz].last_place_name[0] != '\0'){
                    notifica_familia_left(poz, id_fam, clt[poz].last_place_name);
                    clt[poz].last_place_name[0]='\0';
                }
            }

        }else clt[poz].last_place_name[0]='\0';

        //sprintf(raspuns, "[SERVER] LOCATIA pentru %s : ( %f, %f) a fost salvata \n", clt[poz].username,lat, lon );

        snprintf(raspuns, BUF_SIZE, "OK_UPDATE %d %d", x, y);
        trimite_raspuns(fd, raspuns); 
}


void get_location(int fd, int poz){
    char raspuns[BUF_SIZE], nume[30], nume_loc[50];;
    int lat, lon, id_fam, loc_on;
    time_t last;

        if(clt[poz].logat == false){
            trimite_raspuns(fd, "[SERVER] eroare, clientul nu este conectat");
            return;
        }

        int n=sscanf(scrisoare.parametri, "%s", nume); 
        if(n != 1){
            trimite_raspuns(fd, "ERR_PARAM");
            return;
        }

        int f= familie_comuna(db, clt[poz].username, nume, &id_fam);
        if( f == 0){
            trimite_raspuns(fd, "[SERVER] Acest user nnu exista in fam dvs;");
            return;
        }
         
        if(f == -1){
            trimite_raspuns(fd, "[SERVER] get location:eroare la bd");
            return;
        }
         
        int l= get_usr_loc(db, nume, &lat, &lon, &loc_on, &last);
        if (l ==0 ){
            trimite_raspuns(fd, "[sv] nu exista acest user ");
            return;
        }

        if(l ==-1){//......
           return;
        }

        int ver = verifica_locatie_prestabilita_bd (db, id_fam, lat, lon, nume_loc);
      
            //daca astau de mai mult timp imt -o locatie prestabilita afisam, de ex: SINCE 12:00 at Camin 
        if(ver == -1){
            trimite_raspuns(fd,"[sv] eroare la bd ");
            return;
        }

            if(ver== 0){ 
             sprintf(raspuns, "OK_COORD %d %d %d %ld", lat, lon, loc_on, (long)last); 
             trimite_raspuns(fd,raspuns);}
            else{ 
             sprintf (raspuns, "OK_LOC %s %d %d %d %ld",nume_loc,lat, lon, loc_on,  (long)last);
             trimite_raspuns(fd,raspuns);}
}

void alerta (int fd, int poz){
    char raspuns[BUF_SIZE];
     if(clt[poz].logat ==false){
            trimite_raspuns(fd, "[SERVER] eroare, clientul nu este conectat");
            return;
        }
      int id=get_fam_id(poz);

            int ok= sos_bd(db, id, clt[poz].username);

            if(ok==1)
          {
          //  sprintf(raspuns, "[SERVER] cerere %s: Am trimis un mesaj de alerta familiei %d. \n", clt[poz].username , id);
            trimite_raspuns(fd, "OK_ALERT_SENT");

            char mesaj_alerta[BUF_SIZE];
          //  sprintf(mesaj_alerta, "SOS ALERTA !!!!!!!!!!!!!!!! de la : %s", clt[poz].username);

            for (int i=0;i<MAX_CLIENTS; i++){
                if(clt[i].logat == false|| clt[poz].fd ==-1)  continue;
                if ( i == poz) continue;

                int k= is_member_bd(db, id, clt[i].username);
                if(k==1 && clt[i].fd != clt[poz].fd ){
                    char mesaj_alerta[BUF_SIZE];
                    snprintf(mesaj_alerta, BUF_SIZE, "SOS_RECEIVE %s", clt[poz].username);
                    trimite_raspuns (clt[i].fd, mesaj_alerta);
                }
            }

         }
            else trimite_raspuns(fd, "[SERVER] EROARE bd");

}
/*
void fam_id(int fd, int poz){//get fam id <username>
   char raspuns[BUF_SIZE], nume[30];
   int id;
    int n= sscanf(scrisoare.parametri, "%s", nume);
        if(n ==1){
            //cautaree id familie ...
            //  int x= caut_clt_nume(nume);
           sprintf(raspuns, "[SERVER]: FAMILIA persoanei cu username-ul %s este NUME_FAMILIE cu id dat .\n", nume);
            trimite_raspuns(fd, raspuns);
        }
        else trimite_raspuns(fd, "[SERVER] EROARE, nr de parametrii gesiti pentru get GET FAMILY ID");
}*/

void logout(int fd, int poz){

  char raspuns[BUF_SIZE], nume[30];
 //   int n= sscanf(scrisoare.parametri, "%s", nume);
    //    if(n ==1){
            if (clt[poz].logat == true){
          //  sprintf(raspuns, "[SERVER]: USERUL CU NUMELE %s a fost deconectat\n", clt[poz].username);
            trimite_raspuns(fd, "OK_LOGOUT");
            clt[poz].logat =false;
        }
        else{
            sprintf(raspuns, "[SERVER]: USERUL CU NUMELE %s nu era conectat \n", clt[poz].username);
            trimite_raspuns(fd, raspuns);
        } 
}


void  add_family(int fd, int poz){//ADD FAMIL <family_name>
    char raspuns[BUF_SIZE], nume[30], nume_fam[30];
    int id;
    int n= sscanf(scrisoare.parametri, " %s", nume_fam); 
        if(n ==1){

           int f = add_fam_bd(db, clt[poz].username, nume_fam, &id);
            if( f == -1) {
               trimite_raspuns(fd, "[sv] eroare ");
               return;
            }
            if(f==1){
            clt[poz].current_fam_id = id;
          //  sprintf(raspuns, "[SERVER](client %s) Ati creeat o noua familie cu numele [%s ]si Id-ul [ %d]. Acum sunteti administratorul acestei familii.\n ", clt[poz].username, nume_fam, id);
            trimite_raspuns(fd, "OK_NEW_FAM");
           }
        }
        else trimite_raspuns(fd, "ERR_PARAM");
}

void  add_location(int fd, int poz){//ADD_LOCATION <ID_FAMILY> <nume_locatie> <latitudine> <longitudine> <raza>
    char raspuns[BUF_SIZE], nume[30], nume_fam[30];
    int id= get_fam_id(poz);

 //   int n= sscanf(scrisoare.parametri, "%d %s %lf %lf %lf", &id, nume, &lat, &lon, &raza);
     int n= sscanf(scrisoare.parametri, "%s ", nume); 
       if(n ==1){
            int lat, lon, raza=5;
            lat=rand()%MAP_W;
            lon=rand()%MAP_H;

            int ok= add_locations_bd(db, id, nume, lat, lon, raza);

            if(ok==1){ 
            sprintf(raspuns, "OK_NEW_LOC %d %d %d", lat, lon, raza);
            trimite_raspuns(fd,raspuns );
            }
            else trimite_raspuns(fd, "[sv] eroare la bd");
        }
        else trimite_raspuns(fd, "ERR_PARAM");
}

void add_members(int fd, int poz){//ADD_MEMBERS  <username>
     char raspuns[BUF_SIZE], nume[30];
     int id=get_fam_id(poz);

       int n= sscanf(scrisoare.parametri, "%s",nume);
        if(n ==1){
           int exists = user_exists_bd(db, nume);
           if (exists == 0){
                trimite_raspuns(fd, "ERR_USER_NOT_FOUND");
                return;
           }
           if (exists == -1){
                trimite_raspuns(fd, "[SERVER] EROARE bd");
                return;
           }

           int mem =is_member_bd(db, id, nume);
           if (mem == 1){
                trimite_raspuns(fd, "ERR_USER_ALREADY_IN_FAM");
                return;
           }

           if (mem == -1){
                trimite_raspuns(fd, "[SERVER] EROARE bd");
                return;
           }

           int ok = add_members_bd(db, id, nume);
           if (ok == 1){
                // sprintf(raspuns, "[SERVER] Ati adaugat un membru nou in familia [ %d ]cu numele %s \n ", id,  nume);
                trimite_raspuns(fd, "OK_NEW_MEM");
           }
           else trimite_raspuns(fd, "[SERVER] EROARE bd");
        }
        else trimite_raspuns(fd, "ERR_PARAM"); 
}

void  join_family(int fd, int poz){ //JOIN_FAMILY <id_family> 
     char raspuns[BUF_SIZE];
     int id;
         int n= sscanf(scrisoare.parametri, "%d", &id);
        if(n ==1){
            int exists = family_exists_db(id);
            if (exists == 0) {
                trimite_raspuns(fd, "ERR_FAMILY_NOT_FOUND");
                return;
            }
            if (exists == -1) {
                trimite_raspuns(fd, "[sv] eroare ");
                return;
            }
            int f = join_fam_bd( db, clt[poz].username, id);
            if( f == -1) {
               trimite_raspuns(fd, "[sv] eroare ");
               return;
            }
            else if (f ==0){
                trimite_raspuns(fd, "ERR_ALREADY_IN_FAM");
               return;
            }

          //  sprintf(raspuns, "[SERVER] Ati intrat in familia cu id [ %d ]\n ", id);
            clt[poz].current_fam_id = id;
            trimite_raspuns(fd,"OK_ENTER_FAM");
        }
         else trimite_raspuns(fd, "ERR_PARAM");

}

void leave_family(int fd, int poz){
     char raspuns[BUF_SIZE];
     int id=get_fam_id(poz);

            int ok= leave_fam_bd(db, clt[poz].username, id);
            if (ok ==1 ){
         //   sprintf(raspuns, "[SERVER] Ati parasit familia cu id:[ %d ]\n ", id);
            trimite_raspuns(fd,"OK_LEAVE_FAM");
            clt[poz].current_fam_id = -1;
            
            for(int i=0; i<3; i++){
                if(clt[poz].id_family[i]==id){
                     clt[poz].id_family[i]=0;
                    break;
                }
            }

           }
           else if(ok ==-1) 
               trimite_raspuns(fd, "[SERVER] EROARE bd ");
           else  
         trimite_raspuns(fd, "[SERVER] Nu faceati parte din familia respectiva");
    
}

void loc_on(int fd, int poz){
    char raspuns[BUF_SIZE];    
            //clt[poz].location=true;
            int ok= location_status_bd(db, clt[poz].username, true);
             if( ok ==1){
             //    sprintf(raspuns, "[SERVER]:(%s) V-ati deschis locatia. Membrii familiei/lamiliilor din care faceti parte vor avea cces la locatia dumneavoastra\n", nume);
                 trimite_raspuns(fd,"OK_LOC_ON");
                return;
            }
            if(ok ==0){
                trimite_raspuns(fd, "[sv] usr inexistent"); return;
            }

           trimite_raspuns(fd, "sv, eroare la bd");
}

void loc_off(int fd, int poz){
    char raspuns[BUF_SIZE];
              int ok= location_status_bd(db, clt[poz].username, false);
             if( ok ==1){
              // sprintf(raspuns, "[SERVER]:(%s) V-ati inchis locatia. Membrii familiei/lamiliilor din care faceti parte nu vor avea acces la locatia dumneavoastra\n", nume);
               trimite_raspuns(fd, "OK_LOC_OFF");
                return;
            }
            if(ok ==0){
                trimite_raspuns(fd, "[sv] usr inexistent"); return;
            }
           
        else trimite_raspuns(fd, "[SERVER] EROARE\n ");
}

void get_state(int fd, int poz){
    char raspuns[10000];
     if(clt[poz].logat ==false){
            trimite_raspuns(fd, "ERROR not logged in");
            return;
        }

    int ok= build_state_bd(db, clt[poz].username, clt[poz].current_fam_id, raspuns);
    if(ok==-1){
        trimite_raspuns(fd, "ERROR BD");
        return;
    }

    trimite_raspuns(fd, raspuns);
}


int procesare_comanda(int fd, char *msg){
    int poz = caut_clt(fd);
    if(poz == -1) return -1;

    printf("[SERVER] cmd=%s params=[%s] from=%s\n",
       scrisoare.comanda, scrisoare.parametri, clt[poz].username);
    fflush(stdout);


    if(strcmp(scrisoare.comanda, "LOGIN") == 0)
        login(fd, poz);
   else if(strcmp(scrisoare.comanda, "REGISTER") == 0)
        register_clt(fd, poz);
     else if (strcmp(scrisoare.comanda, "UPDATE_LOCATION")==0 ) //UPDATE_LOCATION <lat> <lon>
        update_location(fd, poz);
    else if (strcmp(scrisoare.comanda, "GET_LOCATION")==0 ) //GET_LOCATION <username> 
        get_location(fd, poz);
    else if(strcmp(scrisoare.comanda, "SOS")==0 ) //SOS 
        alerta(fd, poz);
    else if (strcmp(scrisoare.comanda, "LOGOUT")== 0) //LOGOUT
        logout(fd, poz);
    else if( strcmp(scrisoare.comanda, "ADD_FAMILY")==0) //ADD FAMILY <family_name>
         add_family(fd, poz);
     else if( strcmp(scrisoare.comanda, "ADD_LOCATION")==0) //ADD_LOCATION <nume_locatie> 
         add_location(fd, poz);
    else if( strcmp(scrisoare.comanda, "ADD_MEMBERS")==0) //ADD_MEMBERS <username>
         add_members(fd, poz);
    else if( strcmp(scrisoare.comanda, "JOIN_FAMILY")==0) //JOIN_FAMILY <id_family>
         join_family(fd, poz);
   // else if( strcmp(scrisoare.comanda, "ADD_FAMILY")==0)
    //     join_family(fd, poz);
   /// else if( strcmp(scrisoare.comanda, "IS_DRIVING")==0)
      //   add_family(fd, poz); 
    else if( strcmp(scrisoare.comanda, "LEAVE_FAMILY")==0) //LEAVE_FAMILY 
         leave_family(fd, poz);
    else if( strcmp(scrisoare.comanda, "TURN_LOCATION_ON")==0) //TURN_LOCATION_ON
         loc_on(fd, poz);
    else if( strcmp(scrisoare.comanda, "TURN_LOCATION_OFF")==0) //TURN_LOCATION_OFF 
         loc_off(fd, poz);
     else if( strcmp(scrisoare.comanda, "GET_STATE")==0) 
        get_state(fd, poz);
     else if(strcmp(scrisoare.comanda, "GET_FAMILIES") == 0)
              get_families_list(fd, poz);
        else if(strcmp(scrisoare.comanda, "SWITCH_FAMILY") == 0)
            switch_family(fd, poz);
     else{
        char raspuns[BUF_SIZE];
        snprintf(raspuns, BUF_SIZE, "[SRRVER] EROARE!! Comanda invalida. Va rog introduceti o comanda din meniu.");
        trimite_raspuns(fd, raspuns);
    }
return 0;
}


int main(int argc, char *argv[] ){

     if ((sd= socket(AF_INET, SOCK_STREAM, 0))== -1)
         ERROR("[SERVER] Eroare la socket()");
    
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    bzero(&server, sizeof(server));
    server.sin_family= AF_INET;
    server.sin_addr.s_addr= htonl(INADDR_ANY);
    server.sin_port= htons(PORT);

    if( bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
        ERROR("[SERVER] Eroare la bind()");

    if(listen(sd, 10) == -1)
       ERROR("[SERVER] Eroare la listen");

    initializare();
    FD_ZERO(&active_fds);
    FD_SET(sd, &active_fds);
     
    tv.tv_sec=1; 
    tv.tv_usec=0;
    max_fds = sd;


     int x= sqlite3_open("db.sqlite", &db);//aceeasi conexiune  pt tot serverul 
     if(x ){
        fprintf(stderr, "[SERVER ]Nu se poate deschide BD\n");
        sqlite3_close(db);
        return(1);
     }

 sqlite3_exec(db, "PRAGMA foreign_keys = ON;", 0, 0, 0);

    while  (1){
        bcopy( (char *)&active_fds, (char *)&read_fds, sizeof(read_fds));

        if (select(max_fds +1, &read_fds, NULL, NULL, &tv)<0)
           ERROR("[SERVER] Eroare la select");

        if (FD_ISSET(sd, &read_fds)){ //vererificam daca avem conexiuni noi
            len = sizeof(len);
            bzero(&from, sizeof(from));

            client= accept(sd, (struct sockaddr *)&from, &len);

            if(client <0)
                ERROR("[SERVER] er la accept");
            
                add_clt(client);

              if( max_fds<client)
                  max_fds= client;
            FD_SET(client , &active_fds);//adaugam noul socket la list ade clienti activi

            printf("[Server] client conectat cu descriptorul %d\n", client);   
          //  trimite_raspuns(client, (char*)meniu);
        }

        for(fd = 0; fd<= max_fds; fd++){ //indiferent daca s a conectat un clt sau nu 
            if(fd != sd && FD_ISSET(fd, &read_fds)){ //citim mesajul 
                char buf[BUF_SIZE];
                int bytes, lg, citit;// car_primite=0;
                 citit = citeste(fd, (char*) &lg, sizeof(int));

                if (citit<=0){
                    printf("[SERVER] client deconectat(eroare la citire)");
                     remove_clt(fd); close(fd);
                    FD_CLR(fd, &active_fds);
                    continue;
                }
                 bzero(buf, BUF_SIZE);
                  citit = citeste(fd, buf,lg);
                                  if (citit<=0){
                    printf("[SERVER] client deconectat, eroare la citirea comenzii");
                     remove_clt(fd); close(fd);
                    FD_CLR(fd, &active_fds);
                    continue;
                }
     
                    buf[lg] ='\0';
                    extragere_comanda(buf, &scrisoare); //extragem informatiile
                    if( procesare_comanda(fd, buf) == -1){
                        printf("[SERVER] CLIENTUl :%d s-a delogat\n", fd);
                        close(fd);
                        FD_CLR(fd, &active_fds);} //scoatem din lista dedescriptori activi
                }
            }
         }

    close(sd);
    sqlite3_close(db);
    return 0;
}
