// main.cpp
#include "global.h"
#include "db.h"
#include "parser.h"
#include "Transaction.h"
#include <iostream>
#include <string>
#include <algorithm>

// Global variable definitions
std::string current_database = "";
std::map<std::string, Table> databases;
bool in_transaction = false;
std::vector<std::pair<std::string, std::function<void()>>> transaction_log;
std::map<std::string, std::map<int, Row>> transaction_backup;

int main() {
    std::cout << "========================================\n";
    std::cout << "   Custom Database Management System    \n";
    std::cout << "========================================\n";
    std::cout << "Commands:\n";
    std::cout << "  - CREATE DATABASE <name>\n";
    std::cout << "  - USE <name>\n";
    std::cout << "  - CREATE TABLE <name> (col1 TYPE, col2 TYPE, ...)\n";
    std::cout << "  - INSERT INTO <td> VALUES (val1, val2, ...)\n";
    std::cout << "  - FIND * FROM <td> [WHERE condition]\n";
    std::cout << "  - UPDATE <td> SET col=val [WHERE condition]\n";
    std::cout << "  - KILL FROM <td> [WHERE condition]\n";
    std::cout << "  - CREATE INDEX ON <td>(<column>)\n";
    std::cout << "  - BEGIN / COMMIT / ROLLBACK\n";
    std::cout << "  - SHOW TABLES\n";
    std::cout << "  - DESCRIBE <td>\n";
    std::cout << "  - EXIT\n";
    std::cout << "========================================\n\n";
    
    DatabaseManager db_manager;
    SQLParser parser(db_manager);
    
    std::string query;
    
    while (true) {
        if (!current_database.empty()) {
            std::cout << current_database << "> ";
        } else {
            std::cout << "> ";
        }
        
        std::getline(std::cin, query);
        
        if (query.empty()) continue;
        
        std::string upper_query = query;
        std::transform(upper_query.begin(), upper_query.end(), upper_query.begin(), ::toupper);
        
        if (upper_query == "EXIT" || upper_query == "QUIT") {
            std::cout << "Goodbye!\n";
            break;
        }
        
        try {
            parser.parse(query);
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
        
        std::cout << "\n";
    }
    
    return 0;
}