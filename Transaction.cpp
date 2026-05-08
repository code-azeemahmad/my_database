// Transaction.cpp
#include "Transaction.h"
#include <iostream>

TransactionManager* TransactionManager::instance = nullptr;

TransactionManager* TransactionManager::getInstance() {
    if (!instance) {
        instance = new TransactionManager();
    }
    return instance;
}

void TransactionManager::begin() {
    if (!in_transaction) {
        in_transaction = true;
        transaction_log.clear();
        transaction_backup.clear();
        std::cout << "Transaction started.\n";
    } else {
        std::cout << "Already in a transaction. Commit or rollback first.\n";
    }
}

void TransactionManager::commit() {
    if (in_transaction) {
        in_transaction = false;
        transaction_log.clear();
        transaction_backup.clear();
        std::cout << "Transaction committed successfully.\n";
    } else {
        std::cout << "No active transaction to commit.\n";
    }
}

void TransactionManager::rollback() {
    if (in_transaction) {
        restore_state();
        in_transaction = false;
        transaction_log.clear();
        transaction_backup.clear();
        std::cout << "Transaction rolled back.\n";
    } else {
        std::cout << "No active transaction to rollback.\n";
    }
}

bool TransactionManager::is_transaction_active() {
    return in_transaction;
}

void TransactionManager::log_operation(const std::string& operation, std::function<void()> undo) {
    if (in_transaction) {
        transaction_log.push_back({operation, undo});
    }
}

void TransactionManager::save_state(const std::string& table_name, int row_id, const Row& row) {
    if (in_transaction && transaction_backup[table_name].find(row_id) == transaction_backup[table_name].end()) {
        transaction_backup[table_name][row_id] = row;
    }
}

void TransactionManager::restore_state() {
    for (auto& pair : transaction_backup) {
        const std::string& table_name = pair.first;
        auto& backup_rows = pair.second;
        
        if (databases.find(table_name) != databases.end()) {
            for (auto& row_pair : backup_rows) {
                int row_id = row_pair.first;
                const Row& row = row_pair.second;
                databases[table_name].rows[row_id] = row;
            }
        }
    }
}