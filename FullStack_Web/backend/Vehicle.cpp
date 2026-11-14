#include "Vehicle.h"
#include <iostream>
using namespace std;

int requiredVehicleSize(const string& type) {
    if (type == "Motorcycle") return 1;
    if (type == "SUV" || type == "Van" || type == "Pickup" ||
        type == "Bus" || type == "Truck") return 3;
    return 2; // Car, Sedan, Taxi, Sports Car, Emergency
}
string sizeName(int size) {
    return size <= 1 ? "Small" : (size == 2 ? "Medium" : "Large");
}
Vehicle::Vehicle() : parkingFee(0), priority(false), hours(0) {}
Vehicle::Vehicle(string number, string type, string owner, bool isPriority)
    : vehicleNumber(number), vehicleType(type), ownerName(owner),
      parkingFee(0), priority(isPriority), hours(0) {}

void Vehicle::displayVehicle() const {
    cout << "Ticket: " << ticketId << " | Number: " << vehicleNumber
         << " | Owner: " << ownerName << " | Type: " << vehicleType
         << " | Slot: " << (parkingSlot.empty() ? "WAITING" : parkingSlot)
         << " | Priority: " << (priority ? "Yes" : "No") << "\n";
}
