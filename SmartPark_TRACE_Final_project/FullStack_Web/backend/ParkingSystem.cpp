#include "ParkingSystem.h"
#include "FeeCalculator.h"
#include "TimeBilling.h"
#include "PriorityQueue.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <system_error>
using namespace std;
namespace fs = std::filesystem;

namespace {
string trimSpaces(const string& s) {
    string result;
    bool lastSpace = true;
    for (unsigned char c : s) {
        if (isspace(c)) {
            if (!lastSpace) result += ' ';
            lastSpace = true;
        } else {
            result += (char)c;
            lastSpace = false;
        }
    }
    if (!result.empty() && result.back() == ' ') result.pop_back();
    return result;
}
string upper(string s) {
    for (char& c : s) c = (char)toupper((unsigned char)c);
    return s;
}
string lower(string s) {
    for (char& c : s) c = (char)tolower((unsigned char)c);
    return s;
}
string stamp() {
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);
    tm local{};
#ifdef _WIN32
    localtime_s(&local, &t);
#else
    localtime_r(&t, &local);
#endif
    ostringstream out;
    out << put_time(&local, "%d %b %Y, %I:%M %p");
    return out.str();
}
string backupStamp() {
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);
    tm local{};
#ifdef _WIN32
    localtime_s(&local, &t);
#else
    localtime_r(&t, &local);
#endif
    ostringstream out;
    out << put_time(&local, "%Y%m%d-%H%M%S") << '-'
        << chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
    return out.str();
}
bool validPlate(const string& plate) {
    static const regex rule("^[A-Z0-9][A-Z0-9 -]{0,18}[A-Z0-9]$");
    return regex_match(plate, rule);
}
int ticketKey(const string& ticket) {
    if (ticket.size() != 11 || ticket.substr(0, 3) != "SP-") return 0;
    for (size_t i = 3; i < ticket.size(); ++i)
        if (!isdigit(static_cast<unsigned char>(ticket[i]))) return 0;
    int key = stoi(ticket.substr(3));
    return key > 0 ? key : 0;
}
bool validType(const string& type) {
    return type == "Car" || type == "Motorcycle" || type == "Van" ||
        type == "Emergency" || type == "Sedan" || type == "SUV" ||
        type == "Sports Car" || type == "Pickup" || type == "Bus" ||
        type == "Taxi" || type == "Truck";
}
bool validOwner(const string& owner) {
    if (owner.empty() || owner.size() > 70) return false;
    for (unsigned char c : owner) if (c < 32 || c == 127) return false;
    return true;
}
void writeRecord(ostream& out, const Vehicle& v) {
    out << quoted(v.ticketId) << ' ' << quoted(v.vehicleNumber) << ' '
        << quoted(v.vehicleType) << ' ' << quoted(v.ownerName) << ' '
        << quoted(v.parkingSlot) << ' ' << quoted(v.entryTime) << ' '
        << quoted(v.exitTime) << ' ' << (v.priority ? 1 : 0) << ' '
        << fixed << setprecision(0) << v.parkingFee << ' ' << v.hours << ' '
        << quoted(v.preferredBuilding) << ' ' << v.preferredFloor << ' '
        << v.entryEpoch << ' ' << v.exitEpoch << ' ' << v.durationSeconds << '\n';
}
bool readRecord(istream& in, Vehicle& v, bool legacy = false, bool exactTimes = false) {
    string line;
    if (!getline(in, line) || line.size() > 1024) return false;
    istringstream row(line);
    int priority;
    if (!(row >> quoted(v.ticketId) >> quoted(v.vehicleNumber)
              >> quoted(v.vehicleType) >> quoted(v.ownerName)
              >> quoted(v.parkingSlot) >> quoted(v.entryTime)
              >> quoted(v.exitTime) >> priority >> v.parkingFee >> v.hours)) return false;
    if (!legacy && !(row >> quoted(v.preferredBuilding) >> v.preferredFloor)) return false;
    if (exactTimes) {
        if (!(row >> v.entryEpoch >> v.exitEpoch >> v.durationSeconds)) return false;
    } else {
        v.entryEpoch = parkingParseStamp(v.entryTime);
        v.exitEpoch = parkingParseStamp(v.exitTime);
        v.durationSeconds = v.exitEpoch >= v.entryEpoch && v.entryEpoch > 0
            ? v.exitEpoch - v.entryEpoch : static_cast<long long>(v.hours) * 3600;
    }
    row >> ws;
    if (!row.eof() || (priority != 0 && priority != 1)) return false;
    v.priority = priority == 1;
    return true;
}
bool readSection(istream& in, const string& name, int& count, int limit) {
    string label;
    if (!(in >> label >> count) || label != name || count < 0 || count > limit) return false;
    string rest;
    getline(in, rest);
    return rest.find_first_not_of(" \t\r") == string::npos;
}
} // namespace

ParkingSystem::ParkingSystem(const fs::path& file)
    : lot(36), nextTicket(1), revenue(0), served(0), dataFile(file) {
    for (const auto& slot : lot.getSlots()) slotCircuit.append(slot.slotNumber);
    graph.rebuild(lot.getFloorsPerBuilding(), lot.getSlotsPerFloor());
    // Never silently overwrite a damaged existing save file.
    std::error_code ec;
    bool existing = fs::exists(dataFile, ec) && !ec;
    if (existing && !load()) {
        fs::path damaged = dataFile;
        damaged += ".corrupt-" + backupStamp();
        fs::rename(dataFile, damaged, ec);
        if (!ec) {
            std::cerr << "WARNING: invalid C++ save moved to " << damaged.string()
                      << ". Starting with empty parking.\n";
        } else {
            // Keep the original file if it could not be preserved.
            std::cerr << "WARNING: C++ save is invalid and could not be moved. "
                      << "Do not park vehicles until it is backed up manually.\n";
        }
    }
}

void ParkingSystem::clearMemory() {
    lot.clear(); active.clear(); waiting.clear(); exitStack.clear(); exitHistory.clear();
    activeBST.clear(); activeAVL.clear(); treeEvents.clear(); ccp.clear();
    nextTicket = 1; revenue = 0; served = 0;
}
bool ParkingSystem::validRecord(const Vehicle& v) const {
    return ticketKey(v.ticketId) != 0 &&
        validPlate(v.vehicleNumber) && validType(v.vehicleType) &&
        validOwner(v.ownerName) &&
        (v.preferredBuilding.empty() || v.preferredBuilding == "A" ||
         v.preferredBuilding == "B" || v.preferredBuilding == "C") &&
        v.preferredFloor >= 0 && v.preferredFloor <= 4 &&
        v.entryTime.size() <= 64 &&
        v.exitTime.size() <= 64 && v.parkingFee >= 0 &&
        v.parkingFee <= 1000000000 && v.hours >= 0 && v.hours <= 200000 &&
        v.entryEpoch >= 0 && v.exitEpoch >= 0 && v.durationSeconds >= 0;
}
// ============ PHASE 5: GRAPH + BFS + DFS ============
// Real-world constraint: a free, size-compatible slot is NOT eligible if all
// access lanes to it are blocked. Keep Phase 3 best-fit as primary ranking;
// among equally sized slots choose the fewer-edge BFS route.
const ParkingSlot* ParkingSystem::bestReachableSlot(const Vehicle& vehicle) const {
    const ParkingSlot* chosen = nullptr;
    std::tuple<int,int,int,int,std::string,std::string> bestScore;
    for(const ParkingSlot& slot : lot.getSlots()) {
        if(!slot.canFit(vehicle) || ccp.heldSlot(slot.slotNumber) ||
            (ccp.evacuation && slot.building==std::string(1,ccp.incident)))continue;
        auto path=graph.pathTo(slot.slotNumber);
        if(path.empty())continue;
        int reservationScore = vehicle.vehicleType == "Emergency" ? (slot.restriction == 1 ? 0 : 1)
                                      : (slot.restriction == 1 ? 1 : 0);
        auto score=std::make_tuple(reservationScore,slot.capacity,(int)path.size()-1,
                                   slot.floor,slot.building,slot.slotNumber);
        if(!chosen || score<bestScore){chosen=&slot;bestScore=score;}
    }
    return chosen;
}
std::string ParkingSystem::describeRouteDecision(const Vehicle& v,const ParkingSlot* slot) const {
    std::ostringstream out;
    if(!slot) {
        bool usableButBlocked=false;
        for(const auto& s : lot.getSlots())
            if(s.canFit(v) && graph.pathTo(s.slotNumber).empty())usableButBlocked=true;
        return usableButBlocked
          ? "No reachable compatible slot: one or more access roads are blocked. Vehicle added to waiting queue."
          : "No available compatible slot for vehicle size, priority and preferred building/floor. Vehicle added to waiting queue.";
    }
    auto path=graph.pathTo(slot->slotNumber);
    out<<"Selected "<<slot->slotNumber<<" ("<<sizeName(slot->capacity)
       <<" capacity). best-fit capacity + BFS reachable route ("<<(path.size()-1)<<" edges): ";
    for(size_t i=0;i<path.size();++i){if(i)out<<" -> ";out<<path[i];}
    if(!v.preferredBuilding.empty())out<<". Preferred Building "<<v.preferredBuilding;
    if(v.preferredFloor)out<<", Floor "<<v.preferredFloor;
    if(slot->restriction==1)out<<". Priority reservation allowed";
    return out.str()+".";
}
bool ParkingSystem::setRoadBlocked(const std::string& from,const std::string& to,bool blocked) {
    if(!graph.hasRoad(from,to))return false;
    bool previous=graph.isBlocked(from,to);
    if(previous==blocked)return true;
    if(!graph.setBlocked(from,to,blocked))return false;
    if(blocked)recoverVipReservations(from,to);
    else {dispatchWaiting();dispatchGroups();} // An accessible compatible slot might now be free.
    if(save())return true;
    // Best effort restore persisted state on I/O failure.
    if(!load())graph.setBlocked(from,to,previous);
    return false;
}
std::vector<RoadEdge> ParkingSystem::roads() const {return graph.roads();}
std::vector<std::string> ParkingSystem::routeToSlot(const std::string& id) const {return graph.pathTo(id);}
bool ParkingSystem::dfsCanReach(const std::string& id) const {return graph.dfsReachable("ENTRY",id);}
int ParkingSystem::closedRoadCount() const {return graph.closedCount();}

// ============ PHASE 4: PRIORITY + FAIRNESS SCHEDULER ============
// Never allocate an unusable space. Among eligible waiting records:
// Emergency > VIP > Normal. Same priority keeps original FIFO arrival order.
// Every successful transfer also updates Linked List, BST and AVL indexes.
vector<Vehicle> ParkingSystem::dispatchWaiting() {
    vector<Vehicle> assigned;
    while (!waiting.isEmpty()) {
        bool moved = false;
        for (Vehicle candidate : waitingByPriority()) {
            if(ccp.evacuation && candidate.vehicleType!="Emergency" && !candidate.priority)continue;
            const ParkingSlot* slot = bestReachableSlot(candidate);
            if (!slot) continue; // Skip incompatible tickets, without removing them.
            const string slotID = slot->slotNumber;
            candidate.parkingSlot = slotID;
            candidate.entryEpoch = parkingNowEpoch();
            candidate.entryTime = parkingFormatEpoch(candidate.entryEpoch); // Clock begins at allocation.
            if (!lot.parkAtSlot(candidate, slotID)) continue;
            if (!waiting.removeNumber(candidate.vehicleNumber)) {
                // Internal guard: never allow duplicate vehicle in parking and queue.
                lot.removeVehicle(candidate.vehicleNumber);
                continue;
            }
            active.insert(candidate);
            activeBST.insert(ticketKey(candidate.ticketId), candidate.vehicleNumber);
            activeAVL.insert(ticketKey(candidate.ticketId), candidate.vehicleNumber);
            treeEvents.push_back({'I', ticketKey(candidate.ticketId), candidate.vehicleNumber});
            assigned.push_back(candidate);
            moved = true;
            break; // Re-check priority and slot availability after every assignment.
        }
        if (!moved) break;
    }
    return assigned;
}
int ParkingSystem::eligibleSlotsFor(const Vehicle& candidate) const {
    int count=0; for(const auto& slot:lot.getSlots())
        if(slot.canFit(candidate)&&!ccp.heldSlot(slot.slotNumber)&&
           !(ccp.evacuation&&slot.building==std::string(1,ccp.incident))&&
           !graph.pathTo(slot.slotNumber).empty())++count;
    return count;
}
bool ParkingSystem::cancelWaiting(const string& rawPlate) {
    string plate = upper(trimSpaces(rawPlate));
    if (!validPlate(plate) || !waiting.removeNumber(plate)) return false;
    return save();
}
bool ParkingSystem::changeWaitingVIP(const string& rawPlate, bool vip) {
    string plate = upper(trimSpaces(rawPlate));
    Vehicle* found = waiting.findNumber(plate);
    if (!found || found->vehicleType == "Emergency") return false;
    found->priority = vip;
    dispatchWaiting(); // e.g. a newly VIP-qualified ticket may use a reserved space.
    return save();
}
ParkResult ParkingSystem::park(const string& rawPlate, const string& type,
                                const string& rawOwner, bool vip,
                                const string& building, int floor) {
    ParkResult answer;
    string plate = upper(trimSpaces(rawPlate));
    string owner = trimSpaces(rawOwner);
    if (!validPlate(plate)) {
        answer.message = "Vehicle number must be 2-20 letters/numbers (spaces or hyphens allowed).";
        return answer;
    }
    if (!validOwner(owner)) { answer.message = "Owner name must have 1-70 valid characters."; return answer; }
    if (!validType(type)) { answer.message = "Invalid vehicle type."; return answer; }
    if (!(building.empty() || building == "A" || building == "B" || building == "C") ||
        floor < 0 || floor > lot.getFloorsPerBuilding()) {
        answer.message = "Invalid building or floor choice."; return answer;
    }
    if(ccp.evacuation && type!="Emergency" && !vip){
        answer.message="NORMAL INBOUND LOCKED: emergency evacuation is active.";
        return answer;
    }
    if (lot.findVehicle(plate) || waiting.containsNumber(plate) || plateInCCP(plate)) {
        answer.message = "This vehicle is already parked or waiting.";
        return answer;
    }
    if (nextTicket > 99999999) { answer.message = "Ticket counter is full."; return answer; }
    Vehicle vehicle(plate, type, owner, vip || type == "Emergency");
    vehicle.preferredBuilding = building;
    vehicle.preferredFloor = floor;
    ostringstream ticket;
    ticket << "SP-" << setw(8) << setfill('0') << nextTicket++;
    vehicle.ticketId = ticket.str();
    vehicle.entryEpoch = parkingNowEpoch();
    vehicle.entryTime = parkingFormatEpoch(vehicle.entryEpoch);
    // Previously waiting drivers get first access to any newly usable free slots.
    dispatchWaiting();
    const ParkingSlot* recommended = bestReachableSlot(vehicle);
    answer.explanation = describeRouteDecision(vehicle, recommended);
    answer.eligibleSlots = eligibleSlotsFor(vehicle);
    if (recommended && lot.parkAtSlot(vehicle, recommended->slotNumber)) {
        vehicle.parkingSlot = recommended->slotNumber;
        active.insert(vehicle);
        activeBST.insert(ticketKey(vehicle.ticketId), vehicle.vehicleNumber);
        activeAVL.insert(ticketKey(vehicle.ticketId), vehicle.vehicleNumber);
        treeEvents.push_back({'I', ticketKey(vehicle.ticketId), vehicle.vehicleNumber});
        answer.message = "Vehicle parked in " + vehicle.parkingSlot + ". " + answer.explanation;
    } else {
        waiting.enqueue(vehicle);
        answer.queued = true;
        answer.message = answer.explanation;
    }
    answer.success = true;
    answer.vehicle = vehicle;
    if (!save()) answer.message += " WARNING: could not save to disk.";
    return answer;
}
ExitResult ParkingSystem::exitVehicle(const string& plate) {
    return exitVehicleAt(plate, parkingNowEpoch());
}
// Compatibility with earlier C++ regression drivers only; hidden from both UI layers.
ExitResult ParkingSystem::exitVehicle(const string& plate, int legacyTestHours) {
    if (legacyTestHours < 1 || legacyTestHours > 720) {
        ExitResult error; error.message = "Hours must be 1-720 (legacy tests)."; return error;
    }
    const Vehicle* current = lot.findVehicle(upper(trimSpaces(plate)));
    if (!current) { ExitResult error; error.message = "Vehicle not found in parking."; return error; }
    long long started = current->entryEpoch ? current->entryEpoch : parkingParseStamp(current->entryTime);
    return exitVehicleAt(plate, started + static_cast<long long>(legacyTestHours) * 3600);
}
ExitResult ParkingSystem::exitVehicleAt(const string& rawPlate, long long exitEpoch) {
    ExitResult answer;
    string plate = upper(trimSpaces(rawPlate));
    Vehicle* found = lot.findVehicle(plate);
    if (!found) { answer.message = "Vehicle not found in parking. Waiting vehicles cannot exit."; return answer; }
    long long started = found->entryEpoch ? found->entryEpoch : parkingParseStamp(found->entryTime);
    if (started <= 0) { answer.message = "Entry time missing. Cannot calculate reliable bill."; return answer; }
    if (exitEpoch < started) { answer.message = "Exit time cannot precede entry time."; return answer; }
    long long seconds = exitEpoch - started;
    // Allow long stays without accidental negative/overflow fees.
    if (seconds > 200000LL * 3600LL) { answer.message = "Invalid parking duration."; return answer; }
    int billableHours = parkingStartedHours(seconds);
    Vehicle removed = lot.removeVehicle(plate);
    active.remove(plate);
    activeBST.remove(ticketKey(removed.ticketId));
    activeAVL.remove(ticketKey(removed.ticketId));
    treeEvents.push_back({'D', ticketKey(removed.ticketId), removed.vehicleNumber});
    removed.entryEpoch = started;
    removed.durationSeconds = seconds;
    removed.hours = billableHours;
    removed.parkingFee = calculateParkingFee(removed.vehicleType, billableHours);
    removed.exitEpoch = exitEpoch;
    removed.exitTime = parkingFormatEpoch(exitEpoch);
    exitStack.push(removed);  // LIFO: most recent receipt on top.
    exitHistory.pushFront(removed); // Doubly Linked List: permanent exit record.
    revenue += static_cast<long long>(removed.parkingFee);
    ++served;
    answer.success = true;
    answer.vehicle = removed;
    answer.freedSlot = removed.parkingSlot;
    answer.message = plate + " exited from " + answer.freedSlot + ".";
    auto assignments = dispatchWaiting(); // Release of slot activates priority queue.
    dispatchGroups();
    if (!assignments.empty()) {
        answer.reassigned = true;
        answer.nextVehicle = assignments.front();
        answer.message += " Queue auto-assigned " + to_string(assignments.size()) + " vehicle(s).";
    }
    if (!save()) answer.message += " WARNING: could not save to disk.";
    return answer;
}
vector<Vehicle> ParkingSystem::parkedVehicles() const {
    vector<Vehicle> rows;
    for (const auto& slot : lot.getSlots()) if (slot.occupied) rows.push_back(slot.vehicle);
    return rows;
}
vector<Vehicle> ParkingSystem::waitingVehicles() const { return waiting.toVector(); }
vector<Vehicle> ParkingSystem::waitingByPriority() const {
    PriorityQueue pq;
    for (const Vehicle& v : waiting.toVector()) pq.insert(v);
    vector<Vehicle> rows;
    while (!pq.isEmpty()) rows.push_back(pq.removeHighestPriority());
    return rows;
}
vector<Vehicle> ParkingSystem::exitRecords() const { return exitStack.toVector(); }
vector<Vehicle> ParkingSystem::historyBackward() const { return exitHistory.backward(); }
vector<Vehicle> ParkingSystem::search(const string& text) const {
    string q = lower(trimSpaces(text));
    vector<Vehicle> hits;
    if (q.empty()) return hits;
    vector<Vehicle> records = parkedVehicles();
    auto wait = waitingVehicles();
    records.insert(records.end(), wait.begin(), wait.end());
    for (const Vehicle& v : records) {
        if (lower(v.vehicleNumber).find(q) != string::npos ||
            lower(v.ownerName).find(q) != string::npos ||
            lower(v.vehicleType).find(q) != string::npos ||
            lower(v.parkingSlot).find(q) != string::npos ||
            lower(v.ticketId).find(q) != string::npos) hits.push_back(v);
    }
    return hits;
}
bool ParkingSystem::configureLayout(int floors, int slotsPerFloor) {
    // Changing capacity with active/waiting vehicles could destroy their assignments.
    if (occupied() || waitingCount() || ccp.holdCount() || ccp.groups.size() ||
        ccp.evacuation || !lot.configure(floors, slotsPerFloor)) return false;
    slotCircuit.clear();
    for (const auto& slot : lot.getSlots()) slotCircuit.append(slot.slotNumber);
    graph.rebuild(floors,slotsPerFloor);
    return save();
}

bool ParkingSystem::setSlotRestriction(const string& id, int mode, const string& note) {
    const ParkingSlot* current = lot.slotById(id);
    if (!current || ccp.heldSlot(id)) return false;
    int originalMode = current->restriction;
    string originalNote = current->restrictionNote;
    if (!lot.setSlotRestriction(id, mode, note)) return false;
    // Reopening normal/reserved slots may serve compatible waiting vehicles.
    if (mode != 2) {dispatchWaiting();dispatchGroups();}
    if (save()) return true;
    // Restore the preceding persisted snapshot if write fails.
    if (!load()) lot.setSlotRestriction(id, originalMode, originalNote);
    return false;
}
string ParkingSystem::previewAllocation(const string& type, bool vip, const string& building, int floor) const {
    if (!validType(type) || !(building.empty() || building == "A" || building == "B" || building == "C") ||
        floor < 0 || floor > lot.getFloorsPerBuilding()) return "Invalid vehicle type or location preference.";
    Vehicle candidate("PREVIEW-01",type,"Preview",vip || type == "Emergency");
    candidate.preferredBuilding=building; candidate.preferredFloor=floor;
    return describeRouteDecision(candidate, bestReachableSlot(candidate));
}

int ParkingSystem::floorsPerBuilding() const { return lot.getFloorsPerBuilding(); }
int ParkingSystem::slotsPerFloor() const { return lot.getSlotsPerFloor(); }
const vector<ParkingSlot>& ParkingSystem::parkingSlots() const { return lot.getSlots(); }
void ParkingSystem::showParkingMap() const {
    lot.displayParkingLot();
    cout << "\n=== PHASE 5: UNREACHABLE SLOTS (ROAD CLOSURES) ===\n";
    int n=0;
    for(const auto& slot:lot.getSlots())
        if(!slot.occupied && !graph.dfsReachable("ENTRY",slot.slotNumber)) {
            cout<<slot.slotNumber<<" cannot be reached from ENTRY.\n"; ++n;
        }
    if(!n)cout<<"All configured parking slots have an access route.\n";
}
void ParkingSystem::showSinglyList() const { active.display(); }
void ParkingSystem::showDoublyForward() const { exitHistory.displayForward(); }
void ParkingSystem::showDoublyBackward() const { exitHistory.displayBackward(); }
void ParkingSystem::showCircularList() const { slotCircuit.display(); }
void ParkingSystem::showQueue() const { waiting.display(); }
void ParkingSystem::showStack() const { exitStack.display(); }
string ParkingSystem::inspectNextSlot() { return slotCircuit.inspectNext(); }
int ParkingSystem::occupied() const { return lot.getOccupiedSlots(); }
int ParkingSystem::available() const {
    int count=0;
    for(const auto& slot:lot.getSlots())
        if(!slot.occupied && slot.restriction==0 && !ccp.heldSlot(slot.slotNumber) &&
           !(ccp.evacuation&&slot.building==std::string(1,ccp.incident))&&
           graph.dfsReachable("ENTRY",slot.slotNumber))++count;
    return count;
}
int ParkingSystem::total() const { return lot.getTotalSlots(); }
int ParkingSystem::waitingCount() const {
    int n=waiting.size();
    for(auto*g=ccp.groups.first();g;g=g->next)n+=g->group.count;
    return n;
}
int ParkingSystem::servedCount() const { return served; }
long long ParkingSystem::totalRevenue() const { return revenue; }
fs::path ParkingSystem::savedFile() const { return dataFile; }
const BSTree& ParkingSystem::bst() const { return activeBST; }
const AVLTree& ParkingSystem::avl() const { return activeAVL; }
optional<Vehicle> ParkingSystem::findParkedByTicket(int ticketNumber, bool balanced) const {
    auto plate = balanced ? activeAVL.vehicleFor(ticketNumber) : activeBST.vehicleFor(ticketNumber);
    if (!plate) return nullopt;
    const Vehicle* found = lot.findVehicle(*plate);
    return found ? optional<Vehicle>(*found) : nullopt;
}
bool ParkingSystem::treeIndexesValid() const {
    if (!activeBST.verify() || !activeAVL.verify()) return false;
    if (activeBST.countNodes() != occupied() || activeAVL.countNodes() != occupied()) return false;
    if (activeBST.inorder() != activeAVL.inorder()) return false;
    for (const Vehicle& v : parkedVehicles()) {
        int id = ticketKey(v.ticketId);
        auto p1 = activeBST.vehicleFor(id), p2 = activeAVL.vehicleFor(id);
        if (!id || !p1 || !p2 || *p1 != v.vehicleNumber || *p2 != v.vehicleNumber) return false;
    }
    return true;
}

bool ParkingSystem::save() const {
    error_code ec;
    if (dataFile.has_parent_path()) fs::create_directories(dataFile.parent_path(), ec);
    if (ec) return false;
    fs::path tmp = dataFile;
    tmp += ".tmp";
    ofstream out(tmp, ios::trunc | ios::binary);
    if (!out) return false;
    out << "SMARTPARK_CPP_V6\n";
    out << "NEXT_TICKET " << nextTicket << '\n';
    out << "REVENUE " << revenue << '\n';
    out << "SERVED " << served << '\n';
    out << "LAYOUT " << floorsPerBuilding() << ' ' << slotsPerFloor() << '\n';
    int restrictedCount = 0;
    for (const auto& s : lot.getSlots()) if (s.restriction) ++restrictedCount;
    out << "SLOT_RULES " << restrictedCount << '\n';
    for (const auto& s : lot.getSlots()) if (s.restriction)
        out << quoted(s.slotNumber) << ' ' << s.restriction << ' ' << quoted(s.restrictionNote) << '\n';
    auto roadList=graph.roads();
    int blockedCount=graph.closedCount();
    out << "BLOCKED_ROADS " << blockedCount << '\n';
    for(const auto& road:roadList) if(road.blocked)
        out << quoted(road.from) << ' ' << quoted(road.to) << '\n';
    auto parked = parkedVehicles();
    auto queued = waitingVehicles();
    auto history = exitRecords();
    out << "PARKED " << parked.size() << '\n';
    for (const auto& v : parked) writeRecord(out, v);
    out << "WAITING " << queued.size() << '\n';
    for (const auto& v : queued) writeRecord(out, v);
    out << "HISTORY " << history.size() << '\n';
    for (const auto& v : history) writeRecord(out, v);
    out << "TREE_EVENTS " << treeEvents.size() << '\n';
    for (const auto& event : treeEvents)
        out << event.op << ' ' << event.key << ' ' << quoted(event.plate) << '\n';
    if(!ccp.write(out))return false;
    out << "END\n";
    out.flush();
    if (!out.good()) return false;
    out.close();
    if (out.fail()) return false;
    fs::rename(tmp, dataFile, ec);
    if (!ec) return true; // POSIX: replaces old file atomically.
    // Windows cannot rename over existing file. Keep a recovery copy first.
    fs::path old = dataFile;
    old += ".previous";
    ec.clear();
    fs::remove(old, ec);
    ec.clear();
    if (!fs::exists(dataFile)) { fs::rename(tmp, dataFile, ec); return !ec; }
    fs::rename(dataFile, old, ec);
    if (ec) return false;
    fs::rename(tmp, dataFile, ec);
    if (ec) {
        error_code restore;
        fs::rename(old, dataFile, restore);
        return false;
    }
    return true;
}

bool ParkingSystem::load() {
    ifstream in(dataFile, ios::binary);
    if (!in) return false; // First run starts empty.
    string header;
    if (!getline(in, header) || (header != "SMARTPARK_CPP_V1" && header != "SMARTPARK_CPP_V2" && header != "SMARTPARK_CPP_V3" && header != "SMARTPARK_CPP_V4" && header != "SMARTPARK_CPP_V5" && header != "SMARTPARK_CPP_V6")) return false;
    bool legacy = header == "SMARTPARK_CPP_V1";
    string label;
    long long newTicket = 0, totalEarned = 0;
    int totalServed = -1;
    if (!(in >> label >> newTicket) || label != "NEXT_TICKET" || newTicket < 1 || newTicket > 99999999) return false;
    if (!(in >> label >> totalEarned) || label != "REVENUE" || totalEarned < 0) return false;
    if (!(in >> label >> totalServed) || label != "SERVED" || totalServed < 0) return false;
    int floors = 2, perFloor = 6;
    if (!legacy && (!(in >> label >> floors >> perFloor) || label != "LAYOUT" ||
                    floors < 1 || floors > 4 || perFloor < 3 || perFloor > 20)) return false;
    // A fresh ParkingLot holds no occupied slots, so layout can be applied before validation.
    if (!lot.configure(floors, perFloor)) return false;
    graph.rebuild(floors,perFloor);
    if (header == "SMARTPARK_CPP_V3" || header == "SMARTPARK_CPP_V4" || header == "SMARTPARK_CPP_V5" || header == "SMARTPARK_CPP_V6") {
        int rules = -1;
        if (!(in >> label >> rules) || label != "SLOT_RULES" || rules < 0 || rules > lot.getTotalSlots()) return false;
        set<string> seenRules;
        for (int i=0; i<rules; ++i) {
            string slotId, reason;
            int mode;
            if (!(in >> quoted(slotId) >> mode >> quoted(reason)) || !seenRules.insert(slotId).second ||
                mode < 1 || mode > 2 || !lot.setSlotRestriction(slotId, mode, reason)) return false;
        }
    }
    if(header == "SMARTPARK_CPP_V4" || header == "SMARTPARK_CPP_V5" || header == "SMARTPARK_CPP_V6") {
        int roads=-1;
        if(!(in >> label >> roads) || label!="BLOCKED_ROADS" || roads<0 || roads>(int)graph.roads().size()) return false;
        for(int i=0;i<roads;++i) {
            string from,to;
            if(!(in >> quoted(from) >> quoted(to)) || graph.isBlocked(from,to) ||
               !graph.setBlocked(from,to,true))return false;
        }
    }
    slotCircuit.clear();
    for (const auto& slot : lot.getSlots()) slotCircuit.append(slot.slotNumber);
    string rest;
    getline(in, rest);
    int count;
    vector<Vehicle> parked, queued, history;
    if (!readSection(in, "PARKED", count, total())) return false;
    for (int i = 0; i < count; ++i) {
        Vehicle v;
        if (!readRecord(in, v, legacy, (header == "SMARTPARK_CPP_V5" || header == "SMARTPARK_CPP_V6")) || !validRecord(v)) return false;
        parked.push_back(v);
    }
    if (!readSection(in, "WAITING", count, 10000)) return false;
    for (int i = 0; i < count; ++i) {
        Vehicle v;
        if (!readRecord(in, v, legacy, (header == "SMARTPARK_CPP_V5" || header == "SMARTPARK_CPP_V6")) || !validRecord(v)) return false;
        queued.push_back(v);
    }
    if (!readSection(in, "HISTORY", count, 100000)) return false;
    for (int i = 0; i < count; ++i) {
        Vehicle v;
        if (!readRecord(in, v, legacy, (header == "SMARTPARK_CPP_V5" || header == "SMARTPARK_CPP_V6")) || !validRecord(v)) return false;
        history.push_back(v);
    }
    // Migrate earlier 12 flat P01-P12 tickets into the first 12 building slots.
    if (legacy) {
        auto migrate = [&](vector<Vehicle>& records) {
            for (auto& v : records) {
                if (v.parkingSlot.size() >= 3 && v.parkingSlot[0] == 'P') {
                    string number = v.parkingSlot.substr(1);
                    if (!number.empty() && all_of(number.begin(), number.end(), [](unsigned char c){return isdigit(c);})) {
                        int idx = stoi(number) - 1;
                        if (idx >= 0 && idx < (int)lot.getSlots().size())
                            v.parkingSlot = lot.getSlots()[idx].slotNumber;
                    }
                }
            }
        };
        migrate(parked); migrate(history);
    }
    if (legacy) {
        set<string> used;
        for (auto& v : parked) {
            const ParkingSlot* candidate = lot.slotById(v.parkingSlot);
            if (!candidate || candidate->capacity < requiredVehicleSize(v.vehicleType) || used.count(v.parkingSlot)) {
                for (const auto& alternative : lot.getSlots()) {
                    if (alternative.capacity >= requiredVehicleSize(v.vehicleType) && !used.count(alternative.slotNumber)) {
                        v.parkingSlot = alternative.slotNumber; break;
                    }
                }
            }
            used.insert(v.parkingSlot);
        }
    }
    // Older Phase 3 files had no TREE_EVENTS section. Both formats remain readable.
    vector<TreeEvent> events;
    string end;
    if (!(in >> end)) return false;
    if (end == "TREE_EVENTS") {
        int eventCount = -1;
        if (!(in >> eventCount) || eventCount < 0 || eventCount > 400000) return false;
        string rest;
        getline(in, rest);
        for (int i = 0; i < eventCount; ++i) {
            string line;
            if (!getline(in,line) || line.size() > 128) return false;
            istringstream input(line);
            TreeEvent e{};
            if (!(input >> e.op >> e.key >> quoted(e.plate)) ||
                (e.op != 'I' && e.op != 'D') || e.key <= 0 || e.key > 99999999 ||
                !validPlate(e.plate)) return false;
            input >> ws;
            if (!input.eof()) return false;
            events.push_back(e);
        }
        if (!(in >> end)) return false;
    }
    int savedEmergency=0;char savedIncident='-';long long savedNextGroup=1;
    std::string savedRecovery;
    traceccp::Hold loadedHolds[traceccp::MAX_HOLDS]; int holdCount=0;
    traceccp::Group loadedGroups[200];int groupCount=0;
    if(header=="SMARTPARK_CPP_V6"){
        if(end!="CCP" || !(in>>savedEmergency>>savedIncident>>savedNextGroup>>quoted(savedRecovery))||
           (savedEmergency!=0&&savedEmergency!=1)||
           (savedEmergency && savedIncident!='A'&&savedIncident!='B'&&savedIncident!='C')||
           savedNextGroup<1||savedNextGroup>10000000||savedRecovery.size()>300)return false;
        if(!readSection(in,"HOLDS",holdCount,traceccp::MAX_HOLDS))return false;
        for(int i=0;i<holdCount;++i){
            std::string line; if(!getline(in,line)||line.size()>1400)return false;
            std::istringstream row(line);auto&h=loadedHolds[i];h.active=true;
            if(!(row>>quoted(h.plate)>>quoted(h.type)>>quoted(h.owner)>>quoted(h.building)
                    >>h.floor>>quoted(h.slot)>>quoted(h.path)) || !validPlate(h.plate)||
               !validOwner(h.owner)||!validType(h.type)||h.path.size()>900||
               h.floor<0||h.floor>floors||!lot.slotById(h.slot)||
               (!h.building.empty()&&h.building!="A"&&h.building!="B"&&h.building!="C"))return false;
            row>>ws;if(!row.eof())return false;
        }
        if(!readSection(in,"GROUPS",groupCount,200))return false;
        for(int i=0;i<groupCount;++i){
            std::string line; if(!getline(in,line)||line.size()>1800)return false;
            std::istringstream row(line);auto&g=loadedGroups[i];
            if(!(row>>quoted(g.id)>>quoted(g.building)>>g.floor>>g.vip>>g.count)||
               g.id.size()>32||g.count<3||g.count>4||g.floor<0||g.floor>floors||
               (!g.building.empty()&&g.building!="A"&&g.building!="B"&&g.building!="C"))return false;
            for(int j=0;j<g.count;++j)if(!(row>>quoted(g.plate[j])>>quoted(g.type[j])>>quoted(g.owner[j]))||
                !validPlate(g.plate[j])||!validType(g.type[j])||!validOwner(g.owner[j]))return false;
            row>>ws;if(!row.eof())return false;
        }
        if(!(in>>end))return false;
    }
    if (end != "END") return false;
    string extra;
    if (in >> extra) return false;
    set<string> plates, ids, takenSlots;
    for (const auto& v : parked) {
        if (v.parkingSlot.empty() || !plates.insert(v.vehicleNumber).second ||
            !ids.insert(v.ticketId).second || !takenSlots.insert(v.parkingSlot).second) return false;
        bool exists = false;
        for (const auto& slot : lot.getSlots()) if (slot.slotNumber == v.parkingSlot) exists = true;
        if (!exists) return false;
        const ParkingSlot* position = lot.slotById(v.parkingSlot);
        if (!legacy && (!position || position->capacity < requiredVehicleSize(v.vehicleType) ||
            position->restriction == 2 ||
            (position->restriction == 1 && !v.priority && v.vehicleType != "Emergency"))) return false;
    }
    for (const auto& v : queued) {
        if (!v.parkingSlot.empty() || !plates.insert(v.vehicleNumber).second ||
            !ids.insert(v.ticketId).second) return false;
    }
    if (totalServed != (int)history.size()) return false;
    // Validate pending/group plates against active tickets, plus any other CCP item.
    std::set<std::string> ccpSeen;
    std::set<std::string> heldSlots;
    for(int i=0;i<holdCount;++i){
        const auto&h=loadedHolds[i];
        if(plates.count(h.plate)||!ccpSeen.insert(h.plate).second||
           !heldSlots.insert(h.slot).second||lot.slotById(h.slot)->occupied ||
           takenSlots.count(h.slot))return false;
    }
    for(int i=0;i<groupCount;++i){
        const auto&g=loadedGroups[i];
        for(int j=0;j<g.count;++j)
            if(plates.count(g.plate[j])||!ccpSeen.insert(g.plate[j]).second)return false;
    }
    if (!events.empty()) {
        BSTree tempBST;
        AVLTree tempAVL;
        for (const auto& e : events) {
            bool b = e.op == 'I' ? tempBST.insert(e.key, e.plate) : tempBST.remove(e.key);
            bool a = e.op == 'I' ? tempAVL.insert(e.key, e.plate) : tempAVL.remove(e.key);
            if (!b || !a) return false;
        }
        if (tempBST.countNodes() != (int)parked.size() ||
            tempAVL.countNodes() != (int)parked.size() ||
            !tempBST.verify() || !tempAVL.verify()) return false;
        for (const auto& v : parked) {
            int key = ticketKey(v.ticketId);
            if (tempBST.vehicleFor(key) != v.vehicleNumber ||
                tempAVL.vehicleFor(key) != v.vehicleNumber) return false;
        }
    }
    // Rebuild all structures only after the entire file is validated.
    clearMemory();
    nextTicket = newTicket;
    revenue = totalEarned;
    served = totalServed;
    for (const auto& v : parked) {
        lot.parkAtSlot(v, v.parkingSlot);
        active.insert(v);
        if (events.empty()) {
            activeBST.insert(ticketKey(v.ticketId), v.vehicleNumber);
            activeAVL.insert(ticketKey(v.ticketId), v.vehicleNumber);
            treeEvents.push_back({'I', ticketKey(v.ticketId), v.vehicleNumber});
        }
    }
    if (!events.empty()) {
        for (const auto& e : events) {
            if (e.op == 'I') { activeBST.insert(e.key,e.plate); activeAVL.insert(e.key,e.plate); }
            else { activeBST.remove(e.key); activeAVL.remove(e.key); }
        }
        treeEvents = move(events);
    }
    for (const auto& v : queued) waiting.enqueue(v);
    ccp.evacuation=savedEmergency!=0;
    ccp.incident=savedEmergency?savedIncident:'-';
    ccp.nextGroupId=savedNextGroup;
    ccp.lastRecovery=savedRecovery;
    for(int i=0;i<holdCount;++i)ccp.addHold(loadedHolds[i]);
    for(int i=0;i<groupCount;++i)ccp.groups.push(loadedGroups[i]);
    for (auto it = history.rbegin(); it != history.rend(); ++it) {
        exitStack.push(*it);
        exitHistory.pushFront(*it);
    }
    return true;
}

bool ParkingSystem::resetWithBackup() {
    error_code ec;
    if (fs::exists(dataFile, ec) && !ec && (occupied() || waitingCount() || served)) {
        fs::path folder = dataFile.parent_path() / "cpp_backups";
        fs::create_directories(folder, ec);
        if (ec) return false;
        fs::path backup = folder / ("before-reset-" + backupStamp() + ".txt");
        fs::copy_file(dataFile, backup, fs::copy_options::none, ec);
        if (ec) return false;
    }
    clearMemory();
    lot.clearRestrictions();
    graph.rebuild(lot.getFloorsPerBuilding(),lot.getSlotsPerFloor());
    return save();
}

#include "TraceCCPMethods.inc"
