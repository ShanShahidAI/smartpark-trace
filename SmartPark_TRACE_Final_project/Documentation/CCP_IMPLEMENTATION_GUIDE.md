# SmartPark TRACE — Complex Computing Extension

**University:** Riphah International University, Islamabad  
**Course:** Data Structures and Algorithms  
**Instructor:** Sir Mueed Ahmad  
**Group:** Muhammad Salman (72373), Shaheryar Khan (72775), Shan (72387)

## Architectural contract

SmartPark ships as **two independent applications**:

1. **Standalone C++:** `Standalone_CPP/SmartPark.cpp` is its complete single source. The administrator uses clean-screen console menus. Source compiles using C++17 and creates local data files.
2. **Web:** Flask authenticates/validates requests and calls the compiled C++ command-line bridge. HTML/CSS/JS presents outputs. Existing SQLite history and admin panels remain independent of the standalone C++ files. C++ is the decision engine for all new CCP features.

The new algorithm layer lives in `FullStack_Web/backend/TraceCCP.h`. The website's `ParkingSystem.cpp` calls the implementations from `TraceCCPMethods.inc`. **Both files are embedded directly in the standalone `.cpp`**, so it has no separate runtime source dependencies.

### Fundamental DSAs in the new features (no STL DSA containers)

| Implemented C++ type | Real operation | Raw DSA |
| --- | --- | --- |
| `traceccp::PlateAVL` | Plate lookup, indexed by plate not ticket | Pointer-linked self-balancing AVL tree |
| `traceccp::RawGraph` | Walking/evacuation routes, blocked edges | Fixed raw adjacency matrix; hand-written linear FIFO BFS |
| `traceccp::PathStack` | Reverse BFS predecessor route into forward instructions | Singly-linked LIFO stack |
| `traceccp::RollbackStack` | Restore provisional VIP state after a path closure | Pointer-linked stack |
| `traceccp::GroupQueue` | Keep 3/4 cars together when contiguous capacity unavailable | Linked group nodes with VIP-first dispatch |
| `ParkingSystem::selectConvoySlots()` | Search contiguous compatible slots | Sliding window, raw `taken[240]` and `int chosen[4]` arrays |

**Important design detail:** Pre-existing SmartPark modules (such as the earlier `ParkingLot`, legacy graph interface, and reporting adapters) still have some STL collection types; those legacy files were preserved to avoid breaking the earlier approved features. The *new CCP algorithms themselves* use raw arrays and pointer-based structures. Do not claim the entire legacy project is STL-free.

## Implemented functionality — exact public methods

```cpp
FindCarResult ParkingSystem::findMyCar(const std::string& plate) const;
bool ParkingSystem::setEvacuation(char building, bool enabled);
std::vector<EvacuationRow> ParkingSystem::evacuationRoutes() const; // output adapter
CCPResponse ParkingSystem::reserveVIP(const std::string& plate,
    const std::string& type, const std::string& owner,
    const std::string& building, int floor);
CCPResponse ParkingSystem::confirmVIP(const std::string& plate);
GroupResponse ParkingSystem::submitConvoy(traceccp::Group group);
std::vector<traceccp::Group> ParkingSystem::groupQueue() const; // output adapter
```

Internal functions:

```cpp
void ParkingSystem::makeCCPGraph(traceccp::RawGraph& graph) const;
void ParkingSystem::recoverVipReservations(const std::string& from,
    const std::string& to);
bool ParkingSystem::selectConvoySlots(const traceccp::Group& group,
    int chosen[4], bool& split) const;
GroupResponse ParkingSystem::placeConvoy(const traceccp::Group& group,
    bool queueIfMissing);
void ParkingSystem::dispatchGroups();
```

### 1. Find My Car — zero-level working

1. Staff enters a plate, e.g. `ABC-123`.
2. C++ reads active parking slots and builds a pointer-based AVL tree with number plate as key and slot as value. It uses rotations to keep search balanced.
3. `PlateAVL::findSlot()` locates the slot and counts comparisons.
4. C++ builds a raw graph from the configured roads and introduces an example `PED-GATE` connector.
5. Raw BFS starts at `PED-GATE` and finds a route to the slot, skipping blocked edges. Each discovered vertex has a predecessor stored in a fixed array.
6. A raw linked Stack reverses the predecessor chain; the console/web prints numbered directions.

This is a **fewest-edge walking route**, not shortest distance in metres. Locations are labelled graph nodes rather than measured left/right turns.

### 2. Emergency Evacuation — working

Admin selects Building A/B/C; `setEvacuation()` saves a persistent evacuation flag. Normal vehicle entry returns `NORMAL INBOUND LOCKED`. Existing waiting normal vehicles are not dispatched while this flag is active. Incoming emergency-priority traffic may still be admitted to unaffected buildings under C++ placement rules.

For each *currently parked* car in the affected building, `nearestSafeExit()` searches candidate `EXIT-A/B/C` nodes, **excluding the affected building's own gate**, with raw BFS. If all routes are blocked, the result is explicitly `NO SAFE ROUTE - REQUEST HUMAN ASSISTANCE` instead of inventing a safe route. `evac-stop` restores normal admissions and dispatches eligible waiting groups.

The routes are **software simulations based on staff-configured road state**, not verified real-world emergency guidance.

### 3. Convoy / Group Parking — working

Input: 3 or 4 unique number plates, types, owners, building/floor preferences, and priority.

- Sliding window examines 3 or 4 consecutive slot indices on the same building/floor, ensuring every car fits its slot, all are available/unrestricted, and each road remains reachable.
- If a 4-slot window is unavailable, search two distinct pairs (`2+2`); for 3 cars, search `2+1`.
- Slots are chosen **for every vehicle before any write**. On any unexpected parking/save failure, code removes all partial vehicles and restores the initial ticket counter and tree events (atomic in-memory rollback).
- If allocation is not possible, the entire convoy enters a single linked **Group Queue node**, not four individual tickets. Group priority uses VIP-first, FIFO inside the same priority.
- Reopening suitable slots or ending an evacuation retries queued groups.

### 4. Dynamic rerouting and rollback — working

A VIP can reserve a **provisional hold** using `reserveVIP()`. This is NOT an occupied parking slot and does not issue a ticket yet. Other vehicles cannot be assigned to the held slot. The initial BFS route is stored with the hold.

If `setRoadBlocked()` closes any edge of that stored route, `recoverVipReservations()` pushes the pending hold on a pointer-linked Rollback Stack, releases the old hold, runs the route/slot selection again, and either:

- reinstates the hold with a new BFS route/compatible slot; or
- moves the VIP into the priority waiting queue when no reachable compatible space remains.

After the driver physically arrives, `confirmVIP()` creates the official occupied vehicle record, ticket, active lists and BST/AVL indexes. It cannot confirm into the evacuated building.

### 5. Persistence and compatibility

New state is written under `SMARTPARK_CPP_V6` with `CCP`, `HOLDS`, and `GROUPS` sections (plus the pre-existing parking state). The loader still accepts V1–V5 save files. The website and independent single-file C++ use separate data files; the website also retains its separate SQLite historical reports.

### 6. Complex Computing Problem — copyable proposal paragraph

> SmartPark TRACE addresses a complex computing problem in multi-building parking management because slot suitability, group adjacency, route accessibility, staff reservations, emergency priorities, and traffic restrictions can conflict. We apply raw Data Structures and Algorithms in C++ to coordinate these constraints: a pointer-based AVL tree locates vehicles by plate number; a graph with BFS finds pedestrian and evacuation routes; a stack supports route reconstruction and transactional rollback; a sliding window searches contiguous slots for family convoys; and linked priority group queues preserve fairness when capacity is fragmented. During simulated emergencies, normal incoming traffic is temporarily locked while the engine recalculates viable exits and reports unreachable cases for human intervention. The resulting system improves explainability, prevents duplicate provisional allocation, and maintains recoverable parking state across restarts.

### 7. Honest algorithm complexity

- Raw BFS: `O(V²)` with the current **adjacency matrix** representation (it scans potential neighbours of each dequeued vertex), with `O(V²)` graph storage; future adjacency-list implementation could achieve `O(V+E)`.
- Plate-based AVL lookup: `O(log n)` for the search after index construction; **in the current bridge** the AVL index is rebuilt from active slots for each lookup, costing `O(n log n)` per request in addition to the lookup.
- Sliding window: up to `O(S × G)` for `S` parking slots and group size `G` (G≤4), excluding BFS route checks.
- Rollback stack push/pop: `O(1)`.
- Linked group queue insertion: `O(1)`; dispatch may examine multiple groups and search candidate windows.

### 8. Validation

`Testing/run_ccp_tests.py` checks 18 scenarios: plate search, route closure, VIP hold uniqueness, road rerouting, priority queue fallback, contiguous convoy, split convoy, group queue persistence/auto-dispatch, evacuation admission lock, safe exit selection, unreachable exits, confirmation restrictions, legacy V5 migration, and exit/billing regression.

`Testing/browser_smoke.py` performs isolated desktop/mobile interface checks using intercepted JSON. It **does not** replace a real Flask-backed Windows integration test.

## Caveats for the viva

- Evacuation mode cannot sense an actual fire or guarantee human safety. All road blocks are manually defined by staff; physical evacuation instructions always take precedence.
- The graph uses illustrative campus nodes; it does not calculate walking metres or turn-by-turn compass directions.
- Provisional VIP holds differ from parked vehicles. Only `confirmVIP()` starts actual parking time and billing.
- The existing standalone admin credential mechanism and the website's authenticated session mechanism are separate by design.
