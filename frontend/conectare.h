#pragma once 

#include<vector>
#include<string>
using namespace std;

struct con{

    int sd=-1;
    int lg=-1;
    int lg_bytes=0;
    char lg_buf[4];

    vector<char> body;//corpul mesajului complet 
    int body_bytes=0; //cati byte am primit din mesaj

    bool msg=false;
    string last;

    bool conecteaza( const char* ip, int port);

    void inchide();
    
    bool trimite( const string& cmd);

    void actualizare();
};
