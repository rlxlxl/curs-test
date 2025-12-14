#ifndef AUTH_HPP
#define AUTH_HPP

#include <string>
#include <map>
#include <crow.h>
#include <mutex>

class Auth {
private:
    std::map<std::string, std::string> sessions; // session_id -> username
    std::mutex sessions_mutex; // Для потокобезопасности
    
public:
    std::string createSession(const std::string& username);
    bool validateSession(const std::string& session_id, std::string& username);
    void logout(const std::string& session_id);
    
    static std::string getSessionFromCookie(const crow::request& req);
    static void setSessionCookie(crow::response& res, const std::string& session_id);
};

#endif