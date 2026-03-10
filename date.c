#include "date.h"
#include <sqlite3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<errno.h>
#include<fcntl.h>
#include<unistd.h>

#include<sys/file.h>
#include<sys/stat.h>
#include<time.h>
//#include<math.h>

/*
prepare+bind+step+finalize

    int sqlite3_prepare_v2(
  sqlite3 *db,            /* Database handle 
  const char *zSql,       /* SQL statement, UTF-8 encoded 
  int nByte,              /* Maximum length of zSql in bytes. 
  sqlite3_stmt **ppStmt,  /* OUT: Statement handle 
  const char **pzTail     /* OUT: Pointer to unused portion of zSql 
);

int sqlite3_bind_text(sqlite3_stmt*,int,const char*,int,void(*)(void*));

int sqlite3_step(sqlite3_stmt*);

int sqlite3_finalize(sqlite3_stmt *pStmt);

*/

int open_bd( sqlite3 **db, const char *nume_bd){
    int a= sqlite3_open(nume_bd, db);
    if(a){
        fprintf(stderr, "[SERVER ]Nu se poate deschide BD\n");
        sqlite3_close(*db);
        return -1; 
    }

 return 1;
}

void close_bd(sqlite3 *db){
    sqlite3_close(db);
}

int login_bd  (sqlite3 *db, char *nume, char *parola){

    /*RETURNEZ:
    2- parola corecta 
    1- parola gresita
    0-nu exista user
    -1  - eroare
    */
        sqlite3_stmt *var=NULL;
        const char *sql_usr ="SELECT 1 from users where username=? ";

        int a= sqlite3_prepare_v2(db, sql_usr, -1, &var, NULL);
        if(a!= SQLITE_OK){
            printf("[BAZA DE date] err la login\n");
            return -1;
        }

    ///daca exista user
    sqlite3_bind_text(var, 1, nume, -1, SQLITE_TRANSIENT); // -1 pt ca strinul se termina cu /n

    a = sqlite3_step(var); //executa interogarea
    sqlite3_finalize(var);
    var = NULL;

    if(a == SQLITE_DONE) return 0; // nu exista username

    if(a!= SQLITE_ROW){//altceva inafara de gasire rand 
        printf("[BD] eroare la login \n");
        return -1;
    }
    
  ///daca e pusa parola corecta 
    const char *sql ="SELECT 1 from users where username=? and password=?;";

     a = sqlite3_prepare_v2(db, sql, -1, &var, NULL);
    if(a!= SQLITE_OK){
        printf("[BAZA DE date] err. la login\n");
        return -1;
    }

    sqlite3_bind_text(var, 1, nume, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(var, 2, parola,-1, SQLITE_TRANSIENT);

    a= sqlite3_step(var);
    sqlite3_finalize(var);

    if( a== SQLITE_ROW) return 2;  //parola corecta (select a gasit cv)
    if( a== SQLITE_DONE) return 1; 

    printf("[BD] Eroare la login \n");
    return -1;
}


int register_clt_bd (sqlite3 *bd, char *nume, char *parola){

    /*retun:
    1- succes
    0- user existent
    -1 - eroare
    */
  
   sqlite3_stmt *var= NULL;
  
    const char *sql_usr ="SELECT 1 from users where username=? ";

    int a= sqlite3_prepare_v2(bd, sql_usr, -1, &var, NULL);
    if(a!= SQLITE_OK){
        printf("[BAZA DE date] err la register\n");
        return -1;
    }

      sqlite3_bind_text(var, 1, nume, -1, SQLITE_TRANSIENT); 
      a = sqlite3_step(var); 
      sqlite3_finalize(var);
    
     if(a == SQLITE_DONE){

        var=NULL;
        const char *sql_ins="INSERT INTO users (username, password) VALUES (?, ?);";
          a= sqlite3_prepare_v2(bd, sql_ins, -1, &var, NULL);
         if(a!= SQLITE_OK){
              printf("[BAZA DE date] err la register\n");
             return -1;}
     
      sqlite3_bind_text(var, 1, nume, -1, SQLITE_TRANSIENT); 
      sqlite3_bind_text(var, 2, parola, -1, SQLITE_TRANSIENT);

         a = sqlite3_step(var); 
         sqlite3_finalize(var);

        if(a == SQLITE_DONE) return 1;
       
    }else 
    if (a == SQLITE_ROW){
        printf("[BD] Exista deja userul cu numele introdus de dvs\n");
        return 0;
    }

    printf("[BD] Eroare la register \n");
    return -1;

}


int exista_user_bd (sqlite3 *db, const char *nume){

  /* returneaza: 0 nu exista , 1 exista , -1 er
  */

  sqlite3_stmt *var= NULL;
  char *sql= "SELECT 1 from users where username = ?;";

  int a = sqlite3_prepare_v2(db, sql, -1, &var, NULL);
  if(a != SQLITE_OK){
     printf("[BD] Eroare la exista_user\n");
     return -1;
  }

  sqlite3_bind_text(var, 1, nume, -1, SQLITE_TRANSIENT);
  a=sqlite3_step(var);
  sqlite3_finalize(var);

  if(a == SQLITE_ROW) return 1; //exista 
   if(a == SQLITE_DONE) return 0; 


printf("[BD] ERoare la cautarea userului \n");
return -1;

}

int get_usr_loc(sqlite3 *db, char *nume, int *lat, int  *lon,int *loc_on, time_t *last_up){
  sqlite3_stmt *var =NULL;
  char *sql ="select location, lat, lon, last_update FROM users where username=?;";

  int a = sqlite3_prepare_v2(db, sql, -1, &var, NULL);
  if(a != SQLITE_OK){
        printf("[BD] ER");
        return -1;
    }
   sqlite3_bind_text(var, 1, nume, -1, SQLITE_TRANSIENT);
   a= sqlite3_step(var);
   if(a==SQLITE_ROW){
    *loc_on = sqlite3_column_int(var, 0);
    *lat= sqlite3_column_int(var, 1);
    *lon= sqlite3_column_int(var, 2);
    *last_up= (time_t)sqlite3_column_int(var, 3);

    sqlite3_finalize(var);
    return 1;
   }

   sqlite3_finalize(var);
   if(a == SQLITE_DONE) return 0;

return -1;
}


int update_location_bd  (sqlite3 *db, char *nume, double lat, double lon, time_t timp){

    sqlite3_stmt *var = NULL;
    char *sql ="UPDATE users set lat=?, lon=?, last_update=? where username=?;";

    int a = sqlite3_prepare_v2(db, sql, -1,&var, NULL);
    if(a != SQLITE_OK){
        printf("[BD] ER");
        return -1;
    }

    sqlite3_bind_double (var, 1,lat);
    sqlite3_bind_double (var, 2,lon);
    sqlite3_bind_int64 (var, 3, timp);
    sqlite3_bind_text(var, 4, nume, -1, SQLITE_TRANSIENT);
  
    a = sqlite3_step(var);
    sqlite3_finalize(var);
    var=NULL;

    if(a != SQLITE_DONE) return -1; //+msg

    if(sqlite3_changes(db) ==0) return 0; //update nu a facut nimic , nu exista user 

return 1;
}

int location_status_bd (sqlite3 *db, char *nume, bool stat){ //on/off

    sqlite3_stmt *var= NULL;

    char *sql="UPDATE users SET location = ? where username =?;";
    int a = sqlite3_prepare_v2(db, sql, -1, &var , NULL);
    if( a != SQLITE_OK) return -1;

    sqlite3_bind_int(var, 1, stat ? 1 : 0);
    sqlite3_bind_text(var, 2, nume, -1, SQLITE_TRANSIENT);
    a= sqlite3_step(var);
    sqlite3_finalize(var);
    var=NULL;

     if(a != SQLITE_DONE) return -1;

    if( sqlite3_changes(db) ==0 ) return 0;
return 1;
}

int verifica_locatie_prestabilita_bd( sqlite3 *db, int id_fam, double lat, double lon, char *nume_loc){
     int ok=0;
    sqlite3_stmt *var= NULL;
   char *sql="select name, lat, lon, raza from family_locations where id_family =?;";
     
    int a = sqlite3_prepare_v2(db, sql, -1, &var, NULL); 
    if( a != SQLITE_OK){
        printf("[BD] Eroare la familie_comuna\n");
        return -1;
    }
    
    sqlite3_bind_int(var, 1, id_fam);

    while(( a= sqlite3_step(var) )== SQLITE_ROW){ //trecem prin toate locatiile familiei
       const unsigned  char *nume2= sqlite3_column_text(var, 0);

        double lat_loc , lon_loc, raza_loc, dist =0.0;
        lat_loc=sqlite3_column_double(var, 1);
        lon_loc=sqlite3_column_double(var, 2);
        raza_loc=sqlite3_column_double(var, 3);

        double a, b;
        a= lat -lat_loc;
        b= lon - lon_loc;
        dist= a*a + b*b;

         if(dist<= raza_loc*raza_loc) {
            strcpy(nume_loc, (const char*)nume2);
            ok=1;
            break; 
        }
    }
    sqlite3_finalize(var);
    if (a!= SQLITE_DONE && a!= SQLITE_ROW && ok==0){
        printf("[BD] Eroare la gasirea unei loc ,,,, \n");
        return -1;
    }

    return ok; 
}

int familie_comuna(sqlite3 *db, char *nume1, char *nume2,int *id_fam){
     *id_fam = -1;  
    sqlite3_stmt *var = NULL;
    char *sql= "select f1.id_family from family_members f1 JOIN family_members f2 ON f1.id_family = f2.id_family where f1.username = ? AND f2.username = ?;";

    int a = sqlite3_prepare_v2(db, sql, -1, &var, NULL);
    if( a != SQLITE_OK){
        printf("[BD] Eroare la familie_comuna\n");
        return -1;
    }

    sqlite3_bind_text(var, 1, nume1, -1 , SQLITE_TRANSIENT);
    sqlite3_bind_text(var, 2, nume2, -1 , SQLITE_TRANSIENT);

    a= sqlite3_step(var);
    if(a == SQLITE_ROW){
        *id_fam =sqlite3_column_int(var, 0);
        sqlite3_finalize(var);
        return 1; //exista
    }

   sqlite3_finalize(var);
   if( a == SQLITE_DONE) return 0; //nu exista

return -1;
}

int add_fam_bd(sqlite3 *db, char *admin, char *nume_fam, int *id_fam){
   sqlite3_stmt *var = NULL;
   char *sql= "INSERT into families (family_name, admin_username) values (?, ?);";

   int a= sqlite3_prepare_v2(db, sql, -1, &var, NULL );
   if( a != SQLITE_OK){
        printf("[BD] Eroare la add_family\n");
        return -1;
    }
 sqlite3_bind_text(var, 1, nume_fam, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(var, 2, admin, -1, SQLITE_TRANSIENT);
  a= sqlite3_step(var);
  sqlite3_finalize(var);
  var=NULL;
  
  if(a != SQLITE_DONE) return -1;

    *id_fam = (int) sqlite3_last_insert_rowid(db);
    
    char *sql2="INSERT INTO family_members (id_family, username) values (?,?);";
    int b= sqlite3_prepare_v2(db, sql2, -1, &var, NULL );
    if( b!= SQLITE_OK){
        printf("[BD] Eroare la add_family\n");
        return -1;
    }
    sqlite3_bind_int(var, 1, *id_fam);
    sqlite3_bind_text(var, 2, admin, -1, SQLITE_TRANSIENT);

    b = sqlite3_step(var);
    sqlite3_finalize(var);
    if(b!= SQLITE_DONE)return -1;
    
    return 1;
}

int join_fam_bd(sqlite3 *db, char *username, int id_fam){

    sqlite3_stmt *var = NULL;
   char *sql= "INSERT into family_members (id_family, username) values (?, ?);";

   int a= sqlite3_prepare_v2(db, sql, -1, &var, NULL );
   if( a != SQLITE_OK){
        printf("[BD] Eroare la add_family\n");
        return -1;
    }

     sqlite3_bind_int(var, 1, id_fam);
    sqlite3_bind_text(var, 2, username, -1, SQLITE_TRANSIENT);


    int b =  sqlite3_step(var);
    sqlite3_finalize(var);

    if(b == SQLITE_DONE) return 1;
    if (b== SQLITE_CONSTRAINT) return 0;

    return -1;
}

//__________

int leave_fam_bd( sqlite3 *db, const char *username, int id_family){

    sqlite3_stmt *var= NULL;
    char *sql= "DELETE from family_members where id_family=? AND username=?;";

    int a= sqlite3_prepare_v2(db, sql, -1, &var, NULL);
    if( a!= SQLITE_OK) return -1;

     sqlite3_bind_int(var, 1, id_family );
    sqlite3_bind_text(var, 2, username, -1, SQLITE_TRANSIENT);

    a=sqlite3_step(var);
    sqlite3_finalize(var);

    if( a != SQLITE_DONE ) return -1;

    if( sqlite3_changes(db) == 0) return 0; //nu era membru al familiei

    return 1;

}

int add_members_bd( sqlite3 *db, int id_family, const char *add_username){
    sqlite3_stmt *var= NULL;
    char *sql= " INSERT into family_members (id_family, username) values (?, ?);";

    int a=sqlite3_prepare_v2(db, sql, -1, &var, NULL);
    if( a!= SQLITE_OK) return -1;

    sqlite3_bind_int(var, 1, id_family);
    sqlite3_bind_text(var, 2, add_username, -1, SQLITE_TRANSIENT);

    a= sqlite3_step(var);
    sqlite3_finalize(var);

    if ( a == SQLITE_DONE) return 1;
    if( a== SQLITE_CONSTRAINT) return 0;

    return -1;
}

int user_exists_bd(sqlite3 *db, const char *username){
    sqlite3_stmt *var = NULL;
    int a;
    char *sql = "SELECT 1 FROM users WHERE username=? LIMIT 1;";

    a = sqlite3_prepare_v2(db, sql, -1, &var, NULL);
    if (a != SQLITE_OK) return -1;

    sqlite3_bind_text(var, 1, username, -1, SQLITE_TRANSIENT);
    a = sqlite3_step(var);
    sqlite3_finalize(var);

    if (a == SQLITE_ROW) return 1;
    if (a == SQLITE_DONE) return 0;
    return -1;
}


int add_locations_bd( sqlite3 *db, int id_family,  const char *nume_loc, double lat, double lon, double raza){

      sqlite3_stmt *var= NULL;
      char *sql= "INSERT into family_locations(id_family, name, lat, lon, raza) VALUES (?, ?, ?,?,?);";

      int a= sqlite3_prepare_v2(db, sql, -1, &var, NULL);
       if( a != SQLITE_OK){
        printf("[BD] Eroare la add_locations_bd\n");
        return -1;
    }

    sqlite3_bind_int(var, 1, id_family);
    sqlite3_bind_text(var, 2, nume_loc, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(var, 3, lat);
    sqlite3_bind_double(var, 4, lon);
    sqlite3_bind_double(var, 5, raza);

    a= sqlite3_step(var);
    sqlite3_finalize(var);

    return (a== SQLITE_DONE) ? 1: -1;
}
/*
int get_family_of_usr_bd(sqlite3 *db, const char *username, char *id_cerut, int id_size){
 
    int gasit =0;
    sqlite3_stmt *var= NULL;
    char *sql =" SELECT id_family from family_members where username= ? ORDER BY id_family;";
 
  int a = sqlite3_prepare_v2(db, sql, -1, &var, NULL);
  if( a!= SQLITE_OK) return -1;

  sqlite3_bind_text(var, 1, username, -1,SQLITE_TRANSIENT );

  while(( a = sqlite3_step(var)) == SQLITE_ROW ){
    int id= sqlite3_column_int(var, 0);
   
  }

  sqlite3_finalize(var);

   if ( a != SQLITE_DONE) return -1;

    return 1;
}*/

int sos_bd(sqlite3 *db, int id_fam, const char *user){
  
    sqlite3_stmt *var = NULL;
    int a;
    char *sql="INSERT INTO sos_alerts (id_family, from_user, message) VALUES (?, ?, 'SOS: HELP NEEDED'); ";

    a= sqlite3_prepare_v2(db, sql, -1, &var, NULL);
    if( a!= SQLITE_OK) return -1;

    sqlite3_bind_int(var, 1, id_fam);
    sqlite3_bind_text(var, 2, user, -1, SQLITE_TRANSIENT);

    a= sqlite3_step(var);
    sqlite3_finalize(var);

     if ( a != SQLITE_DONE) return -1;

     return 1;
}


int is_member_bd( sqlite3 *db,int id_family,  const char *username){
     sqlite3_stmt *var = NULL;
    int a;
    char *sql="SELECT 1 FROM family_members where id_family=? and username =?";

    a= sqlite3_prepare_v2(db, sql, -1, &var, NULL);
    if( a!= SQLITE_OK) return -1;

    sqlite3_bind_int(var, 1, id_family);
    sqlite3_bind_text(var, 2, username, -1, SQLITE_TRANSIENT);

    a= sqlite3_step(var);
    sqlite3_finalize(var);

     if ( a == SQLITE_DONE) return 0;
     if (a == SQLITE_ROW) return 1;

     return -1;
}

int build_state_bd(sqlite3 *db, const char *username, int fam_id_hint, char *out){

    sqlite3_stmt *var=NULL;
    int off=0, id_fam=-1;
    char nume_fam[100]= "Family";

     out[0]='\0';
   if (!db ||!username || !out) return -1;
    if (fam_id_hint > 0) {
        char *sql_hint = "SELECT f.id_family, f.family_name FROM family_members m Join families f on f.id_family = m.id_family where m.username= ? AND f.id_family = ? LIMIT 1;";
        int a = sqlite3_prepare_v2(db, sql_hint, -1, &var, NULL);
        if (a != SQLITE_OK) return -1;

        sqlite3_bind_text(var, 1, username, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(var, 2, fam_id_hint);
        a = sqlite3_step(var);
        if (a == SQLITE_ROW) {
            id_fam = sqlite3_column_int(var, 0);
            const char *num = sqlite3_column_text(var, 1);
            if (num) strcpy(nume_fam, num);
        }
        sqlite3_finalize(var);
    }

    if (id_fam == -1) {
        char *sql = "SELECT f.id_family, f.family_name FROM family_members m Join families f on f.id_family =m.id_family where m.username= ? Order by f.id_family ASC LIMIT 1;";

        int a = sqlite3_prepare_v2(db, sql, -1, &var, NULL);
        if (a != SQLITE_OK) return -1;

        sqlite3_bind_text(var, 1, username, -1, SQLITE_TRANSIENT);
        a = sqlite3_step(var);
        if (a == SQLITE_ROW) {
            id_fam = sqlite3_column_int(var, 0);
            const char *num = sqlite3_column_text(var, 1);
            if (num) strcpy(nume_fam, num);
        }
        sqlite3_finalize(var);
    }

    if( id_fam ==-1){
        strcpy (out, "STATE\nFAMILY -1 -\nME -1 -1 0 0\nMEMBERS 0\nPLACES 0\nEND");
        return 0;
    }

    //pentru sectiunea ME: lat lon last_update ___________
    int x, y, on;
    time_t last_up;

    get_usr_loc(db, (char*) username, &x, &y, &on, &last_up);
 
     //________ numaram membrii ___________________
     int n=0;
     char *sql2="SELECT count(*) from family_members where id_family = ?;";
     int  a2= sqlite3_prepare_v2(db, sql2, -1, &var, NULL);
     if( a2!= SQLITE_OK) return -1;

     sqlite3_bind_int(var, 1, id_fam);
      a2= sqlite3_step(var);
      if(a2 == SQLITE_ROW){
        n= sqlite3_column_int(var, 0);
      }
      sqlite3_finalize(var);

      //________ num loc __________________________

      int k=0;
      char *sql3="SELECT count(*) from family_locations where id_family = ?;";
      int  a3= sqlite3_prepare_v2(db, sql3 , -1, &var, NULL);
      if( a3!= SQLITE_OK) return -1;

      sqlite3_bind_int(var, 1, id_fam);
      a3= sqlite3_step(var);
      if(a3 == SQLITE_ROW){
        k = sqlite3_column_int(var, 0);
      }
      sqlite3_finalize(var);

       char linie[300];
       strcat(out, "STATE\n");

       snprintf(linie, sizeof(linie), "FAMILY %d %s\n", id_fam, nume_fam);
       strcat(out, linie);

       snprintf(linie, sizeof(linie), "ME %d %d %d %ld\n", x, y, on, (long)last_up);
       strcat(out, linie);

       snprintf(linie, sizeof(linie), "MEMBERS %d\n", n);
       strcat(out, linie);


      //_________________________
      char *sql4="select u.username, u.location, u.lat, u.lon from family_members m JOIN users u ON u.username=m.username where m.id_family =?;";
      int  a4= sqlite3_prepare_v2(db, sql4, -1, &var, NULL);
      if( a4!= SQLITE_OK) return -1;
      sqlite3_bind_int(var, 1, id_fam);

      while( sqlite3_step(var) ==SQLITE_ROW)
      {
        const char *n = sqlite3_column_text(var, 0);
        int loc_on= sqlite3_column_int(var, 1);
        int x =sqlite3_column_int(var, 2);
        int y =sqlite3_column_int(var, 3); 

        if(!loc_on){ x=y=-1;}

       snprintf(linie, sizeof(linie), "%s %d %d %d\n", n, loc_on, x, y);
       strcat(out, linie);
      }
     sqlite3_finalize(var);

     //_________________loc ________________________

      snprintf(linie, sizeof(linie), "PLACES %d\n", k);
       strcat(out, linie);

     char *sql5="select name, lat, lon FROM family_locations WHERE id_family=?;" ;
     int  a5= sqlite3_prepare_v2(db, sql5, -1, &var, NULL);
     if( a5!= SQLITE_OK) {
         strcat(out ,"END"); return -1;}

     sqlite3_bind_int(var, 1, id_fam);

      while( sqlite3_step(var) ==SQLITE_ROW)
      { const unsigned  char *n1= sqlite3_column_text(var, 0);
        int x1 =sqlite3_column_int(var, 1);
        int y1 =sqlite3_column_int(var, 2); 

          snprintf(linie, sizeof(linie), "%s %d %d\n", n1, x1, y1);
           strcat(out, linie);
    
      }
        sqlite3_finalize(var);
  
       strcat(out ,"END");
      return 0;
      //la fianl vom avea toate inf pt frontend asa:
      /*
      STATE
      FAMILY id name
      ME x y on last
      MEMBERS n
      ..... user loc_on x, y
      PLACES k
      ..... name x y
      END
      */

}
