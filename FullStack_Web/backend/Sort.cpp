#include "Sort.h"
#include <utility>
using namespace std;
void bubbleSortByNumber(vector<Vehicle>& v) {
    for (int i = 0; i < (int)v.size() - 1; ++i)
        for (int j = 0; j < (int)v.size() - i - 1; ++j)
            if (v[j].vehicleNumber > v[j + 1].vehicleNumber) swap(v[j], v[j + 1]);
}
void bubbleSortByFee(vector<Vehicle>& v) {
    for (int i = 0; i < (int)v.size() - 1; ++i)
        for (int j = 0; j < (int)v.size() - i - 1; ++j)
            if (v[j].parkingFee > v[j + 1].parkingFee) swap(v[j], v[j + 1]);
}
void selectionSortByNumber(vector<Vehicle>& v) {
    for (int i = 0; i < (int)v.size() - 1; ++i) {
        int smallest = i;
        for (int j = i + 1; j < (int)v.size(); ++j)
            if (v[j].vehicleNumber < v[smallest].vehicleNumber) smallest = j;
        if (smallest != i) swap(v[i], v[smallest]);
    }
}
void selectionSortByFee(vector<Vehicle>& v) {
    for (int i = 0; i < (int)v.size() - 1; ++i) {
        int smallest = i;
        for (int j = i + 1; j < (int)v.size(); ++j)
            if (v[j].parkingFee < v[smallest].parkingFee) smallest = j;
        if (smallest != i) swap(v[i], v[smallest]);
    }
}
