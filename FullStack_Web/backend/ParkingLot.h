#ifndef PARKING_LOT_H
#define PARKING_LOT_H
#include <vector>
#include "ParkingSlot.h"
using namespace std;

class ParkingLot {
private:
    vector<ParkingSlot> slots;
    int floorsPerBuilding = 2;
    int slotsPerFloor = 6;
public:
    explicit ParkingLot(int numberOfSlots = 12);
    bool configure(int floors, int eachFloor);
    int getFloorsPerBuilding() const;
    int getSlotsPerFloor() const;
    const ParkingSlot* slotById(const string& id) const;
    const ParkingSlot* bestSlot(const Vehicle& vehicle) const;
    bool setSlotRestriction(const string& id, int mode, const string& note);
    void clearRestrictions();
    std::string explainSelection(const Vehicle& vehicle, const ParkingSlot* chosen) const;
    int suitableAvailable(const Vehicle& vehicle) const;
    int getTotalSlots() const;
    int getAvailableSlots() const;
    int getOccupiedSlots() const;
    bool parkVehicle(Vehicle& vehicle);
    bool parkAtSlot(const Vehicle& vehicle, const string& slotId);
    Vehicle removeVehicle(string vehicleNumber);
    Vehicle* findVehicle(string vehicleNumber);
    const Vehicle* findVehicle(string vehicleNumber) const;
    void displayParkingLot() const;
    const vector<ParkingSlot>& getSlots() const;
    void clear();
};
#endif
