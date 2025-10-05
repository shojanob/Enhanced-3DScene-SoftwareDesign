#pragma once
#include <sqlite3.h>
#include <string>
#include <memory>

class DbHelper {
public:
    explicit DbHelper(const std::string& dbPath);
    ~DbHelper();

    bool isOpen() const { return db_ != nullptr; }
    void close();

    // One-time schema setup (idempotent)
    bool ensureSchema();

    // Profiles: save camera pose + FOV + projection ("ORTHO"|"PERSPECTIVE")
    bool saveCameraProfile(const std::string& name,
                           float x, float y, float z,
                           float fov, const std::string& projection);

    // Telemetry: FPS and frame time (ms)
    bool logTelemetry(double fps, double frameMs);

    // Error log: source + message
    bool logError(const std::string& source, const std::string& message);

private:
    bool exec(const std::string& sql);

private:
    sqlite3* db_ = nullptr;
};
