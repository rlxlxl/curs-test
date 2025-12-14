#include "Auth.hpp"
#include <random>
#include <ctime>
#include <mutex>

std::string generateSessionId() {
    static const char alphanum[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    static std::mt19937 rng(std::time(nullptr));
    static std::uniform_int_distribution<> dist(0, sizeof(alphanum) - 2);
    
    std::string session_id;
    session_id.reserve(32);
    
    for (int i = 0; i < 32; ++i) {
        session_id += alphanum[dist(rng)];
    }
    
    return session_id;
}

std::string Auth::createSession(const std::string& username) {
    std::string session_id = generateSessionId();
    std::lock_guard<std::mutex> lock(sessions_mutex);
    sessions[session_id] = username;
    return session_id;
}

bool Auth::validateSession(const std::string& session_id, std::string& username) {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    auto it = sessions.find(session_id);
    if (it != sessions.end()) {
        username = it->second;
        return true;
    }
    return false;
}

void Auth::logout(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    sessions.erase(session_id);
}

std::string Auth::getSessionFromCookie(const crow::request& req) {
    auto cookie_header = req.get_header_value("Cookie");
    
    if (cookie_header.empty()) {
        return "";
    }
    
    // Ищем session_id в cookie
    size_t pos = cookie_header.find("session_id=");
    if (pos != std::string::npos) {
        pos += 11; // Длина "session_id="
        size_t end = cookie_header.find(";", pos);
        if (end == std::string::npos) {
            end = cookie_header.length();
        }
        std::string session_id = cookie_header.substr(pos, end - pos);
        
        // Удаляем пробелы в начале и конце
        size_t start = session_id.find_first_not_of(" \t");
        size_t end_pos = session_id.find_last_not_of(" \t");
        
        if (start != std::string::npos && end_pos != std::string::npos) {
            return session_id.substr(start, end_pos - start + 1);
        } else if (start != std::string::npos) {
            return session_id.substr(start);
        }
    }
    
    return "";
}

void Auth::setSessionCookie(crow::response& res, const std::string& session_id) {
    res.add_header("Set-Cookie", "session_id=" + session_id + "; Path=/; HttpOnly");
}