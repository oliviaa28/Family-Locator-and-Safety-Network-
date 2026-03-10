#ifndef DATE_H
#define DATE_H

#include<sqlite3.h>
#include <stdbool.h>
#include<time.h>

int open_bd  (sqlite3 **db, const char *nume_bd);
void close_bd(sqlite3 *db);

int login_bd       (sqlite3 *db, char *nume, char *parola);


int register_clt_bd (sqlite3 *db, char *nume, char *parola);
int exista_user_bd (sqlite3 *db, const char *nume);

int update_location_bd (sqlite3 *db, char *nume, double lat, double lon,time_t timp);
int location_status_bd    (sqlite3 *db, char *nume, bool stat);
int verifica_locatie_prestabilita_bd( sqlite3 *db, int id_fam,  double lat, double lon, char *nume_loc);
int get_usr_loc(sqlite3 *db, char *nume, int *lat, int *lon, int *loc_on, time_t *last_up);

int familie_comuna(sqlite3 *db, char *nume1, char *nume2, int *id_fam);
int add_fam_bd(sqlite3 *db, char *admin, char *nume_fam, int *id_fam);
int join_fam_bd(sqlite3 *db, char *username, int id_family);

int leave_fam_bd( sqlite3 *db, const char *username, int id_family);
int add_members_bd( sqlite3 *db,int id_family,  const char *add_username);
int user_exists_bd(sqlite3 *db, const char *username);

int is_member_bd( sqlite3 *db,int id_family,  const char *username);

int add_locations_bd( sqlite3 *db, int id_family,  const char *nume_loc, double lat, double lon, double raza);

//int get_family_of_usr_bd(sqlite3 *db, const char *username, char *id_cerut, int id_size );

int sos_bd(sqlite3 *db, int id_fam, const char *user);

int build_state_bd(sqlite3 *db, const char *username, int fam_id_hint, char *out);
//void get_families_list(sqlite3 *db,int fd, int poz);
#endif
