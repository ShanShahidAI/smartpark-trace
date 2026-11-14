// ============ PHASE 8: WEEKLY/MONTHLY HISTORY & HISTORICAL SLOT REPLAY ============
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
