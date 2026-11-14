#ifndef VEHICLE_STACK_H
#define VEHICLE_STACK_H
#include "Vehicle.h"
#include <vector>
class VehicleStack {
private:
    struct Node {
        Vehicle data;
        Node* next;
        explicit Node(const Vehicle& v) : data(v), next(nullptr) {}
    };
    Node* topNode;
    int count;
public:
    VehicleStack();
    ~VehicleStack();
    VehicleStack(const VehicleStack&) = delete;
    VehicleStack& operator=(const VehicleStack&) = delete;
    void push(const Vehicle& vehicle);
    Vehicle pop();
    Vehicle* peek();
    bool isEmpty() const;
    int size() const;
    void display() const;
    std::vector<Vehicle> toVector() const; // Newest to oldest.
    void clear();
};
#endif
