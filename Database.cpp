#include "Database.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

Database::Database() : connection(nullptr) {}

Database::~Database() {
    disconnect();
}

bool Database::connect(const DBConfig& config) {
    std::string conn_str = "host=" + config.host +
                          " port=" + config.port +
                          " dbname=" + config.dbname +
                          " user=" + config.user +
                          " password=" + config.password;
    
    std::cout << "Connecting to: " << conn_str << std::endl;
    
    connection = PQconnectdb(conn_str.c_str());
    
    if (PQstatus(connection) != CONNECTION_OK) {
        std::cerr << "Connection failed: " << PQerrorMessage(connection) << std::endl;
        return false;
    }
    
    std::cout << "Connected to database successfully!" << std::endl;
    return true;
}

void Database::disconnect() {
    if (connection) {
        PQfinish(connection);
        connection = nullptr;
    }
}

bool Database::authenticateUser(const std::string& username, const std::string& password) {
    std::cout << "DEBUG: Trying to authenticate " << username << std::endl;
    
    const char* param = username.c_str();
    PGresult* res = PQexecParams(connection,
        "SELECT password, role FROM users WHERE username = $1",
        1, NULL, &param, NULL, NULL, 0);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "DEBUG: Query error: " << PQerrorMessage(connection) << std::endl;
        PQclear(res);
        return false;
    }
    
    int rows = PQntuples(res);
    std::cout << "DEBUG: Found " << rows << " users with username " << username << std::endl;
    
    if (rows == 0) {
        PQclear(res);
        return false;
    }
    
    // Получаем пароль из базы
    char* db_password = PQgetvalue(res, 0, 0);
    char* db_role = PQgetvalue(res, 0, 1);
    
    std::cout << "DEBUG: DB password: '" << db_password << "'" << std::endl;
    std::cout << "DEBUG: Input password: '" << password << "'" << std::endl;
    std::cout << "DEBUG: User role: " << db_role << std::endl;
    
    bool authenticated = (password == std::string(db_password));
    
    PQclear(res);
    return authenticated;
}

std::string Database::getUserRole(const std::string& username) {
    const char* param = username.c_str();
    PGresult* res = PQexecParams(connection,
        "SELECT role FROM users WHERE username = $1",
        1, NULL, &param, NULL, NULL, 0);
    
    std::string role = "";
    if (PQntuples(res) > 0) {
        role = PQgetvalue(res, 0, 0);
    }
    
    PQclear(res);
    return role;
}

nlohmann::json Database::getAllIntegrators() {
    PGresult* res = PQexec(connection, 
        "SELECT id, name, city, description FROM integrators ORDER BY name");
    
    nlohmann::json result = nlohmann::json::array();
    
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        nlohmann::json integrator = {
            {"id", std::stoi(PQgetvalue(res, i, 0))},
            {"name", PQgetvalue(res, i, 1)},
            {"city", PQgetvalue(res, i, 2)},
            {"description", PQgetvalue(res, i, 3)}
        };
        result.push_back(integrator);
    }
    
    PQclear(res);
    return result;
}

nlohmann::json Database::getIntegratorById(int id) {
    std::string id_str = std::to_string(id);
    const char* param = id_str.c_str();
    
    PGresult* res = PQexecParams(connection,
        "SELECT id, name, city, description FROM integrators WHERE id = $1",
        1, NULL, &param, NULL, NULL, 0);
    
    nlohmann::json integrator;
    if (PQntuples(res) > 0) {
        integrator["id"] = std::stoi(PQgetvalue(res, 0, 0));
        integrator["name"] = PQgetvalue(res, 0, 1);
        integrator["city"] = PQgetvalue(res, 0, 2);
        integrator["description"] = PQgetvalue(res, 0, 3);
    }
    
    PQclear(res);
    return integrator;
}

bool Database::addIntegrator(const std::string& name, const std::string& city,
                           const std::string& description) {
    const char* params[3] = {name.c_str(), city.c_str(), description.c_str()};
    
    PGresult* res = PQexecParams(connection,
        "INSERT INTO integrators (name, city, description) VALUES ($1, $2, $3)",
        3, NULL, params, NULL, NULL, 0);
    
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    
    if (!success) {
        std::cerr << "DEBUG: Insert failed: " << PQerrorMessage(connection) << std::endl;
    }
    
    PQclear(res);
    
    return success;
}

bool Database::updateIntegrator(int id, const std::string& name, const std::string& city,
                              const std::string& description) {
    std::string id_str = std::to_string(id);
    const char* params[4] = {id_str.c_str(), name.c_str(), city.c_str(), description.c_str()};
    
    PGresult* res = PQexecParams(connection,
        "UPDATE integrators SET name = $2, city = $3, description = $4 WHERE id = $1",
        4, NULL, params, NULL, NULL, 0);
    
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

bool Database::deleteIntegrator(int id) {
    std::string id_str = std::to_string(id);
    const char* param = id_str.c_str();
    
    PGresult* res = PQexecParams(connection,
        "DELETE FROM integrators WHERE id = $1",
        1, NULL, &param, NULL, NULL, 0);
    
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}