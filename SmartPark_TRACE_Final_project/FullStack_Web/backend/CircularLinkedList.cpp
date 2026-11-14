#include "CircularLinkedList.h"
#include <iostream>
using namespace std;
CircularLinkedList::CircularLinkedList() : head(nullptr), tail(nullptr), cursor(nullptr), count(0) {}
CircularLinkedList::~CircularLinkedList() { clear(); }
void CircularLinkedList::append(const string& id) {
    Node* n = new Node(id);
    if (!head) { head = tail = n; n->next = head; cursor = head; }
    else { tail->next = n; tail = n; tail->next = head; }
    ++count;
}
string CircularLinkedList::inspectNext() {
    if (!cursor) return "";
    string id = cursor->slotId;
    cursor = cursor->next;
    return id;
}
vector<string> CircularLinkedList::toVector() const {
    vector<string> rows;
    if (!head) return rows;
    Node* cur = head;
    do { rows.push_back(cur->slotId); cur = cur->next; } while (cur != head);
    return rows;
}
void CircularLinkedList::display() const {
    cout << "HEAD -> ";
    if (head) {
        Node* cur = head;
        do {
            cout << "[" << cur->slotId << "] -> ";
            cur = cur->next;
        } while (cur != head);
        cout << "HEAD (circular)";
    } else cout << "EMPTY";
    cout << "\n";
}
int CircularLinkedList::size() const { return count; }
void CircularLinkedList::clear() {
    if (head) {
        tail->next = nullptr; // Break cycle to free memory safely.
        while (head) { Node* old = head; head = head->next; delete old; }
    }
    tail = cursor = nullptr;
    count = 0;
}
