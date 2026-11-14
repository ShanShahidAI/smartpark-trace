#include "VehicleStack.h"
#include <iostream>
using namespace std;
VehicleStack::VehicleStack() : topNode(nullptr), count(0) {}
VehicleStack::~VehicleStack() { clear(); }
void VehicleStack::push(const Vehicle& vehicle) {
    Node* n = new Node(vehicle);
    n->next = topNode; topNode = n; ++count;
}
Vehicle VehicleStack::pop() {
    if (!topNode) return Vehicle();
    Node* old = topNode;
    Vehicle v = old->data;
    topNode = old->next;
    delete old; --count; return v;
}
Vehicle* VehicleStack::peek() { return topNode ? &topNode->data : nullptr; }
bool VehicleStack::isEmpty() const { return topNode == nullptr; }
int VehicleStack::size() const { return count; }
void VehicleStack::display() const {
    cout << "TOP (latest exit)\n";
    for (Node* cur = topNode; cur; cur = cur->next)
        cout << "  [" << cur->data.vehicleNumber << "] Rs." << cur->data.parkingFee << "\n";
    cout << "BOTTOM\n";
}
vector<Vehicle> VehicleStack::toVector() const {
    vector<Vehicle> rows;
    for (Node* cur = topNode; cur; cur = cur->next) rows.push_back(cur->data);
    return rows;
}
void VehicleStack::clear() { while (topNode) pop(); }
