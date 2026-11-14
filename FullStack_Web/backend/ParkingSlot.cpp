#include "ParkingSlot.h"
#include <iostream>
#include <iomanip>
using namespace std;

ParkingSlot::ParkingSlot() : slotNumber(""), occupied(false) {}
ParkingSlot::ParkingSlot(string number) : slotNumber(number), occupied(false) {}
ParkingSlot::ParkingSlot(string number, string buildingCode, int floorNo, int slotCapacity)
    : slotNumber(number), building(buildingCode), floor(floorNo),
      capacity(slotCapacity), occupied(false) {}

bool ParkingSlot::canFit(const Vehicle& candidate) const {
    return !occupied && restriction != 2 &&
           (restriction != 1 || candidate.priority || candidate.vehicleType == "Emergency") &&
           capacity >= requiredVehicleSize(candidate.vehicleType) &&
           (candidate.preferredBuilding.empty() || candidate.preferredBuilding == building) &&
           (candidate.preferredFloor == 0 || candidate.preferredFloor == floor);
}

bool ParkingSlot::parkVehicle(const Vehicle& newVehicle) {
    if (!canFit(newVehicle)) return false;
    vehicle = newVehicle;
    vehicle.parkingSlot = slotNumber;
    occupied = true;
    return true;
}

Vehicle ParkingSlot::removeVehicle() {
    Vehicle removed = vehicle;
    occupied = false;
    vehicle = Vehicle();
    return removed;
}

void ParkingSlot::displaySlot() const {
    cout << left << setw(14) << slotNumber << " [" << setw(6) << sizeName(capacity)
         << "] " << (occupied ? "Occupied by " + vehicle.vehicleNumber :
                        restriction == 1 ? "Reserved (Priority / Emergency)" :
                        restriction == 2 ? "Disputed / Closed" : "Available")
         << (restrictionNote.empty() ? "" : " | " + restrictionNote) << "\n";
}
