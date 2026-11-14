#include <fstream>
#include <iomanip>
#include <cstdlib>
// SMARTPARK - SINGLE FILE C++17 EDITION
// Riphah International University, Islamabad
// Save as SmartPark.cpp and compile with /std:c++17 in Visual Studio.
// All classes, structs, DSA operations and menu functions are defined in this file.



#ifndef SMARTPARK_PARKING_GRAPH_H
#define SMARTPARK_PARKING_GRAPH_H
// Graph vertices: ENTRY, G-A/B/C (building gates), A-F1 (floor ramp), A-F1-S01 (slots).
// Roads are undirected edges; BFS finds the fewest-edge route, not shortest metres.
// DFS checks reachability independently. Admin can close/reopen EXISTING edges.
#include <algorithm>
#include <iomanip>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

struct RoadEdge {
    std::string from;
    std::string to;
    bool blocked = false;
};

class ParkingGraph {
    std::map<std::string, std::vector<std::string>> adjacency;
    std::set<std::pair<std::string,std::string>> closed;
    static std::pair<std::string,std::string> edgeKey(std::string a,std::string b) {
        if (b<a) std::swap(a,b);
        return {a,b};
    }
    void connect(const std::string& a,const std::string& b) {
        adjacency[a].push_back(b);
        adjacency[b].push_back(a);
    }
public:
    void rebuild(int floors,int eachFloor) {
        adjacency.clear(); closed.clear();
        adjacency["ENTRY"];
        for(char b='A';b<='C';++b) {
            std::string building(1,b);
            connect("ENTRY","G-"+building);
            connect("G-"+building,building+"-F1");
            for(int floor=1;floor<=floors;++floor) {
                std::string ramp=building+"-F"+std::to_string(floor);
                if(floor>1)connect(building+"-F"+std::to_string(floor-1),ramp);
                for(int slot=1;slot<=eachFloor;++slot) {
                    std::ostringstream name;
                    name<<ramp<<"-S"<<std::setfill('0')<<std::setw(2)<<slot;
                    connect(ramp,name.str());
                }
            }
        }
        // Inter-building service roads enable genuine alternative paths if one access lane closes.
        connect("G-A","G-B");
        connect("G-B","G-C");
        for(auto& node : adjacency) std::sort(node.second.begin(),node.second.end());
    }
    bool hasRoad(const std::string& from,const std::string& to) const {
        auto it=adjacency.find(from);
        return it!=adjacency.end() && std::find(it->second.begin(),it->second.end(),to)!=it->second.end();
    }
    bool setBlocked(const std::string& from,const std::string& to,bool blocked) {
        if (from==to || !hasRoad(from,to)) return false;
        auto key=edgeKey(from,to);
        if(blocked) closed.insert(key); else closed.erase(key);
        return true;
    }
    bool isBlocked(const std::string& from,const std::string& to) const {
        return closed.count(edgeKey(from,to))!=0;
    }
    std::vector<RoadEdge> roads() const {
        std::vector<RoadEdge> result;
        for(const auto& node:adjacency)for(const auto& neighbor:node.second) {
            if(node.first<neighbor)result.push_back({node.first,neighbor,isBlocked(node.first,neighbor)});
        }
        return result;
    }
    std::vector<std::string> nodes() const {
        std::vector<std::string> result;
        for(const auto& node:adjacency)result.push_back(node.first);
        return result;
    }
    std::vector<std::string> bfs(const std::string& start,const std::string& target) const {
        if(!adjacency.count(start)||!adjacency.count(target))return {};
        std::queue<std::string> toVisit;
        std::set<std::string> seen;
        std::map<std::string,std::string> parent;
        seen.insert(start);toVisit.push(start);
        while(!toVisit.empty()) {
            std::string current=toVisit.front();toVisit.pop();
            if(current==target) {
                std::vector<std::string> path;
                for(std::string at=target;;at=parent.at(at)) {
                    path.push_back(at); if(at==start)break;
                }
                std::reverse(path.begin(),path.end());
                return path;
            }
            for(const auto& next:adjacency.at(current)) {
                if(isBlocked(current,next)||!seen.insert(next).second)continue;
                parent[next]=current;toVisit.push(next);
            }
        }
        return {};
    }
    bool dfsReachable(const std::string& start,const std::string& target) const {
        if(!adjacency.count(start)||!adjacency.count(target))return false;
        std::vector<std::string> pending{start};
        std::set<std::string> seen;
        while(!pending.empty()) {
            std::string at=pending.back();pending.pop_back();
            if(!seen.insert(at).second)continue;
            if(at==target)return true;
            for(const auto& next:adjacency.at(at))
                if(!isBlocked(at,next) && !seen.count(next))pending.push_back(next);
        }
        return false;
    }
    std::vector<std::string> pathTo(const std::string& slotId) const {return bfs("ENTRY",slotId);}
    int closedCount() const {return static_cast<int>(closed.size());}
};
#endif

// ============ HEADER: Vehicle ============
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



// ============ HEADER: ParkingSlot ============
#ifndef PARKING_SLOT_H
#define PARKING_SLOT_H

using namespace std;

class ParkingSlot {
public:
    string slotNumber;
    string building;
    int floor = 0;
    int capacity = 2; // Small=1 / Medium=2 / Large=3
    bool occupied;
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



// ============ HEADER: ParkingLot ============
#ifndef PARKING_LOT_H
#define PARKING_LOT_H
#include <vector>

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



// ============ HEADER: LinkedList ============
#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <vector>
class LinkedList {
private:
    struct Node {
        Vehicle data;
        Node* next;
        explicit Node(const Vehicle& v) : data(v), next(nullptr) {}
    };
    Node* head;
    int count;
public:
    LinkedList();
    ~LinkedList();
    LinkedList(const LinkedList&) = delete;
    LinkedList& operator=(const LinkedList&) = delete;
    void insert(const Vehicle& vehicle);
    bool remove(string vehicleNumber);
    Vehicle* search(string vehicleNumber);
    int size() const;
    bool isEmpty() const;
    void display() const;
    std::vector<Vehicle> toVector() const;
    void clear();
};
#endif



// ============ HEADER: VehicleQueue ============
#ifndef VEHICLE_QUEUE_H
#define VEHICLE_QUEUE_H

#include <vector>
class VehicleQueue {
private:
    struct Node {
        Vehicle data;
        Node* next;
        explicit Node(const Vehicle& v) : data(v), next(nullptr) {}
    };
    Node *frontNode, *rearNode;
    int count;
public:
    VehicleQueue();
    ~VehicleQueue();
    VehicleQueue(const VehicleQueue&) = delete;
    VehicleQueue& operator=(const VehicleQueue&) = delete;
    void enqueue(const Vehicle& vehicle);
    Vehicle dequeue(); // Standard FIFO example.
    Vehicle dequeueHighestPriority(); // Emergency > VIP > normal, FIFO among equals.
    Vehicle dequeueEligible(int capacity, const string& building, int floor, int restriction = 0);
    bool removeNumber(const string& plate);
    Vehicle* findNumber(const string& plate);
    Vehicle* front();
    bool containsNumber(const string& number) const;
    bool isEmpty() const;
    int size() const;
    void display() const;
    std::vector<Vehicle> toVector() const;
    void clear();
};
#endif



// ============ HEADER: VehicleStack ============
#ifndef VEHICLE_STACK_H
#define VEHICLE_STACK_H

#include <vector>
class VehicleStack {
private:
    struct Node {
        Vehicle data;
        Node* next;
        explicit Node(const Vehicle& v) : data(v), next(nullptr) {}
    };
    Node* topNode;
    int count;
public:
    VehicleStack();
    ~VehicleStack();
    VehicleStack(const VehicleStack&) = delete;
    VehicleStack& operator=(const VehicleStack&) = delete;
    void push(const Vehicle& vehicle);
    Vehicle pop();
    Vehicle* peek();
    bool isEmpty() const;
    int size() const;
    void display() const;
    std::vector<Vehicle> toVector() const; // Newest to oldest.
    void clear();
};
#endif



// ============ HEADER: DoublyLinkedList ============
#ifndef DOUBLY_LINKED_LIST_H
#define DOUBLY_LINKED_LIST_H

#include <vector>
class DoublyLinkedList {
private:
    struct Node {
        Vehicle data;
        Node* prev;
        Node* next;
        explicit Node(const Vehicle& v) : data(v), prev(nullptr), next(nullptr) {}
    };
    Node* head;
    Node* tail;
    int count;
public:
    DoublyLinkedList();
    ~DoublyLinkedList();
    DoublyLinkedList(const DoublyLinkedList&) = delete;
    DoublyLinkedList& operator=(const DoublyLinkedList&) = delete;
    void pushFront(const Vehicle& vehicle);
    std::vector<Vehicle> forward() const;
    std::vector<Vehicle> backward() const;
    void displayForward() const;
    void displayBackward() const;
    int size() const;
    void clear();
};
#endif



// ============ HEADER: CircularLinkedList ============
#ifndef CIRCULAR_LINKED_LIST_H
#define CIRCULAR_LINKED_LIST_H
#include <string>
#include <vector>
class CircularLinkedList {
private:
    struct Node {
        std::string slotId;
        Node* next;
        explicit Node(const std::string& id) : slotId(id), next(nullptr) {}
    };
    Node* head;
    Node* tail;
    Node* cursor;
    int count;
public:
    CircularLinkedList();
    ~CircularLinkedList();
    CircularLinkedList(const CircularLinkedList&) = delete;
    CircularLinkedList& operator=(const CircularLinkedList&) = delete;
    void append(const std::string& slotId);
    std::string inspectNext(); // Returns next slot in a repeating round-robin circle.
    std::vector<std::string> toVector() const;
    void display() const;
    int size() const;
    void clear();
};
#endif



// ============ HEADER: DynamicVehicleArray ============
#ifndef DYNAMIC_VEHICLE_ARRAY_H
#define DYNAMIC_VEHICLE_ARRAY_H

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



// ============ HEADER: BST ============
#ifndef SMARTPARK_BST_H
#define SMARTPARK_BST_H

#include <optional>
#include <string>
#include <vector>
#include <iosfwd>

struct TreeNodeInfo {
    int data;
    std::string vehicleNumber;
    int height;
    int balanceFactor;
    bool hasLeft;
    int left;
    bool hasRight;
    int right;
};

// Straightforward pointer-based BST: the same recursive approach used in class.
// Actual search path follows pointer links, not a generated/simulated tree.
struct TreeSearchTrace {
    bool found = false;
    std::string vehicleNumber;
    std::vector<int> visited; // one key per node comparison
};

struct BSTNode {
    int data;
    std::string vehicleNumber;
    BSTNode* left;
    BSTNode* right;
};

class BSTree {
private:
    BSTNode* root;
    static BSTNode* CreateNode(int data, const std::string& plate);
    static BSTNode* Insert(BSTNode* root, int data, const std::string& plate, bool& inserted);
    static BSTNode* Delete(BSTNode* root, int data, bool& deleted);
    static BSTNode* FindMin(BSTNode* root);
    static const BSTNode* Search(const BSTNode* root, int data);
    static void Inorder(const BSTNode* root, std::vector<int>& output);
    static void Preorder(const BSTNode* root, std::vector<int>& output);
    static void Postorder(const BSTNode* root, std::vector<int>& output);
    static int Height(const BSTNode* root);
    static int CountNodes(const BSTNode* root);
    static int CountLeafNodes(const BSTNode* root);
    static void Clear(BSTNode* root);
    static void Print(const BSTNode* root, std::ostream& out, int depth);
    static void Snapshot(const BSTNode* root, std::vector<TreeNodeInfo>& result);
    static bool Verify(const BSTNode* root, long long minExclusive, long long maxExclusive);
public:
    BSTree();
    ~BSTree();
    BSTree(const BSTree&) = delete;
    BSTree& operator=(const BSTree&) = delete;
    bool insert(int data, const std::string& plate = "");
    bool remove(int data);
    bool search(int data) const;
    TreeSearchTrace traceSearch(int data) const;
    std::optional<std::string> vehicleFor(int data) const;
    std::vector<int> inorder() const;
    std::vector<int> preorder() const;
    std::vector<int> postorder() const;
    std::optional<int> findMin() const;
    std::optional<int> findMax() const;
    int height() const;
    int countNodes() const;
    int countLeafNodes() const;
    std::vector<TreeNodeInfo> snapshot() const;
    void display(std::ostream& out) const;
    bool verify() const;
    void clear();
};
#endif



// ============ HEADER: AVLTree ============
#ifndef SMARTPARK_AVLTREE_H
#define SMARTPARK_AVLTREE_H

#include <optional>
#include <string>
#include <vector>
#include <iosfwd>

struct AVLNode {
    int data;
    std::string vehicleNumber;
    int height;
    AVLNode* left;
    AVLNode* right;
};

class AVLTree {
private:
    AVLNode* root;
    std::vector<std::string> rotationLog;
    static AVLNode* CreateNode(int data, const std::string& plate);
    static int Height(const AVLNode* node);
    static int BalanceFactor(const AVLNode* node);
    static void UpdateHeight(AVLNode* node);
    AVLNode* RightRotate(AVLNode* node);
    AVLNode* LeftRotate(AVLNode* node);
    AVLNode* Rebalance(AVLNode* node);
    AVLNode* Insert(AVLNode* node, int data, const std::string& plate, bool& inserted);
    AVLNode* Delete(AVLNode* node, int data, bool& deleted);
    static const AVLNode* Search(const AVLNode* node, int data);
    static const AVLNode* FindMin(const AVLNode* node);
    static void Inorder(const AVLNode* node, std::vector<int>& output);
    static void Preorder(const AVLNode* node, std::vector<int>& output);
    static void Postorder(const AVLNode* node, std::vector<int>& output);
    static int CountNodes(const AVLNode* node);
    static int CountLeafNodes(const AVLNode* node);
    static void Clear(AVLNode* node);
    static void Print(const AVLNode* node, std::ostream& out, int depth);
    static void Snapshot(const AVLNode* node, std::vector<TreeNodeInfo>& result);
    static bool Verify(const AVLNode* node, long long low, long long high, int& actualHeight);
public:
    AVLTree();
    ~AVLTree();
    AVLTree(const AVLTree&) = delete;
    AVLTree& operator=(const AVLTree&) = delete;
    bool insert(int data, const std::string& plate = "");
    bool remove(int data);
    bool search(int data) const;
    TreeSearchTrace traceSearch(int data) const;
    std::optional<std::string> vehicleFor(int data) const;
    std::vector<int> inorder() const;
    std::vector<int> preorder() const;
    std::vector<int> postorder() const;
    std::optional<int> findMin() const;
    std::optional<int> findMax() const;
    int height() const;
    int countNodes() const;
    int countLeafNodes() const;
    std::vector<TreeNodeInfo> snapshot() const;
    const std::vector<std::string>& lastRotations() const;
    void display(std::ostream& out) const;
    bool verify() const;
    void clear();
};
#endif



// ============ HEADER: FeeCalculator ============
#ifndef FEE_CALCULATOR_H
#define FEE_CALCULATOR_H
#include <string>
using namespace std;
double calculateParkingFee(const string& vehicleType,int hours);
double hourlyParkingRate(const string& vehicleType);
#endif



// ============ HEADER: PriorityQueue ============
#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <vector>
using namespace std;

class PriorityQueue {
private:
    vector<Vehicle> items;
    int priorityValue(const Vehicle& v) const;
public:
    void insert(const Vehicle& v);
    Vehicle removeHighestPriority();
    Vehicle* peek();
    bool isEmpty() const;
    int size() const;
    void display() const;
};
#endif



// ============ HEADER: Search ============
#ifndef SEARCH_H
#define SEARCH_H
#include <vector>

using namespace std;
int linearSearchByNumber(const vector<Vehicle>& v, const string& number);
int linearSearchByOwner(const vector<Vehicle>& v, const string& owner);
int linearSearchBySlot(const vector<Vehicle>& v, const string& slot);
// Binary search needs a list sorted by vehicle number first.
int binarySearchByNumber(const vector<Vehicle>& sortedVehicles, const string& number);
#endif



// ============ HEADER: Sort ============
#ifndef SORT_H
#define SORT_H
#include <vector>

using namespace std;
void bubbleSortByNumber(vector<Vehicle>& v);
void bubbleSortByFee(vector<Vehicle>& v);
void selectionSortByNumber(vector<Vehicle>& v);
void selectionSortByFee(vector<Vehicle>& v);
#endif



// ============ RAW CCP DSA: Graph / AVL / Stack / Group Queue ============
#ifndef SMARTPARK_TRACE_CCP_H
#define SMARTPARK_TRACE_CCP_H
// Core CCP algorithms: only fixed raw arrays, pointers, singly-linked nodes,
// a hand-written FIFO queue, stack, AVL tree and sliding-window traversal.
// std::string is used only to store labels/plates; no STL DSA containers.
#include <string>
#include <ostream>
#include <istream>
#include <iomanip>
#include <cctype>
#include <algorithm>
namespace traceccp {
constexpr int MAX_VERTICES = 360;
constexpr int MAX_HOLDS = 64;
constexpr int MAX_GROUP_SIZE = 4;
struct Path {
    std::string at[MAX_VERTICES];
    int count = 0;
    bool found() const { return count > 0; }
    std::string text() const {
        std::string result;
        for (int i=0;i<count;++i) { if(i) result += " -> "; result += at[i]; }
        return result;
    }
};
class PathStack {
    struct Node {std::string value; Node* next; Node(const std::string& v,Node* n):value(v),next(n){} };
    Node* topNode=nullptr;
public:
    ~PathStack(){std::string ignored;while(pop(ignored)){} }
    void push(const std::string& s) { topNode=new Node(s,topNode); }
    bool pop(std::string& s) {
        if(!topNode)return false;
        Node* old=topNode; s=old->value;topNode=old->next;delete old;return true;
    }
};
// Matrix is a raw array. An edge may exist yet be temporarily blocked.
class RawGraph {
    std::string name[MAX_VERTICES];
    bool edge[MAX_VERTICES][MAX_VERTICES]{};
    bool blocked[MAX_VERTICES][MAX_VERTICES]{};
    int used=0;
    int find(const std::string& s) const {
        for(int i=0;i<used;++i)if(name[i]==s)return i;
        return -1;
    }
    int ensure(const std::string& s){
        int id=find(s); if(id>=0)return id;
        if(used>=MAX_VERTICES)return -1;
        name[used]=s;return used++;
    }
public:
    bool add(const std::string& a,const std::string& b,bool isClosed=false){
        int i=ensure(a),j=ensure(b);if(i<0||j<0||i==j)return false;
        edge[i][j]=edge[j][i]=true;
        blocked[i][j]=blocked[j][i]=isClosed;return true;
    }
    bool has(const std::string& a,const std::string& b) const {
        int i=find(a),j=find(b);return i>=0&&j>=0&&edge[i][j];
    }
    bool isClosed(const std::string& a,const std::string& b) const {
        int i=find(a),j=find(b);return i<0||j<0||!edge[i][j]||blocked[i][j];
    }
    Path bfs(const std::string& start,const std::string& destination) const {
        Path route;
        int src=find(start),dst=find(destination);
        if(src<0||dst<0)return route;
        bool seen[MAX_VERTICES]{};
        int parent[MAX_VERTICES],fifo[MAX_VERTICES],head=0,tail=0;
        for(int i=0;i<used;++i)parent[i]=-1;
        fifo[tail++]=src;seen[src]=true;
        while(head<tail){
            int curr=fifo[head++];if(curr==dst)break;
            for(int i=0;i<used;++i){
                if(edge[curr][i]&&!blocked[curr][i]&&!seen[i]){
                    seen[i]=true;parent[i]=curr;fifo[tail++]=i;
                }
            }
        }
        if(!seen[dst])return route;
        // Backtrack produces destination-first order. Linked stack reverses it.
        PathStack reverse;
        for(int n=dst;n!=-1;n=parent[n]) reverse.push(name[n]);
        std::string step;
        while(reverse.pop(step)&&route.count<MAX_VERTICES)route.at[route.count++]=step;
        return route;
    }
    Path nearestSafeExit(const std::string& start,char incidentBuilding,std::string& gate) const {
        Path best;
        gate.clear();
        for(char b='A';b<='C';++b) {
            if(b==incidentBuilding)continue; // affected building exit is treated as unsafe
            std::string exit=std::string("EXIT-")+b;
            Path candidate=bfs(start,exit);
            if(candidate.found()&&(!best.found()||candidate.count<best.count)){
                best=candidate;gate=exit;
            }
        }
        return best;
    }
};
class PlateAVL { // Raw pointer AVL by number plate, not numeric ticket.
    struct Node {
        std::string plate,slot;
        Node *left,*right;int height;
        Node(const std::string& p,const std::string& s):plate(p),slot(s),left(nullptr),right(nullptr),height(1){}
    };
    Node* root=nullptr;
    static int height(Node* n){return n?n->height:0;}
    static int max2(int a,int b){return a>b?a:b;}
    static void update(Node* n){n->height=1+max2(height(n->left),height(n->right));}
    static Node* right(Node* y){Node*x=y->left;Node*t=x->right;x->right=y;y->left=t;update(y);update(x);return x;}
    static Node* left(Node* x){Node*y=x->right;Node*t=y->left;y->left=x;x->right=t;update(x);update(y);return y;}
    static Node* put(Node* n,const std::string& p,const std::string& s){
        if(!n)return new Node(p,s);
        if(p<n->plate)n->left=put(n->left,p,s);
        else if(p>n->plate)n->right=put(n->right,p,s);
        else {n->slot=s;return n;}
        update(n);int b=height(n->left)-height(n->right);
        if(b>1&&p<n->left->plate)return right(n);
        if(b< -1&&p>n->right->plate)return left(n);
        if(b>1&&p>n->left->plate){n->left=left(n->left);return right(n);}
        if(b< -1&&p<n->right->plate){n->right=right(n->right);return left(n);}
        return n;
    }
    static void clear(Node*n){if(!n)return;clear(n->left);clear(n->right);delete n;}
public:
    PlateAVL()=default;
    PlateAVL(const PlateAVL&)=delete;
    PlateAVL&operator=(const PlateAVL&)=delete;
    ~PlateAVL(){clear(root);}
    void insert(const std::string& plate,const std::string& slot){root=put(root,plate,slot);}
    std::string findSlot(const std::string& plate,int& comparisons) const {
        comparisons=0;Node* n=root;
        while(n){++comparisons;if(n->plate==plate)return n->slot;n=plate<n->plate?n->left:n->right;}
        return "";
    }
};
struct Hold {
    std::string plate,type,owner,building,slot,path;
    int floor=0;
    bool active=false;
};
class RollbackStack {
    struct Node{Hold item;Node*next;Node(const Hold&h,Node*n):item(h),next(n){}};
    Node*top=nullptr;
public:
    ~RollbackStack(){Hold h;while(pop(h)){} }
    void push(const Hold&h){top=new Node(h,top);}
    bool pop(Hold&h){if(!top)return false;Node*n=top;h=n->item;top=n->next;delete n;return true;}
};
struct Group {
    std::string id,building;
    std::string plate[MAX_GROUP_SIZE],owner[MAX_GROUP_SIZE],type[MAX_GROUP_SIZE];
    int count=0,floor=0;bool vip=false;
};
class GroupQueue {
public:
    struct Node {Group group;Node*next;explicit Node(const Group&g):group(g),next(nullptr){}};
private:
    Node *head=nullptr,*tail=nullptr;int length=0;
public:
    GroupQueue()=default;
    GroupQueue(const GroupQueue&)=delete;
    GroupQueue&operator=(const GroupQueue&)=delete;
    ~GroupQueue(){clear();}
    const Node* first()const{return head;}
    int size() const{return length;}
    bool hasPlate(const std::string& plate)const{
        for(Node*n=head;n;n=n->next)for(int i=0;i<n->group.count;++i)
            if(n->group.plate[i]==plate)return true;
        return false;
    }
    bool hasId(const std::string& id)const{
        for(Node*n=head;n;n=n->next)if(n->group.id==id)return true;
        return false;
    }
    void push(const Group&g){Node*n=new Node(g);if(tail)tail->next=n;else head=n;tail=n;++length;}
    bool erase(const std::string& id){
        Node*prev=nullptr;
        for(Node*n=head;n;prev=n,n=n->next)if(n->group.id==id){
            if(prev)prev->next=n->next;else head=n->next;
            if(tail==n)tail=prev;
            delete n;--length;return true;
        }
        return false;
    }
    void clear(){while(head){Node*n=head;head=head->next;delete n;}tail=nullptr;length=0;}
};
class State {
public:
    bool evacuation=false;
    char incident='-';
    Hold holds[MAX_HOLDS];
    GroupQueue groups;
    RollbackStack undo;
    std::string lastRecovery;
    long long nextGroupId=1;
    State()=default;
    State(const State&)=delete;
    State&operator=(const State&)=delete;
    void clear(){evacuation=false;incident='-';nextGroupId=1;lastRecovery.clear();for(auto&h:holds)h=Hold();groups.clear();}
    bool heldSlot(const std::string&slot)const{
        for(const auto&h:holds)if(h.active&&h.slot==slot)return true;
        return false;
    }
    bool heldPlate(const std::string&plate)const{
        for(const auto&h:holds)if(h.active&&h.plate==plate)return true;
        return false;
    }
    int holdCount()const{int n=0;for(const auto&h:holds)if(h.active)++n;return n;}
    Hold* findHold(const std::string&plate){for(auto&h:holds)if(h.active&&h.plate==plate)return &h;return nullptr;}
    bool addHold(const Hold&h){
        if(heldPlate(h.plate)||heldSlot(h.slot))return false;
        for(auto&item:holds)if(!item.active){item=h;item.active=true;return true;}
        return false;
    }
    void removeHold(const std::string&plate){for(auto&h:holds)if(h.active&&h.plate==plate){h=Hold();return;}}
    bool pathUses(const std::string&serialized,const std::string&a,const std::string&b)const{
        if(serialized.empty())return false;
        std::string previous;int start=0;
        for(size_t i=0;i<=serialized.size();++i)if(i==serialized.size()||serialized[i]=='|'){
            std::string current=serialized.substr(start,i-start);
            if((previous==a&&current==b)||(previous==b&&current==a))return true;
            previous=current;start=(int)i+1;
        }
        return false;
    }
    static std::string packPath(const Path&p){
        std::string s;for(int i=0;i<p.count;++i){if(i)s+='|';s+=p.at[i];}return s;
    }
    bool write(std::ostream&out)const{
        out<<"CCP "<<(evacuation?1:0)<<' '<<incident<<' '<<nextGroupId<<' '<<std::quoted(lastRecovery)<<'\n';
        out<<"HOLDS "<<holdCount()<<'\n';
        for(const auto&h:holds)if(h.active)out<<std::quoted(h.plate)<<' '<<std::quoted(h.type)<<' '<<std::quoted(h.owner)
            <<' '<<std::quoted(h.building)<<' '<<h.floor<<' '<<std::quoted(h.slot)<<' '<<std::quoted(h.path)<<'\n';
        out<<"GROUPS "<<groups.size()<<'\n';
        for(auto*n=groups.first();n;n=n->next){
            const Group&g=n->group;
            out<<std::quoted(g.id)<<' '<<std::quoted(g.building)<<' '<<g.floor<<' '<<g.vip<<' '<<g.count;
            for(int i=0;i<g.count;++i)out<<' '<<std::quoted(g.plate[i])<<' '<<std::quoted(g.type[i])<<' '<<std::quoted(g.owner[i]);
            out<<'\n';
        }
        return out.good();
    }
};
} // traceccp
#endif

// ============ HEADER: ParkingSystem ============
#ifndef PARKING_SYSTEM_H
#define PARKING_SYSTEM_H
#include <filesystem>
#include <string>
#include <vector>
#include <optional>










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

    ParkingGraph graph;             // Graph: road network, BFS/DFS and blocked lanes.
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



// ============ MODULE: Vehicle ============

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



// ============ MODULE: ParkingSlot ============

#include <iostream>
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



// ============ MODULE: ParkingLot ============

#include <iostream>
#include <iomanip>
#include <tuple>
using namespace std;

ParkingLot::ParkingLot(int numberOfSlots) {
    (void)numberOfSlots; // New building-wise layout replaces the old flat P01-P12 map.
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



// ============ MODULE: LinkedList ============

#include <iostream>
using namespace std;
LinkedList::LinkedList() : head(nullptr), count(0) {}
LinkedList::~LinkedList() { clear(); }
void LinkedList::insert(const Vehicle& vehicle) {
    Node* n = new Node(vehicle);
    if (!head) head = n;
    else { Node* cur = head; while (cur->next) cur = cur->next; cur->next = n; }
    ++count;
}
bool LinkedList::remove(string number) {
    if (!head) return false;
    if (head->data.vehicleNumber == number) {
        Node* old = head; head = head->next; delete old; --count; return true;
    }
    Node* cur = head;
    while (cur->next) {
        if (cur->next->data.vehicleNumber == number) {
            Node* old = cur->next; cur->next = old->next;
            delete old; --count; return true;
        }
        cur = cur->next;
    }
    return false;
}
Vehicle* LinkedList::search(string number) {
    for (Node* cur = head; cur; cur = cur->next)
        if (cur->data.vehicleNumber == number) return &cur->data;
    return nullptr;
}
int LinkedList::size() const { return count; }
bool LinkedList::isEmpty() const { return head == nullptr; }
void LinkedList::display() const {
    cout << "HEAD -> ";
    for (Node* cur = head; cur; cur = cur->next) {
        cout << "[" << cur->data.vehicleNumber << "]";
        if (cur->next) cout << " -> ";
    }
    cout << " -> NULL\n";
}
vector<Vehicle> LinkedList::toVector() const {
    vector<Vehicle> records;
    for (Node* cur = head; cur; cur = cur->next) records.push_back(cur->data);
    return records;
}
void LinkedList::clear() {
    while (head) { Node* old = head; head = head->next; delete old; }
    count = 0;
}



// ============ MODULE: VehicleQueue ============

#include <iostream>
using namespace std;
namespace {
int priorityOf(const Vehicle& v) {
    if (v.vehicleType == "Emergency") return 3;
    return v.priority ? 2 : 1;
}
}
VehicleQueue::VehicleQueue() : frontNode(nullptr), rearNode(nullptr), count(0) {}
VehicleQueue::~VehicleQueue() { clear(); }
void VehicleQueue::enqueue(const Vehicle& vehicle) {
    Node* n = new Node(vehicle);
    if (!rearNode) frontNode = rearNode = n;
    else { rearNode->next = n; rearNode = n; }
    ++count;
}
Vehicle VehicleQueue::dequeue() {
    if (!frontNode) return Vehicle();
    Node* old = frontNode;
    Vehicle v = old->data;
    frontNode = old->next;
    if (!frontNode) rearNode = nullptr;
    delete old;
    --count;
    return v;
}
Vehicle VehicleQueue::dequeueHighestPriority() {
    if (!frontNode) return Vehicle();
    Node* best = frontNode;
    Node* beforeBest = nullptr;
    Node* previous = frontNode;
    // Strict > comparison retains the earliest arrival for equal priority.
    for (Node* cur = frontNode->next; cur; cur = cur->next) {
        if (priorityOf(cur->data) > priorityOf(best->data)) {
            best = cur;
            beforeBest = previous;
        }
        previous = cur;
    }
    if (!beforeBest) return dequeue();
    beforeBest->next = best->next;
    if (best == rearNode) rearNode = beforeBest;
    Vehicle v = best->data;
    delete best;
    --count;
    return v;
}
Vehicle VehicleQueue::dequeueEligible(int capacity, const string& building, int floor, int restriction) {
    Node* best = nullptr;
    Node* beforeBest = nullptr;
    Node* previous = nullptr;
    for (Node* current = frontNode; current; previous = current, current = current->next) {
        const Vehicle& v = current->data;
        bool eligible = restriction != 2 &&
            (restriction != 1 || v.priority || v.vehicleType == "Emergency") &&
            requiredVehicleSize(v.vehicleType) <= capacity &&
            (v.preferredBuilding.empty() || v.preferredBuilding == building) &&
            (v.preferredFloor == 0 || v.preferredFloor == floor);
        if (eligible && (!best || priorityOf(v) > priorityOf(best->data))) {
            best = current; beforeBest = previous;
        }
    }
    if (!best) return Vehicle();
    if (!beforeBest) return dequeue();
    beforeBest->next = best->next;
    if (best == rearNode) rearNode = beforeBest;
    Vehicle result = best->data;
    delete best; --count;
    return result;
}
bool VehicleQueue::removeNumber(const string& plate) {
    Node* previous = nullptr;
    for (Node* current = frontNode; current; previous = current, current = current->next) {
        if (current->data.vehicleNumber != plate) continue;
        if (previous) previous->next = current->next;
        else frontNode = current->next;
        if (rearNode == current) rearNode = previous;
        delete current;
        --count;
        return true;
    }
    return false;
}
Vehicle* VehicleQueue::findNumber(const string& plate) {
    for (Node* cur=frontNode; cur; cur=cur->next)
        if (cur->data.vehicleNumber == plate) return &cur->data;
    return nullptr;
}
Vehicle* VehicleQueue::front() { return frontNode ? &frontNode->data : nullptr; }
bool VehicleQueue::containsNumber(const string& number) const {
    for (Node* cur = frontNode; cur; cur = cur->next)
        if (cur->data.vehicleNumber == number) return true;
    return false;
}
bool VehicleQueue::isEmpty() const { return !frontNode; }
int VehicleQueue::size() const { return count; }
void VehicleQueue::display() const {
    cout << "FRONT -> ";
    for (Node* cur = frontNode; cur; cur = cur->next) {
        cout << "[" << cur->data.vehicleNumber << "]";
        if (cur->next) cout << " -> ";
    }
    cout << " <- REAR\n";
}
vector<Vehicle> VehicleQueue::toVector() const {
    vector<Vehicle> rows;
    for (Node* cur = frontNode; cur; cur = cur->next) rows.push_back(cur->data);
    return rows;
}
void VehicleQueue::clear() { while (frontNode) dequeue(); }



// ============ MODULE: VehicleStack ============

#include <iostream>
using namespace std;
VehicleStack::VehicleStack() : topNode(nullptr), count(0) {}
VehicleStack::~VehicleStack() { clear(); }
void VehicleStack::push(const Vehicle& vehicle) {
    Node* n = new Node(vehicle);
    n->next = topNode; topNode = n; ++count;
}
Vehicle VehicleStack::pop() {
    if (!topNode) return Vehicle();
    Node* old = topNode;
    Vehicle v = old->data;
    topNode = old->next;
    delete old; --count; return v;
}
Vehicle* VehicleStack::peek() { return topNode ? &topNode->data : nullptr; }
bool VehicleStack::isEmpty() const { return topNode == nullptr; }
int VehicleStack::size() const { return count; }
void VehicleStack::display() const {
    cout << "TOP (latest exit)\n";
    for (Node* cur = topNode; cur; cur = cur->next)
        cout << "  [" << cur->data.vehicleNumber << "] Rs." << cur->data.parkingFee << "\n";
    cout << "BOTTOM\n";
}
vector<Vehicle> VehicleStack::toVector() const {
    vector<Vehicle> rows;
    for (Node* cur = topNode; cur; cur = cur->next) rows.push_back(cur->data);
    return rows;
}
void VehicleStack::clear() { while (topNode) pop(); }



// ============ MODULE: DoublyLinkedList ============

#include <iostream>
using namespace std;
DoublyLinkedList::DoublyLinkedList() : head(nullptr), tail(nullptr), count(0) {}
DoublyLinkedList::~DoublyLinkedList() { clear(); }
void DoublyLinkedList::pushFront(const Vehicle& v) {
    Node* n = new Node(v);
    n->next = head;
    if (head) head->prev = n;
    else tail = n;
    head = n;
    ++count;
}
vector<Vehicle> DoublyLinkedList::forward() const {
    vector<Vehicle> rows;
    for (Node* cur = head; cur; cur = cur->next) rows.push_back(cur->data);
    return rows;
}
vector<Vehicle> DoublyLinkedList::backward() const {
    vector<Vehicle> rows;
    for (Node* cur = tail; cur; cur = cur->prev) rows.push_back(cur->data);
    return rows;
}
void DoublyLinkedList::displayForward() const {
    cout << "NULL <-> ";
    for (Node* cur = head; cur; cur = cur->next) {
        cout << "[" << cur->data.vehicleNumber << "]";
        if (cur->next) cout << " <-> ";
    }
    cout << " <-> NULL (newest to oldest)\n";
}
void DoublyLinkedList::displayBackward() const {
    cout << "NULL <-> ";
    for (Node* cur = tail; cur; cur = cur->prev) {
        cout << "[" << cur->data.vehicleNumber << "]";
        if (cur->prev) cout << " <-> ";
    }
    cout << " <-> NULL (oldest to newest)\n";
}
int DoublyLinkedList::size() const { return count; }
void DoublyLinkedList::clear() {
    while (head) { Node* old = head; head = head->next; delete old; }
    tail = nullptr; count = 0;
}



// ============ MODULE: CircularLinkedList ============

#include <iostream>
using namespace std;
CircularLinkedList::CircularLinkedList() : head(nullptr), tail(nullptr), cursor(nullptr), count(0) {}
CircularLinkedList::~CircularLinkedList() { clear(); }
void CircularLinkedList::append(const string& id) {
    Node* n = new Node(id);
    if (!head) { head = tail = n; n->next = head; cursor = head; }
    else { tail->next = n; tail = n; tail->next = head; }
    ++count;
}
string CircularLinkedList::inspectNext() {
    if (!cursor) return "";
    string id = cursor->slotId;
    cursor = cursor->next;
    return id;
}
vector<string> CircularLinkedList::toVector() const {
    vector<string> rows;
    if (!head) return rows;
    Node* cur = head;
    do { rows.push_back(cur->slotId); cur = cur->next; } while (cur != head);
    return rows;
}
void CircularLinkedList::display() const {
    cout << "HEAD -> ";
    if (head) {
        Node* cur = head;
        do {
            cout << "[" << cur->slotId << "] -> ";
            cur = cur->next;
        } while (cur != head);
        cout << "HEAD (circular)";
    } else cout << "EMPTY";
    cout << "\n";
}
int CircularLinkedList::size() const { return count; }
void CircularLinkedList::clear() {
    if (head) {
        tail->next = nullptr; // Break cycle to free memory safely.
        while (head) { Node* old = head; head = head->next; delete old; }
    }
    tail = cursor = nullptr;
    count = 0;
}



// ============ MODULE: DynamicVehicleArray ============

#include <stdexcept>
DynamicVehicleArray::DynamicVehicleArray() : items(new Vehicle[2]), length(0), capacity(2) {}
DynamicVehicleArray::~DynamicVehicleArray() { delete[] items; }
void DynamicVehicleArray::grow() {
    int newCapacity = capacity * 2;
    Vehicle* bigger = new Vehicle[newCapacity];
    for (int i = 0; i < length; ++i) bigger[i] = items[i];
    delete[] items;
    items = bigger;
    capacity = newCapacity;
}
void DynamicVehicleArray::add(const Vehicle& v) {
    if (length == capacity) grow();
    items[length++] = v;
}
bool DynamicVehicleArray::insertAt(int index, const Vehicle& v) {
    if (index < 0 || index > length) return false;
    if (length == capacity) grow();
    for (int i = length; i > index; --i) items[i] = items[i - 1];
    items[index] = v;
    ++length;
    return true;
}
bool DynamicVehicleArray::removeAt(int index) {
    if (index < 0 || index >= length) return false;
    for (int i = index; i < length - 1; ++i) items[i] = items[i + 1];
    --length;
    return true;
}
int DynamicVehicleArray::searchNumber(const std::string& number) const {
    for (int i = 0; i < length; ++i)
        if (items[i].vehicleNumber == number) return i;
    return -1;
}
const Vehicle& DynamicVehicleArray::at(int index) const {
    if (index < 0 || index >= length) throw std::out_of_range("array index");
    return items[index];
}
int DynamicVehicleArray::size() const { return length; }
int DynamicVehicleArray::getCapacity() const { return capacity; }



// ============ MODULE: BST ============

#include <ostream>
#include <algorithm>
#include <limits>
using namespace std;

BSTree::BSTree() : root(nullptr) {}
BSTree::~BSTree() { clear(); }

BSTNode* BSTree::CreateNode(int data, const string& plate) {
    BSTNode* node = new BSTNode;
    node->data = data;
    node->vehicleNumber = plate;
    node->left = nullptr;
    node->right = nullptr;
    return node;
}
BSTNode* BSTree::Insert(BSTNode* current, int data, const string& plate, bool& inserted) {
    if (current == nullptr) { inserted = true; return CreateNode(data, plate); }
    if (data < current->data) current->left = Insert(current->left, data, plate, inserted);
    else if (data > current->data) current->right = Insert(current->right, data, plate, inserted);
    // The professor's lab ignores duplicate keys.
    return current;
}
BSTNode* BSTree::FindMin(BSTNode* current) {
    if (current == nullptr) return nullptr;
    while (current->left != nullptr) current = current->left;
    return current;
}
BSTNode* BSTree::Delete(BSTNode* current, int data, bool& deleted) {
    if (current == nullptr) return nullptr;
    if (data < current->data) current->left = Delete(current->left, data, deleted);
    else if (data > current->data) current->right = Delete(current->right, data, deleted);
    else {
        deleted = true;
        // Case 1: leaf node. Case 2: only one child.
        if (current->left == nullptr) {
            BSTNode* next = current->right;
            delete current;
            return next;
        }
        if (current->right == nullptr) {
            BSTNode* next = current->left;
            delete current;
            return next;
        }
        // Case 3: two children; copy inorder successor, then remove successor.
        BSTNode* successor = FindMin(current->right);
        current->data = successor->data;
        current->vehicleNumber = successor->vehicleNumber;
        bool removedSuccessor = false;
        current->right = Delete(current->right, successor->data, removedSuccessor);
    }
    return current;
}
const BSTNode* BSTree::Search(const BSTNode* current, int data) {
    if (current == nullptr || current->data == data) return current;
    return data < current->data ? Search(current->left, data) : Search(current->right, data);
}
void BSTree::Inorder(const BSTNode* current, vector<int>& out) {
    if (current == nullptr) return;
    Inorder(current->left, out); out.push_back(current->data); Inorder(current->right, out);
}
void BSTree::Preorder(const BSTNode* current, vector<int>& out) {
    if (current == nullptr) return;
    out.push_back(current->data); Preorder(current->left, out); Preorder(current->right, out);
}
void BSTree::Postorder(const BSTNode* current, vector<int>& out) {
    if (current == nullptr) return;
    Postorder(current->left, out); Postorder(current->right, out); out.push_back(current->data);
}
int BSTree::Height(const BSTNode* current) {
    return current ? 1 + max(Height(current->left), Height(current->right)) : 0;
}
int BSTree::CountNodes(const BSTNode* current) {
    return current ? 1 + CountNodes(current->left) + CountNodes(current->right) : 0;
}
int BSTree::CountLeafNodes(const BSTNode* current) {
    if (current == nullptr) return 0;
    if (current->left == nullptr && current->right == nullptr) return 1;
    return CountLeafNodes(current->left) + CountLeafNodes(current->right);
}
void BSTree::Clear(BSTNode* current) {
    if (current == nullptr) return;
    Clear(current->left); Clear(current->right); delete current;
}
void BSTree::Print(const BSTNode* current, ostream& out, int depth) {
    if (current == nullptr) return;
    Print(current->right, out, depth + 1);
    out << string(depth * 5, ' ') << current->data;
    if (!current->vehicleNumber.empty()) out << " [" << current->vehicleNumber << "]";
    out << '\n';
    Print(current->left, out, depth + 1);
}
void BSTree::Snapshot(const BSTNode* current, vector<TreeNodeInfo>& out) {
    if (current == nullptr) return;
    int leftH = Height(current->left), rightH = Height(current->right);
    out.push_back({current->data, current->vehicleNumber, 1 + max(leftH,rightH), leftH-rightH,
        current->left != nullptr, current->left ? current->left->data : 0,
        current->right != nullptr, current->right ? current->right->data : 0});
    Snapshot(current->left, out); Snapshot(current->right, out);
}
bool BSTree::Verify(const BSTNode* current, long long minKey, long long maxKey) {
    if (!current) return true;
    return current->data > minKey && current->data < maxKey &&
        Verify(current->left, minKey, current->data) &&
        Verify(current->right, current->data, maxKey);
}
bool BSTree::insert(int data, const string& plate) {
    bool added = false; root = Insert(root, data, plate, added); return added;
}
bool BSTree::remove(int data) {
    bool removed = false; root = Delete(root, data, removed); return removed;
}
bool BSTree::search(int data) const { return Search(root, data) != nullptr; }
TreeSearchTrace BSTree::traceSearch(int key) const {
    TreeSearchTrace trace;
    const BSTNode* node = root;
    while (node) {
        trace.visited.push_back(node->data);
        if (node->data == key) {
            trace.found = true;
            trace.vehicleNumber = node->vehicleNumber;
            break;
        }
        node = key < node->data ? node->left : node->right;
    }
    return trace;
}

optional<string> BSTree::vehicleFor(int data) const {
    const BSTNode* node = Search(root, data);
    return node ? optional<string>(node->vehicleNumber) : nullopt;
}
vector<int> BSTree::inorder() const { vector<int> out; Inorder(root, out); return out; }
vector<int> BSTree::preorder() const { vector<int> out; Preorder(root, out); return out; }
vector<int> BSTree::postorder() const { vector<int> out; Postorder(root, out); return out; }
optional<int> BSTree::findMin() const { BSTNode* n = FindMin(root); return n ? optional<int>(n->data) : nullopt; }
optional<int> BSTree::findMax() const {
    const BSTNode* n = root;
    while (n && n->right) n = n->right;
    return n ? optional<int>(n->data) : nullopt;
}
int BSTree::height() const { return Height(root); }
int BSTree::countNodes() const { return CountNodes(root); }
int BSTree::countLeafNodes() const { return CountLeafNodes(root); }
vector<TreeNodeInfo> BSTree::snapshot() const { vector<TreeNodeInfo> out; Snapshot(root, out); return out; }
void BSTree::display(ostream& out) const {
    if (!root) { out << "(empty tree)\n"; return; }
    out << "Tree printed sideways (right child above, left below):\n";
    Print(root, out, 0);
}
bool BSTree::verify() const { return Verify(root, numeric_limits<long long>::min(), numeric_limits<long long>::max()); }
void BSTree::clear() { Clear(root); root = nullptr; }



// ============ MODULE: AVLTree ============

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <ostream>
using namespace std;

AVLTree::AVLTree() : root(nullptr) {}
AVLTree::~AVLTree() { clear(); }
AVLNode* AVLTree::CreateNode(int data, const string& plate) {
    AVLNode* node = new AVLNode;
    node->data = data;
    node->vehicleNumber = plate;
    node->height = 1;
    node->left = nullptr;
    node->right = nullptr;
    return node;
}
int AVLTree::Height(const AVLNode* node) { return node ? node->height : 0; }
int AVLTree::BalanceFactor(const AVLNode* node) {
    return node ? Height(node->left) - Height(node->right) : 0;
}
void AVLTree::UpdateHeight(AVLNode* node) {
    if (node) node->height = 1 + max(Height(node->left), Height(node->right));
}
AVLNode* AVLTree::RightRotate(AVLNode* y) {
    AVLNode* x = y->left;
    AVLNode* t2 = x->right;
    x->right = y;
    y->left = t2;
    UpdateHeight(y);
    UpdateHeight(x);
    return x;
}
AVLNode* AVLTree::LeftRotate(AVLNode* x) {
    AVLNode* y = x->right;
    AVLNode* t2 = y->left;
    y->left = x;
    x->right = t2;
    UpdateHeight(x);
    UpdateHeight(y);
    return y;
}
AVLNode* AVLTree::Rebalance(AVLNode* node) {
    if (!node) return nullptr;
    UpdateHeight(node);
    int bf = BalanceFactor(node);
    if (bf > 1) {
        if (BalanceFactor(node->left) >= 0) {
            rotationLog.push_back("LL case at " + to_string(node->data) + ": right rotation");
            return RightRotate(node);
        }
        rotationLog.push_back("LR case at " + to_string(node->data) + ": left, then right rotation");
        node->left = LeftRotate(node->left);
        return RightRotate(node);
    }
    if (bf < -1) {
        if (BalanceFactor(node->right) <= 0) {
            rotationLog.push_back("RR case at " + to_string(node->data) + ": left rotation");
            return LeftRotate(node);
        }
        rotationLog.push_back("RL case at " + to_string(node->data) + ": right, then left rotation");
        node->right = RightRotate(node->right);
        return LeftRotate(node);
    }
    return node;
}
AVLNode* AVLTree::Insert(AVLNode* node, int data, const string& plate, bool& inserted) {
    if (node == nullptr) { inserted = true; return CreateNode(data, plate); }
    if (data < node->data) node->left = Insert(node->left, data, plate, inserted);
    else if (data > node->data) node->right = Insert(node->right, data, plate, inserted);
    else return node; // Duplicate ignored.
    return Rebalance(node);
}
const AVLNode* AVLTree::FindMin(const AVLNode* node) {
    if (!node) return nullptr;
    while (node->left) node = node->left;
    return node;
}
AVLNode* AVLTree::Delete(AVLNode* node, int data, bool& deleted) {
    if (node == nullptr) return nullptr;
    if (data < node->data) node->left = Delete(node->left, data, deleted);
    else if (data > node->data) node->right = Delete(node->right, data, deleted);
    else {
        deleted = true;
        // Cases 1 & 2: leaf / single child.
        if (node->left == nullptr || node->right == nullptr) {
            AVLNode* child = node->left ? node->left : node->right;
            delete node;
            return child;
        }
        // Case 3: two children: replace with inorder successor.
        const AVLNode* next = FindMin(node->right);
        node->data = next->data;
        node->vehicleNumber = next->vehicleNumber;
        bool deletedNext = false;
        node->right = Delete(node->right, next->data, deletedNext);
    }
    return Rebalance(node); // Deletion can also require AVL rotations.
}
const AVLNode* AVLTree::Search(const AVLNode* node, int data) {
    if (node == nullptr || node->data == data) return node;
    return data < node->data ? Search(node->left, data) : Search(node->right, data);
}
void AVLTree::Inorder(const AVLNode* node, vector<int>& out) {
    if (!node) return;
    Inorder(node->left, out); out.push_back(node->data); Inorder(node->right, out);
}
void AVLTree::Preorder(const AVLNode* node, vector<int>& out) {
    if (!node) return;
    out.push_back(node->data); Preorder(node->left, out); Preorder(node->right, out);
}
void AVLTree::Postorder(const AVLNode* node, vector<int>& out) {
    if (!node) return;
    Postorder(node->left, out); Postorder(node->right, out); out.push_back(node->data);
}
int AVLTree::CountNodes(const AVLNode* node) {
    return node ? 1 + CountNodes(node->left) + CountNodes(node->right) : 0;
}
int AVLTree::CountLeafNodes(const AVLNode* node) {
    if (!node) return 0;
    if (!node->left && !node->right) return 1;
    return CountLeafNodes(node->left) + CountLeafNodes(node->right);
}
void AVLTree::Clear(AVLNode* node) {
    if (!node) return;
    Clear(node->left); Clear(node->right); delete node;
}
void AVLTree::Print(const AVLNode* node, ostream& out, int depth) {
    if (!node) return;
    Print(node->right, out, depth + 1);
    out << string(depth * 5, ' ') << node->data << " (h=" << node->height
        << ", BF=" << BalanceFactor(node) << ')';
    if (!node->vehicleNumber.empty()) out << " [" << node->vehicleNumber << "]";
    out << '\n';
    Print(node->left, out, depth + 1);
}
void AVLTree::Snapshot(const AVLNode* node, vector<TreeNodeInfo>& out) {
    if (!node) return;
    out.push_back({node->data, node->vehicleNumber, node->height, BalanceFactor(node),
        node->left != nullptr, node->left ? node->left->data : 0,
        node->right != nullptr, node->right ? node->right->data : 0});
    Snapshot(node->left, out); Snapshot(node->right, out);
}
bool AVLTree::Verify(const AVLNode* node, long long low, long long high, int& actualHeight) {
    if (!node) { actualHeight = 0; return true; }
    if (node->data <= low || node->data >= high) return false;
    int leftHeight = 0, rightHeight = 0;
    if (!Verify(node->left, low, node->data, leftHeight) ||
        !Verify(node->right, node->data, high, rightHeight)) return false;
    actualHeight = 1 + max(leftHeight, rightHeight);
    return node->height == actualHeight && abs(leftHeight - rightHeight) <= 1;
}
bool AVLTree::insert(int data, const string& plate) {
    rotationLog.clear(); bool added = false;
    root = Insert(root, data, plate, added);
    return added;
}
bool AVLTree::remove(int data) {
    rotationLog.clear(); bool removed = false;
    root = Delete(root, data, removed);
    return removed;
}
bool AVLTree::search(int data) const { return Search(root, data) != nullptr; }
TreeSearchTrace AVLTree::traceSearch(int key) const {
    TreeSearchTrace trace;
    const AVLNode* node = root;
    while (node) {
        trace.visited.push_back(node->data);
        if (node->data == key) {
            trace.found = true;
            trace.vehicleNumber = node->vehicleNumber;
            break;
        }
        node = key < node->data ? node->left : node->right;
    }
    return trace;
}

optional<string> AVLTree::vehicleFor(int data) const {
    const AVLNode* node = Search(root, data);
    return node ? optional<string>(node->vehicleNumber) : nullopt;
}
vector<int> AVLTree::inorder() const { vector<int> out; Inorder(root, out); return out; }
vector<int> AVLTree::preorder() const { vector<int> out; Preorder(root, out); return out; }
vector<int> AVLTree::postorder() const { vector<int> out; Postorder(root, out); return out; }
optional<int> AVLTree::findMin() const {
    const AVLNode* node = FindMin(root);
    return node ? optional<int>(node->data) : nullopt;
}
optional<int> AVLTree::findMax() const {
    const AVLNode* node = root;
    while (node && node->right) node = node->right;
    return node ? optional<int>(node->data) : nullopt;
}
int AVLTree::height() const { return Height(root); }
int AVLTree::countNodes() const { return CountNodes(root); }
int AVLTree::countLeafNodes() const { return CountLeafNodes(root); }
vector<TreeNodeInfo> AVLTree::snapshot() const { vector<TreeNodeInfo> out; Snapshot(root, out); return out; }
const vector<string>& AVLTree::lastRotations() const { return rotationLog; }
void AVLTree::display(ostream& out) const {
    if (!root) { out << "(empty tree)\n"; return; }
    out << "Tree printed sideways (right above, left below):\n";
    Print(root, out, 0);
}
bool AVLTree::verify() const {
    int actualHeight = 0;
    return Verify(root, numeric_limits<long long>::min(), numeric_limits<long long>::max(), actualHeight);
}
void AVLTree::clear() { Clear(root); root = nullptr; rotationLog.clear(); }



// ============ MODULE: FeeCalculator ============


// Current per-hour charges are read from the local admin rate file.
// Missing keys use the original default values.
double hourlyParkingRate(const string& type) {
    long long base = type == "Motorcycle" ? 50 :
        (type == "Van" || type == "Pickup") ? 150 :
        type == "SUV" ? 130 : type == "Sports Car" ? 180 :
        type == "Bus" ? 220 : type == "Truck" ? 250 : 100;
    const char* custom = std::getenv("SMARTPARK_RATE_FILE");
    const string path = custom && *custom ? custom : "data/parking_rates.txt";
    std::ifstream file(path);
    if (file) {
        string key; long long price = 0;
        while (file >> std::quoted(key) >> price) {
            if (key == type && price >= 1 && price <= 1000000) return static_cast<double>(price);
        }
    }
    return static_cast<double>(base);
}
double calculateParkingFee(const string& type, int startedHours) {
    if (startedHours < 1) startedHours = 1;
    return hourlyParkingRate(type) * startedHours;
}




// ============ MODULE: PriorityQueue ============

#include <iostream>
using namespace std;

int PriorityQueue::priorityValue(const Vehicle& v) const {
    if(v.vehicleType=="Emergency") return 3;
    if(v.priority) return 2;
    return 1;
}
void PriorityQueue::insert(const Vehicle& v){items.push_back(v);}
Vehicle PriorityQueue::removeHighestPriority(){
    if(items.empty()) return Vehicle();
    int best=0;
    for(int i=1;i<(int)items.size();++i)
        if(priorityValue(items[i])>priorityValue(items[best])) best=i;
    Vehicle v=items[best];items.erase(items.begin()+best);return v;
}
Vehicle* PriorityQueue::peek(){
    if(items.empty()) return nullptr;
    int best=0;
    for(int i=1;i<(int)items.size();++i)
        if(priorityValue(items[i])>priorityValue(items[best])) best=i;
    return &items[best];
}
bool PriorityQueue::isEmpty()const{return items.empty();}
int PriorityQueue::size()const{return (int)items.size();}
void PriorityQueue::display()const{
    cout<<"Priority Queue:\n";
    for(const auto& v:items) cout<<"- "<<v.vehicleNumber<<" (priority "<<priorityValue(v)<<")\n";
}



// ============ MODULE: Search ============

int linearSearchByNumber(const vector<Vehicle>& v, const string& number) {
    for (int i = 0; i < (int)v.size(); ++i)
        if (v[i].vehicleNumber == number) return i;
    return -1;
}
int linearSearchByOwner(const vector<Vehicle>& v, const string& owner) {
    for (int i = 0; i < (int)v.size(); ++i)
        if (v[i].ownerName == owner) return i;
    return -1;
}
int linearSearchBySlot(const vector<Vehicle>& v, const string& slot) {
    for (int i = 0; i < (int)v.size(); ++i)
        if (v[i].parkingSlot == slot) return i;
    return -1;
}
int binarySearchByNumber(const vector<Vehicle>& sorted, const string& number) {
    int left = 0, right = (int)sorted.size() - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (sorted[mid].vehicleNumber == number) return mid;
        if (sorted[mid].vehicleNumber < number) left = mid + 1;
        else right = mid - 1;
    }
    return -1;
}



// ============ MODULE: Sort ============

#include <utility>
using namespace std;
void bubbleSortByNumber(vector<Vehicle>& v) {
    for (int i = 0; i < (int)v.size() - 1; ++i)
        for (int j = 0; j < (int)v.size() - i - 1; ++j)
            if (v[j].vehicleNumber > v[j + 1].vehicleNumber) swap(v[j], v[j + 1]);
}
void bubbleSortByFee(vector<Vehicle>& v) {
    for (int i = 0; i < (int)v.size() - 1; ++i)
        for (int j = 0; j < (int)v.size() - i - 1; ++j)
            if (v[j].parkingFee > v[j + 1].parkingFee) swap(v[j], v[j + 1]);
}
void selectionSortByNumber(vector<Vehicle>& v) {
    for (int i = 0; i < (int)v.size() - 1; ++i) {
        int smallest = i;
        for (int j = i + 1; j < (int)v.size(); ++j)
            if (v[j].vehicleNumber < v[smallest].vehicleNumber) smallest = j;
        if (smallest != i) swap(v[i], v[smallest]);
    }
}
void selectionSortByFee(vector<Vehicle>& v) {
    for (int i = 0; i < (int)v.size() - 1; ++i) {
        int smallest = i;
        for (int j = i + 1; j < (int)v.size(); ++j)
            if (v[j].parkingFee < v[smallest].parkingFee) smallest = j;
        if (smallest != i) swap(v[i], v[smallest]);
    }
}



// ============ MODULE: ParkingSystem ============



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

// are formatted for the local operator. Legacy V1-V4 minute timestamps migrate.
inline long long parkingNowEpoch() {
    return static_cast<long long>(std::time(nullptr));
}
inline std::string parkingFormatEpoch(long long seconds) {
    std::time_t when = static_cast<std::time_t>(seconds);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &when);
#else
    localtime_r(&when, &local);
#endif
    std::ostringstream text;
    text << std::put_time(&local, "%d %b %Y, %I:%M:%S %p");
    return text.str();
}
inline long long parkingParseStamp(const std::string& text) {
    if (text.empty()) return 0;
    for (const char* format : {"%d %b %Y, %I:%M:%S %p", "%d %b %Y, %I:%M %p"}) {
        std::tm parsed{};
        parsed.tm_isdst = -1;
        std::istringstream input(text);
        input >> std::get_time(&parsed, format);
        if (!input.fail()) {
            input >> std::ws;
            if (input.eof()) {
                std::time_t result = std::mktime(&parsed);
                if (result > 0) return static_cast<long long>(result);
            }
        }
    }
    return 0;
}
inline int parkingStartedHours(long long elapsedSeconds) {
    if (elapsedSeconds < 0) return 0; // Clock cannot precede entry.
    // Even zero elapsed seconds count as one started hour.
    return static_cast<int>(std::max(1LL, (elapsedSeconds + 3599LL) / 3600LL));
}

// History reports use the saved session data; this file tracks admin operations.
inline void phase8AppendAudit(const fs::path& state, const string& action, const string& detail) {
    std::error_code ec;
    if(!state.parent_path().empty()) fs::create_directories(state.parent_path(),ec);
    if(ec)return;
    fs::path path=state;
    path += ".audit.log";
    std::ofstream out(path, std::ios::app);
    if(out)out<<parkingFormatEpoch(parkingNowEpoch())<<" | "<<action<<" | "<<detail<<'\n';
}

[[maybe_unused]] string stamp() {
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
// Real-world constraint: a free, size-compatible slot is NOT eligible if all
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
    if(save()){phase8AppendAudit(dataFile,blocked?"ROAD_BLOCK":"ROAD_OPEN",from+" <-> "+to);return true;}
    // Best effort restore persisted state on I/O failure.
    if(!load())graph.setBlocked(from,to,previous);
    return false;
}
std::vector<RoadEdge> ParkingSystem::roads() const {return graph.roads();}
std::vector<std::string> ParkingSystem::routeToSlot(const std::string& id) const {return graph.pathTo(id);}
bool ParkingSystem::dfsCanReach(const std::string& id) const {return graph.dfsReachable("ENTRY",id);}
int ParkingSystem::closedRoadCount() const {return graph.closedCount();}

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
    bool saved=save();if(saved)phase8AppendAudit(dataFile,"QUEUE_CANCEL",plate);return saved;
}
bool ParkingSystem::changeWaitingVIP(const string& rawPlate, bool vip) {
    string plate = upper(trimSpaces(rawPlate));
    Vehicle* found = waiting.findNumber(plate);
    if (!found || found->vehicleType == "Emergency") return false;
    found->priority = vip;
    dispatchWaiting(); // e.g. a newly VIP-qualified ticket may use a reserved space.
    bool saved=save();if(saved)phase8AppendAudit(dataFile,"QUEUE_PRIORITY",plate+(vip?" VIP":" NORMAL"));return saved;
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
    else phase8AppendAudit(dataFile,answer.queued?"WAITING":"PARK",vehicle.vehicleNumber+" | "+vehicle.ticketId+" | "+vehicle.parkingSlot);
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
    else phase8AppendAudit(dataFile,"EXIT",removed.vehicleNumber+" | "+removed.ticketId+" | Rs."+to_string((long long)removed.parkingFee));
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
    bool saved=save();if(saved)phase8AppendAudit(dataFile,"LAYOUT",to_string(floors)+" floors x "+to_string(slotsPerFloor)+" slots");return saved;
}

bool ParkingSystem::setSlotRestriction(const string& id, int mode, const string& note) {
    const ParkingSlot* current = lot.slotById(id);
    if (!current || ccp.heldSlot(id)) return false;
    int originalMode = current->restriction;
    string originalNote = current->restrictionNote;
    if (!lot.setSlotRestriction(id, mode, note)) return false;
    // Reopening normal/reserved slots may serve compatible waiting vehicles.
    if (mode != 2) {dispatchWaiting();dispatchGroups();}
    if (save()){phase8AppendAudit(dataFile,"SLOT_RULE",id+" | mode="+to_string(mode)+" | "+note);return true;}
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
    cout << "\n=== UNREACHABLE SLOTS (ROAD CLOSURES) ===\n";
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
    bool saved=save();if(saved)phase8AppendAudit(dataFile,"RESET","Parking records reset; backup saved when required");return saved;
}



// ============ SMARTPARK TRACE: RAW DSA COMPLEX COMPUTING ============
void ParkingSystem::makeCCPGraph(traceccp::RawGraph& output) const {
    for (const RoadEdge& road : graph.roads())output.add(road.from,road.to,road.blocked);
    output.add("PED-GATE","ENTRY");
    for(char b='A';b<='C';++b)output.add(std::string("G-")+b,std::string("EXIT-")+b);
}

FindCarResult ParkingSystem::findMyCar(const std::string& rawPlate) const {
    FindCarResult result;
    std::string plate=upper(trimSpaces(rawPlate));
    if(!validPlate(plate)){result.message="Enter a valid plate number.";return result;}
    traceccp::PlateAVL plateTree;
    for(const auto& slot:lot.getSlots())if(slot.occupied)
        plateTree.insert(slot.vehicle.vehicleNumber,slot.slotNumber);
    std::string id=plateTree.findSlot(plate,result.comparisons);
    if(id.empty()){result.message="Vehicle is not currently parked.";return result;}
    result.found=true;result.slot=id;
    traceccp::RawGraph raw;
    makeCCPGraph(raw);
    traceccp::Path p=raw.bfs("PED-GATE",id);
    if(!p.found()) {result.message="Car found, but walking path is blocked. Ask staff for assistance.";return result;}
    result.reachable=true;result.edges=p.count-1;result.path=p.text();
    for(int i=0;i+1<p.count;++i){
        if(i)result.steps+='\n';
        result.steps += std::to_string(i+1)+". Walk from "+p.at[i]+" to "+p.at[i+1];
    }
    result.message="Vehicle found using raw AVL by plate; BFS + linked stack reconstructed walking steps.";
    return result;
}

std::vector<EvacuationRow> ParkingSystem::evacuationRoutes() const {
    std::vector<EvacuationRow> rows; // Output adapter only; shortest-path core uses raw arrays.
    if(!ccp.evacuation)return rows;
    traceccp::RawGraph graphCopy;
    makeCCPGraph(graphCopy);
    for(const auto& slot:lot.getSlots())if(slot.occupied&&slot.building==std::string(1,ccp.incident)){
        EvacuationRow r; r.plate=slot.vehicle.vehicleNumber;r.slot=slot.slotNumber;
        std::string exit;
        traceccp::Path p=graphCopy.nearestSafeExit(slot.slotNumber,ccp.incident,exit);
        r.reachable=p.found();r.exitGate=exit;r.path=p.text();r.edges=p.found()?p.count-1:0;
        if(!r.reachable)r.path="NO SAFE ROUTE - REQUEST HUMAN ASSISTANCE";
        rows.push_back(r);
    }
    return rows;
}

bool ParkingSystem::setEvacuation(char building,bool enabled){
    if(enabled&&building!='A'&&building!='B'&&building!='C')return false;
    bool old=ccp.evacuation;char previous=ccp.incident;
    ccp.evacuation=enabled;ccp.incident=enabled?building:'-';
    if(!enabled){dispatchWaiting();dispatchGroups();}
    if(save())return true;
    if(!load()){ccp.evacuation=old;ccp.incident=previous;}return false;
}

bool ParkingSystem::plateInCCP(const std::string& plate) const {
    return ccp.heldPlate(plate)||ccp.groups.hasPlate(plate);
}

CCPResponse ParkingSystem::reserveVIP(const std::string& rawPlate,const std::string& type,
                                      const std::string& rawOwner,const std::string& building,int floor){
    CCPResponse r;
    std::string plate=upper(trimSpaces(rawPlate)),owner=trimSpaces(rawOwner);
    if(!validPlate(plate)||!validType(type)||!validOwner(owner)||
       (!(building.empty()||building=="A"||building=="B"||building=="C"))||
       floor<0||floor>floorsPerBuilding()){
        r.message="Invalid VIP reservation details.";return r;
    }
    if(lot.findVehicle(plate)||waiting.containsNumber(plate)||plateInCCP(plate)){
        r.message="Plate already parked, waiting or reserved.";return r;
    }
    Vehicle v(plate,type,owner,true);
    v.preferredBuilding=building;v.preferredFloor=floor;
    const ParkingSlot* slot=bestReachableSlot(v);
    if(!slot){r.message="No compatible accessible slot for VIP reservation.";return r;}
    traceccp::RawGraph raw;makeCCPGraph(raw);
    traceccp::Path route=raw.bfs("ENTRY",slot->slotNumber);
    if(!route.found()){r.message="No accessible road.";return r;}
    traceccp::Hold hold;hold.active=true;hold.plate=plate;hold.type=type;hold.owner=owner;
    hold.building=building;hold.floor=floor;hold.slot=slot->slotNumber;
    hold.path=traceccp::State::packPath(route);
    if(!ccp.addHold(hold)){r.message="Reservation capacity reached.";return r;}
    if(!save()){ccp.removeHold(plate);r.message="Could not persist reservation.";return r;}
    r.success=true;r.slot=hold.slot;r.route=route.text();
    r.message="Provisional VIP slot reserved. Confirm arrival before it becomes occupied.";
    return r;
}

CCPResponse ParkingSystem::confirmVIP(const std::string& rawPlate){
    CCPResponse r;std::string plate=upper(trimSpaces(rawPlate));
    traceccp::Hold* held=ccp.findHold(plate);
    if(!held){r.message="VIP reservation not found.";return r;}
    traceccp::Hold snapshot=*held;
    const ParkingSlot* slot=lot.slotById(snapshot.slot);
    if(!slot||slot->occupied||slot->restriction==2||graph.pathTo(snapshot.slot).empty()||
       (ccp.evacuation&&slot->building==std::string(1,ccp.incident))){
        r.message="Reserved slot is no longer usable. Block/reopen road to trigger recovery.";return r;
    }
    Vehicle v(plate,snapshot.type,snapshot.owner,true);
    v.preferredBuilding=snapshot.building;v.preferredFloor=snapshot.floor;
    if(!slot->canFit(v)){r.message="Reserved slot no longer meets vehicle requirements.";return r;}
    ccp.removeHold(plate);
    if(nextTicket>99999999){ccp.addHold(snapshot);r.message="Ticket counter is full.";return r;}
    std::ostringstream ticket;ticket<<"SP-"<<std::setw(8)<<std::setfill('0')<<nextTicket++;
    v.ticketId=ticket.str();v.entryEpoch=parkingNowEpoch();v.entryTime=parkingFormatEpoch(v.entryEpoch);
    v.parkingSlot=snapshot.slot;
    if(!lot.parkAtSlot(v,snapshot.slot)){
        --nextTicket;ccp.addHold(snapshot);r.message="Unable to confirm slot.";return r;
    }
    active.insert(v);activeBST.insert(ticketKey(v.ticketId),v.vehicleNumber);
    activeAVL.insert(ticketKey(v.ticketId),v.vehicleNumber);
    treeEvents.push_back({'I',ticketKey(v.ticketId),v.vehicleNumber});
    if(!save()){
        lot.removeVehicle(plate);active.remove(plate);
        activeBST.remove(ticketKey(v.ticketId));activeAVL.remove(ticketKey(v.ticketId));
        treeEvents.pop_back();--nextTicket;ccp.addHold(snapshot);
        r.message="Could not persist arrival.";return r;
    }
    r.success=true;r.slot=snapshot.slot;r.message="VIP arrival confirmed; ticket "+v.ticketId+" active.";
    return r;
}

std::vector<traceccp::Hold> ParkingSystem::vipReservations() const {
    std::vector<traceccp::Hold> result;
    for(const auto&h:ccp.holds)if(h.active)result.push_back(h);
    return result;
}

void ParkingSystem::recoverVipReservations(const std::string& from,const std::string& to){
    // LIFO rollback records *pre-change provisional* holds; no occupied car is removed.
    for(int i=0;i<traceccp::MAX_HOLDS;++i){
        traceccp::Hold h=ccp.holds[i];
        if(!h.active||!ccp.pathUses(h.path,from,to))continue;
        ccp.undo.push(h);
        ccp.removeHold(h.plate);
        Vehicle v(h.plate,h.type,h.owner,true);
        v.preferredBuilding=h.building;v.preferredFloor=h.floor;
        const ParkingSlot* chosen=bestReachableSlot(v);
        traceccp::Hold rolled;
        if(!ccp.undo.pop(rolled))continue;
        if(chosen){
            traceccp::RawGraph raw;makeCCPGraph(raw);
            traceccp::Path newRoute=raw.bfs("ENTRY",chosen->slotNumber);
            rolled.slot=chosen->slotNumber;rolled.path=traceccp::State::packPath(newRoute);
            ccp.addHold(rolled);
            ccp.lastRecovery="Rollback -> BFS rerouted VIP "+h.plate+" to "+rolled.slot;
        }else{
            // All possible routes blocked: use existing raw linked priority waiting queue.
            ParkResult queued=park(h.plate,h.type,h.owner,true,h.building,h.floor);
            if(queued.success&&queued.queued)
                ccp.lastRecovery="Rollback -> VIP "+h.plate+" moved to Priority Queue";
            else {
                ccp.lastRecovery="Rollback -> could not queue VIP "+h.plate;
                // Do not keep a dead provisional reservation for a blocked road.
            }
        }
    }
}

// A sliding-window search on the slot-array layout. A convoy has to share a
// single building and floor within each contiguous block; no gap is allowed.
bool ParkingSystem::selectConvoySlots(const traceccp::Group& group,int chosen[4],bool& split) const {
    const auto& slots=lot.getSlots();
    if(slots.size()>240)return false;
    bool taken[240]{};
    auto findWindow=[&](int first,int count)->bool{
        for(int start=0;start+count<=(int)slots.size();++start){
            bool fits=true;std::string building=slots[start].building;
            int floor=slots[start].floor;
            int base=-1;
            try {base=std::stoi(slots[start].slotNumber.substr(slots[start].slotNumber.rfind('S')+1));}
            catch(...) {continue;}
            for(int j=0;j<count;++j){
                int index=start+j;
                const ParkingSlot& s=slots[index];
                Vehicle v(group.plate[first+j],group.type[first+j],group.owner[first+j],group.vip);
                v.preferredBuilding=group.building;v.preferredFloor=group.floor;
                if(taken[index]||s.building!=building||s.floor!=floor||
                   s.slotNumber.substr(s.slotNumber.rfind('S')+1)!=[&](){
                       std::ostringstream ss;ss<<std::setw(2)<<std::setfill('0')<<(base+j);return ss.str();
                   }()||!s.canFit(v)||ccp.heldSlot(s.slotNumber)||
                   (ccp.evacuation&&s.building==std::string(1,ccp.incident))||
                   graph.pathTo(s.slotNumber).empty()) {fits=false;break;}
            }
            if(fits){for(int j=0;j<count;++j){chosen[first+j]=start+j;taken[start+j]=true;}return true;}
        }
        return false;
    };
    split=false;
    if(findWindow(0,group.count))return true;
    // Reinitialise, then allow 2+2 (4) or 2+1 (3) in separate contiguous areas.
    for(int i=0;i<240;++i)taken[i]=false;
    if(!findWindow(0,2))return false;
    if(findWindow(2,group.count-2)){split=true;return true;}
    return false;
}

GroupResponse ParkingSystem::placeConvoy(const traceccp::Group& input,bool queueIfMissing){
    GroupResponse r;r.count=input.count;
    if(input.count<3||input.count>4){r.message="Group must contain 3 or 4 vehicles.";return r;}
    if(ccp.evacuation&&!input.vip){r.message="NORMAL INBOUND LOCKED: evacuation mode is active.";return r;}
    int positions[4]{};bool split=false;
    if(!selectConvoySlots(input,positions,split)){
        if(!queueIfMissing){r.message="Group still waiting for sufficient contiguous spaces.";return r;}
        if(ccp.groups.size()>=200){r.message="Group queue full.";return r;}
        ccp.groups.push(input);
        if(!save()){ccp.groups.erase(input.id);r.message="Could not save group queue.";return r;}
        r.success=true;r.queued=true;r.message="No complete 3/4-space arrangement; convoy kept as ONE linked Group Node.";
        return r;
    }
    long long startTicket=nextTicket;
    size_t startEvents=treeEvents.size();
    std::string applied[4];int done=0;
    for(int i=0;i<input.count;++i){
        if(nextTicket>99999999)break;
        Vehicle v(input.plate[i],input.type[i],input.owner[i],input.vip);
        v.preferredBuilding=input.building;v.preferredFloor=input.floor;
        std::ostringstream ticket;ticket<<"SP-"<<std::setw(8)<<std::setfill('0')<<nextTicket++;
        v.ticketId=ticket.str();
        v.entryEpoch=parkingNowEpoch();v.entryTime=parkingFormatEpoch(v.entryEpoch);
        v.parkingSlot=lot.getSlots()[positions[i]].slotNumber;
        if(!lot.parkAtSlot(v,v.parkingSlot))break;
        active.insert(v);activeBST.insert(ticketKey(v.ticketId),v.vehicleNumber);
        activeAVL.insert(ticketKey(v.ticketId),v.vehicleNumber);
        treeEvents.push_back({'I',ticketKey(v.ticketId),v.vehicleNumber});
        applied[done++]=v.vehicleNumber;
        r.slots[i]=v.parkingSlot;
    }
    if(done==input.count&&save()){
        r.success=true;r.split=split;
        r.message=split?"Smart Split: contiguous pairs (2+2 or 2+1) allocated atomically.":
                        "Contiguous sliding-window allocation successful.";
        return r;
    }
    // Transaction rollback on any partial failure (no half-parked convoy).
    for(int i=0;i<done;++i){
        Vehicle v=lot.removeVehicle(applied[i]);active.remove(applied[i]);
        activeBST.remove(ticketKey(v.ticketId));activeAVL.remove(ticketKey(v.ticketId));
    }
    treeEvents.resize(startEvents);nextTicket=startTicket;
    r.message="Convoy transaction rolled back due to an unexpected parking/save failure.";
    return r;
}

GroupResponse ParkingSystem::submitConvoy(traceccp::Group group){
    GroupResponse invalid;invalid.count=group.count;
    if(group.count!=3&&group.count!=4){invalid.message="Convoy size must be 3 or 4.";return invalid;}
    if(group.building!=""&&group.building!="A"&&group.building!="B"&&group.building!="C"){
        invalid.message="Choose Building A/B/C or any.";return invalid;
    }
    if(group.floor<0||group.floor>floorsPerBuilding()){
        invalid.message="Invalid floor.";return invalid;
    }
    for(int i=0;i<group.count;++i){
        group.plate[i]=upper(trimSpaces(group.plate[i]));
        group.owner[i]=trimSpaces(group.owner[i]);
        if(!validPlate(group.plate[i])||!validOwner(group.owner[i])||!validType(group.type[i])){
            invalid.message="Invalid plate, owner or vehicle type in convoy.";return invalid;
        }
        if(lot.findVehicle(group.plate[i])||waiting.containsNumber(group.plate[i])||
           plateInCCP(group.plate[i])){
            invalid.message="One convoy plate already exists in parking, waiting or reservation.";return invalid;
        }
        for(int j=0;j<i;++j)if(group.plate[j]==group.plate[i]){
            invalid.message="Duplicate plate inside convoy.";return invalid;
        }
    }
    if(ccp.evacuation&&!group.vip){invalid.message="Incoming normal convoy blocked during evacuation.";return invalid;}
    group.id="CV-"+std::to_string(ccp.nextGroupId++);
    GroupResponse result=placeConvoy(group,true);
    result.id=group.id;
    if(!result.success)--ccp.nextGroupId;
    return result;
}

std::vector<traceccp::Group> ParkingSystem::groupQueue() const {
    std::vector<traceccp::Group> all;
    for(auto*n=ccp.groups.first();n;n=n->next)all.push_back(n->group);
    return all;
}
void ParkingSystem::dispatchGroups(){
    if(ccp.evacuation)return; // no automatic inbound while the site is being cleared
    bool progressed=true;
    while(progressed){
        progressed=false;
        // Priority Group Nodes first, FIFO inside the same level.
        for(int level=1;level>=0&&!progressed;--level){
            for(auto*n=ccp.groups.first();n;n=n->next){
                traceccp::Group group=n->group;
                if((group.vip?1:0)!=level)continue;
                GroupResponse result=placeConvoy(group,false);
                if(result.success){ccp.groups.erase(group.id);save();progressed=true;break;}
            }
        }
    }
}


// ============ MODULE: main ============
#include <iostream>
#include <iomanip>
#include <limits>
#include <string>
#include <vector>




using namespace std;

// Reads real active data. No synthetic samples or changes to parked vehicles.
#ifndef SMARTPARK_ALGORITHM_LAB_H
#define SMARTPARK_ALGORITHM_LAB_H
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

struct TicketAudit {
    int ticketKey = 0;
    int recordsScanned = 0;
    bool linearFound = false;
    std::string linearPlate;
    TreeSearchTrace bst;
    TreeSearchTrace avl;
    bool indexesValid = false;
};

inline int ticketNumber(const std::string& ticket) {
    auto dash=ticket.find_last_of('-');
    std::string digits=ticket.substr(dash==std::string::npos?0:dash+1);
    if (digits.empty() || digits.size()>9) return -1;
    for(unsigned char ch:digits)if(!std::isdigit(ch))return -1;
    try{return std::stoi(digits);}catch(...){return -1;}
}

inline TicketAudit auditLiveTicket(const ParkingSystem& system,int key) {
    TicketAudit result;
    result.ticketKey=key;
    for (const Vehicle& vehicle : system.parkedVehicles()) {
        ++result.recordsScanned;
        if(ticketNumber(vehicle.ticketId)==key){
            result.linearFound=true;result.linearPlate=vehicle.vehicleNumber;break;
        }
    }
    result.bst=system.bst().traceSearch(key);
    result.avl=system.avl().traceSearch(key);
    result.indexesValid=system.treeIndexesValid();
    return result;
}

struct SortAudit {
    std::vector<Vehicle> sorted;
    long long comparisons=0;
    long long swaps=0;
    std::string algorithm;
    std::string field;
};

inline SortAudit measureSort(std::vector<Vehicle> vehicles, bool selection, const std::string& field) {
    SortAudit a;
    a.sorted=std::move(vehicles);
    a.algorithm=selection?"Selection Sort":"Bubble Sort";
    a.field=field;
    auto greater=[&](const Vehicle& x,const Vehicle& y) {
        ++a.comparisons;
        if(field=="fee")return x.parkingFee>y.parkingFee;
        return x.vehicleNumber>y.vehicleNumber;
    };
    int n=static_cast<int>(a.sorted.size());
    if(selection){
        for(int i=0;i<n-1;++i){
            int best=i;
            for(int j=i+1;j<n;++j)if(greater(a.sorted[best],a.sorted[j]))best=j;
            if(best!=i){std::swap(a.sorted[i],a.sorted[best]);++a.swaps;}
        }
    }else{
        for(int i=0;i<n-1;++i){
            bool changed=false;
            for(int j=0;j<n-i-1;++j)if(greater(a.sorted[j],a.sorted[j+1])){
                std::swap(a.sorted[j],a.sorted[j+1]);++a.swaps;changed=true;
            }
            if(!changed)break;
        }
    }
    return a;
}

inline std::vector<Vehicle> phase6Dataset(const ParkingSystem& system,const std::string& source){
    if(source=="history")return system.exitRecords();
    std::vector<Vehicle> vehicles=system.parkedVehicles();
    std::vector<Vehicle> queue=system.waitingVehicles();
    vehicles.insert(vehicles.end(),queue.begin(),queue.end());
    return vehicles;
}
#endif

// Helper functions keep the main menu short and beginner-friendly.
bool askLine(const string& prompt, string& value) {
    cout << prompt;
    return (bool)getline(cin, value);
}
bool askNumber(const string& prompt, int& number, int low, int high) {
    string input;
    while (askLine(prompt, input)) {
        try {
            size_t end = 0;
            long numberRead = stol(input, &end);
            if (end == input.size() && numberRead >= low && numberRead <= high) {
                number = (int)numberRead;
                return true;
            }
        } catch (...) {}
        cout << "Please enter a whole number from " << low << " to " << high << ".\n";
    }
    return false;
}

#include <cstdint>
#include <random>

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    cout << "\033[2J\033[H"; // ANSI clear for Linux/macOS terminals.
#endif
}
void pauseScreen() {
    string ignored;
    cout << "\nPress Enter to return to Admin Dashboard...";
    getline(cin, ignored);
}

// Local semester-demo credential protection: salted repeated FNV-1a.
// This is NOT a production-strength password hash; no default password is shipped.
string demoPasswordDigest(const string& salt, const string& password) {
    uint64_t hash = UINT64_C(14695981039346656037);
    string input = salt + ":" + password;
    for (int cycle = 0; cycle < 18000; ++cycle) {
        for (unsigned char c : input) {
            hash ^= c;
            hash *= UINT64_C(1099511628211);
        }
        input = to_string(hash) + salt + password;
    }
    ostringstream out;
    out << hex << setw(16) << setfill('0') << hash;
    return out.str();
}

string newSalt() {
    random_device random;
    const auto stamp = chrono::high_resolution_clock::now().time_since_epoch().count();
    ostringstream out;
    out << hex << random() << random() << stamp;
    return out.str();
}

class AdminLogin {
    filesystem::path credentialsFile;
public:
    explicit AdminLogin(const string& stateFile) {
        filesystem::path path(stateFile);
        auto folder = path.parent_path();
        if (folder.empty()) folder = ".";
        credentialsFile = folder / "admin_console.cfg";
    }
    bool firstTimeSetup() {
        if (filesystem::exists(credentialsFile)) return true;
        clearScreen();
        cout << "======= SMARTPARK TRACE | FIRST-TIME ADMIN SETUP =======\n"
             << "Create a separate login for the standalone C++ application.\n"
             << "Credentials file: " << credentialsFile.string() << "\n\n";
        string user, pass, confirm;
        do {
            if (!askLine("New admin username: ", user)) return false;
            if (user.size() < 3 || user.size() > 32 || user.find_first_of(" \t\r\n") != string::npos)
                cout << "Use 3-32 characters with no spaces.\n";
            else break;
        } while (true);
        do {
            if (!askLine("New password (minimum 8 characters): ", pass)) return false;
            if (!askLine("Confirm password: ", confirm)) return false;
            if (pass.size() >= 8 && pass == confirm) break;
            cout << "Password too short or confirmation mismatch. Try again.\n";
        } while (true);
        auto folder = credentialsFile.parent_path();
        error_code error;
        filesystem::create_directories(folder, error);
        if (error) { cout << "Cannot create credential directory: " << error.message() << '\n'; return false; }
        ofstream saved(credentialsFile, ios::trunc);
        if (!saved) { cout << "Cannot save administrator credentials.\n"; return false; }
        saved << user << '\n' << newSalt() << '\n';
        // Reload the generated salt to avoid accidental mismatch.
        saved.flush(); saved.close();
        ifstream read(credentialsFile);
        string savedUser, salt;
        getline(read, savedUser); getline(read, salt);
        ofstream finish(credentialsFile, ios::app);
        finish << demoPasswordDigest(salt, pass) << '\n';
        finish.close();
        cout << "\nAdministrator account created. Please login.\n";
        pauseScreen();
        return true;
    }
    bool authenticate() {
        if (!firstTimeSetup()) return false;
        ifstream input(credentialsFile);
        string expectedUser, salt, digest;
        if (!getline(input, expectedUser) || !getline(input, salt) || !getline(input, digest)) {
            cout << "Admin credential file is invalid. Restore credentials from backup.\n";
            return false;
        }
        for (int attempt = 1; attempt <= 3; ++attempt) {
            clearScreen();
            cout << "========== SMARTPARK TRACE | ADMIN LOGIN ==========\n"
                 << "Attempt " << attempt << " of 3\n\n";
            string user, password;
            if (!askLine("Username: ", user) || !askLine("Password: ", password)) return false;
            if (user == expectedUser && demoPasswordDigest(salt, password) == digest) {
                cout << "\nLogin successful. Welcome, " << user << "!\n";
                pauseScreen();
                return true;
            }
            cout << "Invalid username or password.\n";
            if (attempt < 3) pauseScreen();
        }
        cout << "\nToo many attempts. Exiting.\n";
        return false;
    }
};

void printVehicles(const vector<Vehicle>& vehicles) {
    if (vehicles.empty()) { cout << "No records found.\n"; return; }
    cout << left << setw(14) << "Ticket" << setw(19) << "Number"
         << setw(18) << "Owner" << setw(14) << "Type"
         << setw(10) << "Slot" << "Fee\n";
    cout << string(87, '-') << '\n';
    for (const auto& v : vehicles) {
        string owner = v.ownerName.size() > 16 ? v.ownerName.substr(0, 15) + "~" : v.ownerName;
        cout << left << setw(14) << v.ticketId << setw(19) << v.vehicleNumber
             << setw(18) << owner << setw(14) << v.vehicleType
             << setw(10) << (v.parkingSlot.empty() ? "WAITING" : v.parkingSlot)
             << "Rs." << (long long)v.parkingFee << '\n';
    }
}
void showTypes() {
    const string types[] = {"Car", "Motorcycle", "Van", "Emergency", "Sedan", "SUV",
                            "Sports Car", "Pickup", "Bus", "Taxi", "Truck"};
    cout << "\nVehicle type:\n";
    for (int i = 0; i < 11; ++i) cout << "  " << i + 1 << ". " << types[i] << '\n';
}
string getType(int choice) {
    const string types[] = {"Car", "Motorcycle", "Van", "Emergency", "Sedan", "SUV",
                            "Sports Car", "Pickup", "Bus", "Taxi", "Truck"};
    return types[choice - 1];
}
void parkMenu(ParkingSystem& system) {
    string plate, owner, vipChoice;
    int type;
    if (!askLine("Enter vehicle number: ", plate)) return;
    if (!askLine("Enter owner name: ", owner)) return;
    showTypes();
    if (!askNumber("Choose type (1-11): ", type, 1, 11)) return;
    bool vip = false;
    if (type != 4) {
        if (!askLine("VIP priority? (y/n): ", vipChoice)) return;
        vip = vipChoice == "y" || vipChoice == "Y";
    }
    string buildingChoice, floorChoice;
    if (!askLine("Preferred building (A/B/C, Enter = any): ", buildingChoice)) return;
    buildingChoice = upper(trimSpaces(buildingChoice));
    if (!buildingChoice.empty() && buildingChoice != "A" && buildingChoice != "B" && buildingChoice != "C") {
        cout << "Invalid building.\n"; return;
    }
    int floor = 0;
    if (!askNumber("Preferred floor (0 = any): ", floor, 0, system.floorsPerBuilding())) return;
    ParkResult result = system.park(plate, getType(type), owner, vip, buildingChoice, floor);
    cout << "\n" << result.message << '\n';
    if (result.success) {
        cout << "Ticket ID: " << result.vehicle.ticketId
             << " | Slot: " << (result.queued ? "WAITING" : result.vehicle.parkingSlot)
             << " | Entry: " << result.vehicle.entryTime << '\n';
    }
}
string receiptText(const Vehicle& vehicle) {
    ostringstream o;
    o << "========== SMARTPARK TRACE RECEIPT ==========\n"
      << "Receipt: R-" << vehicle.ticketId << "\n"
      << "Ticket: " << vehicle.ticketId << "\n"
      << "Vehicle: " << vehicle.vehicleNumber << " (" << vehicle.vehicleType << ")\n"
      << "Owner: " << vehicle.ownerName << "\n"
      << "Building / Floor / Slot: " << vehicle.parkingSlot << "\n"
      << "Parked at: " << vehicle.entryTime << "\n"
      << "Exited at: " << vehicle.exitTime << "\n"
      << "Actual duration: " << vehicle.durationSeconds / 3600 << "h "
      << (vehicle.durationSeconds % 3600) / 60 << "m "
      << vehicle.durationSeconds % 60 << "s\n"
      << "Started hours billed: " << vehicle.hours << "\n"
      << "Rate per hour: Rs." << (long long)hourlyParkingRate(vehicle.vehicleType) << "\n"
      << "TOTAL: Rs." << (long long)vehicle.parkingFee << "\n"
      << "=============================================\n";
    return o.str();
}
void exitMenu(ParkingSystem& system) {
    string plate;
    if (!askLine("Vehicle number to exit: ", plate)) return;
    cout << "Entry and exit timestamps determine started-hour billing automatically.\n";
    ExitResult result = system.exitVehicle(plate);
    cout << "\n" << result.message << '\n';
    if (!result.success) return;
    string receipt = receiptText(result.vehicle);
    cout << receipt;
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path folder = system.savedFile().parent_path() / "receipts";
    if (folder.empty()) folder = "receipts";
    fs::create_directories(folder, ec);
    fs::path file = folder / (result.vehicle.ticketId + ".txt");
    std::ofstream out(file);
    if (out) { out << receipt; out.close(); cout << "Receipt saved: " << file.string() << "\n"; }
    else cout << "Receipt is shown above; could not save separate text copy.\n";
    if (result.reassigned)
        cout << "Next waiting vehicle: " << result.nextVehicle.vehicleNumber
             << " -> " << result.nextVehicle.parkingSlot << '\n';
}
void waitingMenu(ParkingSystem& system) {
    while (true) {
        clearScreen();
        cout << "\n========== WAITING QUEUE CONTROL ==========\n";
        cout << "Waiting vehicles: " << system.waitingCount()
             << " | Priority: Emergency > VIP > Normal\n";
        cout << "Within the same priority: oldest waiting record first (FIFO).\n";
        cout << "\n-- Actual linked Queue (arrival order) --\n";
        system.showQueue();
        cout << "\n-- Dispatch order (priority + eligibility) --\n";
        int position = 0;
        for (const Vehicle& v : system.waitingByPriority()) {
            cout << "#" << ++position << " " << setw(14) << left << v.vehicleNumber
                 << " [" << (v.vehicleType == "Emergency" ? "EMERGENCY" : v.priority ? "VIP" : "NORMAL") << "]"
                 << " | Need: " << sizeName(requiredVehicleSize(v.vehicleType))
                 << " | Building: " << (v.preferredBuilding.empty() ? "Any" : v.preferredBuilding)
                 << " | Floor: " << (v.preferredFloor == 0 ? "Any" : to_string(v.preferredFloor))
                 << " | Suitable free: " << system.eligibleSlotsFor(v) << "\n";
        }
        if (position == 0) cout << "(queue empty)\n";
        cout << "\n1. Show full waiting records\n"
             << "2. Cancel waiting ticket by plate\n"
             << "3. Update waiting ticket VIP / Normal\n"
             << "0. Back to Admin Dashboard\n";
        int action;
        if (!askNumber("Choose: ", action, 0, 3) || action == 0) return;
        if (action == 1) printVehicles(system.waitingVehicles());
        else if (action == 2) {
            string plate, confirm;
            if (!askLine("Plate to cancel: ", plate)) return;
            if (!askLine("Confirm cancel (y/n): ", confirm)) return;
            if (confirm == "y" || confirm == "Y")
                cout << (system.cancelWaiting(plate) ? "Ticket cancelled and saved.\n"
                        : "Ticket not found / save failed.\n");
        } else if (action == 3) {
            string plate;
            int vip;
            if (!askLine("Waiting plate: ", plate)) return;
            if (!askNumber("Set priority (0=Normal, 1=VIP): ", vip, 0, 1)) return;
            cout << (system.changeWaitingVIP(plate, vip == 1)
                     ? "Priority updated; available compatible slots rechecked.\n"
                     : "Ticket not found or Emergency tickets cannot be modified.\n");
        }
        pauseScreen();
    }
}
void historyMenu(const ParkingSystem& system) {
    cout << "\n--- Exit History: latest first (Stack) ---\n";
    system.showStack();
    printVehicles(system.exitRecords());
    cout << "\n--- Doubly Linked List: oldest first (reverse) ---\n";
    printVehicles(system.historyBackward());
}
void algorithmsMenu(const ParkingSystem& system) {
    int choice;
    cout << "\n1. Bubble Sort by vehicle number (active + waiting)\n"
         << "2. Selection Sort by vehicle number (active + waiting)\n"
         << "3. Bubble Sort exit history by fee\n"
         << "4. Selection Sort exit history by fee\n"
         << "5. Linear Search by exact vehicle number\n"
         << "6. Binary Search by exact vehicle number\n"
         << "0. Back\n";
    if (!askNumber("Choose algorithm: ", choice, 0, 6) || choice == 0) return;
    vector<Vehicle> vehicles = system.parkedVehicles();
    auto queued = system.waitingVehicles();
    vehicles.insert(vehicles.end(), queued.begin(), queued.end());
    if (choice == 1) { bubbleSortByNumber(vehicles); printVehicles(vehicles); }
    else if (choice == 2) { selectionSortByNumber(vehicles); printVehicles(vehicles); }
    else if (choice == 3 || choice == 4) {
        vehicles = system.exitRecords();
        if (choice == 3) bubbleSortByFee(vehicles);
        else selectionSortByFee(vehicles);
        printVehicles(vehicles);
    } else {
        string plate;
        if (!askLine("Exact vehicle number (e.g. ABC-123): ", plate)) return;
        for (char& c : plate) c = (char)toupper((unsigned char)c);
        int index = -1;
        if (choice == 5) {
            index = linearSearchByNumber(vehicles, plate);
            cout << "Linear Search O(n): ";
        } else {
            selectionSortByNumber(vehicles); // Binary search requires sorted records.
            index = binarySearchByNumber(vehicles, plate);
            cout << "Binary Search O(log n) on sorted array: ";
        }
        if (index < 0) cout << "Not found.\n";
        else { cout << "Found at index " << index << ".\n"; vehicles[index].displayVehicle(); }
    }
}
void structuresMenu(ParkingSystem& system) {
    cout << "\n========== DSA STRUCTURES ==========" << '\n';
    cout << "\n1) Singly Linked List: currently parked vehicles\n";
    system.showSinglyList();
    cout << "\n2) Doubly Linked List: exit history (forward/backward)\n";
    system.showDoublyForward();
    system.showDoublyBackward();
    cout << "\n3) Circular Linked List: building-wise slot inspection route\n";
    system.showCircularList();
    cout << "Next inspection slot: " << system.inspectNextSlot() << '\n';
    cout << "\n4) Queue (FIFO): waiting vehicles\n";
    system.showQueue();
    cout << "\n5) Stack (LIFO): exit records\n";
    system.showStack();
    cout << "\n6) Dynamic Array (new[] / delete[]): active + waiting snapshot\n";
    DynamicVehicleArray records;
    for (const Vehicle& v : system.parkedVehicles()) records.add(v);
    for (const Vehicle& v : system.waitingVehicles()) records.add(v);
    cout << "Size: " << records.size() << " | Capacity: " << records.getCapacity() << '\n';
    for (int i = 0; i < records.size(); ++i)
        cout << "  Array[" << i << "]: " << records.at(i).vehicleNumber << '\n';
    cout << "====================================\n";
}
void statisticsMenu(const ParkingSystem& system) {
    cout << "\n======= SMARTPARK STATISTICS =======\n"
         << "Total parking slots: " << system.total() << '\n'
         << "Occupied: " << system.occupied() << '\n'
         << "Available: " << system.available() << '\n'
         << "Waiting vehicles: " << system.waitingCount() << '\n'
         << "Vehicles served: " << system.servedCount() << '\n'
         << "Total revenue: Rs." << system.totalRevenue() << '\n'
         << "C++ saved data: " << system.savedFile().string() << '\n'
         << "====================================\n";
}

// A tree visit has no loops: Inorder, Preorder and Postorder recurse over nodes.
void printOrder(const string& label, const vector<int>& keys) {
    cout << label << ": ";
    if (keys.empty()) cout << "(empty)";
    for (int key : keys) cout << key << " ";
    cout << '\n';
}
void printTreeOverview(const BSTree& tree) {
    tree.display(cout);
    cout << "Total nodes: " << tree.countNodes() << " | Leaf nodes: "
         << tree.countLeafNodes() << " | Height: " << tree.height() << '\n';
    cout << "Minimum: " << (tree.findMin() ? to_string(*tree.findMin()) : "N/A")
         << " | Maximum: " << (tree.findMax() ? to_string(*tree.findMax()) : "N/A") << '\n';
    printOrder("Inorder  (Left Root Right)", tree.inorder());
    printOrder("Preorder (Root Left Right)", tree.preorder());
    printOrder("Postorder(Left Right Root)", tree.postorder());
}
void printTreeOverview(const AVLTree& tree) {
    tree.display(cout);
    cout << "Total nodes: " << tree.countNodes() << " | Leaf nodes: "
         << tree.countLeafNodes() << " | Height: " << tree.height() << '\n';
    cout << "Minimum: " << (tree.findMin() ? to_string(*tree.findMin()) : "N/A")
         << " | Maximum: " << (tree.findMax() ? to_string(*tree.findMax()) : "N/A") << '\n';
    printOrder("Inorder  (Left Root Right)", tree.inorder());
    printOrder("Preorder (Root Left Right)", tree.preorder());
    printOrder("Postorder(Left Right Root)", tree.postorder());
}
void treesMenu(const ParkingSystem& system) {
    while (true) {
        clearScreen();
        cout << "\n======== BST & AVL TREES ========\n"
             << "1. Live BST of parked ticket IDs\n"
             << "2. Live AVL of parked ticket IDs (height + BF + rotations)\n"
             << "3. Search parked ticket using BST/AVL indexes\n"
             << "4. Validate both live tree indexes\n"
             << "0. Back\n";
        int choice;
        if (!askNumber("Choose: ", choice, 0, 4) || choice == 0) return;
        if (choice == 1) printTreeOverview(system.bst());
        if (choice == 2) {
            printTreeOverview(system.avl());
            if (system.avl().lastRotations().empty()) cout << "Last AVL operation: no rotation needed.\n";
            else for (const auto& log : system.avl().lastRotations()) cout << "ROTATION: " << log << '\n';
        }
        if (choice == 3) {
            int key;
            if (!askNumber("Ticket numeric part (e.g. SP-00000005 -> 5): ", key, 1, 99999999)) return;
            auto bstResult = system.findParkedByTicket(key, false);
            auto avlResult = system.findParkedByTicket(key, true);
            cout << "BST search: " << (bstResult ? "Found" : "Not found") << '\n';
            cout << "AVL search: " << (avlResult ? "Found" : "Not found") << '\n';
            if (avlResult) avlResult->displayVehicle();
        }
        if (choice == 4) cout << (system.treeIndexesValid() ? "PASS: BST and AVL contain exactly the parked vehicles.\n"
                                                          : "ERROR: one or more tree indexes do not match parking.\n");
        pauseScreen();
    }
}

void slotRulesMenu(ParkingSystem& system) {
    while (true) {
        clearScreen();
        cout << "\n========== SMART ALLOCATION & SLOT RULES ==========\n"
             << "1. Show slot restrictions and building map\n"
             << "2. Restrict / reserve / reopen a slot\n"
             << "3. Preview allocation (C++ decision explanation)\n"
             << "0. Back to main menu\n";
        int choice;
        if (!askNumber("Choose: ", choice, 0, 3) || choice == 0) return;
        if (choice == 1) {
            system.showParkingMap();
            cout << "\nRule key: Normal / Reserved (Emergency/VIP) / Disputed (closed).\n";
            cout << "Normal available: " << system.available() << " | Occupied: " << system.occupied() << "\n";
        } else if (choice == 2) {
            string id, note;
            int rule;
            if (!askLine("Slot ID (e.g. A-F1-S02): ", id)) return;
            id = upper(trimSpaces(id));
            if (!askNumber("Rule (0=Normal, 1=Reserved, 2=Disputed): ", rule, 0, 2)) return;
            if (rule != 0) {if (!askLine("Reason (e.g. Maintenance): ", note)) return;}
            if (system.setSlotRestriction(id, rule, note)) {
                cout << "Slot " << id << " rule saved successfully.\n";
                cout << "Blocked/Reserved spaces are not counted as regular available slots.\n";
            } else cout << "Change refused: invalid ID, occupied slot, invalid note, or save error.\n";
        } else {
            showTypes();
            int type, floor;
            string building, vipChoice;
            if (!askNumber("Vehicle type (1-11): ", type, 1, 11)) return;
            if (!askLine("VIP priority? (y/n): ", vipChoice)) return;
            if (!askLine("Preferred Building (A/B/C or Enter): ", building)) return;
            building = upper(trimSpaces(building));
            if (!askNumber("Preferred floor (0=any): ", floor, 0, system.floorsPerBuilding())) return;
            cout << "\nC++ recommendation: " << system.previewAllocation(getType(type),
                   vipChoice == "Y" || vipChoice == "y", building, floor) << "\n";
        }
        pauseScreen();
    }
}

void graphRoutesMenu(ParkingSystem& system) {
    while(true) {
        clearScreen();
        cout<<"\n========== CAMPUS PARKING GRAPH | BFS / DFS ==========\n"
            <<" ENTRY -> Building Gates -> Floor Ramps -> Parking Slots\n"
            <<" 1. List building / ramp access roads (with closure status)\n"
            <<" 2. BFS: Find route from ENTRY to a parking slot\n"
            <<" 3. DFS: Check whether a slot is reachable\n"
            <<" 4. Block or reopen a road segment (Admin)\n"
            <<" 5. Show all blocked roads\n"
            <<" 0. Back to dashboard\n"
            <<" Closed roads: "<<system.closedRoadCount()<<"\n";
        int choice;
        if(!askNumber("Choose: ",choice,0,5)||choice==0)return;
        clearScreen();
        if(choice==1) {
            cout<<"=== MAIN CAMPUS ROADS AND FLOOR RAMPS ===\n";
            for(const auto& r:system.roads())
                if(r.from.find("-S")==string::npos && r.to.find("-S")==string::npos)
                    cout<<left<<setw(11)<<r.from<<" <-> "<<setw(11)<<r.to
                        <<(r.blocked?" [BLOCKED]":" [OPEN]")<<'\n';
            cout<<"\nEvery slot also has a short road to its floor ramp.\n";
        } else if(choice==2 || choice==3) {
            string slot;
            if(!askLine("Destination slot ID (e.g. A-F1-S02): ",slot))return;
            slot=upper(trimSpaces(slot));
            bool exists=false;
            for(const auto& s:system.parkingSlots())if(s.slotNumber==slot)exists=true;
            if(!exists)cout<<"Invalid destination slot.\n";
            else if(choice==3)cout<<"DFS: "<<(system.dfsCanReach(slot)?"REACHABLE":"UNREACHABLE")<<" from ENTRY.\n";
            else {
                auto path=system.routeToSlot(slot);
                if(path.empty())cout<<"BFS: NO ROUTE. All available access paths are blocked.\n";
                else {
                    cout<<"BFS minimum-edge route ("<<path.size()-1<<" edges):\n";
                    for(size_t i=0;i<path.size();++i)cout<<(i?" -> ":"")<<path[i];
                    cout<<"\nNote: shortest in road segments, not metres.\n";
                }
            }
        } else if(choice==4) {
            string from,to;
            int mode;
            cout<<"Use road vertex names e.g. ENTRY, G-A, A-F1, A-F1-S01.\n";
            if(!askLine("Road FROM node: ",from)||!askLine("Road TO node: ",to))return;
            from=upper(trimSpaces(from));to=upper(trimSpaces(to));
            if(!askNumber("1=Block, 0=Reopen: ",mode,0,1))return;
            cout<<(system.setRoadBlocked(from,to,mode==1)?"Road rule saved. Waiting vehicles rechecked if reopened.\n"
                      :"Invalid road: enter two directly connected vertices.\n");
        } else if(choice==5) {
            cout<<"=== CLOSED ROAD SEGMENTS ===\n";
            int count=0;
            for(const auto& r:system.roads())if(r.blocked){
                cout<<r.from<<" <-> "<<r.to<<"\n";++count;
            }
            if(!count)cout<<"None. All roads open.\n";
        }
        pauseScreen();
    }
}

void phase6AlgorithmMenu(const ParkingSystem& system){
    while(true){
        clearScreen();
        cout<<"\n========== LIVE ALGORITHM ANALYSIS ==========\n"
            <<"1. Compare Linear Search / BST / AVL on one parked Ticket ID\n"
            <<"2. Compare Bubble vs Selection Sort (active + waiting plates)\n"
            <<"3. Compare Bubble vs Selection Sort (exit history fees)\n"
            <<"4. Show BST / AVL live integrity and height\n"
            <<"0. Back to Main Menu\n";
        int choice;
        if(!askNumber("Select: ",choice,0,4)||choice==0)return;
        clearScreen();
        if(choice==1){
            int key;
            if(!askNumber("Enter numeric ticket part (SP-00000003 => 3): ",key,1,99999999))return;
            const auto a=auditLiveTicket(system,key);
            cout<<"Ticket "<<key<<" | Parked vehicles: "<<system.parkedVehicles().size()<<"\n";
            cout<<"Linear Search: "<<(a.linearFound?"FOUND":"NOT FOUND")<<" | comparisons "<<a.recordsScanned<<"\n";
            for(const auto& entry:std::vector<std::pair<std::string,TreeSearchTrace>>{{"BST",a.bst},{"AVL",a.avl}}){
                cout<<entry.first<<": "<<(entry.second.found?"FOUND":"NOT FOUND")
                    <<" | comparisons "<<entry.second.visited.size()<<" | path: ";
                for(size_t i=0;i<entry.second.visited.size();++i)
                    cout<<(i?" -> ":"")<<entry.second.visited[i];
                if(entry.second.visited.empty())cout<<"(empty)";
                cout<<"\n";
            }
            cout<<"Live tree indexes: "<<(a.indexesValid?"VALID":"INVALID")<<"\n";
            cout<<"Note: one comparison per inspected record/node; measured on actual parked data.\n";
        }else if(choice==2||choice==3){
            string source=choice==2?"active":"history", field=choice==2?"number":"fee";
            auto records=phase6Dataset(system,source);
            auto b=measureSort(records,false,field),s=measureSort(records,true,field);
            cout<<"Dataset: "<<source<<" | Field: "<<field<<" | Records: "<<records.size()<<"\n";
            cout<<"Bubble:    comparisons="<<b.comparisons<<" swaps="<<b.swaps<<"\n";
            cout<<"Selection: comparisons="<<s.comparisons<<" swaps="<<s.swaps<<"\n";
            cout<<"Sorted records (copy only; parking unchanged):\n";
            printVehicles(b.sorted);
        }else{
            cout<<"Live BST: nodes="<<system.bst().countNodes()<<" height="<<system.bst().height()<<"\n";
            cout<<"Live AVL: nodes="<<system.avl().countNodes()<<" height="<<system.avl().height()<<"\n";
            cout<<"Indexes valid: "<<(system.treeIndexesValid()?"YES":"NO")<<"\n";
        }
        pauseScreen();
    }
}

// One identical C++ analytics algorithm in the independent standalone and web engines.
// Closed sessions come from a Doubly Linked List; active sessions come from the parking lot.
// The parked interval is [entry, exit), so a slot is already free at its exact exit time.
#ifndef SMARTPARK_TRACE8_HISTORY_H
#define SMARTPARK_TRACE8_HISTORY_H
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
namespace trace8 {
struct Session {
    std::string ticket, plate, owner, type, slot, building;
    int floor=0, hours=0;
    long long entry=0, exit=0, fee=0;
};
struct Window {
    long long start=0, end=0;
    std::string period, label;
};
struct Summary {
    int sessions=0, arrivals=0, departures=0, activeNow=0;
    long long revenue=0, occupiedMinutes=0;
    std::map<std::string, std::pair<int,long long>> buildings; // completed sessions, revenue
    std::map<std::string,long long> slotSeconds;
};
inline std::tm localClock(long long epoch) {
    std::time_t raw = static_cast<std::time_t>(epoch);
    std::tm value{};
#ifdef _WIN32
    localtime_s(&value, &raw);
#else
    localtime_r(&raw, &value);
#endif
    return value;
}
inline std::string showDate(long long epoch, const char* pattern="%Y-%m-%d %H:%M") {
    if(epoch<=0) return "-";
    std::tm value=localClock(epoch);
    std::ostringstream out;
    out<<std::put_time(&value,pattern);
    return out.str();
}
inline bool parseDate(const std::string& date, std::tm& out) {
    if(date.size()!=10 || date[4]!='-' || date[7]!='-')return false;
    for(size_t i=0;i<date.size();++i)
        if(i!=4 && i!=7 && (date[i]<'0'||date[i]>'9'))return false;
    out={}; out.tm_year=std::stoi(date.substr(0,4))-1900;
    out.tm_mon=std::stoi(date.substr(5,2))-1;
    out.tm_mday=std::stoi(date.substr(8,2)); out.tm_isdst=-1;
    if(out.tm_year < 100 || out.tm_year > 300 || out.tm_mon<0 || out.tm_mon>11 || out.tm_mday<1 || out.tm_mday>31)return false;
    std::tm original=out;
    std::time_t epoch=std::mktime(&out);
    if(epoch<0)return false;
    return out.tm_year==original.tm_year && out.tm_mon==original.tm_mon && out.tm_mday==original.tm_mday;
}
inline bool parseMoment(const std::string& input, long long& epoch) {
    if(input.size()!=16 || (input[10]!=' ' && input[10]!='T') || input[13]!=':') return false;
    std::tm date{};
    if(!parseDate(input.substr(0,10),date))return false;
    for(int i: {11,12,14,15})if(input[i]<'0'||input[i]>'9')return false;
    int hh=std::stoi(input.substr(11,2)), mm=std::stoi(input.substr(14,2));
    if(hh>23||mm>59)return false;
    date.tm_hour=hh; date.tm_min=mm; date.tm_sec=0;date.tm_isdst=-1;
    std::tm expected=date;
    std::time_t result=std::mktime(&date);
    if(result<0 || date.tm_year!=expected.tm_year || date.tm_mon!=expected.tm_mon ||
       date.tm_mday!=expected.tm_mday || date.tm_hour!=hh || date.tm_min!=mm)return false;
    epoch=static_cast<long long>(result);
    return true;
}
inline bool makeWindow(const std::string& period, const std::string& anchor, Window& w) {
    if(period!="week" && period!="month")return false;
    std::tm start{};
    if(!parseDate(anchor,start))return false;
    if(period=="week") start.tm_mday -= (start.tm_wday+6)%7; // Monday
    else start.tm_mday=1;
    start.tm_isdst=-1;
    std::time_t begin=std::mktime(&start);
    if(begin<0)return false;
    std::tm endTm=start;
    if(period=="week")endTm.tm_mday+=7;
    else endTm.tm_mon+=1;
    endTm.tm_isdst=-1;
    std::time_t until=std::mktime(&endTm);
    if(until<=begin)return false;
    w.start=static_cast<long long>(begin);
    w.end=static_cast<long long>(until);
    w.period=period;
    w.label=showDate(w.start,"%Y-%m-%d")+" to "+showDate(w.end,"%Y-%m-%d")+" (end exclusive)";
    return true;
}
inline Session fromVehicle(const Vehicle& v) {
    Session row;
    row.ticket=v.ticketId;row.plate=v.vehicleNumber;row.owner=v.ownerName;
    row.type=v.vehicleType;row.slot=v.parkingSlot;
    if(v.parkingSlot.size()>=4 && v.parkingSlot[1]=='-') {
        row.building=v.parkingSlot.substr(0,1);
        size_t start=v.parkingSlot.find("-F");
        if(start!=std::string::npos && start+2<v.parkingSlot.size()) {
            try {row.floor=std::stoi(v.parkingSlot.substr(start+2));}catch(...) {}
        }
    }
    row.entry=v.entryEpoch ? v.entryEpoch : parkingParseStamp(v.entryTime);
    row.exit=v.exitEpoch ? v.exitEpoch : parkingParseStamp(v.exitTime);
    row.fee=static_cast<long long>(v.parkingFee); row.hours=v.hours;
    return row;
}
inline std::vector<Session> allSessions(const ParkingSystem& system) {
    std::vector<Session> rows;
    for(const auto& v:system.exitRecords())rows.push_back(fromVehicle(v));
    for(const auto& v:system.parkedVehicles())rows.push_back(fromVehicle(v));
    return rows;
}
inline bool intersects(const Session& row, const Window& w) {
    return row.entry>0 && row.entry<w.end && (row.exit==0 || row.exit>w.start);
}
inline Summary summarize(const std::vector<Session>& rows, const Window& w, long long now=parkingNowEpoch()) {
    Summary result;
    for(const auto& row:rows) {
        if(row.entry<=0)continue;
        if(w.start<=row.entry && row.entry<w.end)++result.arrivals;
        if(row.exit>0 && w.start<=row.exit && row.exit<w.end) {
            ++result.departures;result.revenue+=row.fee;
            auto& perBuilding=result.buildings[row.building];
            ++perBuilding.first;perBuilding.second+=row.fee;
        }
        if(!intersects(row,w))continue;
        ++result.sessions;
        if(row.exit==0)++result.activeNow;
        long long lower=std::max(row.entry,w.start);
        long long upper=std::min(w.end,row.exit>0?row.exit:now);
        upper=std::min(upper,now); // Do not count future occupancy as actual usage
        if(upper>lower) {
            long long seconds=upper-lower;
            result.slotSeconds[row.slot]+=seconds;
        }
    }
    long long totalSeconds=0;
    for(const auto& pair:result.slotSeconds)totalSeconds+=pair.second;
    result.occupiedMinutes=totalSeconds/60;
    return result;
}
inline std::vector<Session> filterSessions(const std::vector<Session>& rows, const Window& w,
      const std::string& building="", int floor=0, const std::string& slot="", const std::string& plate="") {
    std::vector<Session> found;
    for(const auto& row:rows) {
        if(!intersects(row,w))continue;
        if(!building.empty() && row.building!=building)continue;
        if(floor>0 && row.floor!=floor)continue;
        if(!slot.empty() && row.slot!=slot)continue;
        if(!plate.empty() && row.plate!=plate)continue;
        found.push_back(row);
    }
    std::sort(found.begin(),found.end(),[](const Session& a,const Session& b){
        return a.entry!=b.entry ? a.entry>b.entry : a.ticket>b.ticket;
    });
    return found;
}
inline std::optional<Session> whoOccupied(const std::vector<Session>& rows,
                                         const std::string& slot,long long moment) {
    std::optional<Session> result;
    for(const auto& row:rows) {
        if(row.slot==slot && row.entry>0 && row.entry<=moment && (row.exit==0 || row.exit>moment)) {
            if(!result || row.entry>result->entry)result=row;
        }
    }
    return result;
}
} // namespace trace8
#endif

void parkingRatesMenu() {
    const vector<string> types = {"Car", "Motorcycle", "Van", "Emergency", "Sedan", "SUV", "Sports Car", "Pickup", "Bus", "Taxi", "Truck"};
    while (true) {
        clearScreen();
        cout << "SMARTPARK | PARKING RATES (PKR PER STARTED HOUR)\n" << string(55,'-') << "\n";
        for(size_t i=0;i<types.size();++i)
            cout << setw(2) << i+1 << ". " << left << setw(18) << types[i] << " Rs. " << (long long)hourlyParkingRate(types[i]) << "\n";
        cout << "0. Back\n";
        int choice=0;
        if(!askNumber("Select vehicle type to edit: ",choice,0,(int)types.size()) || choice==0)return;
        int amount=0;
        if(!askNumber("New hourly rate (Rs. 1-1000000): ",amount,1,1000000))return;
        const char* env=std::getenv("SMARTPARK_RATE_FILE");
        fs::path path=env && *env ? env : "data/parking_rates.txt";
        std::error_code ec;
        if(path.has_parent_path())fs::create_directories(path.parent_path(),ec);
        if(ec){cout<<"Could not create rate folder.\n";pauseScreen();continue;}
        map<string,long long> all;
        for(const auto& name:types)all[name]=(long long)hourlyParkingRate(name);
        all[types[choice-1]]=amount;
        fs::path temp=path.string()+".tmp";
        {
            ofstream output(temp,ios::trunc);
            for(const auto& row:all)output<<quoted(row.first)<<" "<<row.second<<"\n";
            output.flush();
            if(!output.good()){cout<<"Cannot write rate file.\n";pauseScreen();continue;}
        }
        fs::path backup=path.string()+".bak";
        fs::remove(backup,ec);
        ec.clear();
        if(fs::exists(path))fs::rename(path,backup,ec);
        if(!ec){fs::rename(temp,path,ec);}
        if(ec && fs::exists(backup)){std::error_code restore;fs::rename(backup,path,restore);}
        if(!ec){std::error_code cleanup;fs::remove(backup,cleanup);}
        cout<<(ec ? "Rate update failed; old rates restored." : "Hourly rate updated and saved.")<<"\n";
        pauseScreen();
    }
}

void phase8HistoryMenu(const ParkingSystem& system) {
    while(true) {
        clearScreen();
        cout<<"\n======== HISTORICAL REPORT CENTER ========\n"
            <<"1. Weekly History & Revenue (Monday-Sunday)\n"
            <<"2. Monthly History & Revenue\n"
            <<"3. Who occupied a slot at a past date/time?\n"
            <<"4. Show recent completed sessions\n"
            <<"5. Admin action audit log\n"
            <<"0. Back to Main Menu\n";
        int choice;
        if(!askNumber("Select: ",choice,0,5)||choice==0)return;
        clearScreen();
        auto sessions=trace8::allSessions(system);
        if(choice==1 || choice==2) {
            string date;
            if(!askLine("Enter a date (YYYY-MM-DD): ",date))return;
            trace8::Window window;
            if(!trace8::makeWindow(choice==1?"week":"month",date,window)) {
                cout<<"Invalid date. Example: 2026-11-13\n";
                pauseScreen();continue;
            }
            cout<<"\n"<<window.label<<"\n";
            string building,slot,plate;
            int floor=0;
            if(!askLine("Building filter (A/B/C, Enter for all): ",building))return;
            building=upper(trimSpaces(building));
            if(!building.empty() && building!="A" && building!="B" && building!="C") {
                cout<<"Invalid building.\n";pauseScreen();continue;
            }
            if(!askNumber("Floor (0=all, 1-4): ",floor,0,4))return;
            if(!askLine("Slot (Enter=all, e.g. A-F1-S02): ",slot))return;
            slot=upper(trimSpaces(slot));
            if(!askLine("Vehicle plate (Enter=all): ",plate))return;
            plate=upper(trimSpaces(plate));
            auto rows=trace8::filterSessions(sessions,window,building,floor,slot,plate);
            auto metrics=trace8::summarize(rows,window); //  filtered summary must match filtered records
            cout<<"Selected records: Arrivals="<<metrics.arrivals<<" | Departures="<<metrics.departures
                <<" | Revenue=Rs."<<metrics.revenue<<" | Occupied vehicle-minutes="<<metrics.occupiedMinutes<<"\n";
            cout<<"Matching sessions: "<<rows.size()<<" (including stays crossing date boundaries)\n";
            cout<<left<<setw(13)<<"Ticket"<<setw(13)<<"Plate"<<setw(13)<<"Slot"
                <<setw(19)<<"Entry"<<setw(19)<<"Exit"<<"Fee\n"<<string(85,'-')<<"\n";
            size_t maxRows=std::min(rows.size(),(size_t)60);
            for(size_t i=0;i<maxRows;++i) {
                const auto& v=rows[i];
                cout<<left<<setw(13)<<v.ticket<<setw(13)<<v.plate<<setw(13)<<v.slot
                    <<setw(19)<<trace8::showDate(v.entry)<<setw(19)<<(v.exit?trace8::showDate(v.exit):"PARKED")
                    <<"Rs."<<v.fee<<"\n";
            }
            if(rows.size()>maxRows)cout<<"Showing first 60; narrow filters to see more.\n";
            cout<<"\nRevenue by building (based on completed exits in period):\n";
            for(const auto& b:metrics.buildings)
                cout<<"Building "<<b.first<<": "<<b.second.first<<" exits, Rs."<<b.second.second<<"\n";
            cout<<"\nMost-used slots by occupied time in selected period:\n";
            vector<pair<string,long long>> ranked(metrics.slotSeconds.begin(),metrics.slotSeconds.end());
            sort(ranked.begin(),ranked.end(),[](const auto& a,const auto& b){return a.second>b.second;});
            for(size_t i=0;i<std::min(ranked.size(),(size_t)5);++i)
                cout<<ranked[i].first<<": "<<ranked[i].second/60<<" occupied minutes\n";
        }else if(choice==3) {
            string slot,when;
            if(!askLine("Slot ID (e.g. A-F1-S02): ",slot) ||
               !askLine("Date/time (YYYY-MM-DD HH:MM): ",when))return;
            slot=upper(trimSpaces(slot));
            long long moment=0;
            if(!trace8::parseMoment(when,moment) || moment>parkingNowEpoch()) {
                cout<<"Enter a valid past local date/time.\n";pauseScreen();continue;
            }
            bool exists=false;
            for(const auto& s:system.parkingSlots())if(s.slotNumber==slot){exists=true;break;}
            if(!exists) {cout<<"Slot does not exist in current building layout.\n";pauseScreen();continue;}
            auto found=trace8::whoOccupied(sessions,slot,moment);
            if(found)cout<<"OCCUPIED: "<<found->plate<<" | "<<found->ticket
                <<" | Entry "<<trace8::showDate(found->entry)
                <<" | Exit "<<(found->exit?trace8::showDate(found->exit):"Still parked")<<"\n";
            else cout<<"NO RECORDED OCCUPANCY at that time (not proof the physical slot was free).\n";
        }else if(choice==5){
            fs::path audit=system.savedFile();audit+=".audit.log";
            ifstream in(audit);
            cout<<"Persisted C++ admin actions: "<<audit.string()<<"\n";
            if(!in)cout<<"No events saved yet.\n";
            else {
                vector<string> lines;string line;
                while(getline(in,line))lines.push_back(line);
                size_t start=lines.size()>60?lines.size()-60:0;
                for(size_t i=lines.size();i>start;--i)cout<<lines[i-1]<<"\n";
            }
        }else{
            cout<<"Completed sessions stored in Doubly Linked List + file:\n";
            size_t count=0;
            for(const auto& row:sessions)if(row.exit>0) {
                cout<<row.ticket<<" | "<<row.plate<<" | "<<row.slot<<" | "
                    <<trace8::showDate(row.entry)<<" to "<<trace8::showDate(row.exit)
                    <<" | Rs."<<row.fee<<"\n";
                if(++count==40)break;
            }
            if(!count)cout<<"No completed sessions yet.\n";
        }
        pauseScreen();
    }
}

// ============ SMART SAFETY & GROUP OPERATIONS ============
void smartSafetyMenu(ParkingSystem& parking){
    while(true){
        clearScreen();
        cout<<"\n======= SMART SAFETY & GROUP OPERATIONS =======\n"
            <<"1. Find My Car (Plate AVL + Walking BFS + Stack)\n"
            <<"2. Start Building Evacuation\n"
            <<"3. End Evacuation\n"
            <<"4. Provisional VIP Reservation\n"
            <<"5. Confirm VIP Arrival\n"
            <<"6. Family / Convoy Parking (3-4 cars)\n"
            <<"7. View VIP Holds & Group Queue\n"
            <<"0. Back to Admin Menu\n";
        int option;if(!askNumber("Choose: ",option,0,7))return;
        if(option==0)return;
        clearScreen();
        if(option==1){
            string plate;if(askLine("Number plate: ",plate)){
                auto r=parking.findMyCar(plate);
                cout<<r.message<<'\n';
                if(r.found)cout<<"Slot: "<<r.slot<<" | AVL comparisons: "<<r.comparisons<<'\n';
                if(r.reachable)cout<<"Walking directions ("<<r.edges<<" edges):\n"<<r.steps<<'\n';
            }
        }else if(option==2){
            string building;
            if(askLine("Affected building (A/B/C): ",building)){
                building=upper(trimSpaces(building));
                bool ok=building.size()==1&&parking.setEvacuation(building[0],true);
                cout<<(ok?"Evacuation activated. Normal inbound traffic is locked.\n":"Invalid building or could not save.\n");
                if(ok){
                    auto rows=parking.evacuationRoutes();
                    if(rows.empty())cout<<"No parked vehicles inside affected building.\n";
                    for(const auto&r:rows)cout<<r.plate<<" ["<<r.slot<<"] -> "
                       <<(r.reachable?r.path:"NO SAFE EXIT ROUTE - REQUEST ASSISTANCE")<<'\n';
                }
            }
        }else if(option==3){
            cout<<(parking.setEvacuation('-',false)?"Evacuation ended. Admission unlocked.\n":"Save failure.\n");
        }else if(option==4){
            string plate,owner,building;int kind,floor;
            if(!askLine("VIP vehicle plate: ",plate)||!askLine("Owner: ",owner))return;
            showTypes();if(!askNumber("Type (1-11): ",kind,1,11)||
                !askLine("Preferred building (A/B/C or Enter): ",building)||
                !askNumber("Floor (0=any): ",floor,0,parking.floorsPerBuilding()))return;
            auto r=parking.reserveVIP(plate,getType(kind),owner,upper(trimSpaces(building)),floor);
            cout<<r.message<<"\n";if(r.success)cout<<"Hold: "<<r.slot<<" | Route: "<<r.route<<'\n';
        }else if(option==5){
            string plate;if(!askLine("Arriving VIP plate: ",plate))return;
            auto r=parking.confirmVIP(plate);
            cout<<r.message<<"\n";
        }else if(option==6){
            traceccp::Group group;
            if(!askNumber("How many cars (3-4): ",group.count,3,4))return;
            if(!askLine("Preferred building (A/B/C or Enter): ",group.building)||
               !askNumber("Floor (0=any): ",group.floor,0,parking.floorsPerBuilding()))return;
            group.building=upper(trimSpaces(group.building));
            string priority;
            if(!askLine("VIP group (y/n): ",priority))return;
            group.vip=priority=="y"||priority=="Y";
            for(int i=0;i<group.count;++i){
                cout<<"Vehicle "<<(i+1)<<" / "<<group.count<<"\n";
                int kind;
                if(!askLine("Plate: ",group.plate[i])||!askLine("Owner: ",group.owner[i]))return;
                showTypes();if(!askNumber("Type: ",kind,1,11))return;
                group.type[i]=getType(kind);
            }
            auto r=parking.submitConvoy(group);
            cout<<r.message<<"\n";
            if(r.success){cout<<"Group ID: "<<r.id<<'\n';
                if(!r.queued)for(int i=0;i<r.count;++i)cout<<group.plate[i]<<" -> "<<r.slots[i]<<'\n';
            }
        }else if(option==7){
            cout<<"Evacuation: "<<(parking.evacuationActive()?"ACTIVE":"OFF")<<'\n';
            cout<<"Latest recovery: "<<parking.lastRecovery()<<'\n';
            for(const auto&h:parking.vipReservations())cout<<"VIP HOLD "<<h.plate<<" / "<<h.slot<<"\n";
            for(const auto&g:parking.groupQueue()){
                cout<<"GROUP "<<g.id<<" ("<<g.count<<" vehicles, "<<(g.vip?"VIP":"Normal")<<")\n";
                for(int i=0;i<g.count;++i)cout<<"  - "<<g.plate[i]<<'\n';
            }
        }
        pauseScreen();
    }
}

void showMenu() {
    cout << "\n=========== SMARTPARK C++ ===========\n"
         << " 1. Park Vehicle\n"
         << " 2. Exit Vehicle / Automatic Hourly Receipt\n"
         << " 3. Search Vehicle (number, owner, type, slot, ticket)\n"
         << " 4. Display Parking Slots\n"
         << " 5. Waiting Queue & Priority Queue\n"
         << " 6. Exit History (Stack / Doubly List)\n"
         << " 7. Searching & Sorting Algorithms\n"
         << " 8. Show DSA Structures (including Circular List)\n"
         << " 9. Statistics\n"
         << "10. Save Data\n"
         << "11. Reset Parking Data (creates backup)\n"
         << "12. BST & AVL Trees (live vehicle indexes)\n"
         << "13. Logout (return to Admin Login)\n"
         << "14. Buildings, Floors & Slot Configuration\n"
         << "15. Smart Allocation Preview & Slot Restrictions\n"
         << "16. Graph Routes / BFS / DFS / Blocked Roads\n"
         << "17. Live Algorithm Performance Lab\n"
         << "18. Weekly / Monthly History & Slot Lookup\n"
         << "19. Parking Rates (Admin)\n"
         << "20. Smart Safety / Find My Car / Convoys\n"
         << " 0. Exit Program\n"
         << "=====================================\n";
}
int main(int argc, char* argv[]) {
    string dataPath = "data/cpp_state.txt";
    if (argc == 3 && string(argv[1]) == "--data") dataPath = argv[2];
    else if (argc != 1) {
        cerr << "Usage: smartpark_cpp.exe [--data path/to/cpp_state.txt]\n";
        return 2;
    }
    AdminLogin admin(dataPath);
    ParkingSystem parking(dataPath);

    bool exitProgram = false;
    while (!exitProgram && admin.authenticate()) {
        while (true) {
            clearScreen();
            cout << "SMARTPARK TRACE | ADMIN CONTROL CENTER\n";
            cout << "-----------------------------------------------\n";
            cout << "Total: " << parking.total() << "  Available: " << parking.available()
                 << "  Occupied: " << parking.occupied() << "  Waiting: " << parking.waitingCount() << '\n'
                 << "Revenue: Rs." << parking.totalRevenue() << " | Saved: " << dataPath << '\n';
            showMenu();
            int choice;
            if (!askNumber("Choose a menu option: ", choice, 0, 20)) { exitProgram = true; break; }
            if (choice == 0) { exitProgram = true; break; }
            if (choice == 13) { cout << "Logged out.\n"; break; }
            clearScreen();
            cout << "SMARTPARK TRACE | ADMIN OPERATION " << choice << "\n"
                 << string(52, '-') << '\n';
            if (choice == 1) parkMenu(parking);
            else if (choice == 2) exitMenu(parking);
            else if (choice == 3) {
                string query;
                if (askLine("Enter search keyword: ", query)) printVehicles(parking.search(query));
            }
            else if (choice == 4) parking.showParkingMap();
            else if (choice == 5) waitingMenu(parking);
            else if (choice == 6) historyMenu(parking);
            else if (choice == 7) algorithmsMenu(parking);
            else if (choice == 8) structuresMenu(parking);
            else if (choice == 9) statisticsMenu(parking);
            else if (choice == 10) cout << (parking.save() ? "Data saved.\n" : "Save failed.\n");
            else if (choice == 15) slotRulesMenu(parking);
            else if (choice == 16) graphRoutesMenu(parking);
            else if (choice == 17) phase6AlgorithmMenu(parking);
            else if (choice == 18) phase8HistoryMenu(parking);
            else if (choice == 19) parkingRatesMenu();
            else if (choice == 20) smartSafetyMenu(parking);
            else if (choice == 14) {
                cout << "Buildings: A / B / C | Floors each: " << parking.floorsPerBuilding()
                     << " | Slots per floor: " << parking.slotsPerFloor() << "\n";
                cout << "Total Capacity: " << parking.total() << " | Occupied: " << parking.occupied() << "\n";
                cout << "Capacity categories: Small / Medium / Large\n";
                cout << "NOTE: Reconfiguration only when no vehicles parked or waiting.\n";
                string confirm;
                if (askLine("Change floor and slot counts? (y/n): ", confirm) && (confirm == "y" || confirm == "Y")) {
                    int floors, slots;
                    if (askNumber("Floors per building (1-4): ", floors, 1, 4) &&
                        askNumber("Slots per floor (3-20): ", slots, 3, 20)) {
                        cout << (parking.configureLayout(floors, slots) ? "Layout updated and saved.\n"
                            : "Configuration denied. Ensure parking and queue are empty.\n");
                    }
                }
            }
            else if (choice == 12) treesMenu(parking);
            else if (choice == 11) {
                string confirm;
                if (askLine("Type RESET to clear all C++ data (backup created): ", confirm)) {
                    if (confirm == "RESET")
                        cout << (parking.resetWithBackup() ? "Data reset successfully.\n" : "Reset failed.\n");
                    else cout << "Reset cancelled.\n";
                }
            }
            pauseScreen();
        }
    }
    cout << "Goodbye! C++ data remains saved.\n";
    return 0;
}
