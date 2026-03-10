#include "conectare.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include<fcntl.h>

#include<stdbool.h>
#include <cstring>

using namespace std;


    bool con::conecteaza( const char* ip, int port){//127.0.0.1 2728
        sd= socket(AF_INET, SOCK_STREAM, 0);
        if(sd<0) return false;

        sockaddr_in serv;
        serv.sin_family=AF_INET;
        serv.sin_port = htons (port);
        serv.sin_addr.s_addr=inet_addr(ip);

        if (connect(sd, (sockaddr*)&serv , sizeof(serv))<0) {
            close(sd);
            sd=-1;
            return false;
        }

        int flg= fcntl(sd, F_GETFL, 0);
        fcntl(sd, F_SETFL, flg|O_NONBLOCK); 
        return true;
    }

    void con::inchide(){
        if(sd!= -1) { close(sd), sd=-1; }
    }
    
    bool con::trimite( const string& cmd){ //trimitem comanda cmd lungime mesaj + mesaj 
         if(sd== -1) return false;

         int lg= (int)cmd.size();

         int n= write(sd, &lg, sizeof(int));
         if(n<=0) return false;

         if(lg >0){
            int n2 = write(sd, cmd.c_str(), lg);
            if(n2 <=0 )return false;
         }
         return true;
    }

    void con::actualizare(){
        msg=false;
        last.clear();

        if(sd== -1) return;

        fd_set rdfs;
        FD_ZERO(&rdfs);
        FD_SET(sd, &rdfs); 
        //timeout 0 ca s a nu se blocheszse
 
        timeval tv;
        tv.tv_sec=0;
        tv.tv_usec =0;

        int r= select(sd+1, &rdfs, NULL, NULL, &tv);
        if (r <=0 ) return;//nu este nimic de citit 

        if(lg ==-1){
            int x= 4- lg_bytes; //4 =sizeof(int)
            int nr=(int) read(sd, lg_buf+lg_bytes, x);
            if(nr <= 0 ) return;

            lg_bytes+=nr;
            if(lg_bytes <4) return;

            memcpy(&lg, lg_buf, 4);
            if(lg<0 ){  inchide(); return;}

            body.clear();
            body.resize(lg);
            body_bytes=0;
        }

        if(lg >=0){//lungimea pe care o asteptam 
            int x= lg - body_bytes;

            if(x <=0){
                msg= true;
                last=""; 
            }
            else {
                int nr=(int) read(sd, body.data() +body_bytes , x);
                 if(nr<=0) return;
                 body_bytes+=nr;

                 if (body_bytes <lg) return;//msg ul nu este complet inca
            
                    last.assign( body.begin(), body.end());
                    msg=true;

                }
                body.clear();
                lg=-1;
                lg_bytes= body_bytes=0;
        }
    }


