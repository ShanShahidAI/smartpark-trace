#include "PriorityQueue.h"
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
