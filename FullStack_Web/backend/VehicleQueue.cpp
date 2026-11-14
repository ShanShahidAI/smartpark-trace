#include "VehicleQueue.h"
#include <iostream>
using namespace std;
namespace {
int priorityOf(const Vehicle& v) {
    if (v.vehicleType == "Emergency") return 3;
    return v.priority ? 2 : 1;
}
}
VehicleQueue::VehicleQueue() : frontNode(nullptr), rearNode(nullptr), count(0) {}
VehicleQueue::~VehicleQueue() { clear(); }
void VehicleQueue::enqueue(const Vehicle& vehicle) {
    Node* n = new Node(vehicle);
    if (!rearNode) frontNode = rearNode = n;
    else { rearNode->next = n; rearNode = n; }
    ++count;
}
Vehicle VehicleQueue::dequeue() {
    if (!frontNode) return Vehicle();
    Node* old = frontNode;
    Vehicle v = old->data;
    frontNode = old->next;
    if (!frontNode) rearNode = nullptr;
    delete old;
    --count;
    return v;
}
Vehicle VehicleQueue::dequeueHighestPriority() {
    if (!frontNode) return Vehicle();
    Node* best = frontNode;
    Node* beforeBest = nullptr;
    Node* previous = frontNode;
    // Strict > comparison retains the earliest arrival for equal priority.
    for (Node* cur = frontNode->next; cur; cur = cur->next) {
        if (priorityOf(cur->data) > priorityOf(best->data)) {
            best = cur;
            beforeBest = previous;
        }
        previous = cur;
    }
    if (!beforeBest) return dequeue();
    beforeBest->next = best->next;
    if (best == rearNode) rearNode = beforeBest;
    Vehicle v = best->data;
    delete best;
    --count;
    return v;
}
Vehicle VehicleQueue::dequeueEligible(int capacity, const string& building, int floor, int restriction) {
    Node* best = nullptr;
    Node* beforeBest = nullptr;
    Node* previous = nullptr;
    for (Node* current = frontNode; current; previous = current, current = current->next) {
        const Vehicle& v = current->data;
        bool eligible = restriction != 2 &&
            (restriction != 1 || v.priority || v.vehicleType == "Emergency") &&
            requiredVehicleSize(v.vehicleType) <= capacity &&
            (v.preferredBuilding.empty() || v.preferredBuilding == building) &&
            (v.preferredFloor == 0 || v.preferredFloor == floor);
        if (eligible && (!best || priorityOf(v) > priorityOf(best->data))) {
            best = current; beforeBest = previous;
        }
    }
    if (!best) return Vehicle();
    if (!beforeBest) return dequeue();
    beforeBest->next = best->next;
    if (best == rearNode) rearNode = beforeBest;
    Vehicle result = best->data;
    delete best; --count;
    return result;
}
bool VehicleQueue::removeNumber(const string& plate) {
    Node* previous = nullptr;
    for (Node* current = frontNode; current; previous = current, current = current->next) {
        if (current->data.vehicleNumber != plate) continue;
        if (previous) previous->next = current->next;
        else frontNode = current->next;
        if (rearNode == current) rearNode = previous;
        delete current;
        --count;
        return true;
    }
    return false;
}
Vehicle* VehicleQueue::findNumber(const string& plate) {
    for (Node* cur=frontNode; cur; cur=cur->next)
        if (cur->data.vehicleNumber == plate) return &cur->data;
    return nullptr;
}
Vehicle* VehicleQueue::front() { return frontNode ? &frontNode->data : nullptr; }
bool VehicleQueue::containsNumber(const string& number) const {
    for (Node* cur = frontNode; cur; cur = cur->next)
        if (cur->data.vehicleNumber == number) return true;
    return false;
}
bool VehicleQueue::isEmpty() const { return !frontNode; }
int VehicleQueue::size() const { return count; }
void VehicleQueue::display() const {
    cout << "FRONT -> ";
    for (Node* cur = frontNode; cur; cur = cur->next) {
        cout << "[" << cur->data.vehicleNumber << "]";
        if (cur->next) cout << " -> ";
    }
    cout << " <- REAR\n";
}
vector<Vehicle> VehicleQueue::toVector() const {
    vector<Vehicle> rows;
    for (Node* cur = frontNode; cur; cur = cur->next) rows.push_back(cur->data);
    return rows;
}
void VehicleQueue::clear() { while (frontNode) dequeue(); }
