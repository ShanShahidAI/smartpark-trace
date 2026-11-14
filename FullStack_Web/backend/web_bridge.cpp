// SmartPark Phase 4: C++ engine executable called by the Flask HTTP adapter.
// All parking mutations and BST/AVL tree operations happen in the C++ classes.
// Protocol: smartpark_bridge <parking-file> <command> [args...]
#include "ParkingSystem.h"
#include "FeeCalculator.h"
#include "AlgorithmLab.h"
#include "TimeBilling.h"
#include "HistoryAnalytics.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
using namespace std;
namespace fs = std::filesystem;

namespace {
// One lock across all bridge processes, not just Flask threads.
// Prevents lost updates/double-allocation when two staff submit together.
class FileLock {
    fs::path folder;
    bool held = false;
public:
    explicit FileLock(const fs::path& state) : folder(state.string() + ".lockdir") {
        std::error_code ec;
        if (folder.has_parent_path()) fs::create_directories(folder.parent_path(),ec);
        for(int attempt=0; attempt<100 && !held; ++attempt) {
            ec.clear();
            held = fs::create_directory(folder,ec);
            if (!held) std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    bool acquired() const { return held; }
    ~FileLock() { if (held) { std::error_code ec; fs::remove(folder,ec); } }
};
string jsonString(const string& s) {
    string out = "\"";
    constexpr char hex[] = "0123456789abcdef";
    for (unsigned char c : s) {
        switch(c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    out += "\\u00"; out += hex[c>>4]; out += hex[c&15];
                } else out += char(c);
        }
    }
    return out + '"';
}
string vehicleJson(const Vehicle& v) {
    ostringstream out;
    out << "{\"ticketId\":" << jsonString(v.ticketId)
        << ",\"vehicleNumber\":" << jsonString(v.vehicleNumber)
        << ",\"vehicleType\":" << jsonString(v.vehicleType)
        << ",\"ownerName\":" << jsonString(v.ownerName)
        << ",\"parkingSlot\":" << jsonString(v.parkingSlot)
        << ",\"entryTime\":" << jsonString(v.entryTime)
        << ",\"exitTime\":" << jsonString(v.exitTime)
        << ",\"parkingFee\":" << static_cast<long long>(v.parkingFee)
        << ",\"priority\":" << (v.priority ? "true" : "false")
        << ",\"hours\":" << v.hours
        << ",\"entryEpoch\":" << v.entryEpoch
        << ",\"exitEpoch\":" << v.exitEpoch
        << ",\"durationSeconds\":" << v.durationSeconds
        << ",\"sizeRequired\":" << jsonString(sizeName(requiredVehicleSize(v.vehicleType)))
        << ",\"preferredBuilding\":" << jsonString(v.preferredBuilding)
        << ",\"preferredFloor\":" << v.preferredFloor << '}';
    return out.str();
}
string vehiclesJson(const vector<Vehicle>& vehicles) {
    string out = "[";
    for (size_t i=0;i<vehicles.size();++i) {
        if(i) out += ',';
        out += vehicleJson(vehicles[i]);
    }
    return out + ']';
}
string intsJson(const vector<int>& values) {
    string out = "[";
    for (size_t i=0;i<values.size();++i) {
        if (i) out += ',';
        out += to_string(values[i]);
    }
    return out + ']';
}
string stringsJson(const vector<string>& values) {
    string out = "[";
    for (size_t i=0;i<values.size();++i) {
        if(i) out += ',';
        out += jsonString(values[i]);
    }
    return out + ']';
}
template<typename Tree>
string treeJson(const Tree& tree) {
    auto nodes = tree.snapshot();
    string result = "{\"root\":";
    result += nodes.empty() ? "null" : to_string(nodes.front().data);
    result += ",\"height\":" + to_string(tree.height()) + ",\"nodeCount\":" + to_string(tree.countNodes());
    result += ",\"leafCount\":" + to_string(tree.countLeafNodes());
    result += ",\"valid\":" + string(tree.verify() ? "true" : "false");
    result += ",\"inorder\":" + intsJson(tree.inorder());
    result += ",\"preorder\":" + intsJson(tree.preorder());
    result += ",\"postorder\":" + intsJson(tree.postorder());
    result += ",\"nodes\":[";
    for(size_t i=0;i<nodes.size();++i) {
        auto n=nodes[i];
        if (i) result += ',';
        result += "{\"key\":" + to_string(n.data) + ",\"vehicleNumber\":" + jsonString(n.vehicleNumber);
        result += ",\"height\":" + to_string(n.height) + ",\"bf\":" + to_string(n.balanceFactor);
        result += ",\"left\":" + (n.hasLeft ? to_string(n.left) : "null");
        result += ",\"right\":" + (n.hasRight ? to_string(n.right) : "null");
        result += '}';
    }
    return result + "]}";
}
string bothTrees(const BSTree& bst,const AVLTree& avl) {
    return "{\"bst\":" + treeJson(bst) + ",\"avl\":" + treeJson(avl)
        + ",\"indexesValid\":" + (bst.inorder() == avl.inorder() && bst.verify() && avl.verify() ? "true" : "false") + '}';
}
string routeJson(const ParkingSystem& system,const string& slot) {
    auto path=system.routeToSlot(slot);
    bool valid=false;
    for(const auto& position:system.parkingSlots())if(position.slotNumber==slot)valid=true;
    if(!valid)return "{\"error\":\"Unknown parking slot\"}";
    string message=path.empty() ? "No accessible route: one or more road segments are blocked."
       : "BFS selected the minimum number of connected road segments (not necessarily shortest distance).";
    return "{\"slot\":"+jsonString(slot)+",\"reachable\":"+(path.empty()?"false":"true")+
       ",\"dfsReachable\":"+(system.dfsCanReach(slot)?"true":"false")+
       ",\"edges\":"+(path.empty()?"0":to_string(path.size()-1))+
       ",\"path\":"+stringsJson(path)+",\"message\":"+jsonString(message)+"}";
}
string graphJson(const ParkingSystem& system) {
    const auto roads=system.roads();
    string result="{\"origin\":\"ENTRY\",\"closedCount\":"+to_string(system.closedRoadCount())+",\"roads\":[";
    for(size_t i=0;i<roads.size();++i){
        if(i)result+=',';
        const auto& r=roads[i];
        result+="{\"from\":"+jsonString(r.from)+",\"to\":"+jsonString(r.to)+
          ",\"blocked\":"+(r.blocked?"true":"false")+'}';
    }
    result+="]}";
    return result;
}
string stateJson(const ParkingSystem& s) {
    string result="{\"slots\":[";
    const auto holds=s.vipReservations();
    bool first=true;
    for (const ParkingSlot& slot : s.parkingSlots()) {
        if (!first) result+=',';
        first=false;
        result += "{\"id\":" + jsonString(slot.slotNumber);
        result += ",\"building\":" + jsonString(slot.building);
        result += ",\"floor\":" + to_string(slot.floor);
        result += ",\"size\":" + jsonString(sizeName(slot.capacity));
        const bool held=std::any_of(holds.begin(),holds.end(),[&](const auto&h){return h.slot==slot.slotNumber;});
        result += ",\"occupied\":" + string(slot.occupied ? "true" : "false");
        result += ",\"held\":" + string(held?"true":"false");
        result += ",\"restriction\":" + to_string(slot.restriction);
        result += ",\"restrictionNote\":" + jsonString(slot.restrictionNote);
        result += ",\"reachable\":" + string(s.dfsCanReach(slot.slotNumber)?"true":"false");
        result += ",\"status\":" + jsonString(slot.occupied ? "Occupied" : held ? "VIP Hold" : slot.restriction==1 ? "Reserved" : slot.restriction==2 ? "Disputed" : !s.dfsCanReach(slot.slotNumber) ? "Unreachable" : "Available");
        result += ",\"vehicle\":" + (slot.occupied ? vehicleJson(slot.vehicle) : "null");
        result += '}';
    }
    result += "],\"buildings\":[";
    for(char b='A';b<='C';++b) {
        if(b!='A')result+=',';
        int total=0,used=0,restricted=0,unreachable=0;
        for (const auto& slot:s.parkingSlots()) if(slot.building==string(1,b)){
            ++total; if(slot.occupied)++used; if(!slot.occupied && slot.restriction)++restricted;
            if(!slot.occupied && !slot.restriction && !s.dfsCanReach(slot.slotNumber))++unreachable;
        }
        int holdsInBuilding=0;
        for(const auto&h:holds)if(h.slot.size()&&h.slot[0]==b)++holdsInBuilding;
        result += "{\"id\":" + jsonString(string(1,b)) + ",\"total\":" + to_string(total)
               + ",\"occupied\":" + to_string(used) + ",\"available\":" + to_string(std::max(0,total-used-restricted-unreachable-holdsInBuilding))
               + ",\"restricted\":" + to_string(restricted) + ",\"unreachable\":" + to_string(unreachable) + "}";
    }
    result += "],\"layout\":{\"floorsPerBuilding\":" + to_string(s.floorsPerBuilding())
           + ",\"slotsPerFloor\":" + to_string(s.slotsPerFloor())
           + ",\"buildingCount\":3}";
    result+= ",\"waiting\":"+vehiclesJson(s.waitingVehicles());
    result+= ",\"history\":"+vehiclesJson(s.exitRecords());
    result+= ",\"totalSlots\":"+to_string(s.total());
    result+= ",\"available\":"+to_string(s.available());
    int restricted=0; for(const auto& slot : s.parkingSlots()) if(!slot.occupied && slot.restriction) ++restricted;
    result+= ",\"restricted\":"+to_string(restricted);
    int unreachable=0;
    for(const auto& slot:s.parkingSlots()) if(!slot.occupied && !s.dfsCanReach(slot.slotNumber)) ++unreachable;
    result+= ",\"unreachable\":"+to_string(unreachable);
    result+= ",\"vipHolds\":"+to_string(holds.size());
    result+= ",\"occupied\":"+to_string(s.occupied());
    result+= ",\"waitingCount\":"+to_string(s.waitingCount());
    result+= ",\"revenue\":"+to_string(s.totalRevenue());
    result+= ",\"served\":"+to_string(s.servedCount());
    return result+'}';
}
string historySessionJson(const trace8::Session& s) {
    return "{\"ticketId\":"+jsonString(s.ticket)+",\"vehicleNumber\":"+jsonString(s.plate)
      +",\"ownerName\":"+jsonString(s.owner)+",\"vehicleType\":"+jsonString(s.type)
      +",\"parkingSlot\":"+jsonString(s.slot)+",\"building\":"+jsonString(s.building)
      +",\"floor\":"+to_string(s.floor)+",\"entryEpoch\":"+to_string(s.entry)
      +",\"exitEpoch\":"+to_string(s.exit)+",\"hours\":"+to_string(s.hours)
      +",\"parkingFee\":"+to_string(s.fee)+"}";
}
string historySummaryJson(const trace8::Summary& s) {
    string result="{\"sessions\":"+to_string(s.sessions)+",\"arrivals\":"+to_string(s.arrivals)
        +",\"departures\":"+to_string(s.departures)+",\"revenue\":"+to_string(s.revenue)
        +",\"occupiedMinutes\":"+to_string(s.occupiedMinutes)+",\"buildings\":[";
    bool first=true;
    for(const auto& b:s.buildings) {
        if(!first)result+=',';first=false;
        result+="{\"building\":"+jsonString(b.first)+",\"departures\":"+to_string(b.second.first)
            +",\"revenue\":"+to_string(b.second.second)+"}";
    }
    result+="] ,\"topSlots\":[";
    vector<pair<string,long long>> ranked(s.slotSeconds.begin(),s.slotSeconds.end());
    sort(ranked.begin(),ranked.end(),[](const auto& a,const auto& b){return a.second>b.second;});
    for(size_t i=0;i<min(ranked.size(),(size_t)8);++i){
        if(i)result+=',';
        result+="{\"slot\":"+jsonString(ranked[i].first)+",\"occupiedMinutes\":"+to_string(ranked[i].second/60)+"}";
    }
    return result+"]}";
}

int fail(const string& error) { cout << "{\"error\":" << jsonString(error) << "}\n"; return 3; }
optional<int> parsePositive(const string& text,int low=1,int high=99999999) {
    if (text.empty()||text.size()>10||!all_of(text.begin(),text.end(),[](unsigned char c){return isdigit(c)!=0;}))return nullopt;
    try { long long n=stoll(text); if(n>=low&&n<=high)return (int)n; }catch(...) {}
    return nullopt;
}
struct Operation {string tree; char type; int key;};
bool readPractice(const fs::path& file,vector<Operation>& ops){
    if(!fs::exists(file))return true;
    ifstream in(file);
    string tree,kind,key;
    while(in >> tree >> kind >> key) {
        auto parsed=parsePositive(key);
        if((tree!="bst"&&tree!="avl")||(kind!="+"&&kind!="-")||!parsed||ops.size()>=2000)return false;
        ops.push_back({tree,kind[0],*parsed});
    }
    return in.eof();
}
bool savePractice(const fs::path& path,const vector<Operation>& ops) {
    error_code ec;
    if(path.has_parent_path())fs::create_directories(path.parent_path(),ec);
    if(ec)return false;
    auto tmp=path; tmp+=".tmp";
    ofstream out(tmp,ios::trunc);
    if(!out)return false;
    for(auto& op:ops)out<<op.tree<<' '<<op.type<<' '<<op.key<<'\n';
    out.close(); if(!out)return false;
    fs::rename(tmp,path,ec);
    if(!ec)return true;
    // Windows rename cannot replace an existing file.
    fs::path old=path;old+=".previous";ec.clear();fs::remove(old,ec);
    ec.clear(); if(fs::exists(path))fs::rename(path,old,ec);
    if(ec)return false;
    fs::rename(tmp,path,ec);
    if(ec){error_code restore;fs::rename(old,path,restore);return false;}
    return true;
}
void replayPractice(const vector<Operation>& ops,BSTree& bst,AVLTree& avl){
    for(auto& op:ops){
        if(op.tree=="bst") {if(op.type=='+')bst.insert(op.key);else bst.remove(op.key);}
        else {if(op.type=='+')avl.insert(op.key);else avl.remove(op.key);}
    }
}
}

int main(int argc,char* argv[]) {
    if(argc<3)return fail("Usage: smartpark_bridge <data-file> <command> [arguments]");
    string file=argv[1],cmd=argv[2];
    FileLock writeLock(file);
    if(!writeLock.acquired()) return fail("Parking engine is busy; close duplicate instances or recover the stale .lockdir after verifying no bridge is running.");
    ParkingSystem system(file);
    try {
        if(cmd=="find-my-car"){
            if(argc!=4)return fail("Plate required.");
            FindCarResult r=system.findMyCar(argv[3]);
            cout<<"{\"found\":"<<(r.found?"true":"false")<<",\"reachable\":"<<(r.reachable?"true":"false")
                <<",\"slot\":"<<jsonString(r.slot)<<",\"path\":"<<jsonString(r.path)
                <<",\"steps\":"<<jsonString(r.steps)<<",\"comparisons\":"<<r.comparisons
                <<",\"edges\":"<<r.edges<<",\"message\":"<<jsonString(r.message)<<"}\n";return 0;
        }
        if(cmd=="ccp-state" || cmd=="evacuate" || cmd=="evac-stop"){
            if(cmd=="evacuate"){
                if(argc!=4 || string(argv[3]).size()!=1 || argv[3][0]<'A'||argv[3][0]>'C')
                    return fail("Choose emergency Building A, B or C.");
                if(!system.setEvacuation(argv[3][0],true))return fail("Could not save evacuation state.");
            }
            if(cmd=="evac-stop"&&!system.setEvacuation('-',false))return fail("Could not stop evacuation.");
            cout<<"{\"active\":"<<(system.evacuationActive()?"true":"false")
                <<",\"building\":"<<jsonString(system.evacuationActive()?string(1,system.evacuationBuilding()):"")
                <<",\"lastRecovery\":"<<jsonString(system.lastRecovery())
                <<",\"evacuationRoutes\":[";
            auto rows=system.evacuationRoutes();
            for(size_t i=0;i<rows.size();++i){const auto&r=rows[i];if(i)cout<<',';
                cout<<"{\"plate\":"<<jsonString(r.plate)<<",\"slot\":"<<jsonString(r.slot)
                    <<",\"exitGate\":"<<jsonString(r.exitGate)<<",\"path\":"<<jsonString(r.path)
                    <<",\"reachable\":"<<(r.reachable?"true":"false")<<",\"edges\":"<<r.edges<<'}';
            }
            cout<<"],\"holds\":[";
            auto holds=system.vipReservations();
            for(size_t i=0;i<holds.size();++i){const auto&h=holds[i];if(i)cout<<',';
                cout<<"{\"plate\":"<<jsonString(h.plate)<<",\"slot\":"<<jsonString(h.slot)
                    <<",\"route\":"<<jsonString(h.path)<<'}';
            }
            cout<<"],\"groups\":[";
            auto groups=system.groupQueue();
            for(size_t i=0;i<groups.size();++i){const auto&g=groups[i];if(i)cout<<',';
                cout<<"{\"id\":"<<jsonString(g.id)<<",\"count\":"<<g.count
                    <<",\"building\":"<<jsonString(g.building)<<",\"vip\":"<<(g.vip?"true":"false")
                    <<",\"plates\":[";
                for(int j=0;j<g.count;++j){if(j)cout<<',';cout<<jsonString(g.plate[j]);}
                cout<<"]}";
            }
            cout<<"]}\n";return 0;
        }
        if(cmd=="vip-reserve"){
            if(argc!=8)return fail("vip-reserve PLATE TYPE OWNER BUILDING FLOOR");
            auto floor=parsePositive(argv[7],0,system.floorsPerBuilding());
            if(!floor)return fail("Invalid floor.");
            CCPResponse r=system.reserveVIP(argv[3],argv[4],argv[5],argv[6],*floor);
            if(!r.success)return fail(r.message);
            cout<<"{\"message\":"<<jsonString(r.message)<<",\"slot\":"<<jsonString(r.slot)
                <<",\"route\":"<<jsonString(r.route)<<"}\n";return 0;
        }
        if(cmd=="vip-confirm"){
            if(argc!=4)return fail("Plate required.");
            CCPResponse r=system.confirmVIP(argv[3]);
            if(!r.success)return fail(r.message);
            cout<<"{\"message\":"<<jsonString(r.message)<<",\"slot\":"<<jsonString(r.slot)<<"}\n";return 0;
        }
        if(cmd=="convoy"){
            if(argc<8)return fail("Convoy count/building/floor/priority/vehicles required.");
            auto count=parsePositive(argv[3],3,4);
            if(!count || argc!=7+3*(*count))return fail("Provide 3 or 4 valid convoy vehicles.");
            auto floor=parsePositive(argv[5],0,system.floorsPerBuilding());
            if(!floor || (string(argv[6])!="0"&&string(argv[6])!="1"))return fail("Invalid floor/priority.");
            traceccp::Group group;
            group.count=*count;group.building=argv[4];group.floor=*floor;group.vip=string(argv[6])=="1";
            for(int i=0;i<group.count;++i){
                group.plate[i]=argv[7+3*i];group.type[i]=argv[8+3*i];group.owner[i]=argv[9+3*i];
            }
            GroupResponse r=system.submitConvoy(group);
            if(!r.success)return fail(r.message);
            cout<<"{\"message\":"<<jsonString(r.message)<<",\"groupId\":"<<jsonString(r.id)
                <<",\"queued\":"<<(r.queued?"true":"false")
                <<",\"split\":"<<(r.split?"true":"false")<<",\"slots\":[";
            if(!r.queued)for(int i=0;i<r.count;++i){if(i)cout<<',';cout<<jsonString(r.slots[i]);}
            cout<<"]}\n";return 0;
        }
        if(cmd=="history-report") {
            if(argc!=5)return fail("Usage: history-report week/month YYYY-MM-DD");
            trace8::Window w;
            if(!trace8::makeWindow(argv[3],argv[4],w))return fail("Invalid week/month or date.");
            auto records=trace8::allSessions(system);
            auto summary=trace8::summarize(records,w);
            auto rows=trace8::filterSessions(records,w);
            cout<<"{\"period\":"<<jsonString(w.period)<<",\"label\":"<<jsonString(w.label)
                <<",\"startEpoch\":"<<w.start<<",\"endEpoch\":"<<w.end
                <<",\"summary\":"<<historySummaryJson(summary)<<",\"rows\":[";
            for(size_t i=0;i<rows.size();++i){if(i)cout<<',';cout<<historySessionJson(rows[i]);}
            cout<<"]}\n";return 0;
        }
        if(cmd=="slot-at") {
            if(argc!=5)return fail("Usage: slot-at SLOT YYYY-MM-DDTHH:MM");
            string slot=argv[3];
            bool exists=false;
            for(const auto& position:system.parkingSlots())if(position.slotNumber==slot)exists=true;
            if(!exists)return fail("Parking slot not found.");
            long long when=0;
            if(!trace8::parseMoment(argv[4],when) || when>parkingNowEpoch())
                return fail("Enter a valid past local date/time.");
            auto found=trace8::whoOccupied(trace8::allSessions(system),slot,when);
            cout<<"{\"slot\":"<<jsonString(slot)<<",\"atEpoch\":"<<when
                <<",\"occupied\":"<<(found?"true":"false")
                <<",\"vehicle\":"<<(found?historySessionJson(*found):"null")<<"}\n";
            return 0;
        }
        if(cmd=="health") {cout<<"{\"status\":\"ok\",\"engine\":\"C++\",\"treesValid\":"<<(system.treeIndexesValid()?"true":"false")<<"}\n";return 0;}
        if(cmd=="state") {cout<<stateJson(system)<<'\n';return 0;}
        if(cmd=="graph") {cout<<graphJson(system)<<'\n';return 0;}
        if(cmd=="route") {
            if(argc!=4)return fail("Route requires a slot ID.");
            bool found=false;
            for(const auto& item:system.parkingSlots())if(item.slotNumber==argv[3])found=true;
            if(!found)return fail("Parking slot not found.");
            cout<<routeJson(system,argv[3])<<'\n';return 0;
        }
        if(cmd=="road-block") {
            if(argc!=6 || (string(argv[5])!="0" && string(argv[5])!="1"))
                return fail("Road update requires from, to and blocked 0/1.");
            if(!system.setRoadBlocked(argv[3],argv[4],string(argv[5])=="1"))
                return fail("Road segment not found or could not save change.");
            cout<<"{\"message\":\"Road restriction updated.\",\"graph\":"<<graphJson(system)
                <<",\"state\":"<<stateJson(system)<<"}\n";return 0;
        }
        if(cmd=="slot-rule") {
            if(argc!=6) return fail("Set slot rule: slotId, mode(0=normal,1=reserved,2=disputed), note.");
            auto rule=parsePositive(argv[4],0,2);
            if(!rule || string(argv[5]).size()>90) return fail("Invalid slot restriction or note.");
            if(!system.setSlotRestriction(argv[3],*rule,argv[5]))
                return fail("Cannot change this slot: occupied, invalid ID, or save failure.");
            cout<<stateJson(system)<<'\n';return 0;
        }
        if(cmd=="recommend") {
            if(argc!=7)return fail("Recommend requires type, VIP flag, building and floor.");
            auto floor=parsePositive(argv[6],0,system.floorsPerBuilding());
            if(!floor)return fail("Invalid floor.");
            string reason=system.previewAllocation(argv[3],string(argv[4])=="1",argv[5],*floor);
            cout<<"{\"explanation\":"<<jsonString(reason)<<"}\n";return 0;
        }
        if(cmd=="configure") {
            if(argc!=5)return fail("Configure requires floors (1-4) and slots per floor (3-20).");
            auto floors=parsePositive(argv[3],1,4), slots=parsePositive(argv[4],3,20);
            if(!floors || !slots)return fail("Invalid floor or slot count.");
            if(!system.configureLayout(*floors,*slots))
                return fail("Parking and waiting list must be empty to change the layout.");
            cout<<stateJson(system)<<'\n';return 0;
        }
        if(cmd=="queue-state") {
            auto arrival=system.waitingVehicles(), order=system.waitingByPriority();
            int emergencies=0,vips=0,normals=0;
            string items="[";
            for(size_t i=0;i<order.size();++i){
                const Vehicle& v=order[i];
                if(i)items+=',';
                string tier=v.vehicleType=="Emergency" ? "Emergency" : v.priority ? "VIP" : "Normal";
                if(tier=="Emergency")++emergencies; else if(tier=="VIP")++vips; else ++normals;
                items+="{\"position\":"+to_string(i+1)+",\"tier\":"+jsonString(tier)
                   +",\"eligibleSlots\":"+to_string(system.eligibleSlotsFor(v))
                   +",\"vehicle\":"+vehicleJson(v)+"}";
            }
            items+=']';
            cout<<"{\"arrival\":"<<vehiclesJson(arrival)
                <<",\"dispatch\":"<<items
                <<",\"counts\":{\"emergency\":"<<emergencies
                <<",\"vip\":"<<vips<<",\"normal\":"<<normals<<"}}\n";
            return 0;
        }
        if(cmd=="queue-cancel") {
            if(argc!=4)return fail("Cancel waiting requires vehicle plate.");
            if(!system.cancelWaiting(argv[3]))return fail("Waiting ticket not found or save failed.");
            cout<<"{\"message\":\"Waiting ticket cancelled.\",\"state\":"<<stateJson(system)<<"}\n";
            return 0;
        }
        if(cmd=="queue-vip") {
            if(argc!=5 || (string(argv[4])!="0"&&string(argv[4])!="1"))return fail("Set VIP requires plate and VIP flag 0/1.");
            if(!system.changeWaitingVIP(argv[3],string(argv[4])=="1"))
                return fail("Waiting ticket not found or Emergency ticket cannot be changed.");
            cout<<"{\"message\":\"Waiting priority updated.\",\"state\":"<<stateJson(system)<<"}\n";
            return 0;
        }
        if(cmd=="dsa") {
            auto parked=system.parkedVehicles(),queued=system.waitingVehicles(),history=system.exitRecords(),priority=system.waitingByPriority();
            cout<<"{\"linkedList\":"<<vehiclesJson(parked)<<",\"queue\":"<<vehiclesJson(queued)
                <<",\"doublyForward\":"<<vehiclesJson(history)
                <<",\"doublyBackward\":"<<vehiclesJson(system.historyBackward())
                <<",\"stack\":"<<vehiclesJson(vector<Vehicle>(history.begin(),history.begin()+min(history.size(),(size_t)8)))
                <<",\"priorityQueue\":"<<vehiclesJson(priority)
                <<",\"metrics\":{\"linkedListNodes\":"<<parked.size()<<",\"queueNodes\":"<<queued.size()
                <<",\"stackRecords\":"<<history.size()<<",\"priorityItems\":"<<priority.size()<<"}}\n";
            return 0;
        }
        if(cmd=="ticket-audit") {
            if(argc!=4)return fail("Ticket audit requires a numeric Ticket ID.");
            auto key=parsePositive(argv[3]);
            if(!key)return fail("Ticket ID must be 1 to 99999999.");
            auto a=auditLiveTicket(system,*key);
            auto probe=[&](const TreeSearchTrace& t){
                return string("{\"found\":")+(t.found?"true":"false")+
                    ",\"plate\":"+jsonString(t.vehicleNumber)+
                    ",\"comparisons\":"+to_string(t.visited.size())+
                    ",\"path\":"+intsJson(t.visited)+'}';
            };
            cout<<"{\"ticket\":"<<a.ticketKey<<",\"activeCount\":"<<system.parkedVehicles().size()
                <<",\"indexesValid\":"<<(a.indexesValid?"true":"false")
                <<",\"linear\":{\"found\":"<<(a.linearFound?"true":"false")
                <<",\"plate\":"<<jsonString(a.linearPlate)<<",\"comparisons\":"<<a.recordsScanned<<"}"
                <<",\"bst\":"<<probe(a.bst)<<",\"avl\":"<<probe(a.avl)<<"}\n";
            return 0;
        }
        if(cmd=="sort-audit") {
            if(argc!=5)return fail("Sort audit requires source active/history and field number/fee.");
            string source=argv[3], field=argv[4];
            if((source!="active"&&source!="history") || (field!="number"&&field!="fee"))return fail("Invalid sort field or data source.");
            auto dataset=phase6Dataset(system,source);
            auto bubble=measureSort(dataset,false,field),selection=measureSort(dataset,true,field);
            if(dataset.size()>2000)return fail("Too many items to display in algorithm lab.");
            cout<<"{\"source\":"<<jsonString(source)<<",\"field\":"<<jsonString(field)
                <<",\"count\":"<<dataset.size()
                <<",\"bubble\":{\"comparisons\":"<<bubble.comparisons<<",\"swaps\":"<<bubble.swaps<<"}"
                <<",\"selection\":{\"comparisons\":"<<selection.comparisons<<",\"swaps\":"<<selection.swaps<<"}"
                <<",\"resultsIdentical\":"<<(bubble.sorted.size()==selection.sorted.size()&&
                    equal(bubble.sorted.begin(),bubble.sorted.end(),selection.sorted.begin(),
                    [](const Vehicle& a,const Vehicle& b){return a.ticketId==b.ticketId;})?"true":"false")
                <<",\"sorted\":"<<vehiclesJson(bubble.sorted)<<"}\n";
            return 0;
        }
        if(cmd=="trees") {cout<<bothTrees(system.bst(),system.avl())<<'\n';return 0;}
        if(cmd=="park") {
            if(argc!=7 && argc!=9)return fail("Park requires plate, type, owner, VIP, optional building and floor.");
            string before=bothTrees(system.bst(),system.avl());
            string building = argc==9 ? argv[7] : "";
            int floor=0;
            if(argc==9) {
                auto parsed=parsePositive(argv[8],0,system.floorsPerBuilding());
                if(!parsed)return fail("Invalid preferred floor.");
                floor=*parsed;
            }
            ParkResult result=system.park(argv[3],argv[4],argv[5],string(argv[6])=="1",building,floor);
            if(!result.success)return fail(result.message);
            cout<<"{\"queued\":"<<(result.queued?"true":"false")<<",\"message\":"<<jsonString(result.message)
                <<",\"vehicle\":"<<vehicleJson(result.vehicle)
                <<",\"explanation\":"<<jsonString(result.explanation)
                <<",\"eligibleSlots\":"<<result.eligibleSlots
                <<",\"route\":"<<(result.queued?"null":routeJson(system,result.vehicle.parkingSlot))
                <<",\"treeEvent\":{\"action\":\"park\",\"before\":"<<before
                <<",\"after\":"<<bothTrees(system.bst(),system.avl())
                <<",\"rotations\":"<<(result.queued ? "[]" : stringsJson(system.avl().lastRotations()))<<"}}\n";
            return 0;
        }
        if(cmd=="exit") {
            if(argc!=4 && argc!=5)return fail("Exit requires a vehicle plate only.");
            string before=bothTrees(system.bst(),system.avl());
            ExitResult result;
            if(argc==5) {
#ifdef SMARTPARK_TEST_MODE
                // Test-only compatibility for old regression suites.
                auto hours=parsePositive(argv[4],1,720);
                if(!hours)return fail("Hours must be a whole number from 1 to 720.");
                result=system.exitVehicle(argv[3],*hours);
#else
                return fail("Manual hours are disabled. Entry and exit clocks decide the fee.");
#endif
            } else result=system.exitVehicle(argv[3]);
            if(!result.success)return fail(result.message);
            cout<<"{\"message\":"<<jsonString(result.message)<<",\"bill\":{\"vehicleNumber\":"<<jsonString(result.vehicle.vehicleNumber)
                <<",\"slot\":"<<jsonString(result.freedSlot)<<",\"hours\":"<<result.vehicle.hours
                <<",\"fee\":"<<static_cast<long long>(result.vehicle.parkingFee)
                <<",\"hourlyRate\":"<<static_cast<long long>(hourlyParkingRate(result.vehicle.vehicleType))
                <<",\"ticketId\":"<<jsonString(result.vehicle.ticketId)
                <<",\"receiptId\":"<<jsonString("R-"+result.vehicle.ticketId)
                <<",\"vehicleType\":"<<jsonString(result.vehicle.vehicleType)
                <<",\"ownerName\":"<<jsonString(result.vehicle.ownerName)
                <<",\"entryTime\":"<<jsonString(result.vehicle.entryTime)
                <<",\"exitTime\":"<<jsonString(result.vehicle.exitTime)
                <<",\"durationSeconds\":"<<result.vehicle.durationSeconds<<"},\"assignedFromQueue\":";
            cout<<(result.reassigned?vehicleJson(result.nextVehicle):"null");
            cout<<",\"treeEvent\":{\"action\":\"exit\",\"before\":"<<before
                <<",\"after\":"<<bothTrees(system.bst(),system.avl())
                <<",\"rotations\":"<<stringsJson(system.avl().lastRotations())<<"}}\n";
            return 0;
        }
        if(cmd=="search") {
            if(argc!=4)return fail("Search requires a query.");
            cout<<"{\"results\":"<<vehiclesJson(system.search(argv[3]))<<"}\n";return 0;
        }
        if(cmd=="reset") {
            if(!system.resetWithBackup())return fail("Reset failed: backup or file error.");
            cout<<stateJson(system)<<'\n';return 0;
        }
        if(cmd=="practice") {
            if(argc<6)return fail("Practice requires file, tree and action.");
            fs::path practiceFile=argv[3];string name=argv[4],action=argv[5];
            if(name!="bst"&&name!="avl")return fail("Choose BST or AVL.");
            if(action!="view"&&action!="insert"&&action!="delete"&&action!="search"&&action!="clear")return fail("Invalid practice action.");
            vector<Operation> ops;
            if(!readPractice(practiceFile,ops))return fail("Practice data file is damaged; please back it up before continuing.");
            BSTree bst; AVLTree avl;replayPractice(ops,bst,avl);
            string before=bothTrees(bst,avl);
            optional<int> key;
            if(action=="insert"||action=="delete"||action=="search") {
                if(argc!=7 || !(key=parsePositive(argv[6])))return fail("Enter a valid number from 1 to 99999999.");
            }
            bool changed=false,found=false;
            if(action=="clear") {
                auto previous=ops.size();ops.erase(remove_if(ops.begin(),ops.end(),[&](const Operation&o){return o.tree==name;}),ops.end());
                changed=(ops.size()!=previous);
                bst.clear();avl.clear();replayPractice(ops,bst,avl);
            } else if(action=="search")found=name=="bst"?bst.search(*key):avl.search(*key);
            else if(action=="insert")changed=name=="bst"?bst.insert(*key):avl.insert(*key);
            else if(action=="delete")changed=name=="bst"?bst.remove(*key):avl.remove(*key);
            if(changed) {
                if(action=="insert"||action=="delete")ops.push_back({name,action=="insert"?'+':'-',*key});
                if(ops.size()>2000 || !savePractice(practiceFile,ops))return fail("Could not save practice tree.");
            }
            string rotations= name=="avl" && (action=="insert"||action=="delete") ? stringsJson(avl.lastRotations()):"[]";
            cout<<"{\"trees\":"<<bothTrees(bst,avl)
                <<",\"before\":"<<before<<",\"action\":"<<jsonString(action)
                <<",\"tree\":"<<jsonString(name)<<",\"changed\":"<<(changed?"true":"false")
                <<",\"found\":"<<(found?"true":"false")
                <<",\"rotations\":"<<rotations<<"}\n";
            return 0;
        }
        return fail("Unknown C++ bridge command.");
    } catch(const exception& e){return fail(string("C++ operation failed: ")+e.what());}
}
