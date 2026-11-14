#ifndef VEHICLE_QUEUE_H
#define VEHICLE_QUEUE_H
#include "Vehicle.h"
#include <vector>
class VehicleQueue {
private:
    struct Node {
        Vehicle data;
        Node* next;
        explicit Node(const Vehicle& v) : data(v), next(nullptr) {}
    };
    Node *frontNode, *rearNode;
    int count;
public:
    VehicleQueue();
    ~VehicleQueue();
    VehicleQueue(const VehicleQueue&) = delete;
    VehicleQueue& operator=(const VehicleQueue&) = delete;
    void enqueue(const Vehicle& vehicle);
    Vehicle dequeue(); // Standard FIFO example.
    Vehicle dequeueHighestPriority(); // Emergency > VIP > normal, FIFO among equals.
    Vehicle dequeueEligible(int capacity, const string& building, int floor, int restriction = 0);
    bool removeNumber(const string& plate);
    Vehicle* findNumber(const string& plate);
    Vehicle* front();
    bool containsNumber(const string& number) const;
    bool isEmpty() const;
    int size() const;
    void display() const;
    std::vector<Vehicle> toVector() const;
    void clear();
};
#endif
