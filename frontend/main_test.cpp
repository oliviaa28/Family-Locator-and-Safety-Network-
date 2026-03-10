#include "raylib.h"
#include "conectare.h"

#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <cmath>
using namespace std;

const int LOGIN_W = 1288, LOGIN_H = 730, MAIN_W = 1334, MAIN_H = 750;
const int DESIGN_W = 1334, DESIGN_H = 750;
const int MAP_W = 640, MAP_H = 385;
bool DEBUG_UI =false;
static Font g_ui_font = {0};
static bool g_ui_font_loaded = false;
static const float UI_FONT_SPACING = 1.0f;
static const float POP_LOGOUT_SCALE = 0.85f;
static const float POP_SOS_SCALE = 0.90f;

static Font get_ui_font()
{
    return g_ui_font_loaded ? g_ui_font : GetFontDefault();
}

static void DrawTextF(const char *text, int posX, int posY, int fontSize, Color color)
{
    Font font = get_ui_font();
    DrawTextEx(font, text, (Vector2){(float)posX, (float)posY}, (float)fontSize, UI_FONT_SPACING, color);
}

static int MeasureTextF(const char *text, int fontSize)
{
    Font font = get_ui_font();
    Vector2 size = MeasureTextEx(font, text, (float)fontSize, UI_FONT_SPACING);
    return (int)size.x;
}

#define DrawText DrawTextF
#define MeasureText MeasureTextF
//____________________________________________________________________________________________
static void img_centrat(Texture2D img)
{
    int x = 0, y = 0;
    y = (GetScreenHeight() - img.height) / 2;
    x = (GetScreenWidth() - img.width) / 2;
    DrawTexture(img, x, y, WHITE);
}

static void draw_loading_screen(Texture2D img)
{
    BeginDrawing();
    ClearBackground(Color{235, 230, 255, 255});
    if (img.id)
    {
        float sw = (float)GetScreenWidth() / (float)img.width;
        float sh = (float)GetScreenHeight() / (float)img.height;
        float s = (sw < sh) ? sw : sh;
        s *= 0.9f;
        Rectangle src = {0, 0, (float)img.width, (float)img.height};
        Rectangle dst = {0, 0, img.width * s, img.height * s};
        dst.x = (GetScreenWidth() - dst.width) * 0.5f;
        dst.y = (GetScreenHeight() - dst.height) * 0.5f;
        DrawTexturePro(img, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
    }
    else
    {
        DrawText("LOADING...", GetScreenWidth() / 2 - 80, GetScreenHeight() / 2 - 10, 24, DARKBLUE);
    }
    EndDrawing();
}

static float match_popup_scale(Texture2D ref_img, Texture2D img)
{
    if (!ref_img.id || !img.id)
        return 1.0f;
    float sw = (float)ref_img.width / (float)img.width;
    float sh = (float)ref_img.height / (float)img.height;
    return (sw < sh) ? sw : sh;
}

static Rectangle rect_center_scaled(Texture2D img, float scale)
{
    Rectangle r;
    r.x = 0;
    r.y = 0;
    r.width = (float)img.width * scale;
    r.height = (float)img.height * scale;
    r.x = (GetScreenWidth() - r.width) / 2.0f;
    r.y = (GetScreenHeight() - r.height) / 2.0f;
    return r;
}

static void img_centrat_scaled(Texture2D img, float scale)
{
    if (!img.id)
        return;
    Rectangle src = {0, 0, (float)img.width, (float)img.height};
    Rectangle dst = rect_center_scaled(img, scale);
    DrawTexturePro(img, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
}

string trim(string s)
{
    int i = 0;
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ' || s.back() == '\t'))
        s.pop_back();

    while (i < (int)s.size() && (s[i] == '\n' || s[i] == '\r' || s[i] == ' ' || s[i] == '\t'))
        i++;

    return s.substr(i);
}

static bool car_valid(char c)
{
    if (isalpha((unsigned char)c))
        return true;
    if (isdigit((unsigned char)c))
        return true;
    if (c == '_' || c == '@' || c == '.' || c == ' ')
        return true;
    return false;
}

static void input_text(string &txt, bool activ, int max_lg)
{
    if (!activ)
        return;

    int key = GetCharPressed();
    while (key > 0)
    {
        if (car_valid((char)key))
        {
            if ((int)txt.size() < max_lg)
                txt.push_back((char)key);
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && !txt.empty())
        txt.pop_back();
}

bool mouse_in(Rectangle r)
{
    return CheckCollisionPointRec(GetMousePosition(), r);
}
bool click_in(Rectangle r)
{
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mouse_in(r))
        return true;
    return false;
}

float inghesuie_f(float x, float a, float b)
{
    if (x < a)
        return a;
    if (x > b)
        return b;
    return x;
}

static string format_time_text(long ts)
{
    if (ts <= 0)
        return "-";
    time_t t = (time_t)ts;
    struct tm *tmv = localtime(&t);
    if (!tmv)
        return "-";
    char buf[32];
    strftime(buf, sizeof(buf), "%d.%m %H:%M", tmv);
    return string(buf);
}

void pin_scalat(Texture2D pin, Vector2 pos, float w, float h)
{
    if (!pin.id)
        return;

    Rectangle src, dst;
    src = {0, 0, (float)pin.width, (float)pin.height};
    dst = {pos.x, pos.y, w, h};
    Vector2 o = {w / 2, h};

    DrawTexturePro(pin, src, dst, o, 0.0f, WHITE);
}

Vector2 fereastra_harta(Rectangle harta, int x, int y)
{
    float a, b;
    a = harta.x + (float)x * harta.width / (float)MAP_W;
    b = harta.y + (float)y * harta.height / (float)MAP_H;
    return {a, b};
}

bool incepe_cu(const string &s, const char *p)
{
    if (s.rfind(p, 0) == 0)
        return true;
    return false;
}

static float RX(float x, float sx) { return x * sx; }
static float RY(float y, float sy) { return y * sy; }
static float RW(float w, float sx) { return w * sx; }
static float RH(float h, float sy) { return h * sy; }

enum ScreenState
{
    SCR_LOGIN = 0,
    SCR_MAIN,
    SCR_NO_FAMILY
};

enum TipPopup
{
    POP_NIMIC = 0,
    POP_INFO_LOGIN_EMPTY,
    POP_INVALID_REQUEST,
    POP_USER_INEXISTENT,
    POP_PAROLA_GRESITA,
    POP_REGISTER_OK,
    POP_USER_OCUPAT,

    POP_ADD_MEMBER,
    POP_ADD_PLACE,
    POP_LOCATION_INFO,
    POP_LOGOUT_CONFIRM,
    POP_SOS_SELF,
    POP_SOS_MSG,

    POP_JOIN_FAMILY,
    POP_LEAVE_FAMILY,POP_CREATE_FAMILY,

    POP_FAMILY_NOT_FOUND,
    POP_FAMILY_JOIN_OK,
    POP_FAMILY_CREATE_OK,
    POP_FAMILY_ERROR,
    POP_PLACE_ERROR,
    POP_MEMBER_NOT_FOUND,
    POP_MEMBER_ALREADY,
    POP_LOC_ARRIVED,
    POP_LOC_LEFT

};

enum LocPopupKind
{
    LOC_POP_NONE = 0,
    LOC_POP_COORD,
    LOC_POP_LOC,
    LOC_POP_OFF,
    LOC_POP_PLACE
};

static TipPopup popup_login(const string &msg)
{
    if (msg == "ERR_USER_NOT_FOUND")
        return POP_USER_INEXISTENT;
    if (msg == "ERR_WRONG_PASS")
        return POP_PAROLA_GRESITA;
    if (msg == "ERR_USER_TAKEN")
        return POP_USER_OCUPAT;
    if (msg == "OK_REGISTER")
        return POP_REGISTER_OK;
    return POP_NIMIC;
}

struct Member
{
    string name;
    int loc_on;
    int x, y;
};

struct ItemFamilie
{
    int id;
    string nume;
};
vector<ItemFamilie> my_families;

vector<ItemFamilie> extragere_fam(const string &txt)
{

    vector<string> linii;
    vector<ItemFamilie> lista;
    string linie;
    if (txt.empty())
        return lista;

    for (char ch : txt)
    {
        if (ch == '\n')
        {
            linii.push_back(trim(linie));
            linie.clear();
        }
        else
        {
            if (ch != '\r')
                linie.push_back(ch);
        }
    }
    if (!linie.empty())
        linii.push_back(trim(linie));

    for (int i = 0; i < (int)linii.size(); i++)
    {
        string ln = trim(linii[i]);

        if (ln == "FAMILIES_LIST")
        {
            continue;
        }
        if (ln == "END" || ln == "EMPTY")
            break;

        int id = -1;
        if (sscanf(ln.c_str(), "%d", &id) == 1)
        {
            int p = 0;
            while (p < (int)ln.size() && (ln[p] >= '0' && ln[p] <= '9'))
                p++; // ignoram id
            while (p < (int)ln.size() && ln[p] == ' ')
                p++; // ig spatii

            if (p < (int)ln.size())
            {
                string nume = trim(ln.substr(p));
                if (!nume.empty())
                {
                    ItemFamilie a;
                    a.id = id;
                    a.nume = nume;
                    lista.push_back(a);
                }
            }
        }
    }
    return lista;
}

struct Place
{
    string name;
    int x, y;
};

struct State
{
    int fam_id = -1;
    string fam_name = "-";
    int me_x = -1, me_y = -1, me_on = 0;
    long me_last = 0;

    vector<Member> members;
    vector<Place> places;
};

State extragere(const string &txt)
{
    State st;
    if (txt.empty())
        return st;

    vector<string> linii;
    string linie;

    for (char ch : txt)
    {
        if (ch == '\n')
        {
            linii.push_back(trim(linie));
            linie.clear();
        }
        else
        {
            if (ch != '\r')
                linie.push_back(ch);
        }
    }
    if (!linie.empty())
        linii.push_back(trim(linie));

    for (int i = 0; i < (int)linii.size();)
    {
        string ln = trim(linii[i]);

        if (ln == "STATE")
        {
            i++;
            continue;
        }
        if (ln == "END")
            break;

        if (incepe_cu(ln, "FAMILY "))
        {
            int id = -1;
            if (sscanf(ln.c_str(), "FAMILY %d", &id) == 1)
            {
                st.fam_id = id;
                int p = 7;
                while (p < (int)ln.size() && ln[p] == ' ')
                    p++;
                while (p < (int)ln.size() && (ln[p] >= '0' && ln[p] <= '9'))
                    p++;
                while (p < (int)ln.size() && ln[p] == ' ')
                    p++;

                if (p < (int)ln.size())
                {
                    st.fam_name = trim(ln.substr(p));
                    if (st.fam_name.empty())
                        st.fam_name = "-";
                }
            }
            i++;
            continue;
        }

        if (incepe_cu(ln, "ME "))
        {
            int x = -1, y = -1, on = 0;
            long last = 0;
            if (sscanf(ln.c_str(), "ME %d %d %d %ld", &x, &y, &on, &last) == 4)
            {
                st.me_x = x;
                st.me_y = y;
                st.me_on = on;
                st.me_last = last;
            }
            i++;
            continue;
        }

        if (incepe_cu(ln, "MEMBERS "))
        {
            int n = 0;
            if (sscanf(ln.c_str(), "MEMBERS %d", &n) == 1)
            {
                st.members.clear();
                st.members.reserve(n);
                i++;

                for (int j = 0; j < n && i < (int)linii.size(); j++, i++)
                {
                    char usr_name[120];
                    int on = 0, x = 0, y = 0;
                    int ok = sscanf(linii[i].c_str(), "%119s %d %d %d", usr_name, &on, &x, &y);
                    if (ok == 4)
                    {
                        Member m;
                        m.name = usr_name;
                        m.loc_on = on;
                        m.x = x;
                        m.y = y;
                        st.members.push_back(m);
                    }
                }
                continue;
            }
        }

        if (incepe_cu(ln, "PLACES "))
        {
            int k = 0;
            if (sscanf(ln.c_str(), "PLACES %d", &k) == 1)
            {
                st.places.clear();
                st.places.reserve(k);
                i++;

                for (int j = 0; j < k && i < (int)linii.size(); j++, i++)
                {
                    char name[200];
                    int x = 0, y = 0;
                    int ok = sscanf(linii[i].c_str(), "%s %d %d", name, &x, &y);
                    if (ok == 3)
                    {
                        Place p;
                        p.name = name;
                        p.x = x;
                        p.y = y;
                        st.places.push_back(p);
                    }
                }
                continue;
            }
        }
        i++;
    }
    return st;
}

Texture2D textura_popup(TipPopup tip, Texture2D popup_add_member, Texture2D popup_add_place, Texture2D popup_logout_btn, Texture2D popup_loc_req, Texture2D popup_loc_req2,
                        Texture2D popup_loc_arrived, Texture2D popup_loc_left, Texture2D popup_sos_self, Texture2D popup_sos_msg,
                        Texture2D pop_no_user, Texture2D pop_bad_pass, Texture2D pop_reg_ok, Texture2D pop_taken, Texture2D popup_join_img, Texture2D popup_create_img,
                        Texture2D popup_leave_confirm, Texture2D popup_invalid_req, Texture2D popup_user_not_exist, Texture2D popup_fam_id_invalid,
                        Texture2D popup_already_in_fam, Texture2D popup_now_in_fam, Texture2D popup_usr_already_in_fam)
{
    if (tip == POP_ADD_MEMBER)
        return popup_add_member;
    if (tip == POP_ADD_PLACE)
        return popup_add_place;
    if (tip == POP_LOGOUT_CONFIRM)
        return popup_logout_btn;
    if (tip == POP_SOS_SELF)
        return popup_sos_self;
    if (tip == POP_SOS_MSG)
        return popup_sos_msg;
    if (tip == POP_LOC_ARRIVED)
        return popup_loc_arrived;
    if (tip == POP_LOC_LEFT)
        return popup_loc_left;

    if (tip == POP_LOCATION_INFO)
    {
        if (popup_loc_req.id)
            return popup_loc_req;
        return popup_loc_req2;
    }

    if (tip == POP_USER_INEXISTENT)
        return pop_no_user;
    if (tip == POP_PAROLA_GRESITA)
        return pop_bad_pass;
    if (tip == POP_REGISTER_OK)
        return pop_reg_ok;
    if (tip == POP_USER_OCUPAT)
        return pop_taken;
    if (tip == POP_INFO_LOGIN_EMPTY || tip == POP_INVALID_REQUEST)
        return popup_invalid_req;

    if (tip == POP_FAMILY_NOT_FOUND)
        return popup_fam_id_invalid.id ? popup_fam_id_invalid : popup_invalid_req;
    if (tip == POP_FAMILY_JOIN_OK || tip == POP_FAMILY_CREATE_OK)
        return popup_now_in_fam;
    if (tip == POP_FAMILY_ERROR)
        return popup_already_in_fam.id ? popup_already_in_fam : popup_invalid_req;
    if (tip == POP_PLACE_ERROR)
        return popup_invalid_req;
    if (tip == POP_MEMBER_NOT_FOUND)
        return popup_user_not_exist.id ? popup_user_not_exist : popup_invalid_req;
    if (tip == POP_MEMBER_ALREADY)
        return popup_usr_already_in_fam.id ? popup_usr_already_in_fam : popup_invalid_req;

    if (tip == POP_JOIN_FAMILY)
        return popup_join_img;
    if (tip == POP_CREATE_FAMILY)
        return popup_create_img;
    if (tip == POP_LEAVE_FAMILY)
        return popup_leave_confirm;

    Texture2D none = {0};
    return none;
}

Rectangle rect_center(Texture2D img)
{
    Rectangle r;
    r.x = 0;
    r.y = 0;
    r.width = (float)img.width;
    r.height = (float)img.height;
    r.x = (GetScreenWidth() - r.width) / 2.0f;
    r.y = (GetScreenHeight() - r.height) / 2.0f;
    return r;
}

int main()
{
    InitWindow(LOGIN_W, LOGIN_H, "FAMILY LOCATOR");
    SetTargetFPS(60);
    Texture2D loading_img = LoadTexture("img/loading.jpeg");
    draw_loading_screen(loading_img);
    g_ui_font = LoadFontEx("Tinos-Regular.ttf", 32, 0, 0);
    g_ui_font_loaded = g_ui_font.texture.id != 0;
    if (g_ui_font_loaded)
        SetTextureFilter(g_ui_font.texture, TEXTURE_FILTER_BILINEAR);

    Texture2D login_page = LoadTexture("img/log in.png");
    Texture2D pop_no_user = LoadTexture("img/nu_exista_user_popup.png");
    Texture2D pop_bad_pass = LoadTexture("img/parola_incorecta_popup.png");
    Texture2D pop_reg_ok = LoadTexture("img/registration_ok_popup.png");
    Texture2D pop_taken = LoadTexture("img/username_taken_popup.png");

    Texture2D fundal_main = LoadTexture("img/main_page.jpeg");
    Texture2D pin_blue = LoadTexture("img/blue_pin.png");
    Texture2D pin_red = LoadTexture("img/red_pin.png");
    Texture2D pin_place = LoadTexture("img/pin.png");

    Texture2D popup_add_member = LoadTexture("img/add_member_popup.png");
    Texture2D popup_add_place = LoadTexture("img/add_place_popup.png");
    Texture2D popup_logout_btn = LoadTexture("img/log_out.jpeg");
    Texture2D popup_loc_req = LoadTexture("img/location_req.png");
    Texture2D popup_loc_req2 = LoadTexture("img/loc_req.jpeg");
    Texture2D popup_loc_arrived = LoadTexture("img/usr_has_arrived_popup.jpeg");
    Texture2D popup_loc_left = LoadTexture("img/usr_has_left_popup.jpeg");
    Texture2D popup_loc_off = LoadTexture("img/location_req_off.png");
    Texture2D popup_place_info = LoadTexture("img/place_info_popup.png");
    Texture2D popup_sos_self = LoadTexture("img/sos_popup.jpeg");
    Texture2D popup_sos_msg = LoadTexture("img/alert_msg_pipup.png");

    Texture2D box_fam_img = LoadTexture("img/fam_name_box.png");
    Texture2D antet_places = LoadTexture("img/antet_places.png");

    Texture2D popup_join_img = LoadTexture("img/join_family_popup.png");
    Texture2D popup_create_img = LoadTexture("img/create_family_popup.png");
    Texture2D popup_leave_confirm = LoadTexture("img/leave_fam.jpeg");
    Texture2D popup_invalid_req = LoadTexture("img/invalid_req.jpeg");
    Texture2D popup_user_not_exist = LoadTexture("img/this_usr_does_not_exist.jpeg");
    Texture2D popup_fam_id_invalid = LoadTexture("img/the_fam_id_is_invalid.jpeg");
    Texture2D popup_already_in_fam = LoadTexture("img/you_already_part_of_the_fam.jpeg");
    Texture2D popup_now_in_fam = LoadTexture("img/you_are_now_part_of_the_fam.jpeg");
    Texture2D popup_usr_already_in_fam = LoadTexture("img/usr_already_in_fam.jpeg");

    con conexiune;
    bool srv_ok = conexiune.conecteaza("127.0.0.1", 2728);

    // butoane la login
    Rectangle b_user = {850, 300, 238, 80};
    Rectangle b_pass = {850, 400, 238, 80};
    Rectangle b_log = {733, 520, 160, 80};
    Rectangle b_reg = {945, 520, 180, 80};
    Rectangle b_exit = {15, 630, 160, 70};

    Rectangle hartaD = {510, 150, 755, 440};
    Rectangle listaD = {50, 150, 420, 420};

    Vector2 poz_onD = {320, 650};
    Rectangle buton_onD = {265, 650, 150, 55};
    int marime_onD = 40;

    Rectangle b_sosD = {500, 635, 90, 90};
    Rectangle b_updateD = {930, 635, 90, 90};
    Rectangle b_placesD = {1080, 635, 90, 90};
    Rectangle b_setD = {1190, 635, 90, 90};

    Rectangle box_famD = {430, 30, 470, 55};
    Rectangle btn_popup_ok; // pentru submit la popup

    string user, pass;
    int in = 0, framesCounter = 0;

    ScreenState scr = SCR_LOGIN;
    TipPopup tip_pop = POP_NIMIC;
    bool pop_activ = false;

    string raw_state;
    State stare;

    int membru_selectat = -1, loc_selectat = -1;

    string tmp_member_name, mesaj_popup_loc;

    float timp_poll = 0.0f;
    const float POLL_SEC = 0.6f;

    bool sos_pending = false;
    bool sos_sent = false;
    float sos_timer = 0.0f;
    string sos_from_name;

    bool meniu_fam = false;
    bool meniu_places = false;
    string join_id_txt, create_name_txt;
    string place_name_txt;

    enum LastAction { ACT_NONE = 0, ACT_JOIN_FAMILY, ACT_CREATE_FAMILY, ACT_ADD_PLACE, ACT_ADD_MEMBER, ACT_SWITCH_FAMILY, ACT_LEAVE_FAMILY };
    int last_action = ACT_NONE;

    LocPopupKind loc_popup_kind = LOC_POP_NONE;
    string loc_req_user;
    string loc_user_txt, loc_location_txt, loc_lat_txt, loc_lon_txt, loc_last_txt;
    string loc_place_txt, loc_admin_txt, loc_coords_txt;
    string notif_user_txt, notif_loc_txt;

    while (!WindowShouldClose())
    {

        framesCounter++;
        Vector2 m = GetMousePosition();

        bool click_consumat = false;
        if (srv_ok)
        {
            conexiune.actualizare();
            if (conexiune.msg)
            {
                conexiune.msg = false;

                string msg = trim(conexiune.last);

                if (msg == "OK_LOGIN")
                {
                    scr = SCR_MAIN;
                    SetWindowSize(MAIN_W, MAIN_H);
                    conexiune.trimite("GET_STATE");
                }
                else if (msg.rfind("STATE", 0) == 0)
                {
                    raw_state = msg;
                    stare = extragere(raw_state);

                    if (stare.fam_id == -1)
                        scr = SCR_NO_FAMILY;
                    else
                        scr = SCR_MAIN;

                    if (membru_selectat >= (int)stare.members.size())
                        membru_selectat = -1;
                    if (loc_selectat >= (int)stare.places.size())
                        loc_selectat = -1;
                }
                else if (msg.rfind("OK_UPDATE", 0) == 0)
                {
                    conexiune.trimite("GET_STATE");
                }
                else if (msg == "OK_LOC_ON" || msg == "OK_LOC_OFF")
                {
                    if (msg == "OK_LOC_ON")
                        stare.me_on = 1;
                    else
                        stare.me_on = 0;
                    conexiune.trimite("GET_STATE");
                }
                else if (msg.rfind("OK_COORD", 0) == 0)
                {
                    loc_popup_kind = LOC_POP_NONE;
                    loc_user_txt.clear();
                    loc_location_txt.clear();
                    loc_lat_txt.clear();
                    loc_lon_txt.clear();
                    loc_last_txt.clear();
                    int lat = 0, lon = 0, loc_on = 0;
                    long last = 0;
                    if (sscanf(msg.c_str(), "OK_COORD %d %d %d %ld", &lat, &lon, &loc_on, &last) == 4)
                    {
                        loc_user_txt = loc_req_user;
                        if (loc_user_txt.empty())
                            loc_user_txt = "-";
                        loc_lat_txt = to_string(lat);
                        loc_lon_txt = to_string(lon);
                        loc_last_txt = format_time_text(last);
                        loc_location_txt.clear();
                        loc_popup_kind = loc_on ? LOC_POP_COORD : LOC_POP_OFF;
                    }
                    tip_pop = POP_LOCATION_INFO;
                    pop_activ = true;
                }
                else if (msg.rfind("OK_LOC", 0) == 0)
                {
                    loc_popup_kind = LOC_POP_NONE;
                    loc_user_txt.clear();
                    loc_location_txt.clear();
                    loc_lat_txt.clear();
                    loc_lon_txt.clear();
                    loc_last_txt.clear();
                    char loc_name[120];
                    int lat = 0, lon = 0, loc_on = 0;
                    long last = 0;
                    if (sscanf(msg.c_str(), "OK_LOC %119s %d %d %d %ld", loc_name, &lat, &lon, &loc_on, &last) == 5)
                    {
                        loc_user_txt = loc_req_user;
                        if (loc_user_txt.empty())
                            loc_user_txt = "-";
                        loc_location_txt = loc_name;
                        loc_lat_txt = to_string(lat);
                        loc_lon_txt = to_string(lon);
                        loc_last_txt = format_time_text(last);
                        loc_popup_kind = loc_on ? LOC_POP_LOC : LOC_POP_OFF;
                    }
                    tip_pop = POP_LOCATION_INFO;
                    pop_activ = true;
                }
                else if (msg.rfind("SOS_RECEIVE", 0) == 0)
                {
                    char from_name[120];
                    sos_from_name.clear();
                    if (sscanf(msg.c_str(), "SOS_RECEIVE %119s", from_name) == 1)
                        sos_from_name = from_name;
                    tip_pop = POP_SOS_MSG;
                    pop_activ = true;
                }
                else if (msg.rfind("NOTIF_ARRIVED", 0) == 0)
                {
                    char uname[120], locname[120];
                    if (sscanf(msg.c_str(), "NOTIF_ARRIVED %119s %119s", uname, locname) == 2)
                    {
                        notif_user_txt = uname;
                        notif_loc_txt = locname;
                        tip_pop = POP_LOC_ARRIVED;
                        pop_activ = true;
                    }
                }
                else if (msg.rfind("NOTIF_LEFT", 0) == 0)
                {
                    char uname[120], locname[120];
                    if (sscanf(msg.c_str(), "NOTIF_LEFT %119s %119s", uname, locname) == 2)
                    {
                        notif_user_txt = uname;
                        notif_loc_txt = locname;
                        tip_pop = POP_LOC_LEFT;
                        pop_activ = true;
                    }
                }
                else if (msg.rfind("FAMILIES_LIST", 0) == 0)
                {
                    my_families = extragere_fam(msg);
                    meniu_fam = true;
                }
                else if (msg == "OK_ENTER_FAM")
                {
                    tip_pop = POP_FAMILY_JOIN_OK;
                    pop_activ = true;
                    conexiune.trimite("GET_STATE");
                    last_action = ACT_NONE;
                }
                else if (msg == "OK_NEW_FAM")
                {
                    tip_pop = POP_FAMILY_CREATE_OK;
                    pop_activ = true;
                    conexiune.trimite("GET_STATE");
                    last_action = ACT_NONE;
                }
                else if (msg == "OK_LEAVE_FAM" || msg == "OK_SWITCHED")
                {
                    conexiune.trimite("GET_STATE");
                    last_action = ACT_NONE;
                }
                else if (msg == "OK_NEW_MEM")
                {
                    conexiune.trimite("GET_STATE");
                    last_action = ACT_NONE;
                }
                else if (msg.rfind("OK_NEW_LOC", 0) == 0)
                {
                    tip_pop = POP_NIMIC;
                    pop_activ = false;
                    conexiune.trimite("GET_STATE");
                    last_action = ACT_NONE;
                }
                else if (msg == "ERR_USER_NOT_FOUND")
                {
                    if (last_action == ACT_ADD_MEMBER)
                        tip_pop = POP_MEMBER_NOT_FOUND;
                    else
                        tip_pop = POP_USER_INEXISTENT;
                    pop_activ = true;
                    last_action = ACT_NONE;
                }
                else if (msg == "ERR_USER_ALREADY_IN_FAM")
                {
                    tip_pop = POP_MEMBER_ALREADY;
                    pop_activ = true;
                    last_action = ACT_NONE;
                }
                else if (msg == "ERR_FAMILY_NOT_FOUND")
                {
                    tip_pop = POP_FAMILY_NOT_FOUND;
                    pop_activ = true;
                    last_action = ACT_NONE;
                }
                else if (msg == "ERR_ALREADY_IN_FAM")
                {
                    tip_pop = POP_FAMILY_ERROR;
                    pop_activ = true;
                    last_action = ACT_NONE;
                }
                else if (msg == "ERR_NOT_MEMBER")
                {
                    tip_pop = POP_INVALID_REQUEST;
                    pop_activ = true;
                    last_action = ACT_NONE;
                }
                else if (msg == "ERR_PARAM")
                {
                    if (last_action == ACT_ADD_PLACE)
                        tip_pop = POP_PLACE_ERROR;
                    else if (last_action == ACT_ADD_MEMBER)
                        tip_pop = POP_MEMBER_NOT_FOUND;
                    else
                        tip_pop = POP_INVALID_REQUEST;
                    pop_activ = true;
                    last_action = ACT_NONE;
                }
                else
                {
                    TipPopup p = popup_login(msg);
                    if (p != POP_NIMIC)
                    {
                        tip_pop = p;
                        pop_activ = true;
                    }
                }
            }
        }

        if ((scr == SCR_MAIN || scr == SCR_NO_FAMILY) && srv_ok && !pop_activ)
        {
            timp_poll += GetFrameTime();
            if (timp_poll >= POLL_SEC)
            {
                timp_poll = 0.0f;
                conexiune.trimite("GET_STATE");
            }
        }

        if (pop_activ && tip_pop == POP_SOS_SELF && sos_pending)
        {
            sos_timer += GetFrameTime();
            if (!sos_sent && sos_timer >= 10.0f)
            {
                if (srv_ok)
                    conexiune.trimite("SOS");
                sos_sent = true;
            }
        }
        else
        {
            sos_pending = false;
            sos_sent = false;
            sos_timer = 0.0f;
        }

        float sx = (float)GetScreenWidth() / (float)DESIGN_W;
        float sy = (float)GetScreenHeight() / (float)DESIGN_H;

        Rectangle hartaS = {RX(hartaD.x, sx), RY(hartaD.y, sy), RW(hartaD.width, sx), RH(hartaD.height, sy)};
        Rectangle listaS = {RX(listaD.x, sx), RY(listaD.y, sy), RW(listaD.width, sx), RH(listaD.height, sy)};

        Vector2 poz_on = {RX(poz_onD.x, sx), RY(poz_onD.y, sy)};
        Rectangle buton_on = {RX(buton_onD.x, sx), RY(buton_onD.y, sy), RW(buton_onD.width, sx), RH(buton_onD.height, sy)};
        int marime_on = (int)RH((float)marime_onD, sy);

        Rectangle b_sos = {RX(b_sosD.x, sx), RY(b_sosD.y, sy), RW(b_sosD.width, sx), RH(b_sosD.height, sy)};
        Rectangle b_update = {RX(b_updateD.x, sx), RY(b_updateD.y, sy), RW(b_updateD.width, sx), RH(b_updateD.height, sy)};
        Rectangle b_places = {RX(b_placesD.x, sx), RY(b_placesD.y, sy), RW(b_placesD.width, sx), RH(b_placesD.height, sy)};
        Rectangle b_set = {RX(b_setD.x, sx), RY(b_setD.y, sy), RW(b_setD.width, sx), RH(b_setD.height, sy)};

        // family box + pop-out menu (OVERLAY, NU MUTAM UI)
        Rectangle box_fam = {RX(box_famD.x, sx), RY(box_famD.y, sy), RW(box_famD.width, sx), RH(box_famD.height, sy)};
        float menu_w = box_fam.width;
        float menu_h = RH(160, sy);
        Rectangle menu_r = {box_fam.x, box_fam.y + box_fam.height + RH(6, sy), menu_w, menu_h};

        float pad = RH(10, sy);
        float btn_h = RH(42, sy);
        float header = RH(34, sy);

        Rectangle btn_join = {menu_r.x + pad, menu_r.y + header + pad, menu_r.width - 2 * pad, btn_h};
        Rectangle btn_create = {menu_r.x + pad, btn_join.y + btn_join.height + pad, menu_r.width - 2 * pad, btn_h};
        Rectangle btn_leave = {menu_r.x + pad, btn_create.y + btn_create.height + pad, menu_r.width - 2 * pad, btn_h};

        //_________________________________________________________________________pagina login____________________________________
        if (scr == SCR_LOGIN)
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                if (pop_activ)
                {
                    pop_activ = false;
                    tip_pop = POP_NIMIC;
                }
                else
                {
                    if (mouse_in(b_user))in = 1;
                    else if (mouse_in(b_pass))in = 2;
                    else in = 0;

                    if (mouse_in(b_exit))
                    {
                        if (srv_ok)
                            conexiune.trimite("LOGOUT");
                        break;
                    }

                    if (mouse_in(b_log))
                    {
                        if (srv_ok && !user.empty() && !pass.empty())
                        {
                            conexiune.trimite("LOGIN " + user + " " + pass);
                        }
                        else
                        {
                            tip_pop = POP_INVALID_REQUEST;
                            pop_activ = true;
                        }
                    }

                    if (mouse_in(b_reg))
                    {
                        if (srv_ok && !user.empty() && !pass.empty())
                        {
                            conexiune.trimite("REGISTER " + user + " " + pass);
                        }
                        else
                        {
                            tip_pop = POP_INVALID_REQUEST;
                            pop_activ = true;
                        }
                    }
                }
            }

            input_text(user, (in == 1 && !pop_activ), 20);
            input_text(pass, (in == 2 && !pop_activ), 20);
        }

        //_____________________________________________________pagina pr________________________________________
        if (scr == SCR_MAIN || scr == SCR_NO_FAMILY)
        {

           // ====== FAMILY MENU INPUT (TOT INPUT, NU IN DRAW) ======
if(!pop_activ && meniu_fam && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
{
    float item_h = RH(45, sy);
    float total_h = RH(40, sy) + (my_families.size() * item_h) + (2 * item_h) + 20;
    if(stare.fam_id != -1) total_h += item_h + 10;

    Rectangle r_meniu = { box_fam.x, box_fam.y + box_fam.height + 5, box_fam.width, total_h };

    // click in afara -> inchidem
    if(!CheckCollisionPointRec(m, r_meniu) && !CheckCollisionPointRec(m, box_fam))
    {
        meniu_fam = false;
        click_consumat = true;
    }
    else
    {
        float y = r_meniu.y + RH(45, sy); // dupa header

        // 1) click pe una din familii
        bool gasit = false;
        for(int i=0; i<(int)my_families.size(); i++)
        {
            Rectangle r_item = { r_meniu.x, y, r_meniu.width, item_h };
            if(CheckCollisionPointRec(m, r_item))
            {
                string cmd = "SWITCH_FAMILY " + to_string(my_families[i].id);
                conexiune.trimite(cmd);
                conexiune.trimite("GET_STATE");   // IMPORTANT: refresh imediat
                meniu_fam = false;
                gasit = true;
                click_consumat = true;
                break;
            }
            y += item_h;
        }

        if(!gasit)
        {
            y += 10; // linie + padding

            // 2) JOIN
            Rectangle r_join = { r_meniu.x, y, r_meniu.width, item_h };
            if(CheckCollisionPointRec(m, r_join))
            {
                tip_pop = POP_JOIN_FAMILY;
                pop_activ = true;
                meniu_fam = false;
                join_id_txt.clear();
                click_consumat = true;
            }
            y += item_h;

            // 3) CREATE
            Rectangle r_create = { r_meniu.x, y, r_meniu.width, item_h };
            if(CheckCollisionPointRec(m, r_create))
            {
                tip_pop = POP_CREATE_FAMILY;
                pop_activ = true;
                meniu_fam = false;
                create_name_txt.clear();
                click_consumat = true;
            }
            y += item_h;

            // 4) LEAVE (doar daca esti intr-o familie)
            if(stare.fam_id != -1)
            {
                y += 10;
                Rectangle r_leave = { r_meniu.x, y, r_meniu.width, item_h };
                if(CheckCollisionPointRec(m, r_leave))
                {
                    tip_pop = POP_LEAVE_FAMILY;
                    pop_activ = true;
                    meniu_fam = false;
                    click_consumat = true;
                }
            }
        }
    }
}

// deschidere meniu (doar cand nu e deschis)
if(!pop_activ && !meniu_fam && !click_consumat && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
{
    if(CheckCollisionPointRec(m, box_fam))
    {
        meniu_places = false;
        conexiune.trimite("GET_FAMILIES");
        // meniu_fam se va face true cand vine mesajul FAMILIES_LIST
        click_consumat = true;
    }
}

            // ====== PLACES MENU INPUT ======
            float places_w = RW(320, sx);
            float places_item_h = RH(42, sy);
            float places_header_h = RH(40, sy);
            float places_add_h = RH(42, sy);
            float places_inner_pad = RW(2, sx);
            int places_count = (int)stare.places.size();
            if (places_count < 1) places_count = 1;
            float places_list_h = places_item_h * (float)places_count;
            float places_h = places_header_h + places_list_h + places_add_h;
            Rectangle places_menu = {b_places.x - places_w + b_places.width, b_places.y - places_h - RH(10, sy), places_w, places_h};
            places_menu.x = inghesuie_f(places_menu.x, RW(10, sx), GetScreenWidth() - places_menu.width - RW(10, sx));
            places_menu.y = inghesuie_f(places_menu.y, RH(10, sy), GetScreenHeight() - places_menu.height - RH(10, sy));

            if(!pop_activ && meniu_places && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                if(!CheckCollisionPointRec(m, places_menu))
                {
                    meniu_places = false;
                    click_consumat = true;
                }
                else
                {
                    float y = places_menu.y + places_header_h;
                    float item_x = places_menu.x + places_inner_pad;
                    float item_w = places_menu.width - 2 * places_inner_pad;
                    bool gasit = false;
                    for(int i = 0; i < (int)stare.places.size(); i++)
                    {
                        Rectangle r_item = {item_x, y, item_w, places_item_h};
                        if(CheckCollisionPointRec(m, r_item))
                        {
                            loc_selectat = i;
                            membru_selectat = -1;
                            meniu_places = false;
                            gasit = true;
                            click_consumat = true;
                            break;
                        }
                        y += places_item_h;
                    }

                    Rectangle r_add = {item_x, places_menu.y + places_menu.height - places_add_h, item_w, places_add_h};
                    if(!gasit && CheckCollisionPointRec(m, r_add))
                    {
                        tip_pop = POP_ADD_PLACE;
                        pop_activ = true;
                        meniu_places = false;
                        place_name_txt.clear();
                        click_consumat = true;
                    }
                }
            }

            if(meniu_places && IsKeyPressed(KEY_ESCAPE)){
                meniu_places = false;
                click_consumat = true;
            }

            // deschidere meniul de locatiicou
            if(!pop_activ && !meniu_places && !click_consumat && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                if(CheckCollisionPointRec(m, b_places))
                {
                    meniu_fam = false;
                    meniu_places = true;
                    click_consumat = true;
                }
            }


            // restul input-ului (blocat cand click_consumat==true sau meniu_fam==true)
            if (!pop_activ && !meniu_fam && !meniu_places)
            {

                if (click_in(buton_on) && srv_ok)
                {
                    if (stare.me_on)
                        conexiune.trimite("TURN_LOCATION_OFF");
                    else
                        conexiune.trimite("TURN_LOCATION_ON");

                    conexiune.trimite("GET_STATE");
                }

                if (mouse_in(b_sos) && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
                {
                    tip_pop = POP_SOS_SELF;
                    pop_activ = true;
                    sos_pending = true;
                    sos_sent = false;
                    sos_timer = 0.0f;
                }

                // butoane jos
                if (click_in(b_update) && srv_ok)
                    conexiune.trimite("UPDATE_LOCATION");

                if (click_in(b_set))
                {
                    tip_pop = POP_LOGOUT_CONFIRM;
                    pop_activ = true;
                }
            }
            else
            {
            }

            //______________________lista membriii______________________
            if (scr == SCR_MAIN && !pop_activ && !meniu_fam && !meniu_places)
            {

                float headerH = listaS.height * 0.18f;
                if (headerH < RH(60, sy))
                    headerH = RH(60, sy);
                if (headerH > RH(90, sy))
                    headerH = RH(90, sy);

                float listTop = listaS.y + headerH;
                float listH = listaS.height - headerH;
                float addH = listH * 0.16f;

                if (addH < RH(50, sy))
                    addH = RH(50, sy);
                if (addH > RH(70, sy))
                    addH = RH(70, sy);

                Rectangle rAdd = {listaS.x, listaS.y + listaS.height - addH, listaS.width, addH};

                if (click_in(rAdd))
                {
                    tip_pop = POP_ADD_MEMBER;
                    pop_activ = true;
                    tmp_member_name.clear();
                }

                int cnt = (int)stare.members.size();
                if (cnt < 1)
                    cnt = 1;

                float slotsH = listH - addH;
                float slotH = slotsH / (float)cnt;

                for (int i = 0; i < (int)stare.members.size(); i++)
                {
                    Rectangle rSlot = {listaS.x, listTop + i * slotH, listaS.width, slotH};

                    if (click_in(rSlot))
                    {
                        membru_selectat = i;
                        loc_selectat = -1;
                    }

                    float pin_size = RH(60, sy);
                    Rectangle rPin = {rSlot.x + rSlot.width - RW(70, sx),
                                      rSlot.y + (slotH - pin_size) * 0.5f,
                                      RW(60, sx), pin_size};
                    if (click_in(rPin) && srv_ok)
                    {
                        membru_selectat = i;
                        loc_selectat = -1;
                        loc_req_user = stare.members[i].name;
                        conexiune.trimite("GET_LOCATION " + stare.members[i].name);
                    }
                }

                for (int i = 0; i < (int)stare.places.size(); i++)
                {
                    Vector2 p = fereastra_harta(hartaS, stare.places[i].x, stare.places[i].y);

                    float w = RW(24, sx), h = RH(34, sy);

                    Rectangle rp = {p.x - w / 2.0f, p.y - h, w, h};

                    if (click_in(rp))
                    {
                        loc_selectat = i;
                        membru_selectat = -1;

                        tip_pop = POP_LOCATION_INFO;
                        pop_activ = true;
                        loc_popup_kind = LOC_POP_PLACE;
                        loc_place_txt = stare.places[i].name;
                        loc_admin_txt = stare.fam_name;
                        if (loc_admin_txt.empty())
                            loc_admin_txt = "-";
                        loc_coords_txt = to_string(stare.places[i].x) + ", " + to_string(stare.places[i].y);
                        mesaj_popup_loc = "PLACE " + stare.places[i].name;
                    }
                }
            }

            // popups input
            if (pop_activ)
            {

                // inchidere popup text (fara imagine)
      if(tip_pop == POP_FAMILY_NOT_FOUND || tip_pop == POP_FAMILY_JOIN_OK || tip_pop == POP_FAMILY_CREATE_OK || tip_pop == POP_FAMILY_ERROR || tip_pop == POP_PLACE_ERROR || tip_pop == POP_MEMBER_NOT_FOUND || tip_pop == POP_MEMBER_ALREADY){
         if(IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
            pop_activ = false;
              tip_pop = POP_NIMIC;
               }
            }

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                {
                    Texture2D pimg = textura_popup(tip_pop, popup_add_member, popup_add_place, popup_logout_btn, popup_loc_req, popup_loc_req2, popup_loc_arrived, popup_loc_left, popup_sos_self, popup_sos_msg,
                                                   pop_no_user, pop_bad_pass, pop_reg_ok, pop_taken, popup_join_img, popup_create_img, popup_leave_confirm, popup_invalid_req,
                                                   popup_user_not_exist, popup_fam_id_invalid, popup_already_in_fam, popup_now_in_fam, popup_usr_already_in_fam);
                    if (tip_pop == POP_LOCATION_INFO)
                    {
                        if (loc_popup_kind == LOC_POP_PLACE && popup_place_info.id)
                            pimg = popup_place_info;
                        else if (loc_popup_kind == LOC_POP_OFF && popup_loc_off.id)
                            pimg = popup_loc_off;
                        else if (loc_popup_kind == LOC_POP_LOC && popup_loc_req.id)
                            pimg = popup_loc_req;
                        else if (popup_loc_req.id)
                            pimg = popup_loc_req;
                    }

                    float popup_scale = 1.0f;
                    if (tip_pop == POP_LOGOUT_CONFIRM || tip_pop == POP_LEAVE_FAMILY)
                        popup_scale = match_popup_scale(pop_bad_pass, pimg);
                    if (tip_pop == POP_SOS_SELF || tip_pop == POP_SOS_MSG)
                        popup_scale = POP_SOS_SCALE;
                    if (tip_pop == POP_FAMILY_NOT_FOUND || tip_pop == POP_FAMILY_JOIN_OK || tip_pop == POP_FAMILY_CREATE_OK ||
                        tip_pop == POP_FAMILY_ERROR || tip_pop == POP_PLACE_ERROR || tip_pop == POP_MEMBER_NOT_FOUND ||
                        tip_pop == POP_MEMBER_ALREADY || tip_pop == POP_INVALID_REQUEST || tip_pop == POP_INFO_LOGIN_EMPTY)
                        popup_scale = match_popup_scale(pop_bad_pass, pimg);
                    Rectangle pr = rect_center_scaled(pimg, popup_scale);
                    Rectangle btn_ok_rect = {pr.x + pr.width / 2 + 80, pr.y + 110, 70, 70};

                    ////
                    Rectangle btn_close = {pr.x + pr.width - 60, pr.y + 10, 50, 50};

                    if (click_in(btn_close))
                    {
                        pop_activ = false;
                        tip_pop = POP_NIMIC;
                        loc_popup_kind = LOC_POP_NONE;
                    }

                    bool actiune_confirmata = IsKeyPressed(KEY_ENTER);
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(m, btn_ok_rect))
                        actiune_confirmata = true;
                    ////

                    if (click_in(btn_close))
                    {
                        pop_activ = false;
                        tip_pop = POP_NIMIC;
                        loc_popup_kind = LOC_POP_NONE;
                    }

                    if (pimg.id)
                    {
                        Rectangle xBtn = {pr.x + pr.width - 70, pr.y + 10, 60, 60};
                        if (CheckCollisionPointRec(m, xBtn))
                        {
                            pop_activ = false;
                            tip_pop = POP_NIMIC;
                            loc_popup_kind = LOC_POP_NONE;
                        }

                        if (tip_pop == POP_SOS_SELF || tip_pop == POP_SOS_MSG)
                        {
                            Rectangle back_btn = {pr.x + pr.width * 0.04f, pr.y + pr.height * 0.80f, pr.width * 0.08f, pr.height * 0.12f};
                            if (CheckCollisionPointRec(m, back_btn))
                            {
                                pop_activ = false;
                                tip_pop = POP_NIMIC;
                            }
                        }

                        if (tip_pop == POP_SOS_SELF)
                        {
                            Rectangle exit_btn = {pr.x + pr.width * 0.36f, pr.y + pr.height * 0.82f, pr.width * 0.48f, pr.height * 0.12f};
                            if (CheckCollisionPointRec(m, exit_btn))
                            {
                                pop_activ = false;
                                tip_pop = POP_NIMIC;
                            }
                        }
                    }
                }

                if (tip_pop == POP_JOIN_FAMILY)
                {
                    input_text(join_id_txt, true, 10);
                    if (IsKeyPressed(KEY_ENTER) && srv_ok)
                    {
                        if (!join_id_txt.empty())
                        {
                            last_action = ACT_JOIN_FAMILY;
                            conexiune.trimite("JOIN_FAMILY " + join_id_txt);
                            conexiune.trimite("GET_STATE");
                        }
                        pop_activ = false;
                        tip_pop = POP_NIMIC;
                    }
                }

                if (tip_pop == POP_LEAVE_FAMILY)
                {
                    if (IsKeyPressed(KEY_ENTER) && srv_ok)
                    {
                        last_action = ACT_LEAVE_FAMILY;
                        conexiune.trimite("LEAVE_FAMILY");
                        conexiune.trimite("GET_STATE");
                        pop_activ = false;
                        tip_pop = POP_NIMIC;
                    }
                }

                if (tip_pop == POP_LOGOUT_CONFIRM)
                {
                    if (IsKeyPressed(KEY_ENTER))
                    {
                        if (srv_ok)
                            conexiune.trimite("LOGOUT");
                        user.clear();
                        pass.clear();
                        scr = SCR_LOGIN;
                        pop_activ = false;
                        tip_pop = POP_NIMIC;
                        SetWindowSize(LOGIN_W, LOGIN_H);
                    }
                }

                if (tip_pop == POP_CREATE_FAMILY)
                {
                    input_text(create_name_txt, true, 20); // Citim tastatura
                    if (IsKeyPressed(KEY_ENTER) && srv_ok)
                    {
                        if (!create_name_txt.empty())
                        {
                            last_action = ACT_CREATE_FAMILY;
                            conexiune.trimite("ADD_FAMILY " + create_name_txt);
                            conexiune.trimite("GET_STATE");
                        }
                        pop_activ = false;
                        tip_pop = POP_NIMIC;
                    }
                }

                if (tip_pop == POP_ADD_MEMBER)
                {
                    input_text(tmp_member_name, true, 20);
                    Rectangle pr = rect_center(popup_add_member);
                    Rectangle btn_add = {pr.x + pr.width / 2 - 60, pr.y + pr.height - 110, 120, 60};
                    if ((IsKeyPressed(KEY_ENTER) || click_in(btn_add)) && srv_ok)
                    {
                        if (!tmp_member_name.empty())
                        {
                            last_action = ACT_ADD_MEMBER;
                            conexiune.trimite("ADD_MEMBERS " + tmp_member_name);
                            conexiune.trimite("GET_STATE");
                        }
                        pop_activ = false;
                        tip_pop = POP_NIMIC;
                    }
                }

                if (tip_pop == POP_ADD_PLACE)
                {
                    input_text(place_name_txt, true, 20);
                    Rectangle pr = rect_center(popup_add_place);
                    Rectangle btn_create = {pr.x + pr.width / 2 - 80, pr.y + pr.height - 110, 160, 60};
                    if ((IsKeyPressed(KEY_ENTER) || click_in(btn_create)) && srv_ok)
                    {
                        if (!place_name_txt.empty())
                        {
                            last_action = ACT_ADD_PLACE;
                            conexiune.trimite("ADD_LOCATION " + place_name_txt);
                            conexiune.trimite("GET_STATE");
                        }
                        pop_activ = false;
                        tip_pop = POP_NIMIC;
                    }
                }

            }
        }

        //____________________________________________________________DESEN____________________________________
        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (scr == SCR_LOGIN)
        {

            if (login_page.id)
                DrawTexture(login_page, 0, 0, WHITE);

            DrawText(user.c_str(), (int)b_user.x + 10, (int)b_user.y + 25, 25, BLACK);
            if (in == 1)
            {
                if (((framesCounter / 20) % 2) == 0 && user.length() < 15)
                {
                    DrawText("_", (int)(b_user.x + 8 + MeasureText(user.c_str(), 25)), (int)(b_user.y + 12), 40, BLACK);
                }
            }

            string stelute(pass.size(), '*');
            DrawText(stelute.c_str(), (int)b_pass.x + 10, (int)b_pass.y + 25, 25, BLACK);
            if (in == 2)
            {
                if (((framesCounter / 20) % 2) == 0 && pass.length() < 15)
                {
                    DrawText("_", (int)(b_pass.x + 8 + MeasureText(stelute.c_str(), 25)), (int)(b_pass.y + 12), 40, BLACK);
                }
            }

            if (pop_activ)
            {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.45f));

                Texture2D pimg = textura_popup(tip_pop, popup_add_member, popup_add_place, popup_logout_btn, popup_loc_req, popup_loc_req2, popup_loc_arrived, popup_loc_left, popup_sos_self, popup_sos_msg,
                                               pop_no_user, pop_bad_pass, pop_reg_ok, pop_taken, popup_join_img, popup_create_img, popup_leave_confirm, popup_invalid_req,
                                               popup_user_not_exist, popup_fam_id_invalid, popup_already_in_fam, popup_now_in_fam, popup_usr_already_in_fam);
                if (pimg.id)
                {
                    float popup_scale = 1.0f;
                    if (tip_pop == POP_INFO_LOGIN_EMPTY || tip_pop == POP_INVALID_REQUEST)
                        popup_scale = match_popup_scale(pop_bad_pass, pimg);
                    img_centrat_scaled(pimg, popup_scale);
                }
            }
        }

        if (scr == SCR_MAIN || scr == SCR_NO_FAMILY)
        {

            if (fundal_main.id)
                DrawTexture(fundal_main, 0, 0, WHITE);

            // ===== box familie sus + nume =====
            if (box_fam_img.id)
            {
                DrawTexturePro(box_fam_img,
                               (Rectangle){0, 0, (float)box_fam_img.width, (float)box_fam_img.height},
                               box_fam, (Vector2){0, 0}, 0.0f, WHITE);
            }
            else
            {
                DrawRectangleRec(box_fam, RAYWHITE);
            }
            Color fam_border = Color{180, 140, 220, 255};
            DrawRectangleLinesEx(box_fam, RW(3, sx), fam_border);

            string nume = stare.fam_name;
            if (stare.fam_id == -1)
                nume = "NO FAMILY";
            int fam_font = (int)RH(26, sy);
            Font ui_font = get_ui_font();
            Vector2 fam_size = MeasureTextEx(ui_font, nume.c_str(), (float)fam_font, UI_FONT_SPACING);
            int fam_text_x = (int)(box_fam.x + (box_fam.width - fam_size.x) * 0.5f);
            int fam_text_y = (int)(box_fam.y + (box_fam.height - fam_size.y) * 0.5f);
            DrawText(nume.c_str(), fam_text_x, fam_text_y, fam_font, BLACK);

            //_____________________________________________________________
            if (stare.me_on)
                DrawText("ON", (int)poz_on.x, (int)poz_on.y, marime_on, GREEN);
            else
                DrawText("OFF", (int)poz_on.x, (int)poz_on.y, marime_on, RED);


            if (DEBUG_UI)
            {
                DrawRectangleLines((int)hartaS.x, (int)hartaS.y, (int)hartaS.width, (int)hartaS.height, RED);
                DrawRectangleLines((int)listaS.x, (int)listaS.y, (int)listaS.width, (int)listaS.height, BLUE);
                DrawRectangleLines((int)buton_on.x, (int)buton_on.y, (int)buton_on.width, (int)buton_on.height, ORANGE);
                DrawRectangleLines((int)b_sos.x, (int)b_sos.y, (int)b_sos.width, (int)b_sos.height, GREEN);
                DrawRectangleLines((int)b_update.x, (int)b_update.y, (int)b_update.width, (int)b_update.height, GREEN);
                DrawRectangleLines((int)b_places.x, (int)b_places.y, (int)b_places.width, (int)b_places.height, GREEN);
                DrawRectangleLines((int)b_set.x, (int)b_set.y, (int)b_set.width, (int)b_set.height, GREEN);
            }

            float s = (sx < sy) ? sx : sy;
            float memPinS = 34 * s;
            float plcPinS = 50 * s;
            float memPinW = memPinS, memPinH = memPinS, plcPinW = plcPinS, plcPinH = plcPinS;

            if (loc_selectat >= 0 && loc_selectat < (int)stare.places.size())
            {
                Place &pl = stare.places[loc_selectat];
                Vector2 p = fereastra_harta(hartaS, pl.x, pl.y);
                p.x = inghesuie_f(p.x, hartaS.x + plcPinW / 2.0f, hartaS.x + hartaS.width - plcPinW / 2.0f);
                p.y = inghesuie_f(p.y, hartaS.y + plcPinH, hartaS.y + hartaS.height);
                Vector2 hp = {p.x, p.y - plcPinH * 0.5f};
                DrawCircleV(hp, RH(20, sy), Fade(Color{0, 110, 200, 255}, 0.35f));
            }

            for (int i = 0; i < (int)stare.members.size(); i++)
            {
                Member &mb = stare.members[i];

                if (mb.loc_on == 0)
                    continue;
                if (mb.x < 0 || mb.y < 0)
                    continue;

                Vector2 p = fereastra_harta(hartaS, mb.x, mb.y);

                p.x = inghesuie_f(p.x, hartaS.x + memPinW / 2.0f, hartaS.x + hartaS.width - memPinW / 2.0f);
                p.y = inghesuie_f(p.y, hartaS.y + memPinH, hartaS.y + hartaS.height);

                if (i == membru_selectat)
                    pin_scalat(pin_red, p, memPinW, memPinH);
                else
                    pin_scalat(pin_blue, p, memPinW, memPinH);
            }

            for (int i = 0; i < (int)stare.places.size(); i++)
            {
                Place &pl = stare.places[i];
                if (pl.x < 0 || pl.y < 0)
                    continue;

                Vector2 p = fereastra_harta(hartaS, pl.x, pl.y);

                p.x = inghesuie_f(p.x, hartaS.x + plcPinW / 2.0f, hartaS.x + hartaS.width - plcPinW / 2.0f);
                p.y = inghesuie_f(p.y, hartaS.y + plcPinH, hartaS.y + hartaS.height);

                pin_scalat(pin_place, p, plcPinW, plcPinH);
            }

            // lista
            if (scr == SCR_MAIN)
            {

                float headerH = listaS.height * 0.18f;
                if (headerH < RH(60, sy))
                    headerH = RH(60, sy);
                if (headerH > RH(90, sy))
                    headerH = RH(90, sy);

                Rectangle rHeader = {listaS.x, listaS.y, listaS.width, headerH};

                DrawRectangleRec(rHeader, Color{210, 200, 255, 255});
                DrawRectangleLines((int)rHeader.x, (int)rHeader.y, (int)rHeader.width, (int)rHeader.height, BLACK);

                DrawText("FAMILY MEMBERS", (int)(rHeader.x + RW(18, sx)), (int)(rHeader.y + headerH * 0.25f), (int)RH(28, sy), BLACK);

                float listTop = listaS.y + headerH, listH = listaS.height - headerH;

                float addH = listH * 0.16f;
                if (addH < RH(50, sy))
                    addH = RH(50, sy);
                if (addH > RH(70, sy))
                    addH = RH(70, sy);

                Rectangle rAdd = {listaS.x, listaS.y + listaS.height - addH, listaS.width, addH};

                int n = (int)stare.members.size();
                if (n < 1)
                    n = 1;

                float slotsH = listH - addH, slotH = slotsH / (float)n;

                for (int i = 0; i < (int)stare.members.size(); i++)
                {
                    Rectangle rSlot = {listaS.x, listTop + i * slotH, listaS.width, slotH};

                    Color bg;
                    if (i == membru_selectat)
                        bg = Color{230, 225, 255, 255};
                    else
                        bg = RAYWHITE;

                    DrawRectangleRec(rSlot, bg);

                    if (i > 0)
                        DrawLine((int)rSlot.x, (int)rSlot.y, (int)(rSlot.x + rSlot.width), (int)rSlot.y, BLACK);

                    DrawRectangleLines((int)rSlot.x, (int)rSlot.y, (int)rSlot.width, (int)rSlot.height, BLACK);

                    bool is_me = (!user.empty() && stare.members[i].name == user);
                    string display_name = stare.members[i].name;
                    if (is_me)
                        display_name += " (me)";
                    Color name_col = is_me ? DARKBLUE : BLACK;
                    DrawText(display_name.c_str(),
                             (int)(rSlot.x + RW(18, sx)),
                             (int)(rSlot.y + slotH * 0.30f),
                             (int)RH(22, sy), name_col);

                    int on_font = (int)RH(20, sy);
                    float onoff_y = rSlot.y + slotH * 0.30f;
                    if (stare.members[i].loc_on)
                    {
                        DrawText("ON",
                                 (int)(rSlot.x + rSlot.width - RW(140, sx)),
                                 (int)onoff_y,
                                 on_font, DARKGREEN);
                    }
                    else
                    {
                        DrawText("OFF",
                                 (int)(rSlot.x + rSlot.width - RW(140, sx)),
                                 (int)onoff_y,
                                 on_font, MAROON);
                    }

                    int cx = (int)(rSlot.x + rSlot.width - RW(40, sx));
                    int cy = (int)(onoff_y + on_font * 0.5f);
                    float rr = (float)RH(14, sy);
                    if (stare.members[i].loc_on)
                    {
                        DrawCircle(cx, cy, rr, PURPLE);
                        DrawCircleLines(cx, cy, rr, BLACK);
                    }
                    else
                    {
                        DrawCircle(cx, cy, rr, DARKGRAY);
                        DrawCircleLines(cx, cy, rr, BLACK);
                    }
                }

                DrawRectangleRec(rAdd, RAYWHITE);
                DrawRectangleLines((int)rAdd.x, (int)rAdd.y, (int)rAdd.width, (int)rAdd.height, BLACK);

                float cx = rAdd.x + RW(35, sx), cy = rAdd.y + rAdd.height / 2;

                DrawCircleLines((int)cx, (int)cy, (float)RH(16, sy), BLACK);
                DrawLine((int)(cx - RW(8, sx)), (int)cy, (int)(cx + RW(8, sx)), (int)cy, BLACK);
                DrawLine((int)cx, (int)(cy - RH(8, sy)), (int)cx, (int)(cy + RH(8, sy)), BLACK);

                DrawText("ADD MEMBER", (int)(rAdd.x + RW(80, sx)), (int)(rAdd.y + rAdd.height * 0.32f), (int)RH(20, sy), BLACK);
            }

            // ====== FAMILY MENU DRAW (ULTIMUL -> OVERLAY) ======
           // MENIU FAMILIE DINAMIC (STIL RESTAURAT)
            if(meniu_fam){
                DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(), Fade(BLACK, 0.15f)); 

                float item_h = RH(45, sy);
                // Calcul inaltime: Header(40) + Lista + Join + Create + Padding(20)
                float total_h = RH(40, sy) + (my_families.size() * item_h) + (2 * item_h) + 20;
                
                // Daca suntem deja intr-o familie, mai adaugam loc pentru LEAVE
                if(stare.fam_id != -1) total_h += item_h + 10;

                Rectangle r_meniu = { box_fam.x, box_fam.y + box_fam.height + 5, box_fam.width, total_h };
                
                // Fundal Meniu (Alb + Contur)
                DrawRectangleRec(r_meniu, RAYWHITE);
                DrawRectangleLines((int)r_meniu.x, (int)r_meniu.y, (int)r_meniu.width, (int)r_meniu.height, DARKGRAY);
                
                // Header "FAMILY MEMBERS" / "SELECT FAMILY"
                Rectangle r_header = { r_meniu.x, r_meniu.y, r_meniu.width, RH(40, sy) };
                DrawRectangleRec(r_header, Color{235,230,255,255}); // Movul tau deschis
                DrawRectangleLines((int)r_header.x, (int)r_header.y, (int)r_header.width, (int)r_header.height, LIGHTGRAY);
                DrawText("MY FAMILIES", (int)(r_header.x + 20), (int)(r_header.y + 10), 20, BLACK);

                float y_curent = r_meniu.y + RH(45, sy); 

                // 1.f LISTA FAMILII (Dinamic)
                for(int i=0; i<my_families.size(); i++){
                    Rectangle r_item = { r_meniu.x, y_curent, r_meniu.width, item_h };
                    
                    // Hover Effect
                    if(CheckCollisionPointRec(m, r_item)){
                        DrawRectangleRec(r_item, Fade(Color{235,230,255,255}, 0.5f)); // Mov la hover
                    }

                    // Textul (Daca e familia curenta, o marcam cu albastru si >)
                    if(my_families[i].id == stare.fam_id)
                        DrawText(TextFormat("> %s", my_families[i].nume.c_str()), (int)(r_item.x + 20), (int)(r_item.y + 10), 25, DARKBLUE);
                    else
                        DrawText(my_families[i].nume.c_str(), (int)(r_item.x + 20), (int)(r_item.y + 10), 25, BLACK);

                    y_curent += item_h;
                }

                // Linie despartitoare
                DrawLine((int)r_meniu.x, (int)y_curent, (int)(r_meniu.x + r_meniu.width), (int)y_curent, LIGHTGRAY);
                y_curent += 10;

                // 2.f BUTON JOIN
                Rectangle r_join = { r_meniu.x, y_curent, r_meniu.width, item_h };
                if(CheckCollisionPointRec(m, r_join)){
                    DrawRectangleRec(r_join, Fade(GRAY, 0.2f));
                    if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                        tip_pop = POP_JOIN_FAMILY; pop_activ = true; meniu_fam = false; join_id_txt = "";
                    }
                }
                DrawText("+ Join Family", (int)(r_join.x + 20), (int)(r_join.y + 10), 25, BLACK);
                y_curent += item_h;

                // 3.f BUTON CREATE
                Rectangle r_create = { r_meniu.x, y_curent, r_meniu.width, item_h };
                if(CheckCollisionPointRec(m, r_create)){
                     DrawRectangleRec(r_create, Fade(GRAY, 0.2f));
                     if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                        tip_pop = POP_CREATE_FAMILY; pop_activ = true; meniu_fam = false; create_name_txt = "";
                    }
                }
                DrawText("+ Create Family", (int)(r_create.x + 20), (int)(r_create.y + 10), 25, BLACK);
                y_curent += item_h;

                // 4.f BUTON LEAVE FAMILY (Doar daca suntem intr-o familie)
                if(stare.fam_id != -1){
                    // Linie despartitoare
                    DrawLine((int)r_meniu.x, (int)y_curent, (int)(r_meniu.x + r_meniu.width), (int)y_curent, LIGHTGRAY);
                    y_curent += 10;

                    Rectangle r_leave = { r_meniu.x, y_curent, r_meniu.width, item_h };
                    if(CheckCollisionPointRec(m, r_leave)){
                         DrawRectangleRec(r_leave, Fade(RED, 0.1f)); // Usor rosu la hover
                         if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                            tip_pop = POP_LEAVE_FAMILY; pop_activ = true; meniu_fam = false;
                        }
                    }
                    DrawText("Leave Family", (int)(r_leave.x + 20), (int)(r_leave.y + 10), 25, MAROON);
                }
            }

            if (meniu_places)
            {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.15f));

                float places_w = RW(320, sx);
                float places_item_h = RH(42, sy);
                float places_header_h = RH(40, sy);
                float places_add_h = RH(42, sy);
                float places_inner_pad = RW(2, sx);
                int places_count = (int)stare.places.size();
                if (places_count < 1) places_count = 1;
                float places_list_h = places_item_h * (float)places_count;
                float places_h = places_header_h + places_list_h + places_add_h;
                Rectangle places_menu = {b_places.x - places_w + b_places.width, b_places.y - places_h - RH(10, sy), places_w, places_h};
                places_menu.x = inghesuie_f(places_menu.x, RW(10, sx), GetScreenWidth() - places_menu.width - RW(10, sx));
                places_menu.y = inghesuie_f(places_menu.y, RH(10, sy), GetScreenHeight() - places_menu.height - RH(10, sy));

                DrawRectangleRec(places_menu, RAYWHITE);
                DrawRectangleLinesEx(places_menu, RW(2, sx), BLACK);

                Rectangle r_header = {places_menu.x, places_menu.y, places_menu.width, places_header_h};
                if (antet_places.id)
                {
                    DrawTexturePro(antet_places,
                                   (Rectangle){0, 0, (float)antet_places.width, (float)antet_places.height},
                                   r_header, (Vector2){0, 0}, 0.0f, WHITE);
                }
                else
                {
                    DrawRectangleRec(r_header, Color{235, 230, 255, 255});
                    DrawRectangleLines((int)r_header.x, (int)r_header.y, (int)r_header.width, (int)r_header.height, LIGHTGRAY);
                    DrawText("PLACES", (int)(r_header.x + 20), (int)(r_header.y + 10), 20, BLACK);
                }

                float y = places_menu.y + places_header_h;
                float item_x = places_menu.x + places_inner_pad;
                float item_w = places_menu.width - 2 * places_inner_pad;
                if (stare.places.empty())
                {
                    Rectangle r_item = {item_x, y, item_w, places_item_h};
                    DrawRectangleRec(r_item, RAYWHITE);
                    DrawRectangleLines((int)r_item.x, (int)r_item.y, (int)r_item.width, (int)r_item.height, LIGHTGRAY);
                    DrawText("NO PLACES", (int)(r_item.x + 20), (int)(r_item.y + 10), 20, DARKGRAY);
                    y += places_item_h;
                }
                else
                {
                    for (int i = 0; i < (int)stare.places.size(); i++)
                    {
                        Rectangle r_item = {item_x, y, item_w, places_item_h};
                        Color bg = (i == loc_selectat) ? Color{230, 225, 255, 255} : RAYWHITE;
                        DrawRectangleRec(r_item, bg);
                        DrawRectangleLines((int)r_item.x, (int)r_item.y, (int)r_item.width, (int)r_item.height, LIGHTGRAY);
                        DrawText(stare.places[i].name.c_str(), (int)(r_item.x + 20), (int)(r_item.y + 10), 20, BLACK);
                        y += places_item_h;
                    }
                }

                Rectangle r_add = {item_x, places_menu.y + places_menu.height - places_add_h, item_w, places_add_h};
                DrawRectangleRec(r_add, Color{245, 240, 255, 255});
                DrawRectangleLines((int)r_add.x, (int)r_add.y, (int)r_add.width, (int)r_add.height, LIGHTGRAY);
                DrawText("+ ADD PLACE", (int)(r_add.x + 20), (int)(r_add.y + 10), 20, BLACK);
            }
            if (pop_activ && (scr == SCR_MAIN || scr == SCR_NO_FAMILY))
            {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.45f));

                Texture2D pimg = textura_popup(tip_pop, popup_add_member, popup_add_place, popup_logout_btn, popup_loc_req, popup_loc_req2, popup_loc_arrived, popup_loc_left, popup_sos_self, popup_sos_msg,
                                               pop_no_user, pop_bad_pass, pop_reg_ok, pop_taken, popup_join_img, popup_create_img, popup_leave_confirm, popup_invalid_req,
                                               popup_user_not_exist, popup_fam_id_invalid, popup_already_in_fam, popup_now_in_fam, popup_usr_already_in_fam);
                if (tip_pop == POP_LOCATION_INFO)
                {
                    if (loc_popup_kind == LOC_POP_PLACE && popup_place_info.id)
                        pimg = popup_place_info;
                    else if (loc_popup_kind == LOC_POP_OFF && popup_loc_off.id)
                        pimg = popup_loc_off;
                    else if (loc_popup_kind == LOC_POP_LOC && popup_loc_req.id)
                        pimg = popup_loc_req;
                    else if (popup_loc_req.id)
                        pimg = popup_loc_req;
                }
                if (pimg.id)
                {
                    float popup_scale = 1.0f;
                    if (tip_pop == POP_LOGOUT_CONFIRM || tip_pop == POP_LEAVE_FAMILY)
                        popup_scale = match_popup_scale(pop_bad_pass, pimg);
                    if (tip_pop == POP_SOS_SELF || tip_pop == POP_SOS_MSG)
                        popup_scale = POP_SOS_SCALE;
                    if (tip_pop == POP_FAMILY_NOT_FOUND || tip_pop == POP_FAMILY_JOIN_OK || tip_pop == POP_FAMILY_CREATE_OK ||
                        tip_pop == POP_FAMILY_ERROR || tip_pop == POP_PLACE_ERROR || tip_pop == POP_MEMBER_NOT_FOUND ||
                        tip_pop == POP_MEMBER_ALREADY || tip_pop == POP_INVALID_REQUEST || tip_pop == POP_INFO_LOGIN_EMPTY)
                        popup_scale = match_popup_scale(pop_bad_pass, pimg);
                    img_centrat_scaled(pimg, popup_scale);
                    Rectangle pr = rect_center_scaled(pimg, popup_scale);

                    // --- JOIN FAMILY POPUP TEXT ---
                    if (tip_pop == POP_JOIN_FAMILY)
                    {
                        DrawText(join_id_txt.c_str(), (int)pr.x + 190, (int)pr.y + 115, 35, BLACK);

                        if ((framesCounter / 30) % 2 == 0)
                            DrawText("_", (int)pr.x + 190 + MeasureText(join_id_txt.c_str(), 35), (int)pr.y + 115, 35, BLACK);
                    }

                    // --- CREATE FAMILY POPUP TEXT ---
                    if (tip_pop == POP_CREATE_FAMILY)
                    {
                        DrawText(create_name_txt.c_str(), (int)pr.x + 190, (int)pr.y + 125, 35, BLACK);

                        if ((framesCounter / 30) % 2 == 0)
                            DrawText("_", (int)pr.x + 190 + MeasureText(create_name_txt.c_str(), 35), (int)pr.y + 125, 35, BLACK);
                    }

                    if (tip_pop == POP_ADD_MEMBER)
                    {
                        Rectangle in_box = {pr.x + 190, pr.y + 115, 240, 45};
                        DrawText(tmp_member_name.c_str(), (int)in_box.x + 10, (int)in_box.y + 8, 26, BLACK);
                        if ((framesCounter / 30) % 2 == 0)
                            DrawText("_", (int)in_box.x + 10 + MeasureText(tmp_member_name.c_str(), 26), (int)in_box.y + 8, 26, BLACK);
                    }

                    if (tip_pop == POP_ADD_PLACE)
                    {
                        DrawText(place_name_txt.c_str(), (int)pr.x + 200, (int)pr.y + 120, 26, BLACK);
                        if ((framesCounter / 30) % 2 == 0)
                            DrawText("_", (int)pr.x + 200 + MeasureText(place_name_txt.c_str(), 26), (int)pr.y + 120, 26, BLACK);
                    }

                    if (tip_pop == POP_SOS_MSG)
                    {
                        string from = sos_from_name.empty() ? "-" : sos_from_name;
                        int font = (int)(pr.height * 0.12f);
                        if (font < 20) font = 20;
                        if (font > 48) font = 48;
                        Rectangle pill = {pr.x + pr.width * 0.36f, pr.y + pr.height * 0.09f, pr.width * 0.40f, pr.height * 0.18f};
                        int text_w = MeasureText(from.c_str(), font);
                        DrawText(from.c_str(), (int)(pill.x + (pill.width - text_w) / 2), (int)(pill.y + (pill.height - font) / 2), font, WHITE);
                    }

                    if (tip_pop == POP_SOS_SELF)
                    {
                        int sec_left = (int)ceilf(10.0f - sos_timer);
                        if (sec_left < 0) sec_left = 0;
                        string cnt = to_string(sec_left);
                        int font = (int)(pr.height * 0.12f);
                        if (font < 20) font = 20;
                        if (font > 48) font = 48;
                        Rectangle pill = {pr.x + pr.width * 0.07f , pr.y + pr.height * 0.58f, pr.width * 0.22f, pr.height * 0.16f};
                        int text_w = MeasureText(cnt.c_str(), font);
                        DrawText(cnt.c_str(), (int)(pill.x + (pill.width - text_w) / 2), (int)(pill.y + (pill.height - font) / 2), font, WHITE);
                    }

                    if (tip_pop == POP_LOC_ARRIVED || tip_pop == POP_LOC_LEFT)
                    {
                        int font = (int)(pr.height * 0.08f);
                        if (font < 18) font = 18;
                        if (font > 34) font = 34;
                        Rectangle user_box = {pr.x + pr.width * 0.40f, pr.y + pr.height * 0.22f, pr.width * 0.46f, pr.height * 0.12f};
                        Rectangle loc_box = {pr.x + pr.width * 0.30f, pr.y + pr.height * 0.72f, pr.width * 0.52f, pr.height * 0.12f};
                        int user_w = MeasureText(notif_user_txt.c_str(), font);
                        int loc_w = MeasureText(notif_loc_txt.c_str(), font);
                        DrawText(notif_user_txt.c_str(), (int)(user_box.x + (user_box.width - user_w) / 2), (int)(user_box.y + (user_box.height - font) / 2), font, BLACK);
                        DrawText(notif_loc_txt.c_str(), (int)(loc_box.x + (loc_box.width - loc_w) / 2), (int)(loc_box.y + (loc_box.height - font) / 2), font, BLACK);
                    }

                    if (tip_pop == POP_LOCATION_INFO)
                    {
                        int font_main = (int)(pr.height * 0.07f);
                        if (font_main < 16) font_main = 16;
                        if (font_main > 26) font_main = 26;
                        int font_small = font_main - 4;
                        if (font_small < 14) font_small = 14;

                        if (loc_popup_kind == LOC_POP_COORD)
                        {
                            Rectangle ubox = {pr.x + pr.width * 0.42f, pr.y + pr.height * 0.23f, pr.width * 0.50f, pr.height * 0.12f};
                            Rectangle cbox = {pr.x + pr.width * 0.42f, pr.y + pr.height * 0.40f, pr.width * 0.50f, pr.height * 0.22f};
                            Rectangle lbox = {pr.x + pr.width * 0.42f, pr.y + pr.height * 0.72f, pr.width * 0.50f, pr.height * 0.12f};

                            string lon_line = "LONGITUDE: " + loc_lon_txt;
                            string lat_line = "LATITUDE: " + loc_lat_txt;

                            DrawText(loc_user_txt.c_str(), (int)(ubox.x + 8), (int)(ubox.y + 6), font_main, BLACK);
                            DrawText(lon_line.c_str(), (int)(cbox.x + 8), (int)(cbox.y + 20), font_small, BLACK);
                            DrawText(lat_line.c_str(), (int)(cbox.x + 8), (int)(cbox.y + cbox.height / 2), font_small, BLACK);
                            DrawText(loc_last_txt.c_str(), (int)(lbox.x + 15), (int)(lbox.y + 6), font_main, BLACK);
                        }
                        else if (loc_popup_kind == LOC_POP_LOC)
                        {
                            Rectangle ubox = {pr.x + pr.width * 0.42f, pr.y + pr.height * 0.23f, pr.width * 0.50f, pr.height * 0.12f};
                            Rectangle cbox = {pr.x + pr.width * 0.42f, pr.y + pr.height * 0.40f, pr.width * 0.50f, pr.height * 0.22f};
                            Rectangle lbox = {pr.x + pr.width * 0.42f, pr.y + pr.height * 0.72f, pr.width * 0.50f, pr.height * 0.12f};

                            DrawText(loc_user_txt.c_str(), (int)(ubox.x + 8), (int)(ubox.y + 6), font_main, BLACK);
                            DrawText(loc_location_txt.c_str(), (int)(cbox.x + 8), (int)(cbox.y + (cbox.height - font_main) / 2), font_main, BLACK);
                            DrawText(loc_last_txt.c_str(), (int)(lbox.x + 15), (int)(lbox.y + 6), font_main, BLACK);
                        }
                        else if (loc_popup_kind == LOC_POP_OFF)
                        {
                            Rectangle nbox = {pr.x + pr.width * 0.28f, pr.y + pr.height * 0.28f, pr.width * 0.48f, pr.height * 0.18f};
                            DrawText(loc_user_txt.c_str(), (int)(nbox.x + 15), (int)(nbox.y + 18), font_main, BLACK);
                        }
                        else if (loc_popup_kind == LOC_POP_PLACE)
                        {
                            Rectangle nbox = {pr.x + pr.width * 0.42f, pr.y + pr.height * 0.22f, pr.width * 0.50f, pr.height * 0.14f};
                            Rectangle abox = {pr.x + pr.width * 0.42f, pr.y + pr.height * 0.48f, pr.width * 0.50f, pr.height * 0.14f};
                            Rectangle cbox = {pr.x + pr.width * 0.42f, pr.y + pr.height * 0.72f, pr.width * 0.50f, pr.height * 0.14f};

                            DrawText(loc_place_txt.c_str(), (int)(nbox.x + 8), (int)(nbox.y + 12), font_main, BLACK);
                            DrawText(loc_admin_txt.c_str(), (int)(abox.x + 8), (int)(abox.y + 12), font_main, BLACK);
                            DrawText(loc_coords_txt.c_str(), (int)(cbox.x + 40), (int)(cbox.y + 30), font_main, BLACK);
                        }
                        else
                        {
                            DrawText(mesaj_popup_loc.c_str(), (int)pr.x + 60, (int)pr.y + (int)(pr.height - 80), 18, BLACK);
                        }
                    }
                }
            }

        }
          EndDrawing();
    }

    CloseWindow();
    return 0;
}
