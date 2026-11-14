#include "ParkingSystem.h"
// ============ PHASE 6: ALGORITHM PERFORMANCE LAB ============
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
