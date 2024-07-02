




























cmake_minimum_required(VERSION 3.10)
project(my_web_server)

# C++17 표준 사용 설정
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(my_web_server main.cpp)

# cpp-httplib 라이브러리 포함
include_directories(cpp-httplib)

# SQLite3 라이브러리 찾기 및 포함
find_package(SQLite3 REQUIRED)
include_directories(${SQLITE3_INCLUDE_DIRS})

# nlohmann/json 라이브러리 포함
include_directories(external/json/single_include)

# 타겟 라이브러리 링크
target_link_libraries(my_web_server ${SQLite3_LIBRARIES})

