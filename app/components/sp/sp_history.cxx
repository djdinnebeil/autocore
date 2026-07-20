/**
\file sp_history.cxx
\brief Implements Spotify listening-history persistence.
*/
module sp_c;

import visual;
import sp_x;
import <sqlite3.h>;

string get_datetime_stamp_local() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buffer[20];  // "YYYY-MM-DD HH:MM:SS" = 19 characters + null
    sprintf_s(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond);
    return std::string(buffer);
}

/*
\todo run a test for speed
track_spotify_history
vs.
track_spotify_history_update_or_insert
*/
void track_spotify_history_update_or_insert(const SongMetadata& meta) {
    sp_logger.logg("track_spotify_history_update_or_insert() called");
    static const std::string db_path = R"(.\star\sp_history.db)";
    const std::string timestamp = get_datetime_stamp_local();

    sqlite3* db = nullptr;
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        sp_logger.logg("Error opening database: {}", sqlite3_errmsg(db));
        return;
    }

    // Step 1: Check if the item exists
    const char* select_sql = R"sql(
        SELECT id, playcount FROM track_history
        WHERE name = ? AND artist = ? AND album = ?
    )sql";

    if (sqlite3_prepare_v2(db, select_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sp_logger.logg("Error preparing SELECT: {}", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_text(stmt, 1, meta.name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, meta.artist.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, meta.album.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    bool exists = (rc == SQLITE_ROW);
    int id = 0;
    int playcount = 0;

    if (exists) {
        id = sqlite3_column_int(stmt, 0);
        playcount = sqlite3_column_int(stmt, 1);
    }

    sqlite3_finalize(stmt); // always finalize before reuse

    if (exists) {
        // Step 2: Update if it exists
        const char* update_sql = R"sql(
            UPDATE track_history
            SET playcount = ?, last_played = ?
            WHERE id = ?
        )sql";

        if (sqlite3_prepare_v2(db, update_sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, playcount + 1);
            sqlite3_bind_text(stmt, 2, timestamp.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 3, id);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                sp_logger.logg("Error updating track: {}", sqlite3_errmsg(db));
            }
        }
        else {
            sp_logger.logg("Error preparing UPDATE: {}", sqlite3_errmsg(db));
        }
    }
    else {
        // Step 3: Insert new row
        const char* insert_sql = R"sql(
            INSERT INTO track_history
            (name, artist, album, duration, playcount, created_at, last_played)
            VALUES (?, ?, ?, ?, 1, ?, ?)
        )sql";

        if (sqlite3_prepare_v2(db, insert_sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, meta.name.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, meta.artist.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, meta.album.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 4, meta.duration_seconds);
            sqlite3_bind_text(stmt, 5, timestamp.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 6, timestamp.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                sp_logger.logg("Error inserting new track: {}", sqlite3_errmsg(db));
            }
        }
        else {
            sp_logger.logg("Error preparing INSERT: {}", sqlite3_errmsg(db));
        }
    }

    if (stmt) sqlite3_finalize(stmt);
    sqlite3_close(db);
    sp_logger.logg("track_spotify_history_update_or_insert() finished");
}

/*
\todo run a test for speed
track_spotify_history
vs.
track_spotify_history_update_or_insert
*/
void track_spotify_history(const SongMetadata& meta) {
    sp_logger.logg("track_spotify_history() called");
    static const string db_path = R"(.\star\sp_history.db)";

    const string timestamp = get_datetime_stamp_local();

    // Open database
    sqlite3* db = nullptr;
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        sp_logger.logg("Error opening database: {}", sqlite3_errmsg(db));
        return;
    }

    const char* upsert_sql = R"sql(
        INSERT INTO track_history (name, artist, album, duration, playcount, created_at, last_played)
        VALUES (?, ?, ?, ?, 1, ?, ?)
        ON CONFLICT(name, artist, album) DO UPDATE SET
            playcount = track_history.playcount + 1,
            last_played = excluded.last_played;
    )sql";

    if (sqlite3_prepare_v2(db, upsert_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sp_logger.logg("Error preparing UPSERT: {}", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    // Bind values (only 5, since playcount is hardcoded to 1)
    sqlite3_bind_text(stmt, 1, meta.name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, meta.artist.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, meta.album.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, meta.duration_seconds);
    sqlite3_bind_text(stmt, 5, timestamp.c_str(), -1, SQLITE_STATIC); // created_at
    sqlite3_bind_text(stmt, 6, timestamp.c_str(), -1, SQLITE_STATIC); // last_played

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sp_logger.logg("Error executing UPSERT: {}\n", sqlite3_errmsg(db));
    }

    if (stmt) {
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    sp_logger.logg("track_spotify_history() finished");
}
