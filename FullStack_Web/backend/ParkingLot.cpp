#include "ParkingLot.h"
#include <iostream>
#include <iomanip>
#include <tuple>
#include <sstream>
using namespace std;

ParkingLot::ParkingLot(int numberOfSlots) {
    (void)numberOfSlots; // New Phase 2 layout replaces the old flat P01-P12 map.
    configure(2, 6);   // 3 buildings x 2 floors x 6 slots = 36 defaults.
}
bool ParkingLot::configure(int floors, int eachFloor) {
    if (floors < 1 || floors > 4 || eachFloor < 3 || eachFloor > 20 ||
        getOccupiedSlots() != 0) return false;
    floorsPerBuilding = floors;
    slotsPerFloor = eachFloor;
    slots.clear();
    // Capacity sequence is deliberately repeated in each floor.
    // Small slot + multiple medium + large positions; every floor can host large vehicles.
    const int sizes[] = {1, 2, 3, 2, 3, 2};
    for (char b = 'A'; b <= 'C'; ++b) {
        for (int f = 1; f <= floorsPerBuilding; ++f) {
            for (int s = 1; s <= slotsPerFloor; ++s) {
                ostringstream id;
                id << b << "-F" << f << "-S" << setfill('0') << setw(2) << s;
                slots.emplace_back(id.str(), string(1,b), f, sizes[(s-1)%6]);
            }
        }
    }
    return true;
}
int ParkingLot::getFloorsPerBuilding() const { return floorsPerBuilding; }
int ParkingLot::getSlotsPerFloor() const { return slotsPerFloor; }
const ParkingSlot* ParkingLot::slotById(const string& id) const {
    for (const auto& slot : slots) if (slot.slotNumber == id) return &slot;
    return nullptr;
}

// Phase 3: select the smallest compatible slot first; use lower floor / building ID
// only as stable tie breakers. This is not shortest-path routing (Phase 5).
const ParkingSlot* ParkingLot::bestSlot(const Vehicle& vehicle) const {
    const ParkingSlot* best = nullptr;
    auto score = [&vehicle](const ParkingSlot& s) {
        // Preserve large slots for large vehicles. Reserved slots are last choice
        // for priority vehicles when ordinary compatible slots exist.
        return std::make_tuple(vehicle.vehicleType == "Emergency" ? (s.restriction == 1 ? 0 : 1) : (s.restriction == 1 ? 1 : 0),
                               s.capacity, s.floor, s.building, s.slotNumber);
    };
    for (const ParkingSlot& s : slots)
        if (s.canFit(vehicle) && (!best || score(s) < score(*best))) best = &s;
    return best;
}
bool ParkingLot::setSlotRestriction(const string& id, int mode, const string& note) {
    if (mode < 0 || mode > 2 || note.size() > 90 ||
        note.find('\n') != string::npos || note.find('\r') != string::npos) return false;
    for (auto& slot : slots) {
        if (slot.slotNumber != id) continue;
        // Never move an occupied vehicle into a closed/reserved state silently.
        if (slot.occupied && mode != 0) return false;
        slot.restriction = mode;
        slot.restrictionNote = mode ? note : "";
        return true;
    }
    return false;
}
void ParkingLot::clearRestrictions() {
    for (auto& slot : slots) { slot.restriction = 0; slot.restrictionNote.clear(); }
}
string ParkingLot::explainSelection(const Vehicle& v, const ParkingSlot* chosen) const {
    int matchingSize = 0, viable = 0, occupiedMatching = 0, restricted = 0, preferenceMisses = 0;
    for (const auto& s : slots) {
        if (s.capacity < requiredVehicleSize(v.vehicleType)) continue;
        ++matchingSize;
        if (( !v.preferredBuilding.empty() && v.preferredBuilding != s.building) ||
            (v.preferredFloor != 0 && v.preferredFloor != s.floor)) { ++preferenceMisses; continue; }
        if (s.occupied) { ++occupiedMatching; continue; }
        if (s.restriction == 2 || (s.restriction == 1 && !v.priority && v.vehicleType != "Emergency"))
            { ++restricted; continue; }
        ++viable;
    }
    ostringstream text;
    if (chosen) {
        text << "Selected " << chosen->slotNumber << ": " << sizeName(requiredVehicleSize(v.vehicleType))
             << " vehicle fits " << sizeName(chosen->capacity) << " slot; "
             << "best-fit capacity, then floor/building order. ";
        if (!v.preferredBuilding.empty()) text << "Building " << v.preferredBuilding << " requested. ";
        if (v.preferredFloor > 0) text << "Floor " << v.preferredFloor << " requested. ";
        if (chosen->restriction == 1) text << "Priority reservation allowed. ";
        text << viable << " eligible slot(s) checked.";
    } else {
        text << "No eligible slot: " << matchingSize << " size-compatible, " << preferenceMisses
             << " outside preference, " << occupiedMatching << " occupied, " << restricted
             << " restricted, " << viable << " assignable. Added to waiting queue.";
    }
    return text.str();
}

int ParkingLot::suitableAvailable(const Vehicle& v) const {
    int count = 0;
    for (const auto& slot : slots) if (slot.canFit(v)) ++count;
    return count;
}
int ParkingLot::getTotalSlots() const { return (int)slots.size(); }
int ParkingLot::getAvailableSlots() const {
    int count = 0;
    for (const auto& slot : slots) if (!slot.occupied && slot.restriction == 0) ++count;
    return count;
}
int ParkingLot::getOccupiedSlots() const {
    int count = 0;
    for (const auto& slot : slots) if (slot.occupied) ++count;
    return count;
}

bool ParkingLot::parkVehicle(Vehicle& vehicle) {
    if (vehicle.vehicleNumber.empty() || findVehicle(vehicle.vehicleNumber)) return false;
    const ParkingSlot* selected = bestSlot(vehicle);
    if (selected && parkAtSlot(vehicle, selected->slotNumber)) {
        vehicle.parkingSlot = selected->slotNumber;
        return true;
    }
    return false;
}
bool ParkingLot::parkAtSlot(const Vehicle& vehicle, const string& slotId) {
    if (vehicle.vehicleNumber.empty() || findVehicle(vehicle.vehicleNumber)) return false;
    for (auto& slot : slots)
        if (slot.slotNumber == slotId) return slot.parkVehicle(vehicle);
    return false;
}
Vehicle ParkingLot::removeVehicle(string number) {
    for (auto& slot : slots)
        if (slot.occupied && slot.vehicle.vehicleNumber == number) return slot.removeVehicle();
    return Vehicle();
}
Vehicle* ParkingLot::findVehicle(string number) {
    for (auto& slot : slots)
        if (slot.occupied && slot.vehicle.vehicleNumber == number) return &slot.vehicle;
    return nullptr;
}
const Vehicle* ParkingLot::findVehicle(string number) const {
    for (const auto& slot : slots)
        if (slot.occupied && slot.vehicle.vehicleNumber == number) return &slot.vehicle;
    return nullptr;
}
void ParkingLot::displayParkingLot() const {
    cout << "\n============== BUILDING-WISE PARKING MAP ==============\n";
    string lastBuilding;
    int lastFloor = -1;
    for (const auto& slot : slots) {
        if (lastBuilding != slot.building || lastFloor != slot.floor) {
            cout << "\n--- Building " << slot.building << " | Floor " << slot.floor << " ---\n";
            lastBuilding = slot.building; lastFloor = slot.floor;
        }
        slot.displaySlot();
    }
    cout << "=======================================================\n";
}
const vector<ParkingSlot>& ParkingLot::getSlots() const { return slots; }
void ParkingLot::clear() {
    for (auto& slot : slots) if (slot.occupied) slot.removeVehicle();
}
