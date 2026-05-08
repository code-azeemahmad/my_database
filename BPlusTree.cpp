// BPlusTree.cpp
#ifndef BPLUSTREE_CPP
#define BPLUSTREE_CPP

#include "BPlusTree.h"

template<typename K, typename V>
BPlusTree<K,V>::BPlusTree(int deg) : degree(deg) {
    root = new Node(true);
}

template<typename K, typename V>
BPlusTree<K,V>::~BPlusTree() {
    std::queue<Node*> q;
    if (root) q.push(root);
    while (!q.empty()) {
        Node* node = q.front();
        q.pop();
        if (!node->is_leaf) {
            for (auto child : node->children) {
                if (child) q.push(child);
            }
        }
        delete node;
    }
}

template<typename K, typename V>
void BPlusTree<K,V>::insert(const K& key, const V& value) {
    Node* leaf = find_leaf(key);
    
    // Insert into leaf
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    int pos = it - leaf->keys.begin();
    leaf->keys.insert(it, key);
    leaf->values.insert(leaf->values.begin() + pos, value);
    
    // Check if leaf needs splitting
    if (leaf->keys.size() >= (size_t)degree) {
        // Create new leaf node
        Node* new_leaf = new Node(true);
        int mid = degree / 2;
        
        // Move second half to new leaf
        new_leaf->keys.assign(leaf->keys.begin() + mid, leaf->keys.end());
        new_leaf->values.assign(leaf->values.begin() + mid, leaf->values.end());
        leaf->keys.erase(leaf->keys.begin() + mid, leaf->keys.end());
        leaf->values.erase(leaf->values.begin() + mid, leaf->values.end());
        
        // Link leaves
        new_leaf->next = leaf->next;
        leaf->next = new_leaf;
        
        // Insert into parent
        K middle_key = new_leaf->keys[0];
        // This would need parent handling - simplified for now
    }
}

template<typename K, typename V>
typename BPlusTree<K,V>::Node* BPlusTree<K,V>::find_leaf(const K& key) {
    Node* current = root;
    while (current && !current->is_leaf) {
        int i = 0;
        while (i < (int)current->keys.size() && key >= current->keys[i]) {
            i++;
        }
        if (i < (int)current->children.size()) {
            current = current->children[i];
        } else {
            break;
        }
    }
    return current;
}

template<typename K, typename V>
std::vector<V> BPlusTree<K,V>::search(const K& key) {
    std::vector<V> results;
    Node* leaf = find_leaf(key);
    
    if (leaf) {
        for (size_t i = 0; i < leaf->keys.size(); i++) {
            if (leaf->keys[i] == key) {
                results.push_back(leaf->values[i]);
            }
        }
    }
    return results;
}

template<typename K, typename V>
std::vector<V> BPlusTree<K,V>::range_search(const K& start, const K& end) {
    std::vector<V> results;
    Node* current = find_leaf(start);
    
    while (current) {
        for (size_t i = 0; i < current->keys.size(); i++) {
            if (current->keys[i] >= start && current->keys[i] <= end) {
                results.push_back(current->values[i]);
            }
        }
        current = current->next;
    }
    return results;
}

template<typename K, typename V>
void BPlusTree<K,V>::remove(const K& key) {
    Node* leaf = find_leaf(key);
    if (leaf) {
        for (size_t i = 0; i < leaf->keys.size(); i++) {
            if (leaf->keys[i] == key) {
                leaf->keys.erase(leaf->keys.begin() + i);
                leaf->values.erase(leaf->values.begin() + i);
                break;
            }
        }
    }
}

template<typename K, typename V>
void BPlusTree<K,V>::print() {
    if (root) {
        print_node(root, 0);
    }
}

template<typename K, typename V>
void BPlusTree<K,V>::print_node(Node* node, int level) {
    if (!node) return;
    
    std::cout << std::string(level * 2, ' ');
    std::cout << "Node (leaf=" << node->is_leaf << "): ";
    for (auto key : node->keys) {
        std::cout << key << " ";
    }
    std::cout << std::endl;
    
    if (!node->is_leaf) {
        for (auto child : node->children) {
            print_node(child, level + 1);
        }
    }
}

#endif