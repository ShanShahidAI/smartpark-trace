#include "DoublyLinkedList.h"
#include <iostream>
using namespace std;
DoublyLinkedList::DoublyLinkedList() : head(nullptr), tail(nullptr), count(0) {}
DoublyLinkedList::~DoublyLinkedList() { clear(); }
void DoublyLinkedList::pushFront(const Vehicle& v) {
    Node* n = new Node(v);
    n->next = head;
    if (head) head->prev = n;
    else tail = n;
    head = n;
    ++count;
}
vector<Vehicle> DoublyLinkedList::forward() const {
    vector<Vehicle> rows;
    for (Node* cur = head; cur; cur = cur->next) rows.push_back(cur->data);
    return rows;
}
vector<Vehicle> DoublyLinkedList::backward() const {
    vector<Vehicle> rows;
    for (Node* cur = tail; cur; cur = cur->prev) rows.push_back(cur->data);
    return rows;
}
void DoublyLinkedList::displayForward() const {
    cout << "NULL <-> ";
    for (Node* cur = head; cur; cur = cur->next) {
        cout << "[" << cur->data.vehicleNumber << "]";
        if (cur->next) cout << " <-> ";
    }
    cout << " <-> NULL (newest to oldest)\n";
}
void DoublyLinkedList::displayBackward() const {
    cout << "NULL <-> ";
    for (Node* cur = tail; cur; cur = cur->prev) {
        cout << "[" << cur->data.vehicleNumber << "]";
        if (cur->prev) cout << " <-> ";
    }
    cout << " <-> NULL (oldest to newest)\n";
}
int DoublyLinkedList::size() const { return count; }
void DoublyLinkedList::clear() {
    while (head) { Node* old = head; head = head->next; delete old; }
    tail = nullptr; count = 0;
}
