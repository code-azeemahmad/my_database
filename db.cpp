#include "db.h"
#include <iomanip>
#include <cmath>
#include <regex>
#include <algorithm>
#include <fstream>
#include <sstream>

DatabaseManager::DatabaseManager() {
    txn_manager = TransactionManager::getInstance();
}

void DatabaseManager::create_database(const std::string& db_name) {
    if (databases.find(db_name) != databases.end()) {
        std::cout << "Database '" << db_name << "' already exists.\n";
        return;
    }
    
    databases[db_name] = Table();
    databases[db_name].name = db_name;
    std::cout << "Database '" << db_name << "' created.\n";
    
    // Create directory for database files
    std::string command = "mkdir " + db_name;
    system(command.c_str());
}

void DatabaseManager::use_database(const std::string& db_name) {
    if (databases.find(db_name) == databases.end()) {
        std::cout << "Database '" << db_name << "' does not exist.\n";
        return;
    }
    
    current_database = db_name;
    std::cout << "Using database '" << db_name << "'.\n";
}

void DatabaseManager::create_table(const std::string& table_name, const std::vector<Column>& columns) {
    if (current_database.empty()) {
        std::cout << "No database selected. Use 'USE database_name' first.\n";
        return;
    }
    
    Table& db = databases[current_database];
    
    if (!db.columns.empty()) {
        std::cout << "Table '" << table_name << "' already exists.\n";
        return;
    }
    
    db.name = table_name;
    db.columns = columns;
    
    // Find primary key
    for (const auto& col : columns) {
        if (col.constraints.is_primary_key) {
            db.primary_key_column = col.name;
            break;
        }
    }
    
    std::cout << "Table '" << table_name << "' created with " << columns.size() << " columns.\n";
}

void DatabaseManager::insert(const std::string& table_name, const std::vector<std::string>& values) {
    if (current_database.empty()) {
        std::cout << "No database selected.\n";
        return;
    }
    
    Table& table = databases[current_database];
    
    if (table.columns.empty()) {
        std::cout << "Table '" << table_name << "' does not exist.\n";
        return;
    }
    
    if (values.size() != table.columns.size()) {
        std::cout << "Expected " << table.columns.size() << " values, got " << values.size() << "\n";
        return;
    }
    
    // Validate data types and constraints
    Row new_row;
    for (size_t i = 0; i < table.columns.size(); i++) {
        if (!validate_data_type(table.columns[i], values[i])) {
            std::cout << "Invalid data type for column '" << table.columns[i].name << "'\n";
            return;
        }
        
        if (table.columns[i].constraints.not_null && values[i].empty()) {
            std::cout << "Column '" << table.columns[i].name << "' cannot be NULL\n";
            return;
        }
        
        new_row.values.push_back(values[i]);
    }
    
    // Save state for transaction
    if (txn_manager->is_transaction_active()) {
        txn_manager->save_state(table_name, table.next_row_id, new_row);
    }
    
    // Insert the row
    int row_id = table.next_row_id++;
    table.rows[row_id] = new_row;
    
    // Update indexes
    for (size_t i = 0; i < table.columns.size(); i++) {
        update_index(table_name, table.columns[i].name, "", values[i], row_id);
    }
    
    std::cout << "Inserted row with ID: " << row_id << "\n";
}

void DatabaseManager::select(const std::string& what, const std::string& from, 
                             const std::string& where_condition) {
    if (current_database.empty()) {
        std::cout << "No database selected.\n";
        return;
    }
    
    Table& table = databases[current_database];
    
    if (table.columns.empty()) {
        std::cout << "Table '" << from << "' does not exist.\n";
        return;
    }
    
    // Parse what columns to select
    std::vector<std::string> select_columns;
    if (what == "*") {
        for (const auto& col : table.columns) {
            select_columns.push_back(col.name);
        }
    } else {
        std::stringstream ss(what);
        std::string col;
        while (std::getline(ss, col, ',')) {
            col.erase(0, col.find_first_not_of(" \t"));
            col.erase(col.find_last_not_of(" \t") + 1);
            select_columns.push_back(col);
        }
    }
    
    // Print header
    std::cout << "\n";
    for (const auto& col : select_columns) {
        std::cout << std::setw(20) << std::left << col;
    }
    std::cout << "\n";
    std::cout << std::string(select_columns.size() * 20, '-') << "\n";
    
    // Select rows
    int count = 0;
    for (const auto& pair : table.rows) {
        int row_id = pair.first;
        const Row& row = pair.second;
        
        if (row.is_deleted) continue;
        
        // Evaluate WHERE condition
        if (!where_condition.empty() && !evaluate_condition(row, table.columns, where_condition)) {
            continue;
        }
        
        // Print row
        for (const auto& col_name : select_columns) {
            // Find column index
            int col_index = -1;
            for (size_t i = 0; i < table.columns.size(); i++) {
                if (table.columns[i].name == col_name) {
                    col_index = i;
                    break;
                }
            }
            
            if (col_index != -1 && col_index < (int)row.values.size()) {
                std::cout << std::setw(20) << std::left << row.values[col_index];
            } else {
                std::cout << std::setw(20) << std::left << "NULL";
            }
        }
        std::cout << "\n";
        count++;
    }
    
    std::cout << "\n" << count << " row(s) returned.\n";
}

void DatabaseManager::update(const std::string& table_name, const std::string& set_clause, 
                             const std::string& where_condition) {
    if (current_database.empty()) {
        std::cout << "No database selected.\n";
        return;
    }
    
    Table& table = databases[current_database];
    
    // Parse SET clause (e.g., "salary = 50000, bonus = 1000")
    std::map<std::string, std::string> updates;
    std::stringstream ss(set_clause);
    std::string assignment;
    while (std::getline(ss, assignment, ',')) {
        size_t eq_pos = assignment.find('=');
        if (eq_pos != std::string::npos) {
            std::string col = assignment.substr(0, eq_pos);
            std::string val = assignment.substr(eq_pos + 1);
            col.erase(0, col.find_first_not_of(" \t"));
            col.erase(col.find_last_not_of(" \t") + 1);
            val.erase(0, val.find_first_not_of(" \t"));
            val.erase(val.find_last_not_of(" \t") + 1);
            updates[col] = val;
        }
    }
    
    int updated_count = 0;
    for (auto& pair : table.rows) {
        int row_id = pair.first;
        Row& row = pair.second;
        
        if (row.is_deleted) continue;
        
        if (!where_condition.empty() && !evaluate_condition(row, table.columns, where_condition)) {
            continue;
        }
        
        // Save state for transaction
        if (txn_manager->is_transaction_active()) {
            txn_manager->save_state(table_name, row_id, row);
        }
        
        // Apply updates
        for (auto& update_pair : updates) {
            const std::string& col_name = update_pair.first;
            const std::string& new_value = update_pair.second;
            
            // Find column index
            for (size_t i = 0; i < table.columns.size(); i++) {
                if (table.columns[i].name == col_name) {
                    std::string old_value = row.values[i];
                    row.values[i] = new_value;
                    update_index(table_name, col_name, old_value, new_value, row_id);
                    break;
                }
            }
        }
        
        updated_count++;
    }
    
    std::cout << "Updated " << updated_count << " row(s).\n";
}

void DatabaseManager::delete_from(const std::string& table_name, const std::string& where_condition) {
    if (current_database.empty()) {
        std::cout << "No database selected.\n";
        return;
    }
    
    Table& table = databases[current_database];
    
    int deleted_count = 0;
    auto it = table.rows.begin();
    while (it != table.rows.end()) {
        int row_id = it->first;
        Row& row = it->second;
        
        if (row.is_deleted) {
            ++it;
            continue;
        }
        
        if (!where_condition.empty() && !evaluate_condition(row, table.columns, where_condition)) {
            ++it;
            continue;
        }
        
        // Save state for transaction
        if (txn_manager->is_transaction_active()) {
            txn_manager->save_state(table_name, row_id, row);
        }
        
        row.is_deleted = true;
        deleted_count++;
        ++it;
    }
    
    std::cout << "Deleted " << deleted_count << " row(s).\n";
}

void DatabaseManager::create_index(const std::string& table_name, const std::string& column) {
    if (current_database.empty()) {
        std::cout << "No database selected.\n";
        return;
    }
    
    Table& table = databases[current_database];
    
    // Find column index
    int col_index = -1;
    for (size_t i = 0; i < table.columns.size(); i++) {
        if (table.columns[i].name == column) {
            col_index = i;
            break;
        }
    }
    
    if (col_index == -1) {
        std::cout << "Column '" << column << "' not found in table '" << table_name << "'\n";
        return;
    }
    
    // Create B+ tree index
    auto index = std::make_unique<BPlusTree<std::string, int>>();
    
    // Index existing rows
    for (const auto& pair : table.rows) {
        int row_id = pair.first;
        const Row& row = pair.second;
        if (!row.is_deleted && col_index < (int)row.values.size()) {
            index->insert(row.values[col_index], row_id);
        }
    }
    
    table.indexes[column] = std::move(index);
    std::cout << "Index created on '" << column << "' for table '" << table_name << "'\n";
}

void DatabaseManager::show_tables() {
    if (current_database.empty()) {
        std::cout << "No database selected.\n";
        return;
    }
    
    Table& table = databases[current_database];
    
    if (table.columns.empty()) {
        std::cout << "No tables in database '" << current_database << "'\n";
        return;
    }
    
    std::cout << "Table: " << table.name << "\n";
    std::cout << "Columns: ";
    for (const auto& col : table.columns) {
        std::cout << col.name << " ";
    }
    std::cout << "\n";
    std::cout << "Rows: " << table.rows.size() << "\n";
}

void DatabaseManager::describe_table(const std::string& table_name) {
    if (current_database.empty()) {
        std::cout << "No database selected.\n";
        return;
    }
    
    Table& table = databases[current_database];
    
    if (table.columns.empty()) {
        std::cout << "Table '" << table_name << "' does not exist.\n";
        return;
    }
    
    std::cout << "\n" << std::setw(20) << std::left << "Column"
              << std::setw(15) << std::left << "Type"
              << std::setw(15) << std::left << "Nullable"
              << std::setw(15) << std::left << "Key" << "\n";
    std::cout << std::string(65, '-') << "\n";
    
    for (const auto& col : table.columns) {
        std::string type_str;
        switch (col.type) {
            case DataType::INT: type_str = "INT"; break;
            case DataType::STRING: type_str = "STRING(" + std::to_string(col.size) + ")"; break;
            case DataType::FLOAT: type_str = "FLOAT"; break;
            case DataType::BOOL: type_str = "BOOL"; break;
            default: type_str = "UNKNOWN";
        }
        
        std::string nullable = col.constraints.not_null ? "NO" : "YES";
        std::string key = col.constraints.is_primary_key ? "PRI" : "";
        
        std::cout << std::setw(20) << std::left << col.name
                  << std::setw(15) << std::left << type_str
                  << std::setw(15) << std::left << nullable
                  << std::setw(15) << std::left << key << "\n";
    }
    std::cout << "\n";
}

bool DatabaseManager::validate_data_type(const Column& col, const std::string& value) {
    switch (col.type) {
        case DataType::INT:
            try {
                std::stoi(value);
                return true;
            } catch (...) {
                return false;
            }
        case DataType::STRING:
            return value.length() <= (size_t)col.size;
        case DataType::FLOAT:
            try {
                std::stof(value);
                return true;
            } catch (...) {
                return false;
            }
        case DataType::BOOL:
            return value == "true" || value == "false" || value == "0" || value == "1";
        default:
            return true;
    }
}

bool DatabaseManager::evaluate_condition(const Row& row, const std::vector<Column>& columns, 
                                         const std::string& condition) {
    // Parse simple conditions like "column = value", "column > value", etc.
    std::vector<std::string> operators = {"<=", ">=", "!=", "=", "<", ">"};
    std::string op;
    size_t op_pos = std::string::npos;
    
    for (const auto& oper : operators) {
        op_pos = condition.find(oper);
        if (op_pos != std::string::npos) {
            op = oper;
            break;
        }
    }
    
    if (op_pos == std::string::npos) {
        return true; // No condition or unsupported
    }
    
    std::string left = condition.substr(0, op_pos);
    std::string right = condition.substr(op_pos + op.length());
    
    left.erase(0, left.find_first_not_of(" \t"));
    left.erase(left.find_last_not_of(" \t") + 1);
    right.erase(0, right.find_first_not_of(" \t"));
    right.erase(right.find_last_not_of(" \t") + 1);
    
    // Remove quotes if present
    if (right.front() == '"' || right.front() == '\'') {
        right = right.substr(1, right.length() - 2);
    }
    
    // Find column index
    int col_index = -1;
    for (size_t i = 0; i < columns.size(); i++) {
        if (columns[i].name == left) {
            col_index = i;
            break;
        }
    }
    
    if (col_index == -1 || col_index >= (int)row.values.size()) {
        return false;
    }
    
    const std::string& col_value = row.values[col_index];
    
    // Compare based on data type
    if (columns[col_index].type == DataType::INT) {
        int left_val = std::stoi(col_value);
        int right_val = std::stoi(right);
        
        if (op == "=") return left_val == right_val;
        if (op == "!=") return left_val != right_val;
        if (op == "<") return left_val < right_val;
        if (op == ">") return left_val > right_val;
        if (op == "<=") return left_val <= right_val;
        if (op == ">=") return left_val >= right_val;
    } else {
        // String comparison
        if (op == "=") return col_value == right;
        if (op == "!=") return col_value != right;
        if (op == "<") return col_value < right;
        if (op == ">") return col_value > right;
        if (op == "<=") return col_value <= right;
        if (op == ">=") return col_value >= right;
    }
    
    return false;
}

void DatabaseManager::update_index(const std::string& table_name, const std::string& column,
                                   const std::string& old_value, const std::string& new_value, int row_id) {
    Table& table = databases[current_database];
    
    if (table.indexes.find(column) != table.indexes.end()) {
        if (!old_value.empty()) {
            table.indexes[column]->remove(old_value);
        }
        table.indexes[column]->insert(new_value, row_id);
    }
}

bool DatabaseManager::validate_foreign_key(const std::string& table_name, const std::string& column_name, 
                                           const std::string& value, const std::string& operation) {
    // Implementation for foreign key validation
    return true; // Simplified for now
}

void DatabaseManager::enforce_constraints(const std::string& table_name, int row_id, const Row& row) {
    // Implementation for constraint enforcement
}