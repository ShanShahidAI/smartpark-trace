#ifndef SMARTPARK_BST_H
#define SMARTPARK_BST_H

#include <optional>
#include <string>
#include <vector>
#include <iosfwd>

// A snapshot lets Phase 4 draw the exact live tree, not a separate fake tree.
struct TreeNodeInfo {
    int data;
    std::string vehicleNumber;
    int height;
    int balanceFactor;
    bool hasLeft;
    int left;
    bool hasRight;
    int right;
};

// Straightforward pointer-based BST: the same recursive approach used in class.
// Actual search path follows pointer links, not a generated/simulated tree.
struct TreeSearchTrace {
    bool found = false;
    std::string vehicleNumber;
    std::vector<int> visited; // one key per node comparison
};

struct BSTNode {
    int data;
    std::string vehicleNumber;
    BSTNode* left;
    BSTNode* right;
};

class BSTree {
private:
    BSTNode* root;
    static BSTNode* CreateNode(int data, const std::string& plate);
    static BSTNode* Insert(BSTNode* root, int data, const std::string& plate, bool& inserted);
    static BSTNode* Delete(BSTNode* root, int data, bool& deleted);
    static BSTNode* FindMin(BSTNode* root);
    static const BSTNode* Search(const BSTNode* root, int data);
    static void Inorder(const BSTNode* root, std::vector<int>& output);
    static void Preorder(const BSTNode* root, std::vector<int>& output);
    static void Postorder(const BSTNode* root, std::vector<int>& output);
    static int Height(const BSTNode* root);
    static int CountNodes(const BSTNode* root);
    static int CountLeafNodes(const BSTNode* root);
    static void Clear(BSTNode* root);
    static void Print(const BSTNode* root, std::ostream& out, int depth);
    static void Snapshot(const BSTNode* root, std::vector<TreeNodeInfo>& result);
    static bool Verify(const BSTNode* root, long long minExclusive, long long maxExclusive);
public:
    BSTree();
    ~BSTree();
    BSTree(const BSTree&) = delete;
    BSTree& operator=(const BSTree&) = delete;
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
    void display(std::ostream& out) const;
    bool verify() const;
    void clear();
};
#endif
