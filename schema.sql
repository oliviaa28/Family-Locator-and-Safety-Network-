PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS users (
    username TEXT PRIMARY KEY,
    password TEXT NOT NULL,
    location INTEGER NOT NULL default 1,
    lat REAL,
    lon REAL,
    last_update INTEGER NOT NULL default (strftime('%s','now'))
);

CREATE TABLE IF NOT EXISTS families(
    id_family INTEGER PRIMARY KEY AUTOINCREMENT,
    admin_username TEXT NOT NULL,
    family_name TEXT NOT NULL,
    FOREIGN KEY (admin_username) REFERENCES users(username) 

);

CREATE TABLE IF NOT EXISTS family_locations(
    id_location INTEGER PRIMARY KEY AUTOINCREMENT,
    id_family INTEGER NOT NULL,
    name TEXT ,
    lat INTEGER not null,
    lon INTEGER not null,
    raza INTEGER not null,
    FOREIGN KEY (id_family) REFERENCES families(id_family)
);

CREATE TABLE IF NOT EXISTS family_members(
    id_family INTEGER not null,
    username TEXT not null,
    PRIMARY KEY (id_family, username),
    FOREIGN KEY (id_family) REFERENCES families(id_family),
    FOREIGN KEY (username) REFERENCES users(username)
);

CREATE TABLE sos_alerts(
    id_alert INTEGER PRIMARY KEY AUTOINCREMENT,
    id_family INTEGER NOT NULL,
    from_user TEXT NOT NULL,
    message TEXT DEFAULT 'SOS: HELP NEEDED',
    FOREIGN KEY (id_family) REFERENCES families(id_family),
    FOREIGN KEY (from_user) REFERENCES users(username)
);