# SmartPark TRACE — Exact New CCP C++ Implementation

Riphah International University, Islamabad | DSA | Sir Mueed Ahmad

Muhammad Salman (72373), Shaheryar Khan (72775), Shan (72387)

This is a verbatim readable copy of the **implemented** raw-pointer C++ feature source. The actual compiled versions are `backend/TraceCCP.h`, `backend/TraceCCPMethods.inc`, and their embedded single-file standalone equivalent at `Standalone_CPP/SmartPark.cpp`. The `ParkingSystem` declarations, connected mutations and save/load implementation are in `backend/ParkingSystem.h` and `ParkingSystem.cpp`.

## Raw data structures and algorithms (`TraceCCP.h`)

```cpp
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

```

## Live parking engine methods (`TraceCCPMethods.inc`)

```cpp
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
    ccp.evacuation=old;ccp.incident=previous;return false;
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


```
