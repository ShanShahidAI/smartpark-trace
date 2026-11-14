#ifndef LINKED_LIST_H
#define LINKED_LIST_H
#include "Vehicle.h"
#include <vector>
class LinkedList {
private:
    struct Node {
        Vehicle data;
        Node* next;
        explicit Node(const Vehicle& v) : data(v), next(nullptr) {}
    };
    Node* head;
    int count;
public:
    LinkedList();
    ~LinkedList();
    LinkedList(const LinkedList&) = delete;
    LinkedList& operator=(const LinkedList&) = delete;
    void insert(const Vehicle& vehicle);
    bool remove(string vehicleNumber);
    Vehicle* search(string vehicleNumber);
    int size() const;
    bool isEmpty() const;
    void display() const;
    std::vector<Vehicle> toVector() const;
    void clear();
};
#endif
