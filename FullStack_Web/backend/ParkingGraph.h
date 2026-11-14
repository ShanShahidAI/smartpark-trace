#ifndef SMARTPARK_PARKING_GRAPH_H
#define SMARTPARK_PARKING_GRAPH_H
// PHASE 5: Undirected, unweighted road graph for three campus buildings.
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
