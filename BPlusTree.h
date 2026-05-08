// BPlusTree.h
#ifndef BPLUSTREE_H
#define BPLUSTREE_H

#include <vector>
#include <iostream>
#include <algorithm>
#include <queue>
#include <memory>

template<typename K, typename V>
class BPlusTree {
private:
    struct Node {
        bool is_leaf;
        std::vector<K> keys;
        std::vector<V> values;
        std::vector<Node*> children;
        Node* next;
        
        Node(bool leaf) : is_leaf(leaf), next(nullptr) {}
    };
    
    Node* root;
    int degree;
    
    void split_child(Node* parent, int index);
    void insert_non_full(Node* node, const K& key, const V& value);
    Node* find_leaf(const K& key);
    void print_node(Node* node, int level);
    
public:
    BPlusTree(int deg = 4);
    ~BPlusTree();
    
    void insert(const K& key, const V& value);
    std::vector<V> search(const K& key);
    std::vector<V> range_search(const K& start, const K& end);
    void remove(const K& key);
    void print();
};

// Include the implementation
#include "BPlusTree.cpp"

#endif