#ifndef DOUBLY_LINKED_LIST_H
#define DOUBLY_LINKED_LIST_H
#include "Vehicle.h"
#include <vector>
class DoublyLinkedList {
private:
    struct Node {
        Vehicle data;
        Node* prev;
        Node* next;
        explicit Node(const Vehicle& v) : data(v), prev(nullptr), next(nullptr) {}
    };
    Node* head;
    Node* tail;
    int count;
public:
    DoublyLinkedList();
    ~DoublyLinkedList();
    DoublyLinkedList(const DoublyLinkedList&) = delete;
    DoublyLinkedList& operator=(const DoublyLinkedList&) = delete;
    void pushFront(const Vehicle& vehicle);
    std::vector<Vehicle> forward() const;
    std::vector<Vehicle> backward() const;
    void displayForward() const;
    void displayBackward() const;
    int size() const;
    void clear();
};
#endif
