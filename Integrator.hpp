#ifndef INTEGRATOR_HPP
#define INTEGRATOR_HPP

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct Integrator {
    int id;
    std::string name;
    std::string city;
    std::string description;
    
    json toJson() const;
    static Integrator fromJson(const json& j);
};

#endif