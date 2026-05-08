// db.h - Add these missing method declarations
#ifndef DB_H
#define DB_H

#include "global.h"
#include "BPlusTree.h"
#include "Transaction.h"

class DatabaseManager {
private:
    TransactionManager* txn_manager;
    
    bool validate_foreign_key(const std::string& table_name, const std::string& column_name, 
                               const std::string& value, const std::string& operation = "INSERT");
    bool validate_data_type(const Column& col, const std::string& value);
    void enforce_constraints(const std::string& table_name, int row_id, const Row& row);
    void update_index(const std::string& table_name, const std::string& column, 
                      const std::string& old_value, const std::string& new_value, int row_id);
    
public:
    DatabaseManager();
    
    void create_database(const std::string& db_name);
    void use_database(const std::string& db_name);
    void drop_database(const std::string& db_name);
    
    void create_table(const std::string& table_name, const std::vector<Column>& columns);
    void drop_table(const std::string& table_name);
    void create_index(const std::string& table_name, const std::string& column);
    
    void insert(const std::string& table_name, const std::vector<std::string>& values);
    void select(const std::string& what, const std::string& from, 
                const std::string& where_condition = "");
    void update(const std::string& table_name, const std::string& set_clause, 
                const std::string& where_condition);
    void delete_from(const std::string& table_name, const std::string& where_condition);
    
    void show_tables();
    void describe_table(const std::string& table_name);
    
    std::vector<std::map<std::string, std::string>> parse_condition(const std::string& condition);
    bool evaluate_condition(const Row& row, const std::vector<Column>& columns, 
                           const std::string& condition);
};

#endif