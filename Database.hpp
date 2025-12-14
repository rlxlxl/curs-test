#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include <vector>
#include <map>
#include <libpq-fe.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct DBConfig {
    std::string host;
    std::string port;
    std::string dbname;
    std::string user;
    std::string password;
};

class Database {
private:
    PGconn* connection;
    
public:
    Database();
    ~Database();
    
    bool connect(const DBConfig& config);
    void disconnect();
    
    // User operations
    bool authenticateUser(const std::string& username, const std::string& password);
    std::string getUserRole(const std::string& username);
    
    // Integrator operations
    json getAllIntegrators();
    json getIntegratorById(int id);
    bool addIntegrator(const std::string& name, const std::string& city, 
                       const std::string& description);
    bool updateIntegrator(int id, const std::string& name, const std::string& city,
                         const std::string& description);
    bool deleteIntegrator(int id);
};

#endif