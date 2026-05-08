#include "parser.h"
#include <algorithm>
#include <cctype>
#include <regex>

SQLParser::SQLParser(DatabaseManager& db) : db_manager(db) {}

void SQLParser::parse(const std::string& query) {
    std::string upper_query = query;
    std::transform(upper_query.begin(), upper_query.end(), upper_query.begin(), ::toupper);
    
    if (upper_query.find("CREATE DATABASE") != std::string::npos) {
        parse_create_database(query);
    }
    else if (upper_query.find("USE") != std::string::npos && upper_query.find("USE") == 0) {
        parse_use(query);
    }
    else if (upper_query.find("CREATE TABLE") != std::string::npos) {
        parse_create_table(query);
    }
    else if (upper_query.find("INSERT INTO") != std::string::npos) {
        parse_insert(query);
    }
    else if (upper_query.find("FIND") != std::string::npos || upper_query.find("SELECT") != std::string::npos) {
        parse_select(query);
    }
    else if (upper_query.find("UPDATE") != std::string::npos) {
        parse_update(query);
    }
    else if (upper_query.find("KILL") != std::string::npos || upper_query.find("DELETE") != std::string::npos) {
        parse_delete(query);
    }
    else if (upper_query.find("CREATE INDEX") != std::string::npos) {
        parse_create_index(query);
    }
    else if (upper_query.find("BEGIN") != std::string::npos) {
        TransactionManager::getInstance()->begin();
    }
    else if (upper_query.find("COMMIT") != std::string::npos) {
        TransactionManager::getInstance()->commit();
    }
    else if (upper_query.find("ROLLBACK") != std::string::npos) {
        TransactionManager::getInstance()->rollback();
    }
    else if (upper_query.find("SHOW TABLES") != std::string::npos) {
        db_manager.show_tables();
    }
    else if (upper_query.find("DESCRIBE") != std::string::npos) {
        std::string table = query.substr(query.find("DESCRIBE") + 8);
        db_manager.describe_table(trim(table));
    }
    else {
        std::cout << "Unknown command or not implemented yet.\n";
    }
}

void SQLParser::parse_create_database(const std::string& query) {
    // CREATE DATABASE db_name
    size_t start = query.find("DATABASE") + 8;
    std::string db_name = trim(query.substr(start));
    db_manager.create_database(db_name);
}

void SQLParser::parse_use(const std::string& query) {
    // USE db_name
    std::string db_name = trim(query.substr(3));
    db_manager.use_database(db_name);
}

void SQLParser::parse_create_table(const std::string& query) {
    // CREATE TABLE table_name (col1 TYPE constraints, col2 TYPE constraints, ...)
    size_t start = query.find("TABLE") + 5;
    size_t paren_start = query.find('(');
    
    std::string table_name = trim(query.substr(start, paren_start - start));
    
    std::string columns_def = query.substr(paren_start + 1, query.length() - paren_start - 2);
    
    std::vector<Column> columns;
    std::vector<std::string> col_defs = split(columns_def, ',');
    
    for (const auto& col_def : col_defs) {
        Column col;
        std::stringstream ss(col_def);
        std::string token;
        std::vector<std::string> tokens;
        
        while (ss >> token) {
            tokens.push_back(token);
        }
        
        if (tokens.size() >= 2) {
            col.name = tokens[0];
            col.type = parse_data_type(tokens[1]);
            
            if (col.type == DataType::STRING && tokens[1].find("STRING") != std::string::npos) {
                // Parse STRING(size)
                size_t size_start = tokens[1].find('(');
                size_t size_end = tokens[1].find(')');
                if (size_start != std::string::npos && size_end != std::string::npos) {
                    col.size = std::stoi(tokens[1].substr(size_start + 1, size_end - size_start - 1));
                }
            }
            
            // Parse constraints
            for (size_t i = 2; i < tokens.size(); i++) {
                if (tokens[i] == "PRIMARY" && i+1 < tokens.size() && tokens[i+1] == "KEY") {
                    col.constraints.is_primary_key = true;
                    i++;
                }
                else if (tokens[i] == "FOREIGN" && i+2 < tokens.size() && tokens[i+1] == "KEY" && tokens[i+2] == "REFERENCES") {
                    col.constraints.is_foreign_key = true;
                    // Parse references table and column
                    if (i+3 < tokens.size()) {
                        col.constraints.references_table = tokens[i+3];
                        if (i+4 < tokens.size()) {
                            col.constraints.references_column = tokens[i+4];
                        }
                    }
                    i += 2;
                }
                else if (tokens[i] == "NOT_NULL") {
                    col.constraints.not_null = true;
                }
                else if (tokens[i] == "UNIQUE") {
                    col.constraints.is_unique = true;
                }
            }
        }
        
        columns.push_back(col);
    }
    
    db_manager.create_table(table_name, columns);
}

void SQLParser::parse_insert(const std::string& query) {
    // INSERT INTO table_name VALUES (val1, val2, ...)
    size_t into_pos = query.find("INTO") + 4;
    size_t values_pos = query.find("VALUES");
    
    std::string table_name = trim(query.substr(into_pos, values_pos - into_pos));
    
    size_t paren_start = query.find('(', values_pos);
    size_t paren_end = query.find(')', paren_start);
    
    std::string values_str = query.substr(paren_start + 1, paren_end - paren_start - 1);
    std::vector<std::string> values = split(values_str, ',');
    
    for (auto& val : values) {
        val = trim(val);
        // Remove quotes
        if (val.front() == '"' || val.front() == '\'') {
            val = val.substr(1, val.length() - 2);
        }
    }
    
    db_manager.insert(table_name, values);
}

void SQLParser::parse_select(const std::string& query) {
    // FIND * FROM table WHERE condition
    std::string upper_query = query;
    std::transform(upper_query.begin(), upper_query.end(), upper_query.begin(), ::toupper);
    
    size_t from_pos = upper_query.find("FROM");
    std::string what = trim(query.substr(4, from_pos - 4));
    
    size_t where_pos = upper_query.find("WHERE");
    std::string from_table;
    std::string condition;
    
    if (where_pos != std::string::npos) {
        from_table = trim(query.substr(from_pos + 4, where_pos - from_pos - 4));
        condition = trim(query.substr(where_pos + 5));
    } else {
        from_table = trim(query.substr(from_pos + 4));
        condition = "";
    }
    
    db_manager.select(what, from_table, condition);
}

void SQLParser::parse_update(const std::string& query) {
    // UPDATE table SET col1 = val1, col2 = val2 WHERE condition
    size_t set_pos = query.find("SET");
    std::string table_name = trim(query.substr(6, set_pos - 6));
    
    size_t where_pos = query.find("WHERE");
    std::string set_clause;
    std::string condition;
    
    if (where_pos != std::string::npos) {
        set_clause = trim(query.substr(set_pos + 3, where_pos - set_pos - 3));
        condition = trim(query.substr(where_pos + 5));
    } else {
        set_clause = trim(query.substr(set_pos + 3));
        condition = "";
    }
    
    db_manager.update(table_name, set_clause, condition);
}

void SQLParser::parse_delete(const std::string& query) {
    // KILL FROM table WHERE condition
    size_t from_pos = query.find("FROM");
    std::string table_name = trim(query.substr(from_pos + 4));
    
    size_t where_pos = query.find("WHERE");
    std::string condition = "";
    
    if (where_pos != std::string::npos) {
        condition = trim(query.substr(where_pos + 5));
        table_name = trim(query.substr(from_pos + 4, where_pos - from_pos - 4));
    }
    
    db_manager.delete_from(table_name, condition);
}

void SQLParser::parse_create_index(const std::string& query) {
    // CREATE INDEX ON table(column)
    size_t on_pos = query.find("ON");
    size_t paren_start = query.find('(');
    size_t paren_end = query.find(')');
    
    std::string table_name = trim(query.substr(on_pos + 2, paren_start - on_pos - 2));
    std::string column_name = trim(query.substr(paren_start + 1, paren_end - paren_start - 1));
    
    db_manager.create_index(table_name, column_name);
}

DataType SQLParser::parse_data_type(const std::string& type_str) {
    std::string upper_type = type_str;
    std::transform(upper_type.begin(), upper_type.end(), upper_type.begin(), ::toupper);
    
    if (upper_type.find("INT") != std::string::npos) return DataType::INT;
    if (upper_type.find("STRING") != std::string::npos) return DataType::STRING;
    if (upper_type.find("FLOAT") != std::string::npos) return DataType::FLOAT;
    if (upper_type.find("BOOL") != std::string::npos) return DataType::BOOL;
    
    return DataType::UNKNOWN;
}

std::vector<std::string> SQLParser::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string SQLParser::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}