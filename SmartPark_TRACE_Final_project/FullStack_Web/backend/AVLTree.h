#ifndef SMARTPARK_AVLTREE_H
#define SMARTPARK_AVLTREE_H

#include "BST.h" // Shared tree snapshot shape for the future web visualizer.
#include <optional>
#include <string>
#include <vector>
#include <iosfwd>

struct AVLNode {
    int data;
    std::string vehicleNumber;
    int height;
    AVLNode* left;
    AVLNode* right;
};

class AVLTree {
private:
    AVLNode* root;
    std::vector<std::string> rotationLog;
    static AVLNode* CreateNode(int data, const std::string& plate);
    static int Height(const AVLNode* node);
    static int BalanceFactor(const AVLNode* node);
    static void UpdateHeight(AVLNode* node);
    AVLNode* RightRotate(AVLNode* node);
    AVLNode* LeftRotate(AVLNode* node);
    AVLNode* Rebalance(AVLNode* node);
    AVLNode* Insert(AVLNode* node, int data, const std::string& plate, bool& inserted);
    AVLNode* Delete(AVLNode* node, int data, bool& deleted);
    static const AVLNode* Search(const AVLNode* node, int data);
    static const AVLNode* FindMin(const AVLNode* node);
    static void Inorder(const AVLNode* node, std::vector<int>& output);
    static void Preorder(const AVLNode* node, std::vector<int>& output);
    static void Postorder(const AVLNode* node, std::vector<int>& output);
    static int CountNodes(const AVLNode* node);
    static int CountLeafNodes(const AVLNode* node);
    static void Clear(AVLNode* node);
    static void Print(const AVLNode* node, std::ostream& out, int depth);
    static void Snapshot(const AVLNode* node, std::vector<TreeNodeInfo>& result);
    static bool Verify(const AVLNode* node, long long low, long long high, int& actualHeight);
public:
    AVLTree();
    ~AVLTree();
    AVLTree(const AVLTree&) = delete;
    AVLTree& operator=(const AVLTree&) = delete;
    bool insert(int data, const std::string& plate = "");
    bool remove(int data);
    bool search(int data) const;
    TreeSearchTrace traceSearch(int data) const;
    std::optional<std::string> vehicleFor(int data) const;
    std::vector<int> inorder() const;
    std::vector<int> preorder() const;
    std::vector<int> postorder() const;
    std::optional<int> findMin() const;
    std::optional<int> findMax() const;
    int height() const;
    int countNodes() const;
    int countLeafNodes() const;
    std::vector<TreeNodeInfo> snapshot() const;
    const std::vector<std::string>& lastRotations() const;
    void display(std::ostream& out) const;
    bool verify() const;
    void clear();
};
#endif
