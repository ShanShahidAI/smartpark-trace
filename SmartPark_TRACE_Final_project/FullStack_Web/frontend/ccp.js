/* SmartPark TRACE — presentation only. C++ owns all decisions and DSA operations. */
(() => {
  'use strict';
  const $ = id => document.getElementById(id);
  const escapeHtml = v => String(v ?? '').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const types = ['Car','Motorcycle','Sedan','SUV','Van','Pickup','Bus','Truck','Sports Car','Taxi'];
  let isBusy=false;
  async function call(url, payload){
    const opts=payload===undefined?{}:{method:'POST',headers:{'Content-Type':'application/json','X-CSRF-Token':window.SMARTPARK_CSRF||''},body:JSON.stringify(payload)};
    const res=await fetch(url,opts);
    const data=await res.json();
    if(!res.ok)throw Error(data.error||`Request failed (${res.status})`);
    return data;
  }
  function text(id,message){if($(id))$(id).textContent=message;}
  async function refresh(){
    const state=await call('/api/ccp/state');
    text('ccpMode',state.active?`EVACUATION ACTIVE — BUILDING ${state.building}`:'Normal operation');
    text('ccpRecovery',state.lastRecovery|| (state.active?'Normal inbound parking is locked.':'Roads and admissions are operating normally.'));
    const evacuations=state.evacuationRoutes||[];
    text('ccpEvacResult',state.active ? (`Building ${state.building} incident — normal arrivals frozen.\n`+
      (evacuations.length?evacuations.map(e=>`${e.plate}: ${e.reachable?e.path:'NO SAFE ROUTE / request assistance'}`).join('\n'):'No parked cars inside the affected building.')) : 'No emergency currently active.');
    const holds=state.holds||[],groups=state.groups||[];
    text('ccpQueueOverview', '');
    const host=$('ccpQueueOverview');
    if(host){
      const holdHtml=holds.map(h=>`<p><b>${escapeHtml(h.plate)}</b> — held ${escapeHtml(h.slot)}<small style="display:block">${escapeHtml(h.route.replaceAll('|',' → '))}</small></p>`).join('')||'<p>No provisional VIP reservations.</p>';
      const groupHtml=groups.map(g=>`<p><b>${escapeHtml(g.id)}</b> — ${g.count} vehicles (${g.vip?'VIP':'Normal'})<small style="display:block">${(g.plates||[]).map(escapeHtml).join(', ')}</small></p>`).join('')||'<p>No group waiting records.</p>';
      host.innerHTML=`<div><b>VIP holds · ${holds.length}</b>${holdHtml}</div><div><b>Group queue · ${groups.length}</b>${groupHtml}</div>`;
    }
  }
  async function guarded(action,target){
    if(isBusy)return; isBusy=true;
    try{await action(); await refresh();}
    catch(e){if(target)text(target,e.message||'Operation failed.');}
    finally{isBusy=false;}
  }
  function renderGroupRows(){
    const count=Number($('ccpGroupSize').value);
    $('ccpVehicleRows').innerHTML=Array.from({length:count},(_,i)=>`<div class="ccp-vehicle-row"><strong>${i+1}</strong><label>Plate<input class="ccp-plate" placeholder="GRP-${String(i+1).padStart(2,'0')}" required maxlength="20"></label><label>Type<select class="ccp-type">${types.map(t=>`<option>${escapeHtml(t)}</option>`).join('')}</select></label><label>Owner<input class="ccp-owner" placeholder="Name" required maxlength="70"></label></div>`).join('');
  }
  $('ccpGroupSize')?.addEventListener('change',renderGroupRows); renderGroupRows();
  $('ccpRefresh')?.addEventListener('click',()=>guarded(()=>Promise.resolve(),null));
  $('findMyCarForm')?.addEventListener('submit',e=>{
    e.preventDefault(); guarded(async()=>{
      const data=await call(`/api/find-my-car?plate=${encodeURIComponent($('ccpFindPlate').value.trim())}`);
      text('ccpFindResult',data.found ?
        (`Slot: ${data.slot}\nAVL comparisons: ${data.comparisons}\n${data.reachable?`Walking route (${data.edges} segments):\n${data.steps}`:'Walking path currently blocked.'}\n${data.message}`):data.message);
    },'ccpFindResult');
  });
  $('evacForm')?.addEventListener('submit',e=>{
    e.preventDefault();guarded(()=>call('/api/ccp/evacuation',{active:true,building:$('ccpEvacBuilding').value}),'ccpEvacResult');
  });
  $('ccpEvacStop')?.addEventListener('click',()=>guarded(()=>call('/api/ccp/evacuation',{active:false}),'ccpEvacResult'));
  $('vipReserveForm')?.addEventListener('submit',e=>{
    e.preventDefault();guarded(async()=>{
      const result=await call('/api/ccp/vip-reserve',{
        plate:$('ccpVipPlate').value.trim(),type:$('ccpVipType').value,owner:$('ccpVipOwner').value.trim(),
        building:$('ccpVipBuilding').value,floor:Number($('ccpVipFloor').value)});
      text('ccpVipResult',`${result.message}\nSlot: ${result.slot}\nRoute: ${result.route}`);
      $('ccpConfirmPlate').value=$('ccpVipPlate').value.trim();
    },'ccpVipResult');
  });
  $('vipConfirmForm')?.addEventListener('submit',e=>{
    e.preventDefault();guarded(async()=>{
      const result=await call('/api/ccp/vip-confirm',{plate:$('ccpConfirmPlate').value.trim()});
      text('ccpVipResult',`${result.message}\nSlot: ${result.slot}`);
    },'ccpVipResult');
  });
  $('convoyForm')?.addEventListener('submit',e=>{
    e.preventDefault();guarded(async()=>{
      const vehicles=[...document.querySelectorAll('#ccpVehicleRows .ccp-vehicle-row')].map(row=>({
        plate:row.querySelector('.ccp-plate').value.trim(),type:row.querySelector('.ccp-type').value,owner:row.querySelector('.ccp-owner').value.trim()}));
      const result=await call('/api/ccp/convoy',{vehicles,building:$('ccpGroupBuilding').value,
        floor:Number($('ccpGroupFloor').value),vip:$('ccpGroupVip').checked});
      text('ccpGroupResult',`${result.groupId}: ${result.message}\n${result.queued?'Group queued as one record.':
        result.slots.map((slot,i)=>vehicles[i].plate+' → '+slot).join('\n')}`);
    },'ccpGroupResult');
  });
  refresh().catch(e=>text('ccpMode',e.message||'Cannot fetch live C++ state'));
})();
