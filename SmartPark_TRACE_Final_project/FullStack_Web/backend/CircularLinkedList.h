#ifndef CIRCULAR_LINKED_LIST_H
#define CIRCULAR_LINKED_LIST_H
#include <string>
#include <vector>
class CircularLinkedList {
private:
    struct Node {
        std::string slotId;
        Node* next;
        explicit Node(const std::string& id) : slotId(id), next(nullptr) {}
    };
    Node* head;
    Node* tail;
    Node* cursor;
    int count;
public:
    CircularLinkedList();
    ~CircularLinkedList();
    CircularLinkedList(const CircularLinkedList&) = delete;
    CircularLinkedList& operator=(const CircularLinkedList&) = delete;
    void append(const std::string& slotId);
    std::string inspectNext(); // Returns next slot in a repeating round-robin circle.
    std::vector<std::string> toVector() const;
    void display() const;
    int size() const;
    void clear();
};
#endif
