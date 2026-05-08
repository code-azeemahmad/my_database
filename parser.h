#ifndef PARSER_H
#define PARSER_H

#include "db.h"

class SQLParser {
private:
    DatabaseManager& db_manager;
    
    void parse_create_table(const std::string& query);
    void parse_insert(const std::string& query);
    void parse_select(const std::string& query);
    void parse_update(const std::string& query);
    void parse_delete(const std::string& query);
    void parse_create_index(const std::string& query);
    void parse_use(const std::string& query);
    void parse_create_database(const std::string& query);
    
    DataType parse_data_type(const std::string& type_str);
    std::vector<std::string> split(const std::string& str, char delimiter);
    std::string trim(const std::string& str);
    
public:
    SQLParser(DatabaseManager& db);
    void parse(const std::string& query);
    bool is_transaction_command(const std::string& query);
};

#endif