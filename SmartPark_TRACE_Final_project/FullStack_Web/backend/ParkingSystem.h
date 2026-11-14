#ifndef PARKING_SYSTEM_H
#define PARKING_SYSTEM_H
#include <filesystem>
#include <string>
#include <vector>
#include <optional>
#include "BST.h"
#include "AVLTree.h"
#include "ParkingLot.h"
#include "ParkingGraph.h"
#include "TraceCCP.h"
#include "LinkedList.h"
#include "VehicleQueue.h"
#include "VehicleStack.h"
#include "DoublyLinkedList.h"
#include "CircularLinkedList.h"


struct FindCarResult {
    bool found=false,reachable=false;
    std::string slot,path,steps,message;
    int comparisons=0,edges=0;
};
struct EvacuationRow {
    std::string plate,slot,exitGate,path;
    bool reachable=false;int edges=0;
};
struct CCPResponse {
    bool success=false;
    std::string slot,route,message;
};
struct GroupResponse {
    bool success=false,queued=false,split=false;
    std::string id,message,slots[4];
    int count=0;
};

struct ParkResult {
    bool success = false;
    bool queued = false;
    std::string message;
    Vehicle vehicle;
    std::string explanation;  // Human-readable DSA decision audit.
    int eligibleSlots = 0;
};
struct ExitResult {
    bool success = false;
    bool reassigned = false;
    std::string message;
    std::string freedSlot;
    Vehicle vehicle;
    Vehicle nextVehicle;
};

class ParkingSystem {
private:
    ParkingLot lot;
    traceccp::State ccp;
    void makeCCPGraph(traceccp::RawGraph& output) const;
    bool plateInCCP(const std::string& plate) const;
    void recoverVipReservations(const std::string& from,const std::string& to);
    bool selectConvoySlots(const traceccp::Group& group,int chosen[4],bool& split) const;
    GroupResponse placeConvoy(const traceccp::Group& group,bool queueIfMissing);
    void dispatchGroups();

    ParkingGraph graph;             // Phase 5: road network, BFS/DFS and blocked lanes.
    LinkedList active;                 // Singly linked list: parked vehicles.
    VehicleQueue waiting;             // Queue: waiting vehicles in arrival order.
    VehicleStack exitStack;           // Stack: latest exit on top.
    DoublyLinkedList exitHistory;     // Doubly list: forward/backward history.
    CircularLinkedList slotCircuit;   // Circular list: parking inspection circuit.
    BSTree activeBST;                  // Live parked ticket ID -> vehicle plate index.
    AVLTree activeAVL;                 // Same active records, automatically balanced.
    struct TreeEvent { char op; int key; std::string plate; };
    std::vector<TreeEvent> treeEvents;  // Replays exact tree shape after restarting.
    long long nextTicket;
    long long revenue;
    int served;
    std::filesystem::path dataFile;

    void clearMemory();
    bool validRecord(const Vehicle& vehicle) const;
    std::vector<Vehicle> dispatchWaiting(); // highest eligible priority first, FIFO on ties
    const ParkingSlot* bestReachableSlot(const Vehicle& vehicle) const;
    std::string describeRouteDecision(const Vehicle& vehicle, const ParkingSlot* slot) const;
public:
    explicit ParkingSystem(const std::filesystem::path& file = "data/cpp_state.txt");
    ParkResult park(const std::string& plate, const std::string& type,
                    const std::string& owner, bool vip = false,
                    const std::string& preferredBuilding = "", int preferredFloor = 0);

    FindCarResult findMyCar(const std::string& plate) const;
    bool setEvacuation(char building,bool enabled);
    bool evacuationActive() const {return ccp.evacuation;}
    char evacuationBuilding() const {return ccp.incident;}
    std::vector<EvacuationRow> evacuationRoutes() const;
    CCPResponse reserveVIP(const std::string& plate,const std::string& type,
                           const std::string& owner,const std::string& building="",int floor=0);
    CCPResponse confirmVIP(const std::string& plate);
    std::vector<traceccp::Hold> vipReservations() const;
    const std::string& lastRecovery() const {return ccp.lastRecovery;}
    GroupResponse submitConvoy(traceccp::Group group);
    std::vector<traceccp::Group> groupQueue() const;

    bool configureLayout(int floors, int slotsPerFloor);
    bool setSlotRestriction(const std::string& slotId, int mode, const std::string& note);
    std::string previewAllocation(const std::string& type, bool vip = false,
                                  const std::string& building = "", int floor = 0) const;
    bool setRoadBlocked(const std::string& from, const std::string& to, bool blocked);
    std::vector<RoadEdge> roads() const;
    std::vector<std::string> routeToSlot(const std::string& id) const;
    bool dfsCanReach(const std::string& id) const;
    int closedRoadCount() const;
    int floorsPerBuilding() const;
    int slotsPerFloor() const;
    const std::vector<ParkingSlot>& parkingSlots() const;
    ExitResult exitVehicle(const std::string& plate); // Production: actual exit clock.
    ExitResult exitVehicleAt(const std::string& plate, long long exitEpoch); // Deterministic testing.
    ExitResult exitVehicle(const std::string& plate, int legacyTestHours); // Legacy test adapter, not in UI.
    std::vector<Vehicle> search(const std::string& query) const;
    std::vector<Vehicle> parkedVehicles() const;
    std::vector<Vehicle> waitingVehicles() const;
    std::vector<Vehicle> waitingByPriority() const;
    int eligibleSlotsFor(const Vehicle& candidate) const;
    bool cancelWaiting(const std::string& plate);
    bool changeWaitingVIP(const std::string& plate, bool vip);
    std::vector<Vehicle> exitRecords() const;
    std::vector<Vehicle> historyBackward() const;
    void showParkingMap() const;
    void showSinglyList() const;
    void showDoublyForward() const;
    void showDoublyBackward() const;
    void showCircularList() const;
    void showQueue() const;
    void showStack() const;
    std::string inspectNextSlot();
    int occupied() const;
    int available() const;
    int total() const;
    int waitingCount() const;
    int servedCount() const;
    long long totalRevenue() const;
    bool save() const;
    bool load();
    bool resetWithBackup();
    std::filesystem::path savedFile() const;
    const BSTree& bst() const;
    const AVLTree& avl() const;
    std::optional<Vehicle> findParkedByTicket(int ticketNumber, bool balanced = true) const;
    bool treeIndexesValid() const;
};
#endif
