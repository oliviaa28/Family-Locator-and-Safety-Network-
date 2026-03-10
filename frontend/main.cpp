//============================== main.cpp ==============================
#include "raylib.h"
#include "conectare.h"

#include <iostream>
#include <vector>
#include <string>
#include <cstdio>   // sscanf
using namespace std;

//==================== CONFIG ====================
static const int LOGIN_W = 1288;
static const int LOGIN_H = 730;

static const int MAIN_W  = 1334;
static const int MAIN_H  = 750;

static const int DESIGN_W = 1334;
static const int DESIGN_H = 750;

static const int MAP_W = 640; // exact ca server
static const int MAP_H = 385;

static const bool DEBUG_UI = false;

//==================== UTIL ====================
static bool car_valid(char c){
    if(isalpha((unsigned char)c)) return true;
    if(isdigit((unsigned char)c)) return true;
    if(c=='_' || c=='@' || c=='.' || c==' ' || c=='-') return true;
    return false;
}

static void input_text(string& txt, bool activ, int max_lg){
    if(!activ) return;
    int key = GetCharPressed();
    while(key > 0){
        if(car_valid((char)key) && (int)txt.size() < max_lg) txt.push_back((char)key);
        key = GetCharPressed();
    }
    if(IsKeyPressed(KEY_BACKSPACE) && !txt.empty()) txt.pop_back();
}

static bool mouse_in(Rectangle r){
    return CheckCollisionPointRec(GetMousePosition(), r);
}
static bool click_in(Rectangle r){
    return IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mouse_in(r);
}

static string trim_copy(string s){
    while(!s.empty() && (s.back()=='\n' || s.back()=='\r' || s.back()==' ' || s.back()=='\t')) s.pop_back();
    int i=0;
    while(i<(int)s.size() && (s[i]==' '||s[i]=='\t'||s[i]=='\n'||s[i]=='\r')) i++;
    return s.substr(i);
}

static Rectangle rect_tex_center(Texture2D img){
    Rectangle r{0,0,(float)img.width,(float)img.height};
    r.x = (GetScreenWidth() - r.width)/2.0f;
    r.y = (GetScreenHeight() - r.height)/2.0f;
    return r;
}

static float clampf(float x, float a, float b){
    if(x < a) return a;
    if(x > b) return b;
    return x;
}

// pin scalat (ancora jos-centru)
static void draw_pin_scaled(Texture2D pin, Vector2 pos, float w, float h){
    if(!pin.id) return;
    Rectangle src = {0, 0, (float)pin.width, (float)pin.height};
    Rectangle dst = {pos.x, pos.y, w, h};
    Vector2 origin = {w/2, h};
    DrawTexturePro(pin, src, dst, origin, 0.0f, WHITE);
}

// conversie coordonate server (x,y pe harta 645x393) -> coordonate ecran in mapRect
static Vector2 map_to_screen(Rectangle mapRect, int x, int y){
    // x in [0..MAP_W], y in [0..MAP_H]
    float fx = mapRect.x + ((float)x / (float)MAP_W) * mapRect.width;
    float fy = mapRect.y + ((float)y / (float)MAP_H) * mapRect.height;
    return {fx, fy};
}

//==================== STATES ====================
enum ScreenState { SCR_LOGIN=0, SCR_MAIN, SCR_NO_FAMILY };

enum PopupType{
    POP_NONE=0,
    POP_INFO_LOGIN_EMPTY,
    POP_USER_NOT_FOUND, POP_WRONG_PASS, POP_REGISTER_OK, POP_USER_TAKEN,
    POP_ADD_MEMBER,
    POP_LOCATION_INFO,
    POP_LOGOUT_CONFIRM,
    POP_SOS_MSG
};

static PopupType popup_from_server_login(const string& s){
    if(s=="ERR_USER_NOT_FOUND") return POP_USER_NOT_FOUND;
    if(s=="ERR_WRONG_PASS") return POP_WRONG_PASS;
    if(s=="ERR_USER_TAKEN") return POP_USER_TAKEN;
    if(s=="OK_REGISTER") return POP_REGISTER_OK;
    return POP_NONE;
}

//==================== STATE PARSED (TEMP DISPLAY) ====================
struct MemberDisp{
    string name;
    int loc_on;
    int x, y; // -1 -1 daca off
};

struct PlaceDisp{
    string name;
    int x, y;
};

struct StateDisp{
    int fam_id = -1;
    string fam_name = "-";

    int me_x = -1, me_y = -1;
    int me_on = 0;
    long me_last = 0;

    vector<MemberDisp> members;
    vector<PlaceDisp> places;
};

// parseaza ultimul mesaj GET_STATE; daca e invalid, intoarce un state gol
static StateDisp parse_state_text(const string& raw){
    StateDisp st;
    if(raw.empty()) return st;

    // lucram linie cu linie
    vector<string> lines;
    lines.reserve(200);

    string cur;
    for(char c : raw){
        if(c=='\n'){
            lines.push_back(trim_copy(cur));
            cur.clear();
        }else if(c!='\r'){
            cur.push_back(c);
        }
    }
    if(!cur.empty()) lines.push_back(trim_copy(cur));

    int i = 0;
    int nlines = (int)lines.size();

    auto starts = [&](const string& s, const char* p){
        return s.rfind(p, 0) == 0;
    };

    while(i < nlines){
        string ln = trim_copy(lines[i]);

        if(ln=="STATE"){ i++; continue; }
        if(ln=="END"){ break; }

        if(starts(ln, "FAMILY ")){
            // FAMILY id name
            int id = -1;
            char name[200]; name[0]='\0';
            // numele poate avea spatii => in varianta ta nu recomand, dar daca exista, luam restul liniei
            // simplu: citim id, apoi restul ca string
            // sscanf cu %n ca sa stim offset
            int off = 0;
            if(sscanf(ln.c_str(), "FAMILY %d %n", &id, &off) >= 1){
                st.fam_id = id;
                if(off > 0 && off < (int)ln.size()){
                    st.fam_name = trim_copy(ln.substr(off));
                    if(st.fam_name.empty()) st.fam_name = "-";
                }
            }
            i++; continue;
        }

        if(starts(ln, "ME ")){
            // ME x y on last
            int x=-1,y=-1,on=0;
            long last=0;
            int ok = sscanf(ln.c_str(), "ME %d %d %d %ld", &x, &y, &on, &last);
            if(ok >= 4){
                st.me_x = x; st.me_y = y;
                st.me_on = on;
                st.me_last = last;
            }
            i++; continue;
        }

        if(starts(ln, "MEMBERS ")){
            int cnt = 0;
            if(sscanf(ln.c_str(), "MEMBERS %d", &cnt) == 1){
                i++;
                st.members.clear();
                st.members.reserve(cnt);
                for(int j=0; j<cnt && i<nlines; j++, i++){
                    string mline = trim_copy(lines[i]);
                    if(mline.empty()){ j--; continue; }

                    // username loc_on x y
                    char uname[120]; int loc_on=0, x=0, y=0;
                    int ok = sscanf(mline.c_str(), "%119s %d %d %d", uname, &loc_on, &x, &y);
                    if(ok == 4){
                        MemberDisp md;
                        md.name = uname;
                        md.loc_on = loc_on;
                        md.x = x; md.y = y;
                        st.members.push_back(md);
                    }
                }
                continue;
            }
        }

        if(starts(ln, "PLACES ")){
            int cnt = 0;
            if(sscanf(ln.c_str(), "PLACES %d", &cnt) == 1){
                i++;
                st.places.clear();
                st.places.reserve(cnt);
                for(int j=0; j<cnt && i<nlines; j++, i++){
                    string pl = trim_copy(lines[i]);
                    if(pl.empty()){ j--; continue; }

                    // name x y (name fara spatii)
                    char pname[200]; int x=0,y=0;
                    int ok = sscanf(pl.c_str(), "%199s %d %d", pname, &x, &y);
                    if(ok == 3){
                        PlaceDisp pd;
                        pd.name = pname;
                        pd.x = x; pd.y = y;
                        st.places.push_back(pd);
                    }
                }
                continue;
            }
        }

        i++;
    }

    return st;
}

//==================== MAIN ====================
int main(){
    InitWindow(LOGIN_W, LOGIN_H, "FAMILY LOCATOR");
    SetTargetFPS(60);

    //==================== TEXTURES LOGIN ====================
    Texture2D login_page   = LoadTexture("img/log in.png");
    Texture2D pop_no_user  = LoadTexture("img/nu_exista_user_popup.png");
    Texture2D pop_bad_pass = LoadTexture("img/parola_incorecta_popup.png");
    Texture2D pop_reg_ok   = LoadTexture("img/registration_ok_popup.png");
    Texture2D pop_taken    = LoadTexture("img/username_taken_popup.png");

    //==================== TEXTURES MAIN ====================
    Texture2D fundal_main  = LoadTexture("img/fundal_main.png"); // 1334x750
    Texture2D pin_blue   = LoadTexture("img/blue_pin.png");
    Texture2D pin_red    = LoadTexture("img/red_pin.png");
    Texture2D pin_place  = LoadTexture("img/pin.png");

    Texture2D popup_add_member = LoadTexture("img/add_member_popup.png");
    Texture2D popup_logout_btn = LoadTexture("img/log_out_popup.png");
    Texture2D popup_loc_req    = LoadTexture("img/location_req.png");
    Texture2D popup_loc_req2   = LoadTexture("img/loc_req.jpeg");
    Texture2D popup_loc_off    = LoadTexture("img/location_req_off.png");
    Texture2D popup_place_info = LoadTexture("img/place_info_popup.png");
    Texture2D popup_sos_msg    = LoadTexture("img/alert_msg_pipup.png");

    //==================== SERVER ====================
    con conexiune;
    bool srv_ok = conexiune.conecteaza("127.0.0.1", 2728);

    //==================== LOGIN HITBOX ====================
    Rectangle b_user = {850, 300, 238, 80};
    Rectangle b_pass = {850, 400, 238, 80};
    Rectangle b_log  = {733, 520, 160, 80};
    Rectangle b_reg  = {945, 520, 180, 80};
    Rectangle b_exit = {15,  630, 160, 70};

    string user, pass;
    int in = 0;
    int framesCounter = 0;

    ScreenState scr = SCR_LOGIN;
    PopupType pop = POP_NONE;
    bool pop_activ = false;

    //==================== MAIN UI COORDS (DESIGN SPACE 1334x750) ====================

    //==================== MAIN: coordonate in "design space" (1334x750) ====================
    // IMPORTANT: astea 4 le potrivesti cu DEBUG_UI=true pana pica perfect

    // (1) zona hartii (dreptunghiul exact al hartii din fundal)
    Rectangle mapLocal_design = { 510, 150, 780, 465 };  // <-- AJUSTEAZA

    // (2) panel membri (stanga)
    Rectangle panel_design = { 50, 150, 420, 420 };     // <-- AJUSTEAZA

    // (3) location ON/OFF: fundalul are "LOCATION:" deja, noi scriem doar ON/OFF
    Vector2 locTextPos_design = { 320, 650 };            // <-- AJUSTEAZA
    Rectangle locToggle_design = { 265, 650, 150, 55 };  // <-- AJUSTEAZA
    int locFontSize_design = 40;                         // marimea textului ON/OFF

    // (4) butoane jos (hitbox only)
    Rectangle b_sos_design    = { 500, 635, 90, 90 };    // <-- AJUSTEAZA
    Rectangle b_update_design = { 930, 635, 90, 90 };   // <-- AJUSTEAZA
    Rectangle b_places_design = { 1080, 635, 90, 90 };   // <-- AJUSTEAZA
    Rectangle b_set_design    = { 1190, 635, 90, 90 };   // <-- AJUSTEAZA

    //==================== STATE TEXT (server truth) ====================
    string state_raw;       // ultimul raspuns GET_STATE
    StateDisp state_disp;   // derivat din state_raw (nu e "adevar", e doar pt draw)

    int selMember = -1;     // doar UI
    int selPlace  = -1;     // doar UI

    // popup input
    string tmp_member_name;
    string loc_popup_text;  // text scurt pt popup location info

    // polling GET_STATE
    float poll_t = 0.0f;
    const float POLL_SEC = 0.6f; // la ~0.6 sec

    // SOS hold
    bool sosHolding = false;
    float sosTime = 0.0f;

    while(!WindowShouldClose()){
        framesCounter++;

        //==================== SERVER UPDATE ====================
        if(srv_ok){
            conexiune.actualizare();
            if(conexiune.msg){
                conexiune.msg = false;
                cout << "RECV: [" << conexiune.last << "]\n";
                string msg = trim_copy(conexiune.last);

                if(conexiune.last == "OK_LOC_ON")  state_disp.me_on = true;
                 if(conexiune.last == "OK_LOC_OFF") state_disp.me_on = false;


                if(msg == "OK_LOGIN"){
                    scr = SCR_MAIN;
                    SetWindowSize(MAIN_W, MAIN_H);

                    // imediat cerem state
                    conexiune.trimite("GET_STATE");
                }
                else if(msg.rfind("STATE", 0) == 0){
                    // e raspunsul nostru mare
                    state_raw = msg;
                    state_disp = parse_state_text(state_raw);

                    // daca nu are familie => SCR_NO_FAMILY (optional)
                    if(state_disp.fam_id == -1) scr = SCR_NO_FAMILY;
                    else scr = SCR_MAIN;

                    // clamp selectie
                    if(selMember >= (int)state_disp.members.size()) selMember = -1;
                    if(selPlace >= (int)state_disp.places.size()) selPlace = -1;
                }
                else if(msg.rfind("OK_UPDATE", 0) == 0){
                    // dupa update, cerem din nou state
                    conexiune.trimite("GET_STATE");
                }
                else if(msg == "OK_LOC_ON"){
                    state_disp.me_on=true;
                     conexiune.trimite("GET_STATE");}
                else if(msg == "OK_LOC_OFF"){
                     state_disp.me_on=false;
                    conexiune.trimite("GET_STATE");
                }
                else if(msg.rfind("OK_COORD",0)==0 || msg.rfind("OK_LOC",0)==0){
                    // raspuns la GET_LOCATION (popup info)
                    loc_popup_text = msg;
                    pop = POP_LOCATION_INFO;
                    pop_activ = true;
                }
                else if(msg == "SOS_RECEIVE"){
                    pop = POP_SOS_MSG;
                    pop_activ = true;
                }
                else{
                    // login popups
                    PopupType p = popup_from_server_login(msg);
                    if(p != POP_NONE){
                        pop = p;
                        pop_activ = true;
                    }
                }
            }
        }

        //==================== SCALE (design -> screen) ====================
        float sx = (float)GetScreenWidth()  / (float)DESIGN_W;
        float sy = (float)GetScreenHeight() / (float)DESIGN_H;

        auto RX = [&](float x){ return x*sx; };
        auto RY = [&](float y){ return y*sy; };
        auto RW = [&](float w){ return w*sx; };
        auto RH = [&](float h){ return h*sy; };

        Rectangle mapLocal = { RX(mapLocal_design.x), RY(mapLocal_design.y), RW(mapLocal_design.width), RH(mapLocal_design.height) };
        Rectangle panelMembers = { RX(panel_design.x), RY(panel_design.y), RW(panel_design.width), RH(panel_design.height) };

        Vector2 locTextPos = { RX(locTextPos_design.x), RY(locTextPos_design.y) };
        Rectangle rLocationToggle = { RX(locToggle_design.x), RY(locToggle_design.y), RW(locToggle_design.width), RH(locToggle_design.height) };
        int locFontSize = (int)RH((float)locFontSize_design);

        Rectangle b_sos    = { RX(b_sos_design.x),    RY(b_sos_design.y),    RW(b_sos_design.width),    RH(b_sos_design.height) };
        Rectangle b_update = { RX(b_update_design.x), RY(b_update_design.y), RW(b_update_design.width), RH(b_update_design.height) };
        Rectangle b_places = { RX(b_places_design.x), RY(b_places_design.y), RW(b_places_design.width), RH(b_places_design.height) };
        Rectangle b_set    = { RX(b_set_design.x),    RY(b_set_design.y),    RW(b_set_design.width),    RH(b_set_design.height) };

        Vector2 m = GetMousePosition();

        //==================== POLL GET_STATE ====================
        if((scr == SCR_MAIN || scr == SCR_NO_FAMILY) && srv_ok && !pop_activ){
            poll_t += GetFrameTime();
            if(poll_t >= POLL_SEC){
                poll_t = 0.0f;
                conexiune.trimite("GET_STATE");
            }
        }

        //==================== INPUT ====================
        if(scr == SCR_LOGIN){
            if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                if(pop_activ){
                    pop_activ = false;
                    pop = POP_NONE;
                }else{
                    if(mouse_in(b_user)) in=1;
                    else if(mouse_in(b_pass)) in=2;
                    else in=0;

                    if(mouse_in(b_exit)){
                        if(srv_ok) conexiune.trimite("LOGOUT");
                        break;
                    }

                    if(mouse_in(b_log)){
                        if(srv_ok && !user.empty() && !pass.empty()){
                            conexiune.trimite("LOGIN " + user + " " + pass);
                        }else{
                            pop = POP_INFO_LOGIN_EMPTY;
                            pop_activ = true;
                        }
                    }

                    if(mouse_in(b_reg)){
                        if(srv_ok && !user.empty() && !pass.empty()){
                            conexiune.trimite("REGISTER " + user + " " + pass);
                        }else{
                            pop = POP_INFO_LOGIN_EMPTY;
                            pop_activ = true;
                        }
                    }
                }
            }

            input_text(user, (in==1 && !pop_activ), 20);
            input_text(pass, (in==2 && !pop_activ), 20);
        }

        if(scr == SCR_MAIN || scr == SCR_NO_FAMILY){
            // location ON/OFF (doar text peste fundal)
            if(!pop_activ && click_in(rLocationToggle) && srv_ok){
                if(state_disp.me_on) conexiune.trimite("TURN_LOCATION_OFF");
                else conexiune.trimite("TURN_LOCATION_ON");

                 conexiune.trimite("GET_STATE");
            }

            // sos hold 10 sec
            if(!pop_activ && mouse_in(b_sos) && IsMouseButtonDown(MOUSE_LEFT_BUTTON)){
                sosHolding = true;
                sosTime += GetFrameTime();
                if(sosTime >= 10.0f){
                    pop = POP_SOS_MSG;
                    pop_activ = true;
                    sosHolding = false;
                    sosTime = 0.0f;
                    if(srv_ok) conexiune.trimite("SOS");
                }
            }else{
                if(!IsMouseButtonDown(MOUSE_LEFT_BUTTON)){
                    sosHolding = false;
                    sosTime = 0.0f;
                }
            }

            // butoane jos
            if(!pop_activ){
                if(click_in(b_update)){
                    if(srv_ok) conexiune.trimite("UPDATE_LOCATION");
                }
                if(click_in(b_places)){
                    // manager places (mai facem dupa)
                }
                if(click_in(b_set)){
                    pop = POP_LOGOUT_CONFIRM;
                    pop_activ = true;
                }
            }

            // list membri (doar in SCR_MAIN, daca are familie)
            if(scr == SCR_MAIN && !pop_activ){
                float headerH = clampf(panelMembers.height * 0.18f, RH(60), RH(90));
                float listTop = panelMembers.y + headerH;
                float listH   = panelMembers.height - headerH;
                float addH    = clampf(listH * 0.16f, RH(50), RH(70));
                Rectangle rAdd = { panelMembers.x, panelMembers.y + panelMembers.height - addH, panelMembers.width, addH };

                // click add member => popup (trimite ADD_MEMBERS la Enter)
                if(click_in(rAdd)){
                    pop = POP_ADD_MEMBER;
                    pop_activ = true;
                    tmp_member_name.clear();
                }

                // sloturi membri
                int cnt = (int)state_disp.members.size();
                if(cnt < 1) cnt = 1;
                float slotsH = listH - addH;
                float slotH = slotsH / (float)cnt;

                for(int i=0; i<(int)state_disp.members.size(); i++){
                    Rectangle rSlot = { panelMembers.x, listTop + i*slotH, panelMembers.width, slotH };

                    if(click_in(rSlot)){
                        selMember = i;
                        selPlace = -1;
                    }

                    // icon pin la dreapta -> GET_LOCATION <username>
                    Rectangle rPin = { rSlot.x + rSlot.width - RW(70), rSlot.y + RH(10), RW(60), RH(60) };
                    if(click_in(rPin) && srv_ok){
                        selMember = i;
                        selPlace = -1;
                        conexiune.trimite("GET_LOCATION " + state_disp.members[i].name);
                        // popup se va deschide cand vine raspuns OK_COORD/OK_LOC
                    }
                }

                // click pe places pins (ca sa arati popup_place_info)
                for(int i=0;i<(int)state_disp.places.size();i++){
                    Vector2 p = map_to_screen(mapLocal, state_disp.places[i].x, state_disp.places[i].y);
                    Rectangle rp = { p.x - RW(12), p.y - RH(34), RW(24), RH(34) };
                    if(click_in(rp)){
                        selPlace = i;
                        selMember = -1;
                        // putem folosi popup_place_info imagine + scriem numele
                        pop = POP_LOCATION_INFO; // refolosim popup info simplu
                        pop_activ = true;
                        loc_popup_text = "PLACE " + state_disp.places[i].name;
                    }
                }
            }

            // popup close doar pe X
            if(pop_activ){
                if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                    Texture2D pimg = {0};
                    if(pop == POP_ADD_MEMBER) pimg = popup_add_member;
                    else if(pop == POP_LOGOUT_CONFIRM) pimg = popup_logout_btn;
                    else if(pop == POP_SOS_MSG) pimg = popup_sos_msg;
                    else if(pop == POP_LOCATION_INFO){
                        // folosim imaginea de location request daca exista, altfel fallback
                        pimg = popup_loc_req.id ? popup_loc_req : popup_loc_req2;
                    }

                    if(pimg.id){
                        Rectangle pr = rect_tex_center(pimg);
                        Rectangle xBtn = { pr.x + pr.width - 70, pr.y + 10, 60, 60 };
                        if(CheckCollisionPointRec(m, xBtn)){
                            pop_activ = false;
                            pop = POP_NONE;
                        }
                    }else{
                        // daca nu avem textura, inchidem cu click in colt (nu prea se intampla)
                    }
                }

                // input popup: ADD_MEMBER (Enter => ADD_MEMBERS <username>)
                if(pop == POP_ADD_MEMBER){
                    input_text(tmp_member_name, true, 20);
                    if(IsKeyPressed(KEY_ENTER) && srv_ok){
                        if(!tmp_member_name.empty()){
                            conexiune.trimite("ADD_MEMBERS " + tmp_member_name);
                            // dupa ce server confirma, noi cerem state iar
                            conexiune.trimite("GET_STATE");
                        }
                        pop_activ = false;
                        pop = POP_NONE;
                    }
                }

                // logout confirm: Enter
                if(pop == POP_LOGOUT_CONFIRM){
                    if(IsKeyPressed(KEY_ENTER)){
                        if(srv_ok) conexiune.trimite("LOGOUT");
                        user.clear(); pass.clear();
                        scr = SCR_LOGIN;
                        pop_activ = false; pop = POP_NONE;
                        SetWindowSize(LOGIN_W, LOGIN_H);
                    }
                }
            }
        }

        //==================== DRAW ====================
        BeginDrawing();
        ClearBackground(RAYWHITE);

        //--------- LOGIN ---------
        if(scr == SCR_LOGIN){
            if(login_page.id) DrawTexture(login_page, 0, 0, WHITE);

            DrawText(user.c_str(), (int)b_user.x + 10, (int)b_user.y + 25, 25, BLACK);
            if(in==1 && ((framesCounter/20)%2)==0){
                DrawText("_", (int)(b_user.x + 8 + MeasureText(user.c_str(), 25)), (int)(b_user.y + 12), 40, BLACK);
            }

            string stelute(pass.size(), '*');
            DrawText(stelute.c_str(), (int)b_pass.x + 10, (int)b_pass.y + 25, 25, BLACK);
            if(in==2 && ((framesCounter/20)%2)==0){
                DrawText("_", (int)(b_pass.x + 8 + MeasureText(stelute.c_str(), 25)), (int)(b_pass.y + 12), 40, BLACK);
            }

            if(pop_activ){
                DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(), Fade(BLACK,0.45f));

                if(pop == POP_INFO_LOGIN_EMPTY){
                    DrawRectangle(GetScreenWidth()/2-220, GetScreenHeight()/2-70, 440, 140, RAYWHITE);
                    DrawRectangleLines(GetScreenWidth()/2-220, GetScreenHeight()/2-70, 440, 140, BLACK);
                    DrawText("Completeaza user + parola!", GetScreenWidth()/2-200, GetScreenHeight()/2-10, 24, BLACK);
                }else{
                    Texture2D pimg = {0};
                    if(pop == POP_USER_NOT_FOUND) pimg = pop_no_user;
                    else if(pop == POP_WRONG_PASS) pimg = pop_bad_pass;
                    else if(pop == POP_REGISTER_OK) pimg = pop_reg_ok;
                    else if(pop == POP_USER_TAKEN) pimg = pop_taken;

                    if(pimg.id){
                        Rectangle pr = rect_tex_center(pimg);
                        DrawTexture(pimg, (int)pr.x, (int)pr.y, WHITE);
                    }
                }
            }
        }

        //--------- MAIN / NO_FAMILY ---------
        if(scr == SCR_MAIN || scr == SCR_NO_FAMILY){
            if(fundal_main.id) DrawTexture(fundal_main, 0, 0, WHITE);

            // ON/OFF (din server: ME ... on ...)
            DrawText(state_disp.me_on ? "ON" : "OFF",
                     (int)locTextPos.x, (int)locTextPos.y,
                     locFontSize,
                     state_disp.me_on ? GREEN : RED);

            // SOS progress
            if(sosHolding){
                float t = sosTime / 10.0f;
                if(t > 1) t = 1;
                DrawRectangle((int)b_sos.x, (int)b_sos.y - (int)RH(12), (int)(b_sos.width * t), (int)RH(6), YELLOW);
            }

            // debug boxes
            if(DEBUG_UI){
                DrawRectangleLines((int)mapLocal.x, (int)mapLocal.y, (int)mapLocal.width, (int)mapLocal.height, RED);
                DrawRectangleLines((int)panelMembers.x, (int)panelMembers.y, (int)panelMembers.width, (int)panelMembers.height, BLUE);
                DrawRectangleLines((int)rLocationToggle.x,(int)rLocationToggle.y,(int)rLocationToggle.width,(int)rLocationToggle.height, ORANGE);
                DrawRectangleLines((int)b_sos.x,(int)b_sos.y,(int)b_sos.width,(int)b_sos.height, GREEN);
                DrawRectangleLines((int)b_update.x,(int)b_update.y,(int)b_update.width,(int)b_update.height, GREEN);
                DrawRectangleLines((int)b_places.x,(int)b_places.y,(int)b_places.width,(int)b_places.height, GREEN);
                DrawRectangleLines((int)b_set.x,(int)b_set.y,(int)b_set.width,(int)b_set.height, GREEN);
            }

            // PINS membri + places (din server)
            // membri: pin rosu pt selectat, albastru pt rest (dar doar daca loc_on si x/y != -1)
            for(int i=0;i<(int)state_disp.members.size();i++){
                const auto& mb = state_disp.members[i];
                if(mb.loc_on == 0) continue;
                if(mb.x < 0 || mb.y < 0) continue;

                Vector2 p = map_to_screen(mapLocal, mb.x, mb.y);
                // clamp in mapRect (siguranta)
                p.x = clampf(p.x, mapLocal.x + RW(10), mapLocal.x + mapLocal.width - RW(10));
                p.y = clampf(p.y, mapLocal.y + RH(10), mapLocal.y + mapLocal.height - RH(10));

                if(i == selMember) draw_pin_scaled(pin_red,  p, RW(28), RH(40));
                else               draw_pin_scaled(pin_blue, p, RW(28), RH(40));
            }

            for(int i=0;i<(int)state_disp.places.size();i++){
                const auto& pl = state_disp.places[i];
                if(pl.x < 0 || pl.y < 0) continue;

                Vector2 p = map_to_screen(mapLocal, pl.x, pl.y);
                p.x = clampf(p.x, mapLocal.x + RW(10), mapLocal.x + mapLocal.width - RW(10));
                p.y = clampf(p.y, mapLocal.y + RH(10), mapLocal.y + mapLocal.height - RH(10));

                draw_pin_scaled(pin_place, p, RW(24), RH(34));
            }// --- dimensiuni pini (aceleași ca în draw_pin_scaled)
float memPinW = RW(28), memPinH = RH(40);
float plcPinW = RW(24), plcPinH = RH(34);

// membri
for(int i=0;i<(int)state_disp.members.size();i++){
    const auto& mb = state_disp.members[i];
    if(mb.loc_on == 0) continue;
    if(mb.x < 0 || mb.y < 0) continue;

    Vector2 p = map_to_screen(mapLocal, mb.x, mb.y);

    // clamp corect pt anchor jos-centru
    p.x = clampf(p.x, mapLocal.x + memPinW/2.0f, mapLocal.x + mapLocal.width  - memPinW/2.0f);
    p.y = clampf(p.y, mapLocal.y + memPinH,       mapLocal.y + mapLocal.height);

    if(i == selMember) draw_pin_scaled(pin_red,  p, memPinW, memPinH);
    else               draw_pin_scaled(pin_blue, p, memPinW, memPinH);
}

// places
for(int i=0;i<(int)state_disp.places.size();i++){
    const auto& pl = state_disp.places[i];
    if(pl.x < 0 || pl.y < 0) continue;

    Vector2 p = map_to_screen(mapLocal, pl.x, pl.y);

    p.x = clampf(p.x, mapLocal.x + plcPinW/2.0f, mapLocal.x + mapLocal.width  - plcPinW/2.0f);
    p.y = clampf(p.y, mapLocal.y + plcPinH,       mapLocal.y + mapLocal.height);

    draw_pin_scaled(pin_place, p, plcPinW, plcPinH);
}


            // LISTA membri (custom, din server)
            if(scr == SCR_MAIN){
                float headerH = clampf(panelMembers.height * 0.18f, RH(60), RH(90));
                Rectangle rHeader = { panelMembers.x, panelMembers.y, panelMembers.width, headerH };

                DrawRectangleRec(rHeader, Color{210,200,255,255});
                DrawRectangleLines((int)rHeader.x,(int)rHeader.y,(int)rHeader.width,(int)rHeader.height, BLACK);
                DrawText("FAMILY MEMBERS", (int)(rHeader.x + RW(18)), (int)(rHeader.y + headerH*0.25f), (int)RH(28), BLACK);

                float listTop = panelMembers.y + headerH;
                float listH   = panelMembers.height - headerH;

                float addH = clampf(listH * 0.16f, RH(50), RH(70));
                Rectangle rAdd = { panelMembers.x, panelMembers.y + panelMembers.height - addH, panelMembers.width, addH };

                int n = (int)state_disp.members.size();
                if(n < 1) n = 1;
                float slotsH = listH - addH;
                float slotH = slotsH / (float)n;

                for(int i=0;i<(int)state_disp.members.size();i++){
                    Rectangle rSlot = { panelMembers.x, listTop + i*slotH, panelMembers.width, slotH };

                    Color bg = (i==selMember)? Color{230,225,255,255} : RAYWHITE;
                    DrawRectangleRec(rSlot, bg);

                    if(i > 0){
                        DrawLine((int)rSlot.x, (int)rSlot.y, (int)(rSlot.x + rSlot.width), (int)rSlot.y, BLACK);
                    }
                    DrawRectangleLines((int)rSlot.x,(int)rSlot.y,(int)rSlot.width,(int)rSlot.height, BLACK);

                    DrawText(state_disp.members[i].name.c_str(),
                             (int)(rSlot.x + RW(18)),
                             (int)(rSlot.y + slotH*0.30f),
                             (int)RH(22),
                             BLACK);

                    // status mic (optional)
                    DrawText(state_disp.members[i].loc_on ? "ON" : "OFF",
                             (int)(rSlot.x + rSlot.width - RW(140)),
                             (int)(rSlot.y + slotH*0.30f),
                             (int)RH(20),
                             state_disp.members[i].loc_on ? DARKGREEN : MAROON);

                    // icon pin
                    Rectangle rPin = { rSlot.x + rSlot.width - RW(70), rSlot.y + RH(10), RW(60), RH(60) };
                    DrawCircle((int)(rPin.x + rPin.width/2), (int)(rPin.y + rPin.height/2), (float)RH(14),
                               state_disp.members[i].loc_on ? PURPLE : DARKGRAY);
                }

                // ADD MEMBER
                DrawRectangleRec(rAdd, RAYWHITE);
                DrawRectangleLines((int)rAdd.x,(int)rAdd.y,(int)rAdd.width,(int)rAdd.height, BLACK);

                float cx = rAdd.x + RW(35);
                float cy = rAdd.y + rAdd.height/2;
                DrawCircleLines((int)cx, (int)cy, (float)RH(16), BLACK);
                DrawLine((int)(cx - RW(8)), (int)cy, (int)(cx + RW(8)), (int)cy, BLACK);
                DrawLine((int)cx, (int)(cy - RH(8)), (int)cx, (int)(cy + RH(8)), BLACK);

                DrawText("ADD MEMBER", (int)(rAdd.x + RW(80)), (int)(rAdd.y + rAdd.height*0.32f), (int)RH(20), BLACK);

            }
        }

        //--------- POPUPS (doar X inchide) ---------
        if(pop_activ && (scr == SCR_MAIN || scr == SCR_NO_FAMILY)){
            DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(), Fade(BLACK,0.45f));

            Texture2D pimg = {0};
            if(pop == POP_ADD_MEMBER) pimg = popup_add_member;
            else if(pop == POP_LOGOUT_CONFIRM) pimg = popup_logout_btn;
            else if(pop == POP_SOS_MSG) pimg = popup_sos_msg;
            else if(pop == POP_LOCATION_INFO){
                // daca membrul e limited, poti afisa popup_loc_off; aici folosim request
                pimg = popup_loc_req.id ? popup_loc_req : popup_loc_req2;
            }

            if(pimg.id){
                Rectangle pr = rect_tex_center(pimg);
                DrawTexture(pimg, (int)pr.x, (int)pr.y, WHITE);

                // text in popup
                if(pop == POP_ADD_MEMBER){
                    DrawText(tmp_member_name.c_str(), (int)pr.x + 340, (int)pr.y + 170, 25, BLACK);
                }

                if(pop == POP_LOCATION_INFO){
                    // afisam mesajul primit: OK_COORD ... / OK_LOC ...
                    // (simplu; daca vrei frumos, facem parsing dedicat)
                    DrawText(loc_popup_text.c_str(), (int)pr.x + 60, (int)pr.y + (int)(pr.height - 80), 18, BLACK);
                }

                if(DEBUG_UI){
                    Rectangle xBtn = { pr.x + pr.width - 70, pr.y + 10, 60, 60 };
                    DrawRectangleLines((int)xBtn.x,(int)xBtn.y,(int)xBtn.width,(int)xBtn.height, RED);
                }
            }
        }

        EndDrawing();
    }

    // cleanup
    UnloadTexture(login_page);
    UnloadTexture(pop_no_user);
    UnloadTexture(pop_bad_pass);
    UnloadTexture(pop_reg_ok);
    UnloadTexture(pop_taken);

    UnloadTexture(fundal_main);
    UnloadTexture(pin_blue);
    UnloadTexture(pin_red);
    UnloadTexture(pin_place);

    UnloadTexture(popup_add_member);
    UnloadTexture(popup_logout_btn);
    UnloadTexture(popup_loc_req);
    UnloadTexture(popup_loc_req2);
    UnloadTexture(popup_loc_off);
    UnloadTexture(popup_place_info);
    UnloadTexture(popup_sos_msg);

    CloseWindow();
    return 0;
}
//============================ end main.cpp =============================
