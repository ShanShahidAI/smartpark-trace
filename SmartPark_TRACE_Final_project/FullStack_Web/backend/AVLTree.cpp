#include "AVLTree.h"
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <ostream>
using namespace std;

AVLTree::AVLTree() : root(nullptr) {}
AVLTree::~AVLTree() { clear(); }
AVLNode* AVLTree::CreateNode(int data, const string& plate) {
    AVLNode* node = new AVLNode;
    node->data = data;
    node->vehicleNumber = plate;
    node->height = 1;
    node->left = nullptr;
    node->right = nullptr;
    return node;
}
int AVLTree::Height(const AVLNode* node) { return node ? node->height : 0; }
int AVLTree::BalanceFactor(const AVLNode* node) {
    return node ? Height(node->left) - Height(node->right) : 0;
}
void AVLTree::UpdateHeight(AVLNode* node) {
    if (node) node->height = 1 + max(Height(node->left), Height(node->right));
}
AVLNode* AVLTree::RightRotate(AVLNode* y) {
    AVLNode* x = y->left;
    AVLNode* t2 = x->right;
    x->right = y;
    y->left = t2;
    UpdateHeight(y);
    UpdateHeight(x);
    return x;
}
AVLNode* AVLTree::LeftRotate(AVLNode* x) {
    AVLNode* y = x->right;
    AVLNode* t2 = y->left;
    y->left = x;
    x->right = t2;
    UpdateHeight(x);
    UpdateHeight(y);
    return y;
}
AVLNode* AVLTree::Rebalance(AVLNode* node) {
    if (!node) return nullptr;
    UpdateHeight(node);
    int bf = BalanceFactor(node);
    if (bf > 1) {
        if (BalanceFactor(node->left) >= 0) {
            rotationLog.push_back("LL case at " + to_string(node->data) + ": right rotation");
            return RightRotate(node);
        }
        rotationLog.push_back("LR case at " + to_string(node->data) + ": left, then right rotation");
        node->left = LeftRotate(node->left);
        return RightRotate(node);
    }
    if (bf < -1) {
        if (BalanceFactor(node->right) <= 0) {
            rotationLog.push_back("RR case at " + to_string(node->data) + ": left rotation");
            return LeftRotate(node);
        }
        rotationLog.push_back("RL case at " + to_string(node->data) + ": right, then left rotation");
        node->right = RightRotate(node->right);
        return LeftRotate(node);
    }
    return node;
}
AVLNode* AVLTree::Insert(AVLNode* node, int data, const string& plate, bool& inserted) {
    if (node == nullptr) { inserted = true; return CreateNode(data, plate); }
    if (data < node->data) node->left = Insert(node->left, data, plate, inserted);
    else if (data > node->data) node->right = Insert(node->right, data, plate, inserted);
    else return node; // Duplicate ignored.
    return Rebalance(node);
}
const AVLNode* AVLTree::FindMin(const AVLNode* node) {
    if (!node) return nullptr;
    while (node->left) node = node->left;
    return node;
}
AVLNode* AVLTree::Delete(AVLNode* node, int data, bool& deleted) {
    if (node == nullptr) return nullptr;
    if (data < node->data) node->left = Delete(node->left, data, deleted);
    else if (data > node->data) node->right = Delete(node->right, data, deleted);
    else {
        deleted = true;
        // Cases 1 & 2: leaf / single child.
        if (node->left == nullptr || node->right == nullptr) {
            AVLNode* child = node->left ? node->left : node->right;
            delete node;
            return child;
        }
        // Case 3: two children: replace with inorder successor.
        const AVLNode* next = FindMin(node->right);
        node->data = next->data;
        node->vehicleNumber = next->vehicleNumber;
        bool deletedNext = false;
        node->right = Delete(node->right, next->data, deletedNext);
    }
    return Rebalance(node); // Deletion can also require AVL rotations.
}
const AVLNode* AVLTree::Search(const AVLNode* node, int data) {
    if (node == nullptr || node->data == data) return node;
    return data < node->data ? Search(node->left, data) : Search(node->right, data);
}
void AVLTree::Inorder(const AVLNode* node, vector<int>& out) {
    if (!node) return;
    Inorder(node->left, out); out.push_back(node->data); Inorder(node->right, out);
}
void AVLTree::Preorder(const AVLNode* node, vector<int>& out) {
    if (!node) return;
    out.push_back(node->data); Preorder(node->left, out); Preorder(node->right, out);
}
void AVLTree::Postorder(const AVLNode* node, vector<int>& out) {
    if (!node) return;
    Postorder(node->left, out); Postorder(node->right, out); out.push_back(node->data);
}
int AVLTree::CountNodes(const AVLNode* node) {
    return node ? 1 + CountNodes(node->left) + CountNodes(node->right) : 0;
}
int AVLTree::CountLeafNodes(const AVLNode* node) {
    if (!node) return 0;
    if (!node->left && !node->right) return 1;
    return CountLeafNodes(node->left) + CountLeafNodes(node->right);
}
void AVLTree::Clear(AVLNode* node) {
    if (!node) return;
    Clear(node->left); Clear(node->right); delete node;
}
void AVLTree::Print(const AVLNode* node, ostream& out, int depth) {
    if (!node) return;
    Print(node->right, out, depth + 1);
    out << string(depth * 5, ' ') << node->data << " (h=" << node->height
        << ", BF=" << BalanceFactor(node) << ')';
    if (!node->vehicleNumber.empty()) out << " [" << node->vehicleNumber << "]";
    out << '\n';
    Print(node->left, out, depth + 1);
}
void AVLTree::Snapshot(const AVLNode* node, vector<TreeNodeInfo>& out) {
    if (!node) return;
    out.push_back({node->data, node->vehicleNumber, node->height, BalanceFactor(node),
        node->left != nullptr, node->left ? node->left->data : 0,
        node->right != nullptr, node->right ? node->right->data : 0});
    Snapshot(node->left, out); Snapshot(node->right, out);
}
bool AVLTree::Verify(const AVLNode* node, long long low, long long high, int& actualHeight) {
    if (!node) { actualHeight = 0; return true; }
    if (node->data <= low || node->data >= high) return false;
    int leftHeight = 0, rightHeight = 0;
    if (!Verify(node->left, low, node->data, leftHeight) ||
        !Verify(node->right, node->data, high, rightHeight)) return false;
    actualHeight = 1 + max(leftHeight, rightHeight);
    return node->height == actualHeight && abs(leftHeight - rightHeight) <= 1;
}
bool AVLTree::insert(int data, const string& plate) {
    rotationLog.clear(); bool added = false;
    root = Insert(root, data, plate, added);
    return added;
}
bool AVLTree::remove(int data) {
    rotationLog.clear(); bool removed = false;
    root = Delete(root, data, removed);
    return removed;
}
bool AVLTree::search(int data) const { return Search(root, data) != nullptr; }
TreeSearchTrace AVLTree::traceSearch(int key) const {
    TreeSearchTrace trace;
    const AVLNode* node = root;
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

optional<string> AVLTree::vehicleFor(int data) const {
    const AVLNode* node = Search(root, data);
    return node ? optional<string>(node->vehicleNumber) : nullopt;
}
vector<int> AVLTree::inorder() const { vector<int> out; Inorder(root, out); return out; }
vector<int> AVLTree::preorder() const { vector<int> out; Preorder(root, out); return out; }
vector<int> AVLTree::postorder() const { vector<int> out; Postorder(root, out); return out; }
optional<int> AVLTree::findMin() const {
    const AVLNode* node = FindMin(root);
    return node ? optional<int>(node->data) : nullopt;
}
optional<int> AVLTree::findMax() const {
    const AVLNode* node = root;
    while (node && node->right) node = node->right;
    return node ? optional<int>(node->data) : nullopt;
}
int AVLTree::height() const { return Height(root); }
int AVLTree::countNodes() const { return CountNodes(root); }
int AVLTree::countLeafNodes() const { return CountLeafNodes(root); }
vector<TreeNodeInfo> AVLTree::snapshot() const { vector<TreeNodeInfo> out; Snapshot(root, out); return out; }
const vector<string>& AVLTree::lastRotations() const { return rotationLog; }
void AVLTree::display(ostream& out) const {
    if (!root) { out << "(empty tree)\n"; return; }
    out << "Tree printed sideways (right above, left below):\n";
    Print(root, out, 0);
}
bool AVLTree::verify() const {
    int actualHeight = 0;
    return Verify(root, numeric_limits<long long>::min(), numeric_limits<long long>::max(), actualHeight);
}
void AVLTree::clear() { Clear(root); root = nullptr; rotationLog.clear(); }
