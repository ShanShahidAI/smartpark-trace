#include "Search.h"
int linearSearchByNumber(const vector<Vehicle>& v, const string& number) {
    for (int i = 0; i < (int)v.size(); ++i)
        if (v[i].vehicleNumber == number) return i;
    return -1;
}
int linearSearchByOwner(const vector<Vehicle>& v, const string& owner) {
    for (int i = 0; i < (int)v.size(); ++i)
        if (v[i].ownerName == owner) return i;
    return -1;
}
int linearSearchBySlot(const vector<Vehicle>& v, const string& slot) {
    for (int i = 0; i < (int)v.size(); ++i)
        if (v[i].parkingSlot == slot) return i;
    return -1;
}
int binarySearchByNumber(const vector<Vehicle>& sorted, const string& number) {
    int left = 0, right = (int)sorted.size() - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (sorted[mid].vehicleNumber == number) return mid;
        if (sorted[mid].vehicleNumber < number) left = mid + 1;
        else right = mid - 1;
    }
    return -1;
}
