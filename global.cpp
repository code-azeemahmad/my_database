// global.cpp
#include "global.h"

// Define global variables
std::string current_database = "";
std::map<std::string, Table> databases;
bool in_transaction = false;
std::vector<std::pair<std::string, std::function<void()>>> transaction_log;
std::map<std::string, std::map<int, Row>> transaction_backup;