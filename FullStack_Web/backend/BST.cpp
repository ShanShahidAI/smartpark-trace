#include "BST.h"
#include <ostream>
#include <algorithm>
#include <limits>
using namespace std;

BSTree::BSTree() : root(nullptr) {}
BSTree::~BSTree() { clear(); }

BSTNode* BSTree::CreateNode(int data, const string& plate) {
    BSTNode* node = new BSTNode;
    node->data = data;
    node->vehicleNumber = plate;
    node->left = nullptr;
    node->right = nullptr;
    return node;
}
BSTNode* BSTree::Insert(BSTNode* current, int data, const string& plate, bool& inserted) {
    if (current == nullptr) { inserted = true; return CreateNode(data, plate); }
    if (data < current->data) current->left = Insert(current->left, data, plate, inserted);
    else if (data > current->data) current->right = Insert(current->right, data, plate, inserted);
    // The professor's lab ignores duplicate keys.
    return current;
}
BSTNode* BSTree::FindMin(BSTNode* current) {
    if (current == nullptr) return nullptr;
    while (current->left != nullptr) current = current->left;
    return current;
}
BSTNode* BSTree::Delete(BSTNode* current, int data, bool& deleted) {
    if (current == nullptr) return nullptr;
    if (data < current->data) current->left = Delete(current->left, data, deleted);
    else if (data > current->data) current->right = Delete(current->right, data, deleted);
    else {
        deleted = true;
        // Case 1: leaf node. Case 2: only one child.
        if (current->left == nullptr) {
            BSTNode* next = current->right;
            delete current;
            return next;
        }
        if (current->right == nullptr) {
            BSTNode* next = current->left;
            delete current;
            return next;
        }
        // Case 3: two children; copy inorder successor, then remove successor.
        BSTNode* successor = FindMin(current->right);
        current->data = successor->data;
        current->vehicleNumber = successor->vehicleNumber;
        bool removedSuccessor = false;
        current->right = Delete(current->right, successor->data, removedSuccessor);
    }
    return current;
}
const BSTNode* BSTree::Search(const BSTNode* current, int data) {
    if (current == nullptr || current->data == data) return current;
    return data < current->data ? Search(current->left, data) : Search(current->right, data);
}
void BSTree::Inorder(const BSTNode* current, vector<int>& out) {
    if (current == nullptr) return;
    Inorder(current->left, out); out.push_back(current->data); Inorder(current->right, out);
}
void BSTree::Preorder(const BSTNode* current, vector<int>& out) {
    if (current == nullptr) return;
    out.push_back(current->data); Preorder(current->left, out); Preorder(current->right, out);
}
void BSTree::Postorder(const BSTNode* current, vector<int>& out) {
    if (current == nullptr) return;
    Postorder(current->left, out); Postorder(current->right, out); out.push_back(current->data);
}
int BSTree::Height(const BSTNode* current) {
    return current ? 1 + max(Height(current->left), Height(current->right)) : 0;
}
int BSTree::CountNodes(const BSTNode* current) {
    return current ? 1 + CountNodes(current->left) + CountNodes(current->right) : 0;
}
int BSTree::CountLeafNodes(const BSTNode* current) {
    if (current == nullptr) return 0;
    if (current->left == nullptr && current->right == nullptr) return 1;
    return CountLeafNodes(current->left) + CountLeafNodes(current->right);
}
void BSTree::Clear(BSTNode* current) {
    if (current == nullptr) return;
    Clear(current->left); Clear(current->right); delete current;
}
void BSTree::Print(const BSTNode* current, ostream& out, int depth) {
    if (current == nullptr) return;
    Print(current->right, out, depth + 1);
    out << string(depth * 5, ' ') << current->data;
    if (!current->vehicleNumber.empty()) out << " [" << current->vehicleNumber << "]";
    out << '\n';
    Print(current->left, out, depth + 1);
}
void BSTree::Snapshot(const BSTNode* current, vector<TreeNodeInfo>& out) {
    if (current == nullptr) return;
    int leftH = Height(current->left), rightH = Height(current->right);
    out.push_back({current->data, current->vehicleNumber, 1 + max(leftH,rightH), leftH-rightH,
        current->left != nullptr, current->left ? current->left->data : 0,
        current->right != nullptr, current->right ? current->right->data : 0});
    Snapshot(current->left, out); Snapshot(current->right, out);
}
bool BSTree::Verify(const BSTNode* current, long long minKey, long long maxKey) {
    if (!current) return true;
    return current->data > minKey && current->data < maxKey &&
        Verify(current->left, minKey, current->data) &&
        Verify(current->right, current->data, maxKey);
}
bool BSTree::insert(int data, const string& plate) {
    bool added = false; root = Insert(root, data, plate, added); return added;
}
bool BSTree::remove(int data) {
    bool removed = false; root = Delete(root, data, removed); return removed;
}
bool BSTree::search(int data) const { return Search(root, data) != nullptr; }
TreeSearchTrace BSTree::traceSearch(int key) const {
    TreeSearchTrace trace;
    const BSTNode* node = root;
    while (node) {
        trace.visited.push_back(node->data);
        if (node->data == key) {
            trace.found = true;
            trace.vehicleNumber = node->vehicleNumber;
            break;
        }
        node = key < node->data ? node->left : node->right;
    }
    return trace;
}

optional<string> BSTree::vehicleFor(int data) const {
    const BSTNode* node = Search(root, data);
    return node ? optional<string>(node->vehicleNumber) : nullopt;
}
vector<int> BSTree::inorder() const { vector<int> out; Inorder(root, out); return out; }
vector<int> BSTree::preorder() const { vector<int> out; Preorder(root, out); return out; }
vector<int> BSTree::postorder() const { vector<int> out; Postorder(root, out); return out; }
optional<int> BSTree::findMin() const { BSTNode* n = FindMin(root); return n ? optional<int>(n->data) : nullopt; }
optional<int> BSTree::findMax() const {
    const BSTNode* n = root;
    while (n && n->right) n = n->right;
    return n ? optional<int>(n->data) : nullopt;
}
int BSTree::height() const { return Height(root); }
int BSTree::countNodes() const { return CountNodes(root); }
int BSTree::countLeafNodes() const { return CountLeafNodes(root); }
vector<TreeNodeInfo> BSTree::snapshot() const { vector<TreeNodeInfo> out; Snapshot(root, out); return out; }
void BSTree::display(ostream& out) const {
    if (!root) { out << "(empty tree)\n"; return; }
    out << "Tree printed sideways (right child above, left below):\n";
    Print(root, out, 0);
}
bool BSTree::verify() const { return Verify(root, numeric_limits<long long>::min(), numeric_limits<long long>::max()); }
void BSTree::clear() { Clear(root); root = nullptr; }
