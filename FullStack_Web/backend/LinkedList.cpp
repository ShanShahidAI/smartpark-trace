#include "LinkedList.h"
#include <iostream>
using namespace std;
LinkedList::LinkedList() : head(nullptr), count(0) {}
LinkedList::~LinkedList() { clear(); }
void LinkedList::insert(const Vehicle& vehicle) {
    Node* n = new Node(vehicle);
    if (!head) head = n;
    else { Node* cur = head; while (cur->next) cur = cur->next; cur->next = n; }
    ++count;
}
bool LinkedList::remove(string number) {
    if (!head) return false;
    if (head->data.vehicleNumber == number) {
        Node* old = head; head = head->next; delete old; --count; return true;
    }
    Node* cur = head;
    while (cur->next) {
        if (cur->next->data.vehicleNumber == number) {
            Node* old = cur->next; cur->next = old->next;
            delete old; --count; return true;
        }
        cur = cur->next;
    }
    return false;
}
Vehicle* LinkedList::search(string number) {
    for (Node* cur = head; cur; cur = cur->next)
        if (cur->data.vehicleNumber == number) return &cur->data;
    return nullptr;
}
int LinkedList::size() const { return count; }
bool LinkedList::isEmpty() const { return head == nullptr; }
void LinkedList::display() const {
    cout << "HEAD -> ";
    for (Node* cur = head; cur; cur = cur->next) {
        cout << "[" << cur->data.vehicleNumber << "]";
        if (cur->next) cout << " -> ";
    }
    cout << " -> NULL\n";
}
vector<Vehicle> LinkedList::toVector() const {
    vector<Vehicle> records;
    for (Node* cur = head; cur; cur = cur->next) records.push_back(cur->data);
    return records;
}
void LinkedList::clear() {
    while (head) { Node* old = head; head = head->next; delete old; }
    count = 0;
}
