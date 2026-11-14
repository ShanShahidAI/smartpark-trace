#ifndef DYNAMIC_VEHICLE_ARRAY_H
#define DYNAMIC_VEHICLE_ARRAY_H
#include "Vehicle.h"
class DynamicVehicleArray {
private:
    Vehicle* items;
    int length;
    int capacity;
    void grow();
public:
    DynamicVehicleArray();
    ~DynamicVehicleArray();
    DynamicVehicleArray(const DynamicVehicleArray&) = delete;
    DynamicVehicleArray& operator=(const DynamicVehicleArray&) = delete;
    void add(const Vehicle& vehicle);
    bool insertAt(int index, const Vehicle& vehicle);
    bool removeAt(int index);
    int searchNumber(const std::string& number) const;
    const Vehicle& at(int index) const;
    int size() const;
    int getCapacity() const;
};
#endif
