#include <iostream>
#include "BPlusTree.h"

int main() {
    std::cout << "Testing BPlusTree compilation...\n";
    
    BPlusTree<int, std::string> tree(4);
    tree.insert(1, "value1");
    tree.insert(2, "value2");
    
    auto results = tree.search(1);
    for (const auto& val : results) {
        std::cout << "Found: " << val << std::endl;
    }
    
    std::cout << "Compilation successful!\n";
    return 0;
}