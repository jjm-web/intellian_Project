#include "httplib.h"
#include <iostream>
#include <fstream>
#include <sqlite3.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <chrono>
#include <ctime>

using json = nlohmann::json;
std::ofstream log_file;
httplib::Server svr;

void init_db() {
    sqlite3 *db;
    char *err_msg = nullptr;
    std::string db_path = std::filesystem::current_path().string() + "/satellite_data.db";
    log_file << "Attempting to open database at: " << db_path << std::endl;
    int rc = sqlite3_open(db_path.c_str(), &db);
    if (rc != SQLITE_OK) {
        log_file << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    const char *drop_sql = "DROP TABLE IF EXISTS data;";
    rc = sqlite3_exec(db, drop_sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        log_file << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    }

    const char *create_sql = "CREATE TABLE IF NOT EXISTS data ("
                             "timestamp TEXT, "
                             "signal_strength REAL, "
                             "latency REAL, "
                             "status TEXT, "
                             "latitude REAL, "
                             "longitude REAL, "
                             "address TEXT, "
                             "gyroscope_x REAL, "
                             "gyroscope_y REAL, "
                             "gyroscope_z REAL, "
                             "temperature REAL, "
                             "humidity REAL);";
    log_file << "Executing SQL: " << create_sql << std::endl;
    rc = sqlite3_exec(db, create_sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        log_file << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    } else {
        log_file << "Database and table created successfully." << std::endl;
    }
    sqlite3_close(db);
    log_file << "Database closed." << std::endl;
}

std::string convert_to_readable_time(long long milliseconds) {
    auto ms = std::chrono::milliseconds(milliseconds);
    auto tp = std::chrono::system_clock::time_point(ms);
    std::time_t time = std::chrono::system_clock::to_time_t(tp);
    std::tm* tm = std::localtime(&time);
    char buffer[100];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm);
    return std::string(buffer);
}

void log_request(const std::string& log_entry) {
    log_file << log_entry << std::endl;
}

int main() {
    log_file.open("server.log", std::ios::out | std::ios::app);
    if (!log_file.is_open()) {
        std::cerr << "Failed to open log file." << std::endl;
        return 1;
    }

    log_file << "Server starting..." << std::endl;
    init_db();

    svr.set_base_dir("./www");

    svr.set_pre_routing_handler([](const httplib::Request &req, httplib::Response &res) {
        if (req.method == "OPTIONS") {
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
            res.status = 204;
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    svr.Post("/api/satellite_data", [](const httplib::Request &req, httplib::Response &res) {
        log_request("POST /api/satellite_data request received.");

        try {
            auto json = json::parse(req.body);

            long long timestamp = json.value("timestamp", 0LL);
            double signal_strength = json.value("signal_strength", 0.0);
            double latency = json.value("latency", 0.0);
            std::string status = json.value("status", "unknown");
            double latitude = json.value("latitude", 0.0);
            double longitude = json.value("longitude", 0.0);
            std::string address = json.value("address", "unknown");
            double gyroscope_x = json["gyroscope"].value("x", 0.0);
            double gyroscope_y = json["gyroscope"].value("y", 0.0);
            double gyroscope_z = json["gyroscope"].value("z", 0.0);
            double temperature = json.value("temperature", 0.0);
            double humidity = json.value("humidity", 0.0);

            std::string readable_time = convert_to_readable_time(timestamp);

            sqlite3 *db;
            char *err_msg = nullptr;

            std::string db_path = std::filesystem::current_path().string() + "/satellite_data.db";
            int rc = sqlite3_open(db_path.c_str(), &db);
            if (rc != SQLITE_OK) {
                std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
                res.set_content("Cannot open database", "text/plain");
                res.status = 500;
                log_request("Failed to open database for POST request.");
                return;
            }

            std::string sql = "INSERT INTO data (timestamp, signal_strength, latency, status, latitude, longitude, address, gyroscope_x, gyroscope_y, gyroscope_z, temperature, humidity) VALUES ('"
                              + readable_time + "', "
                              + std::to_string(signal_strength) + ", "
                              + std::to_string(latency) + ", '"
                              + status + "', "
                              + std::to_string(latitude) + ", "
                              + std::to_string(longitude) + ", '"
                              + address + "', "
                              + std::to_string(gyroscope_x) + ", "
                              + std::to_string(gyroscope_y) + ", "
                              + std::to_string(gyroscope_z) + ", "
                              + std::to_string(temperature) + ", "
                              + std::to_string(humidity) + ");";

            rc = sqlite3_exec(db, sql.c_str(), 0, 0, &err_msg);
            if (rc != SQLITE_OK) {
                std::cerr << "SQL error: " << err_msg << std::endl;
                res.set_content("SQL error: " + std::string(err_msg), "text/plain");
                sqlite3_free(err_msg);
                res.status = 500;
                log_request("SQL error on POST request: " + std::string(err_msg));
            } else {
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_content("{\"message\": \"Data received\"}", "application/json");
                res.status = 201;
                log_request("Data received: " + req.body);
            }

            sqlite3_close(db);
        } catch (const std::exception &e) {
            std::cerr << "Exception: " << e.what() << std::endl;
            res.set_content("Exception: " + std::string(e.what()), "text/plain");
            res.status = 500;
            log_request("Exception on POST request: " + std::string(e.what()));
        }
    });

    svr.Get("/data", [](const httplib::Request &, httplib::Response &res) {
        log_request("GET /data request received.");

        sqlite3 *db;
        char *err_msg = nullptr;

        std::string db_path = std::filesystem::current_path().string() + "/satellite_data.db";
        int rc = sqlite3_open(db_path.c_str(), &db);
        if (rc != SQLITE_OK) {
            res.set_content("Cannot open database", "text/plain");
            log_request("Failed to open database for GET request.");
            return;
        }

        const char *sql = "SELECT * FROM data";

        sqlite3_stmt *stmt;
        rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            res.set_content("Failed to fetch data", "text/plain");
            sqlite3_close(db);
            log_request("Failed to prepare SQL statement for GET request.");
            return;
        }

        json json_data = json::array();
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            json row;
            row["timestamp"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            row["signal_strength"] = sqlite3_column_double(stmt, 1);
            row["latency"] = sqlite3_column_double(stmt, 2);
            row["status"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            row["latitude"] = sqlite3_column_double(stmt, 4);
            row["longitude"] = sqlite3_column_double(stmt, 5);
            row["address"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
            row["gyroscope_x"] = sqlite3_column_double(stmt, 7);
            row["gyroscope_y"] = sqlite3_column_double(stmt, 8);
            row["gyroscope_z"] = sqlite3_column_double(stmt, 9);
            row["temperature"] = sqlite3_column_double(stmt, 10);
            row["humidity"] = sqlite3_column_double(stmt, 11);
            json_data.push_back(row);
        }

        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(json_data.dump(), "application/json");
        log_request("GET request processed successfully.");

        sqlite3_finalize(stmt);
        sqlite3_close(db);
    });

    svr.listen("0.0.0.0", 5000);
    log_file.close();
}

