#include "DbHelper.h"
#include <iostream>

static int noop_cb(void*, int, char**, char**) { return 0; }

DbHelper::DbHelper(const std::string& dbPath) {
    if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "[DbHelper] Error opening DB: " << sqlite3_errmsg(db_) << "\n";
        db_ = nullptr;
    } else {
        ensureSchema();
    }
}

DbHelper::~DbHelper() { close(); }

void DbHelper::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool DbHelper::exec(const std::string& sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), noop_cb, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::cerr << "[DbHelper] SQL error: " << (err ? err : "(unknown)") << "\n";
        if (err) sqlite3_free(err);
        return false;
    }
    return true;
}

bool DbHelper::ensureSchema() {
    if (!db_) return false;

    const char* ddl[] = {
        "CREATE TABLE IF NOT EXISTS profiles ("
            "name TEXT PRIMARY KEY,"
            "camera_pos_x REAL, camera_pos_y REAL, camera_pos_z REAL,"
            "fov REAL, projection TEXT"
        ");",
        "CREATE TABLE IF NOT EXISTS telemetry ("
            "ts DATETIME DEFAULT CURRENT_TIMESTAMP,"
            "fps REAL, frame_ms REAL"
        ");",
        "CREATE TABLE IF NOT EXISTS errors ("
            "ts DATETIME DEFAULT CURRENT_TIMESTAMP,"
            "source TEXT, message TEXT"
        ");"
    };

    for (auto* stmt : ddl) {
        if (!exec(stmt)) return false;
    }
    return true;
}

bool DbHelper::saveCameraProfile(const std::string& name,
                                 float x, float y, float z,
                                 float fov, const std::string& projection) {
    if (!db_) return false;

    const char* sql =
        "INSERT INTO profiles(name, camera_pos_x, camera_pos_y, camera_pos_z, fov, projection) "
        "VALUES(?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(name) DO UPDATE SET "
        " camera_pos_x=excluded.camera_pos_x,"
        " camera_pos_y=excluded.camera_pos_y,"
        " camera_pos_z=excluded.camera_pos_z,"
        " fov=excluded.fov,"
        " projection=excluded.projection;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DbHelper] prepare(saveCameraProfile) failed: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, static_cast<double>(x));
    sqlite3_bind_double(stmt, 3, static_cast<double>(y));
    sqlite3_bind_double(stmt, 4, static_cast<double>(z));
    sqlite3_bind_double(stmt, 5, static_cast<double>(fov));
    sqlite3_bind_text(stmt, 6, projection.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) {
        std::cerr << "[DbHelper] step(saveCameraProfile) failed: "
                  << sqlite3_errmsg(db_) << "\n";
    }
    sqlite3_finalize(stmt);
    return ok;
}

bool DbHelper::logTelemetry(double fps, double frameMs) {
    if (!db_) return false;

    const char* sql =
        "INSERT INTO telemetry(fps, frame_ms) VALUES(?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DbHelper] prepare(logTelemetry) failed: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_double(stmt, 1, fps);
    sqlite3_bind_double(stmt, 2, frameMs);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) {
        std::cerr << "[DbHelper] step(logTelemetry) failed: "
                  << sqlite3_errmsg(db_) << "\n";
    }
    sqlite3_finalize(stmt);
    return ok;
}

bool DbHelper::logError(const std::string& source, const std::string& message) {
    if (!db_) return false;

    const char* sql =
        "INSERT INTO errors(source, message) VALUES(?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DbHelper] prepare(logError) failed: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_text(stmt, 1, source.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, message.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) {
        std::cerr << "[DbHelper] step(logError) failed: "
                  << sqlite3_errmsg(db_) << "\n";
    }
    sqlite3_finalize(stmt);
    return ok;
}
