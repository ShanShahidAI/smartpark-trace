#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H
#include "Vehicle.h"
#include <vector>
using namespace std;

class PriorityQueue {
private:
    vector<Vehicle> items;
    int priorityValue(const Vehicle& v) const;
public:
    void insert(const Vehicle& v);
    Vehicle removeHighestPriority();
    Vehicle* peek();
    bool isEmpty() const;
    int size() const;
    void display() const;
};
#endif
