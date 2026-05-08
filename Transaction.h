#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "global.h"

class TransactionManager {
private:
    static TransactionManager* instance;
    TransactionManager() = default;
    
public:
    static TransactionManager* getInstance();
    
    void begin();
    void commit();
    void rollback();
    void log_operation(const std::string& operation, std::function<void()> undo);
    bool is_transaction_active();  // Add this line
    
    void save_state(const std::string& table_name, int row_id, const Row& row);
    void restore_state();
};

#endif