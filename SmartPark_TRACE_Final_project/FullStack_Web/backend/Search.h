#ifndef SEARCH_H
#define SEARCH_H
#include <vector>
#include "Vehicle.h"
using namespace std;
int linearSearchByNumber(const vector<Vehicle>& v, const string& number);
int linearSearchByOwner(const vector<Vehicle>& v, const string& owner);
int linearSearchBySlot(const vector<Vehicle>& v, const string& slot);
// Binary search needs a list sorted by vehicle number first.
int binarySearchByNumber(const vector<Vehicle>& sortedVehicles, const string& number);
#endif
