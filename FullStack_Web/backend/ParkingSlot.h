#ifndef PARKING_SLOT_H
#define PARKING_SLOT_H
#include "Vehicle.h"
using namespace std;

class ParkingSlot {
public:
    string slotNumber;
    string building;
    int floor = 0;
    int capacity = 2; // Small=1 / Medium=2 / Large=3
    bool occupied;
    // Phase 3: manual administrator restrictions; independent of occupancy.
    int restriction = 0;  // 0=Normal, 1=Priority-Reserved, 2=Disputed/Closed
    string restrictionNote;
    Vehicle vehicle;
    ParkingSlot();
    ParkingSlot(string number);
    ParkingSlot(string number, string buildingCode, int floorNo, int slotCapacity);
    bool canFit(const Vehicle& candidate) const;
    bool parkVehicle(const Vehicle& newVehicle);
    Vehicle removeVehicle();
    void displaySlot() const;
};
#endif
