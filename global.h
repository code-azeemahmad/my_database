// global.h
#ifndef GLOBAL_H
#define GLOBAL_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <memory>
#include <algorithm>
#include <ctime>
#include <functional>
#include <stdexcept>
#include <iomanip>

// Forward declaration
template<typename K, typename V>
class BPlusTree;

// Data types
enum class DataType {
    INT,
    STRING,
    FLOAT,
    BOOL,
    UNKNOWN
};

// Constraints
struct Constraints {
    bool is_primary_key = false;
    bool is_foreign_key = false;
    bool not_null = false;
    bool is_unique = false;
    std::string references_table;
    std::string references_column;
    int check_value = -1;
};

// Column definition
struct Column {
    std::string name;
    DataType type;
    int size = 255;
    Constraints constraints;
};

// Row data
struct Row {
    std::vector<std::string> values;
    bool is_deleted = false;
    int version = 1;
};

// Table metadata
struct Table {
    std::string name;
    std::vector<Column> columns;
    std::map<int, Row> rows;
    int next_row_id = 1;
    std::string primary_key_column;
    std::vector<std::string> foreign_keys;
    std::map<std::string, std::unique_ptr<BPlusTree<std::string, int>>> indexes;
};

// Current database state
extern std::string current_database;
extern std::map<std::string, Table> databases;
extern bool in_transaction;
extern std::vector<std::pair<std::string, std::function<void()>>> transaction_log;
extern std::map<std::string, std::map<int, Row>> transaction_backup;

#endif