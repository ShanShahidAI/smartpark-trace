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

