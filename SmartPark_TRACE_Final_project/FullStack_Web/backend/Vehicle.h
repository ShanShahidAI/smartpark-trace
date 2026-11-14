#ifndef VEHICLE_H
#define VEHICLE_H
#include <string>
using namespace std;

// One vehicle record is used by the parking lot and all DSA structures.
class Vehicle {
public:
    string ticketId, vehicleNumber, vehicleType, ownerName, parkingSlot;
    string entryTime, exitTime;
    double parkingFee;
    bool priority;
    int hours;
    long long entryEpoch = 0;     // Epoch seconds recorded at actual parking start.
    long long exitEpoch = 0;      // Epoch seconds recorded when vehicle exits.
    long long durationSeconds = 0; // Actual parked seconds, before rounding.
    string preferredBuilding; // A/B/C or empty = any building
    int preferredFloor = 0;   // 0 = any floor

    Vehicle();
    Vehicle(string number, string type, string owner, bool isPriority = false);
    void displayVehicle() const;
};
int requiredVehicleSize(const string& type); // Small=1 Medium=2 Large=3
string sizeName(int size);
#endif
