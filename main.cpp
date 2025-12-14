#include "crow.h"
#include "Database.hpp"
#include "Auth.hpp"
#include "Integrator.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <string>


std::string read_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    crow::SimpleApp app;
    
    // Загрузка конфигурации
    std::ifstream config_file("/Users/slavavikentev/Desktop/учеба/семестр 3/прога/curstest/config.json");
    if (!config_file.is_open()) {
        std::cerr << "Cannot open config.json" << std::endl;
        return 1;
    }
    
    nlohmann::json config;
    config_file >> config;
    
    // Инициализация базы данных
    Database db;
    DBConfig db_config = {
        config["db_host"],
        config["db_port"],
        config["db_name"],
        config["db_user"],
        config["db_password"]
    };
    
    if (!db.connect(db_config)) {
        std::cerr << "Failed to connect to database" << std::endl;
        return 1;
    }
    
    // Инициализация системы аутентификации
    Auth auth;
    
    // Статические файлы (HTML, CSS, JS)
    CROW_ROUTE(app, "/")
    ([]() {
        std::string content = read_file("web/login.html");
        if (content.empty()) {
            return crow::response(404, "File not found");
        }
        crow::response res(content);
        res.set_header("Content-Type", "text/html");
        return res;
    });
    
    CROW_ROUTE(app, "/<string>")
    ([](const std::string& filename) {
        std::string filepath = filename;
        if (filename == "main.html") {
            filepath = "web/main.html";
        } else if (filename == "login.html") {
            filepath = "web/login.html";
        }
        
        std::string content = read_file(filepath);
        if (content.empty()) {
            return crow::response(404, "File not found: " + filepath);
        }
        
        // Определяем Content-Type
        std::string content_type = "text/plain";
        if (filepath.find(".html") != std::string::npos) {
            content_type = "text/html";
        } else if (filepath.find(".css") != std::string::npos) {
            content_type = "text/css";
        } else if (filepath.find(".js") != std::string::npos) {
            content_type = "application/javascript";
        }
        
        crow::response res(content);
        res.set_header("Content-Type", content_type);
        return res;
    });
    
    // API для аутентификации
    CROW_ROUTE(app, "/api/login")
    .methods("POST"_method)
    ([&db, &auth](const crow::request& req) {
        auto x = crow::json::load(req.body);
        if (!x) {
            return crow::response(400, "Invalid JSON");
        }
        
        std::string username = x["username"].s();
        std::string password = x["password"].s();
        
        if (db.authenticateUser(username, password)) {
            std::string session_id = auth.createSession(username);
            std::string role = db.getUserRole(username);
            
            crow::json::wvalue response;
            response["success"] = true;
            response["role"] = role;
            
            crow::response res(response);
            Auth::setSessionCookie(res, session_id);
            return res;
        }
        
        return crow::response(401, "Invalid credentials");
    });
    
    CROW_ROUTE(app, "/api/logout")
    .methods("POST"_method)
    ([&auth](const crow::request& req) {
        std::string session_id = Auth::getSessionFromCookie(req);
        if (!session_id.empty()) {
            auth.logout(session_id);
        }
        
        crow::response res;
        res.add_header("Set-Cookie", "session_id=; Path=/; HttpOnly; Max-Age=0");
        return res;
    });
    
    // Middleware для проверки аутентификации
    auto check_auth = [&auth, &db](const crow::request& req, std::string& username, std::string& role) -> bool {
        std::string session_id = Auth::getSessionFromCookie(req);
        if (session_id.empty()) {
            return false;
        }
        
        if (auth.validateSession(session_id, username)) {
            role = db.getUserRole(username);
            return !role.empty();
        }
        
        return false;
    };
    
    // API для интеграторов (требует аутентификации)
    CROW_ROUTE(app, "/api/integrators")
    .methods("GET"_method)
    ([&db, &check_auth](const crow::request& req) {
        std::string username, role;
        if (!check_auth(req, username, role)) {
            return crow::response(401, "Not authenticated");
        }
        
        auto integrators_json = db.getAllIntegrators();
        crow::json::wvalue response;
        
        // Преобразуем nlohmann::json в crow::json
        crow::json::wvalue::list integrators_list;
        for (const auto& integrator : integrators_json) {
            crow::json::wvalue w;
            w["id"] = integrator["id"].get<int>();
            w["name"] = integrator["name"].get<std::string>();
            w["city"] = integrator["city"].get<std::string>();
            w["description"] = integrator["description"].get<std::string>();
            integrators_list.push_back(std::move(w));
        }
        
        response["integrators"] = std::move(integrators_list);
        response["user_role"] = role;
        
        return crow::response(response);
    });
    
    // API для получения одного интегратора
    CROW_ROUTE(app, "/api/integrators/<int>")
    .methods("GET"_method)
    ([&db, &check_auth](const crow::request& req, int id) {
        std::string username, role;
        if (!check_auth(req, username, role)) {
            return crow::response(401, "Not authenticated");
        }
        
        if (role != "admin") {
            return crow::response(403, "Admin only");
        }
        
        auto integrator_json = db.getIntegratorById(id);
        if (integrator_json.empty()) {
            return crow::response(404, "Integrator not found");
        }
        
        crow::json::wvalue response;
        response["id"] = integrator_json["id"].get<int>();
        response["name"] = integrator_json["name"].get<std::string>();
        response["city"] = integrator_json["city"].get<std::string>();
        response["description"] = integrator_json["description"].get<std::string>();
        
        return crow::response(response);
    });
    
    // API для добавления интегратора (только админ)
    CROW_ROUTE(app, "/api/integrators")
    .methods("POST"_method)
    ([&db, &check_auth](const crow::request& req) {
        std::string username, role;
        if (!check_auth(req, username, role)) {
            return crow::response(401, "Not authenticated");
        }
        
        if (role != "admin") {
            return crow::response(403, "Admin only");
        }
        
        auto x = crow::json::load(req.body);
        if (!x) {
            return crow::response(400, "Invalid JSON");
        }
        
        std::string name = x["name"].s();
        std::string city = x["city"].s();
        std::string description = x["description"].s();
        
        if (name.empty() || city.empty()) {
            return crow::response(400, "Name and city are required");
        }
        
        if (db.addIntegrator(name, city, description)) {
            return crow::response(201, "Integrator added");
        }
        
        return crow::response(500, "Failed to add integrator");
    });
    
    // API для обновления интегратора
    CROW_ROUTE(app, "/api/integrators/<int>")
    .methods("PUT"_method)
    ([&db, &check_auth](const crow::request& req, int id) {
        std::string username, role;
        if (!check_auth(req, username, role)) {
            return crow::response(401, "Not authenticated");
        }
        
        if (role != "admin") {
            return crow::response(403, "Admin only");
        }
        
        auto x = crow::json::load(req.body);
        if (!x) {
            return crow::response(400, "Invalid JSON");
        }
        
        std::string name = x["name"].s();
        std::string city = x["city"].s();
        std::string description = x["description"].s();
        
        if (name.empty() || city.empty()) {
            return crow::response(400, "Name and city are required");
        }
        
        if (db.updateIntegrator(id, name, city, description)) {
            return crow::response(200, "Integrator updated");
        }
        
        return crow::response(500, "Failed to update integrator");
    });
    
    // API для удаления интегратора
    CROW_ROUTE(app, "/api/integrators/<int>")
    .methods("DELETE"_method)
    ([&db, &check_auth](const crow::request& req, int id) {
        std::string username, role;
        if (!check_auth(req, username, role)) {
            return crow::response(401, "Not authenticated");
        }
        
        if (role != "admin") {
            return crow::response(403, "Admin only");
        }
        
        if (db.deleteIntegrator(id)) {
            return crow::response(200, "Integrator deleted");
        }
        
        return crow::response(500, "Failed to delete integrator");
    });
    
    // Проверка сессии
    CROW_ROUTE(app, "/api/check-session")
    .methods("GET"_method)
    ([&db, &check_auth](const crow::request& req) {
        std::string username, role;
        if (!check_auth(req, username, role)) {
            crow::json::wvalue response;
            response["authenticated"] = false;
            return crow::response(401, response);
        }
        
        crow::json::wvalue response;
        response["authenticated"] = true;
        response["username"] = username;
        response["role"] = role;
        
        return crow::response(response);
    });
    
    // Запуск сервера
    int port = config["server_port"];
    std::cout << "Server running on port " << port << std::endl;
    app.port(port).multithreaded().run();
    
    return 0;
}