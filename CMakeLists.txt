cmake_minimum_required(VERSION 3.10)
project(integrators_backend)

set(CMAKE_CXX_STANDARD 17)

# Находим Crow (header-only библиотека)
find_path(CROW_INCLUDE_DIR crow.h)
if(NOT CROW_INCLUDE_DIR)
    message(STATUS "Crow not found, downloading...")
    include(FetchContent)
    FetchContent_Declare(
        crow
        GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
        GIT_TAG v1.0+5
    )
    FetchContent_MakeAvailable(crow)
    set(CROW_INCLUDE_DIR ${crow_SOURCE_DIR}/include)
endif()

# Находим PostgreSQL и libpq
find_package(PostgreSQL REQUIRED)

# На Mac с Homebrew библиотеки могут быть в разных местах
find_library(PQ_LIBRARY 
    NAMES pq libpq.dylib libpq.a
    PATHS
        /usr/local/lib
        /usr/lib
        /opt/homebrew/lib
        /opt/homebrew/opt/postgresql@14/lib
        /opt/homebrew/opt/libpq/lib
        ${PostgreSQL_LIBRARY_DIRS}
)

if(NOT PQ_LIBRARY)
    # Попробуем другой способ найти libpq
    execute_process(
        COMMAND brew --prefix libpq
        OUTPUT_VARIABLE BREW_LIBPQ_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    if(BREW_LIBPQ_PREFIX)
        set(PQ_LIBRARY "${BREW_LIBPQ_PREFIX}/lib/libpq.dylib")
        if(EXISTS ${PQ_LIBRARY})
            message(STATUS "Found libpq at: ${PQ_LIBRARY}")
        else()
            message(FATAL_ERROR "libpq not found. Please install: brew install libpq")
        endif()
    else()
        message(FATAL_ERROR "libpq not found. Please install: brew install libpq")
    endif()
endif()

message(STATUS "Found PostgreSQL include dir: ${PostgreSQL_INCLUDE_DIRS}")
message(STATUS "Found libpq: ${PQ_LIBRARY}")

# Находим nlohmann/json
find_package(nlohmann_json 3.2.0 REQUIRED)

include_directories(${CROW_INCLUDE_DIR})
include_directories(${PostgreSQL_INCLUDE_DIRS})

add_executable(integrators_backend
    main.cpp
    Database.cpp
    Auth.cpp
    Integrator.cpp
)

target_link_libraries(integrators_backend
    ${PQ_LIBRARY}
    nlohmann_json::nlohmann_json
)

# Для macOS может потребоваться фреймворки
if(APPLE)
    target_link_libraries(integrators_backend
        "-framework CoreFoundation"
        "-framework Security"
    )
endif()