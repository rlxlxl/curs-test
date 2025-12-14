#include "Integrator.hpp"

json Integrator::toJson() const {
    return {
        {"id", id},
        {"name", name},
        {"city", city},
        {"description", description}
    };
}

Integrator Integrator::fromJson(const json& j) {
    Integrator integrator;
    
    if (j.contains("id")) integrator.id = j["id"];
    if (j.contains("name")) integrator.name = j["name"];
    if (j.contains("city")) integrator.city = j["city"];
    if (j.contains("description")) integrator.description = j["description"];
    
    return integrator;
}