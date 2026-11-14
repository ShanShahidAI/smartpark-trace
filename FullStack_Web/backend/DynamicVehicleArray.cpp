#include "DynamicVehicleArray.h"
#include <stdexcept>
DynamicVehicleArray::DynamicVehicleArray() : items(new Vehicle[2]), length(0), capacity(2) {}
DynamicVehicleArray::~DynamicVehicleArray() { delete[] items; }
void DynamicVehicleArray::grow() {
    int newCapacity = capacity * 2;
    Vehicle* bigger = new Vehicle[newCapacity];
    for (int i = 0; i < length; ++i) bigger[i] = items[i];
    delete[] items;
    items = bigger;
    capacity = newCapacity;
}
void DynamicVehicleArray::add(const Vehicle& v) {
    if (length == capacity) grow();
    items[length++] = v;
}
bool DynamicVehicleArray::insertAt(int index, const Vehicle& v) {
    if (index < 0 || index > length) return false;
    if (length == capacity) grow();
    for (int i = length; i > index; --i) items[i] = items[i - 1];
    items[index] = v;
    ++length;
    return true;
}
bool DynamicVehicleArray::removeAt(int index) {
    if (index < 0 || index >= length) return false;
    for (int i = index; i < length - 1; ++i) items[i] = items[i + 1];
    --length;
    return true;
}
int DynamicVehicleArray::searchNumber(const std::string& number) const {
    for (int i = 0; i < length; ++i)
        if (items[i].vehicleNumber == number) return i;
    return -1;
}
const Vehicle& DynamicVehicleArray::at(int index) const {
    if (index < 0 || index >= length) throw std::out_of_range("array index");
    return items[index];
}
int DynamicVehicleArray::size() const { return length; }
int DynamicVehicleArray::getCapacity() const { return capacity; }
