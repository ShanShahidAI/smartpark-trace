const titles = {dashboard:"Dashboard", parking:"Parking Map", buildings:"Buildings & Floors", routes:"Graph Routes (BFS/DFS)", entry:"Vehicle Entry", exit:"Vehicle Exit", search:"Search Vehicle", queue:"Waiting Queue", history:"Weekly & Monthly History", dsa:"DSA Visualizer", trees:"BST & AVL Trees", stats:"Statistics", settings:"Settings"};
const iconMap = {"Car":"fa-car-side","Motorcycle":"fa-motorcycle","Van":"fa-van-shuttle","Emergency":"fa-truck-medical","Sedan":"fa-car-side","SUV":"fa-car","Sports Car":"fa-gauge-high","Pickup":"fa-truck-pickup","Bus":"fa-bus","Taxi":"fa-taxi","Truck":"fa-truck"};
let data = {};
let isOnline = null;
let toastTimer = null;
let layoutLoaded = false;
let phase9Refreshing = false;
let modalSlot = null;
let activeRates = {};
let rateBusy = false;
function phase9CloseMenu() {
  document.body.classList.remove('phase9-menu-open');
  const button = document.getElementById('phase9MenuBtn');
  if (button) {button.setAttribute('aria-expanded','false'); button.setAttribute('aria-label','Open navigation');}
}
function phase9ToggleMenu() {
  const opened = !document.body.classList.contains('phase9-menu-open');
  document.body.classList.toggle('phase9-menu-open', opened);
  const button = document.getElementById('phase9MenuBtn');
  button.setAttribute('aria-expanded', String(opened));
  button.setAttribute('aria-label', opened ? 'Close navigation' : 'Open navigation');
}

function slotsForFilter() {
  const b = $("filterBuilding")?.value || "";
  const f = Number($("filterFloor")?.value || 0);
  const size = $("filterSize")?.value || "";
  return (data.slots || []).filter(s => (!b || s.building === b) && (!f || s.floor === f) && (!size || s.size === size));
}
function updateFloorChoices(id, count, label = "All floors") {
  const el = $(id); if (!el) return;
  const previous = el.value;
  el.innerHTML = `<option value="0">${safe(label)}</option>` +
    Array.from({length:count}, (_,i) => `<option value="${i+1}">Floor ${i+1}</option>`).join("");
  if ([...el.options].some(o => o.value === previous)) el.value = previous;
}
function renderBuildings() {
  const stats = data.buildings || [];
  $("buildingStats").innerHTML = stats.map(b => {
    const floors = Array.from({length:data.layout.floorsPerBuilding},(_,i)=>i+1).map(f=>{
      const subset=data.slots.filter(s=>s.building===b.id&&s.floor===f);
      return `<div class="phase2-floor"><b>Floor ${f}</b><span>${subset.filter(s=>!s.occupied&&s.restriction===0&&s.reachable!==false).length} reachable free / ${subset.length} slots</span></div>`;
    }).join("");
    return `<div class="panel phase2-building"><div><span class="phase2-building-icon"><i class="fa-solid fa-building"></i></span><h3>Building ${safe(b.id)}</h3><p>${b.available} available · ${b.occupied} occupied</p></div><div class="phase2-capacity"><span style="width:${b.total?Math.round(b.occupied/b.total*100):0}%"></span></div>${floors}</div>`;
  }).join("");
}
function syncLayoutControls() {
  if (!data.layout) return;
  updateFloorChoices("filterFloor",data.layout.floorsPerBuilding);
  updateFloorChoices("entryFloor",data.layout.floorsPerBuilding,"Auto / Any floor");
  if (!layoutLoaded || document.activeElement !== $("layoutFloors")) $("layoutFloors").value = data.layout.floorsPerBuilding;
  if (!layoutLoaded || document.activeElement !== $("layoutSlots")) $("layoutSlots").value = data.layout.slotsPerFloor;
  $("layoutSummary").textContent=`Current: 3 buildings × ${data.layout.floorsPerBuilding} floors × ${data.layout.slotsPerFloor} slots = ${data.totalSlots} parking slots.`;
  layoutLoaded=true;
}
function carIllustration(vehicleType) {
  if (vehicleType === 'Motorcycle') return `<svg class="parking-vehicle-svg" viewBox="0 0 90 130" role="img" aria-label="Motorcycle top view"><ellipse cx="45" cy="20" rx="14" ry="17" fill="#121720"/><ellipse cx="45" cy="109" rx="14" ry="17" fill="#121720"/><path d="M34 30 Q45 21 56 30 L61 99 Q45 109 29 99 Z" fill="#57bdb4" stroke="#a9f5eb" stroke-width="3"/><rect x="28" y="44" width="34" height="13" rx="6" fill="#152f39"/><rect x="33" y="72" width="24" height="19" rx="9" fill="#283744"/><path d="M19 43H71" stroke="#c5e7e1" stroke-width="4" stroke-linecap="round"/></svg>`;
  const long = ['Bus','Truck','Van','Pickup'].includes(vehicleType);
  const body = long ? '#82aace' : vehicleType === 'Emergency' ? '#e5e9ee' : '#5b9ec9';
  return `<svg class="parking-vehicle-svg" viewBox="0 0 90 130" role="img" aria-label="${safe(vehicleType)} top view">
  <ellipse cx="45" cy="109" rx="32" ry="8" fill="rgba(0,0,0,.38)"/>
  <rect x="7" y="28" width="12" height="21" rx="4" fill="#111b27"/><rect x="71" y="28" width="12" height="21" rx="4" fill="#111b27"/>
  <rect x="7" y="83" width="12" height="21" rx="4" fill="#111b27"/><rect x="71" y="83" width="12" height="21" rx="4" fill="#111b27"/>
  <path d="M25 14 Q45 6 65 14 Q72 23 73 34 L73 94 Q71 109 63 116 Q45 123 27 116 Q19 110 17 94 L17 34 Q18 24 25 14Z" fill="${body}" stroke="#def3ff" stroke-width="2"/>
  <path d="M26 36 Q45 30 64 36 L68 57 Q45 65 22 57Z" fill="#21394f" stroke="#accddd" stroke-width="2"/>
  <path d="M23 76 Q45 70 67 76 L64 102 Q45 109 26 102Z" fill="#28455d" stroke="#b7d8e6" stroke-width="2"/>
  <path d="M26 18H64" stroke="#f4fbff" stroke-width="3" opacity=".75"/>
  <path d="M22 111H68" stroke="#c03c41" stroke-width="4"/>
  </svg>`;
}
function renderParkingMap() {
  const slots = slotsForFilter();
  const available = slots.filter(s => !s.occupied && !s.held && s.restriction === 0 && s.reachable !== false).length;
  $("filterCount").textContent = `${slots.length} spaces · ${available} available to allocate`;
  const byZone = new Map();
  slots.forEach(s => {
    const key = `Building ${s.building} · Floor ${s.floor}`;
    if (!byZone.has(key)) byZone.set(key, []);
    byZone.get(key).push(s);
  });
  $("map").innerHTML = [...byZone.entries()].map(([name,group]) => {
    const occupied = group.filter(s => s.occupied).length;
    const bays = group.map(slot => {
      const v = slot.vehicle;
      const current = slot.occupied ? 'busy' : slot.held ? 'ccp-held' : slot.restriction === 2 ? 'phase3-disputed' : slot.restriction === 1 ? 'phase3-reserved' : !slot.reachable ? 'phase5-unreachable' : 'free';
      return `<button type="button" class="slot ${current} ${v?.priority?'slot-vip-vehicle':''}" data-slot="${safe(slot.id)}" aria-label="Parking ${safe(slot.id)}: ${slot.occupied ? 'occupied by '+safe(v.vehicleNumber) : safe(slot.status||'available')}">
        <div class="slot-top"><b>${safe(slot.id)}</b><span>${slot.occupied?'PARKED':slot.held?'VIP HOLD':slot.restriction===1?'RESERVED':slot.restriction===2?'DISPUTED':slot.reachable===false?'NO ROUTE':'FREE'}</span></div>
        <div class="vehicle-stage">${slot.occupied ? carIllustration(v.vehicleType) : '<span class="empty-bay-symbol">P</span>'}</div>
        <div class="slot-bottom"><strong>${slot.occupied?safe(v.vehicleNumber):safe(slot.status||'Available')}</strong><small>${safe(slot.size)} capacity</small></div>
      </button>`;
    }).join('');
    return `<section class="parking-zone"><div class="parking-zone-head"><div><span class="parking-zone-icon"><i class="fa-solid fa-building"></i></span><div><strong>${safe(name)}</strong><small>${group.length} parking bays · ${occupied} occupied</small></div></div><span class="zone-availability">${group.length-occupied} physically free</span></div><div class="parking-bays">${bays}</div><div class="zone-road"><span>DRIVE AISLE</span><i class="fa-solid fa-arrow-right-long"></i></div></section>`;
  }).join('') || `<div class="empty-state">No parking spaces match the selected filters.</div>`;
  document.querySelectorAll("#map [data-slot]").forEach(b => b.addEventListener("click", () => slotInfo(b.dataset.slot)));
}

const $ = id => document.getElementById(id);
const safe = value => String(value ?? "").replace(/[&<>"']/g, c => ({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#39;"}[c]));
const safeNumber = n => Number(n || 0).toLocaleString();
const vehicleIcon = type => `fa-solid ${iconMap[type] || "fa-car-side"}`;

function setOnline(online) {
  if (isOnline === online) return;
  isOnline = online;
  const el = $("serverStatus");
  el.innerHTML = `<i class="fa-solid fa-circle"></i> ${online ? "System Online" : "Server Offline"}`;
  el.classList.toggle("offline", !online);
  if (!online) toast("Server connection lost. Check the terminal and refresh.", true);
}

async function api(url, options) {
  const opts = { ...(options || {}) };
  if (opts.method && !['GET','HEAD'].includes(String(opts.method).toUpperCase())) {
    opts.headers = {...(opts.headers || {}), 'X-CSRF-Token': window.SMARTPARK_CSRF || ''};
  }
  const response = await fetch(url, opts);
  let payload;
  try { payload = await response.json(); } catch (_) { throw new Error("Server returned an invalid response."); }
  if (response.status === 401) {
    window.location.assign('/login');
    throw new Error('Admin session expired. Please log in again.');
  }
  if (!response.ok) throw new Error(payload.error || `Request failed (${response.status}).`);
  return payload;
}

function toast(message, error = false) {
  const t = $("toast");
  t.textContent = message || "An unexpected error occurred.";
  t.className = "show";
  t.classList.toggle("error", error);
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => { t.className = ""; }, 3500);
}

function nav() {
  document.querySelectorAll("[data-page]").forEach(b => b.addEventListener("click", () => go(b.dataset.page)));
}
function go(page) {
  closeSlotInfo();
  const target = $(page);
  if (!target || !target.classList.contains("page")) return;
  document.querySelectorAll(".page").forEach(e => e.classList.remove("active"));
  target.classList.add("active");
  document.querySelectorAll(".nav").forEach(e => {
    const isCurrent=e.dataset.page===page;
    e.classList.toggle("active",isCurrent);
    if(isCurrent) e.setAttribute('aria-current','page');
    else e.removeAttribute('aria-current');
  });
  $("title").textContent = titles[page] || "SmartPark";
  phase9CloseMenu();
  window.scrollTo({top:0, behavior:window.matchMedia("(prefers-reduced-motion: reduce)").matches ? "instant" : "smooth"});
  if(page==="routes")loadRoadGraph().catch(error=>toast(error.message,true));
  if(page==="history" && window.loadPhase8History)window.loadPhase8History();
}

async function refresh() {
  if (phase9Refreshing) return;
  phase9Refreshing = true;
  try {
    const [updated, dsa, queue] = await Promise.all([api("/api/state"), api("/api/dsa"), api("/api/queue")]);
    data = updated;
    render();
    renderDSA(dsa);
    renderQueueControl(queue);
    setOnline(true);
    const stamp = document.getElementById('phase9LastSync');
    if(stamp) stamp.textContent = 'Live data · ' + new Date().toLocaleTimeString([], {hour:'2-digit',minute:'2-digit',second:'2-digit'});
  } catch (error) {
    setOnline(false);
    const stamp = document.getElementById('phase9LastSync');
    if(stamp) stamp.textContent = 'Connection unavailable';
  } finally {phase9Refreshing = false;}
}

function render() {
  $("total").textContent = data.totalSlots;
  const usableSlots = (data.slots || []).filter(slot => !slot.occupied && slot.restriction === 0 && slot.reachable !== false).length;
  $("available").textContent = usableSlots;
  const freeDetail = document.getElementById('phase9FreeDetail');
  if(freeDetail) freeDetail.textContent = `${data.available} physically free · excluding closed/reserved/no-route`;
  $("occupied").textContent = data.occupied;
  $("waiting").textContent = data.waitingCount;
  $("revenue").textContent = "Rs. " + safeNumber(data.revenue);
  $("mini").innerHTML = data.slots.slice(0,12).map(slot => `<div class="mini-slot ${slot.occupied ? "busy" : slot.restriction===0 ? "free" : "phase3-muted"}"><b>${safe(slot.id)}</b><small>${slot.occupied ? safe(slot.vehicle.vehicleNumber) : safe(slot.status||"FREE")}</small></div>`).join("");
  syncSlotRuleChoices();
  updateSlotModal();
  syncLayoutControls();
  renderParkingMap();
  renderBuildings();
  syncRouteSlotChoices();
  $("historyBody").innerHTML = data.history.length
    ? data.history.map(v => `<tr><td><b>${safe(v.vehicleNumber)}</b></td><td>${safe(v.parkingSlot)}</td><td><span class="type-pill">${safe(v.vehicleType)}</span></td><td>${safe(v.ownerName)}</td><td>Rs. ${safeNumber(v.parkingFee)}</td><td>${safe(v.exitTime || "—")}</td></tr>`).join("")
    : `<tr><td colspan="6"><div class="empty-state"><i class="fa-solid fa-clock-rotate-left"></i><b>No exit history yet</b><span>Completed exits will appear here automatically.</span></div></td></tr>`;
  $("queueVisual").innerHTML = data.waiting.length
    ? `<div class="queue"><b class="queue-label">FRONT</b>${data.waiting.map((v, i) => `<div class="node ${v.priority ? "priority-node" : ""}"><i class="${vehicleIcon(v.vehicleType)}"></i><b>${safe(v.vehicleNumber)}</b><small>${v.vehicleType === "Emergency" ? "Emergency" : v.priority ? "Priority" : "Normal"}</small></div>${i < data.waiting.length - 1 ? '<span class="arrow">→</span>' : ""}`).join("")}<b class="queue-label">REAR</b></div>`
    : `<div class="empty-state compact"><i class="fa-solid fa-layer-group"></i><span>Queue is empty.</span></div>`;
  $("recent").innerHTML = data.history.slice(0, 5).map(v => `<div class="activity"><span class="activity-icon"><i class="fa-solid fa-arrow-right-from-bracket"></i></span><div><b>${safe(v.vehicleNumber)}</b><span>Exited ${safe(v.parkingSlot)} · Rs. ${safeNumber(v.parkingFee)}</span></div></div>`).join("") || `<div class="empty-state compact"><i class="fa-solid fa-bolt"></i><span>No activity yet.</span></div>`;
  $("served").textContent = data.served;
  const pct = data.totalSlots ? Math.round(data.occupied / data.totalSlots * 100) : 0;
  $("occPct").textContent = pct + "%";
  $("rev2").textContent = "Rs. " + safeNumber(data.revenue);
  $("bar").style.width = pct + "%";
  $("occText").textContent = `${data.occupied} / ${data.totalSlots} occupied`;
  const types = {};
  data.slots.forEach(s => { if (s.occupied) types[s.vehicle.vehicleType] = (types[s.vehicle.vehicleType] || 0) + 1; });
  $("types").innerHTML = Object.keys(types).length ? Object.entries(types).map(([k,v]) => `<div class="type-row"><span><i class="${vehicleIcon(k)}"></i>${safe(k)}</span><b>${v}</b></div>`).join("") : `<div class="empty-state compact"><span>No vehicles parked.</span></div>`;
}

function renderQueueControl(queue) {
  $("queueEmergencies").textContent = queue.counts.emergency;
  $("queueVips").textContent = queue.counts.vip;
  $("queueNormals").textContent = queue.counts.normal;
  const rows = queue.dispatch || [];
  $("queueBoard").innerHTML = rows.length ? rows.map(row => {
    const v = row.vehicle;
    const priority = row.tier.toLowerCase();
    const preference = `${v.preferredBuilding ? "Building " + safe(v.preferredBuilding) : "Any building"} · ${v.preferredFloor ? "Floor " + v.preferredFloor : "Any floor"}`;
    return `<div class="phase4-ticket"><span class="phase4-rank">#${row.position}</span>
      <div class="phase4-ticket-main"><strong>${safe(v.vehicleNumber)}</strong><small>${safe(v.vehicleType)} · ${preference}</small>
      <small>Waiting since ${safe(v.entryTime)} · Ticket ${safe(v.ticketId)}</small></div>
      <span class="phase4-tag ${priority}">${safe(row.tier)}</span>
      <span class="phase4-eligible">${row.eligibleSlots} compatible free</span></div>`;
  }).join("") : '<div class="empty-state compact">No vehicles are waiting. New arrivals can be assigned normally.</div>';
  const select = $("queuePlate");
  const previous = select.value;
  select.innerHTML = rows.length ? rows.map(r => `<option value="${safe(r.vehicle.vehicleNumber)}">${safe(r.vehicle.vehicleNumber)} · ${safe(r.tier)}</option>`).join("")
    : '<option value="">No waiting vehicles</option>';
  if (rows.some(r=>r.vehicle.vehicleNumber===previous)) select.value=previous;
  $("queueAdminForm").querySelector("button[type='submit']").disabled = !rows.length;
}
async function submitQueueAdmin(event) {
  event.preventDefault();
  const form = event.currentTarget;
  await runAction(form, async () => {
    const plate = $("queuePlate").value;
    const action = $("queueAction").value;
    if (!plate) throw new Error("Select a waiting vehicle.");
    if (action === "cancel" && !window.confirm(`Cancel waiting ticket ${plate}?`)) return;
    const url = action === "cancel" ? "/api/queue/cancel" : "/api/queue/priority";
    const body = action === "cancel" ? {vehicleNumber:plate} : {vehicleNumber:plate,vip:action === "vip"};
    const result = await api(url,{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(body)});
    toast(result.message);
    await refresh();
  });
}
function renderDSA(d) {
  $("linked").innerHTML = d.linkedList.length ? `<div class="chain"><b>HEAD →</b>${d.linkedList.map((v,i) => `<div class="node"><i class="${vehicleIcon(v.vehicleType)}"></i><b>${safe(v.vehicleNumber)}</b><small>${safe(v.parkingSlot)}</small></div>${i < d.linkedList.length - 1 ? "→" : "→ NULL"}`).join("")}</div>` : "HEAD → NULL";
  $("stack").innerHTML = d.stack.length ? `<div class="stack"><b>TOP</b>${d.stack.map(v => `<div class="node"><b>${safe(v.vehicleNumber)}</b><small>${safe(v.parkingSlot)} · Rs. ${safeNumber(v.parkingFee)}</small></div>`).join("")}</div>` : "STACK EMPTY";
  $("qDsa").innerHTML = d.queue.length ? `<div class="chain"><b>FRONT →</b>${d.queue.map(v=>`<div class="node"><b>${safe(v.vehicleNumber)}</b><small>${v.vehicleType === "Emergency" ? "Emergency" : v.priority ? "Priority" : "Normal"}</small></div>`).join(" → ")}<b> ← REAR</b></div>` : "QUEUE EMPTY";
  $("pq").innerHTML = d.priorityQueue.length ? `<div class="priority">${d.priorityQueue.map((v,i)=>`<div><i class="${vehicleIcon(v.vehicleType)}"></i><b>${safe(v.vehicleNumber)}</b><span>${v.vehicleType === "Emergency" ? "EMERGENCY" : v.priority ? "HIGH PRIORITY" : "NORMAL"}${i === 0 ? " · NEXT" : ""}</span></div>`).join("")}</div>` : "NO WAITING VEHICLES";
  const forward = d.doublyForward || [];
  const backward = d.doublyBackward || [];
  $("doubly").innerHTML = forward.length
    ? `<div class="dsa-direction"><b>HEAD / next →</b> ${forward.map(v => `<span class="dsa-pill">${safe(v.vehicleNumber)}</span>`).join(' ⇄ ')}</div><div class="dsa-direction"><b>TAIL / prev ←</b> ${backward.map(v => `<span class="dsa-pill">${safe(v.vehicleNumber)}</span>`).join(' ⇄ ')}</div>`
    : '<div class="empty-state compact">No exit history yet.</div>';
  $("circular").innerHTML = `<div class="circular-chain">${data.slots.map((slot,i)=>`<span class="dsa-pill ${slot.occupied ? 'slot-occupied' : ''}">${safe(slot.id)}</span>${i < data.slots.length-1 ? '<span class="cycle-arrow">→</span>' : ''}`).join('')}<span class="cycle-return">↩ HEAD</span></div>`;
  $("dsaLinkedCount").textContent = d.metrics.linkedListNodes;
  $("dsaQueueCount").textContent = d.metrics.queueNodes;
  $("dsaStackCount").textContent = d.metrics.stackRecords;
  $("dsaPriorityCount").textContent = d.metrics.priorityItems;
}

function formatStay(seconds) {
  const minutes = Math.max(0,Math.floor(seconds / 60));
  const days = Math.floor(minutes / 1440);
  const hours = Math.floor((minutes % 1440) / 60);
  const mins = minutes % 60;
  return `${days ? days + 'd ' : ''}${hours}h ${mins}m`;
}
function closeSlotInfo() {
  modalSlot = null;
  const modal = $("slotModal");
  modal.hidden = true;
  modal.setAttribute("aria-hidden","true");
  document.body.classList.remove("slot-dialog-open");
}
function slotInfo(id) {
  if (!(data.slots || []).some(s=>s.id===id)) return;
  modalSlot = id;
  $("slotModal").hidden = false;
  $("slotModal").setAttribute("aria-hidden","false");
  document.body.classList.add("slot-dialog-open");
  updateSlotModal();
  $("closeSlotModal").focus();
}
function updateSlotModal() {
  if (!modalSlot || $("slotModal").hidden) return;
  const slot = (data.slots||[]).find(s=>s.id===modalSlot);
  if(!slot) return closeSlotInfo();
  const v=slot.vehicle;
  const status=slot.occupied?'Occupied':slot.restriction===2?'Disputed':slot.restriction===1?'Reserved':slot.reachable===false?'Unreachable':'Available';
  const since=Number(v?.entryEpoch||0);
  const elapsed=since>0?Math.max(0,Math.floor(Date.now()/1000)-since):0;
  const billHours=Math.max(1,Math.ceil(elapsed/3600));
  const rate=Number(activeRates[v?.vehicleType]?.hourly||0);
  const money=rate?`Rs. ${safeNumber(billHours*rate)}`:'See exit receipt';
  const rows=(items)=>items.map(([key,value])=>`<div class="slot-modal-row"><span>${safe(key)}</span><strong>${safe(value)}</strong></div>`).join('');
  $("slotModalContent").innerHTML=`
  <div class="slot-modal-hero ${slot.occupied?'is-occupied':'is-free'}">
    <div class="slot-modal-bigicon">${slot.occupied?carIllustration(v.vehicleType):'<span class="empty-bay-symbol">P</span>'}</div>
    <div><div class="slot-chip">${safe(status)}</div><h2 id="slotModalTitle">${safe(slot.id)}</h2><p>Building ${safe(slot.building)} · Floor ${safe(slot.floor)} · ${safe(slot.size)} capacity</p></div>
  </div>
  <div class="slot-modal-content">
    ${slot.occupied ? `<h3>Vehicle information</h3>${rows([
      ['Number plate', v.vehicleNumber], ['Ticket',v.ticketId||'—'],['Vehicle type',v.vehicleType],['Owner',v.ownerName],
      ['Entry time',v.entryTime||'—'], ['Time parked',formatStay(elapsed)], ['Started hours',billHours],
      ['Current hourly rate',rate?'Rs. '+safeNumber(rate):'—'],['Estimated fee at this moment',money],
      ['Priority',v.vehicleType==='Emergency'?'Emergency':v.priority?'VIP':'Normal']
    ])}` : `<h3>Space information</h3>${rows([
      ['Status',status], ['Suitable capacity',slot.size],['Road accessible',slot.reachable===false?'No':'Yes'],
      ['Rule',slot.restriction===1?'Priority only':slot.restriction===2?'Unavailable':'Normal'],
      ['Admin note',slot.restrictionNote||'None']
    ])}`}
    <p class="slot-modal-footnote">${slot.occupied?'Estimated fee updates live; final charge is calculated by C++ at exit.':'Use Settings to update reservations or disputed spaces.'}</p>
  </div>`;
}

function locate(id) {
  const slot = (data.slots || []).find(s=>s.id===id);
  if (slot) {
    $("filterBuilding").value = slot.building;
    $("filterFloor").value = String(slot.floor);
    $("filterSize").value = "";
    renderParkingMap();
  }
  go("parking");
  const el = [...document.querySelectorAll("#map [data-slot]")].find(e => e.dataset.slot === id);
  if (el) {
    el.scrollIntoView({behavior:"smooth", block:"center"});
    el.classList.add("locate-pulse");
    setTimeout(() => el.classList.remove("locate-pulse"), 1800);
    slotInfo(id);
  }
}

function ticket(v,route) {
  const el = $("entryTicket");
  el.hidden = false;
  el.innerHTML = `<div class="ticket"><span class="ticket-badge"><i class="fa-solid fa-ticket"></i></span><h3>Parking Ticket Created</h3><p>Ticket <b>${safe(v.ticketId)}</b></p><hr><p>Vehicle: <b>${safe(v.vehicleNumber)}</b></p><p>Type: ${safe(v.vehicleType)}</p><p>Owner: ${safe(v.ownerName)}</p><p>Slot: <b>${safe(v.parkingSlot)}</b></p><p>Entry: ${safe(v.entryTime)}</p>${route?.reachable?`<p class="phase5-route-ticket"><b>BFS Route:</b> ${route.path.map(safe).join(' &rarr; ')}</p>`:''}</div>`;
  el.scrollIntoView({behavior:"smooth", block:"nearest"});
}

function syncSlotRuleChoices() {
  const sel=$("ruleSlot"); if(!sel)return;
  const current=sel.value;
  sel.innerHTML=(data.slots||[]).map(s=>`<option value="${safe(s.id)}">${safe(s.id)} · ${safe(s.status||'Available')}</option>`).join("");
  if((data.slots||[]).some(s=>s.id===current))sel.value=current;
  const currentSlot=(data.slots||[]).find(s=>s.id===sel.value);
  if(currentSlot && !$("slotRuleForm").contains(document.activeElement)){
    $("ruleMode").value=String(currentSlot.restriction||0);
    $("ruleNote").value=currentSlot.restrictionNote||"";
  }
  $("slotRuleSummary").textContent=`${data.restricted||0} restricted/reserved slot(s), ${data.available||0} normal free slot(s).`;
}
async function previewAllocation() {
  const params=new URLSearchParams({type:$("type").value,vip:$("priority").checked?"1":"0",
    building:$("entryBuilding").value,floor:$("entryFloor").value});
  try {const result=await api("/api/recommend?"+params.toString()); $("allocationPreview").textContent=result.explanation;}
  catch(error){toast(error.message,true);}
}
async function submitSlotRule(event){
  event.preventDefault();
  const slotId=$("ruleSlot").value,mode=Number($("ruleMode").value),note=$("ruleNote").value.trim();
  const found=(data.slots||[]).find(s=>s.id===slotId);
  const status=$("slotRuleStatus");
  if(!found)return toast("Select a valid slot.",true);
  if(found.occupied&&mode!==0)return toast("This space is occupied. Process its exit first.",true);
  const saveButton=$("slotRuleForm").querySelector('button[type="submit"]');
  saveButton.disabled=true;
  status.textContent="Saving rule to C++ engine...";
  try{
    const result=await api("/api/slot-rule",{method:"POST",headers:{"Content-Type":"application/json"},
      body:JSON.stringify({slotId,mode,note})});
    status.textContent=`Saved: ${result.slot.id} — ${result.slot.status}${result.slot.restrictionNote?' · '+result.slot.restrictionNote:''}`;
    toast("Slot restriction saved successfully.");
    data=result.state;
    render();
    $("ruleSlot").value=slotId;
    $("ruleMode").value=String(mode);
    $("ruleNote").value=mode===0?'':note;
  }catch(error){status.textContent=`Not saved: ${error.message}`;toast(error.message,true);}
  finally{saveButton.disabled=false;}
}
let roadData={roads:[],closedCount:0};
let routeData=null;
function syncRouteSlotChoices(){
  const el=$("routeSlot"); if(!el)return;
  const previous=el.value;
  el.innerHTML=(data.slots||[]).map(slot=>`<option value="${safe(slot.id)}">${safe(slot.id)} · ${safe(slot.size)}</option>`).join("");
  if((data.slots||[]).some(s=>s.id===previous))el.value=previous;
}
function graphPosition(node,floors){
  if(node==="ENTRY")return {x:450,y:42};
  let match=/^G-([ABC])$/.exec(node);
  if(match)return {x:{A:155,B:450,C:745}[match[1]],y:145};
  match=/^([ABC])-F([1-4])$/.exec(node);
  if(match)return {x:{A:155,B:450,C:745}[match[1]]-48,y:245+(Number(match[2])-1)*106};
  match=/^([ABC])-F([1-4])-S\d{2}$/.exec(node);
  if(match)return {x:{A:155,B:450,C:745}[match[1]]+72,y:245+(Number(match[2])-1)*106};
  return null;
}
function renderRoadGraph(){
  const roads=roadData.roads||[];
  const floors=Number(data?.layout?.floorsPerBuilding||2);
  const target=$("routeSlot")?.value||"";
  const path=routeData?.path||[];
  const routeEdge=new Set(path.slice(1).map((node,i)=>[node,path[i]].sort().join("|")));
  const nodes=new Set(["ENTRY"]);
  for(const r of roads)for(const n of [r.from,r.to])if(!n.includes("-S"))nodes.add(n);
  if(target)nodes.add(target);
  const coordinates={};
  for(const n of nodes)coordinates[n]=graphPosition(n,floors);
  const visibleRoads=roads.filter(r=>coordinates[r.from]&&coordinates[r.to]);
  const edges=visibleRoads.map(r=>{
    const p=coordinates[r.from],q=coordinates[r.to];
    const key=[r.from,r.to].sort().join("|");
    const highlight=routeEdge.has(key);
    const color=r.blocked?"#ff7787":highlight?"#4be0bb":"#7d8ca8";
    return `<line x1="${p.x}" y1="${p.y}" x2="${q.x}" y2="${q.y}" stroke="${color}" stroke-width="${highlight?6:2.8}" stroke-dasharray="${r.blocked?'8 6':''}" opacity="${r.blocked||highlight?1:.6}" stroke-linecap="round"/>`;
  }).join("");
  const labels=[...nodes].map(n=>{
    const p=coordinates[n];if(!p)return "";
    const active=path.includes(n);
    const isSlot=n.includes("-S");
    const fill=active?"#198977":n==="ENTRY"?"#6754bd":isSlot?"#bb873f":"#303950";
    return `<g><rect x="${p.x-(isSlot?57:48)}" y="${p.y-16}" width="${isSlot?114:96}" height="32" rx="11" fill="${fill}" stroke="${active?'#8cebd5':'#69748c'}" stroke-width="${active?2:1}"/><text x="${p.x}" y="${p.y+4}" text-anchor="middle" font-size="12" fill="#fff" font-weight="600">${safe(n)}</text></g>`;
  }).join("");
  $("roadGraph").innerHTML=`<svg viewBox="0 0 900 ${300+Math.max(0,floors-1)*106}" role="img" aria-label="Parking buildings and road graph" xmlns="http://www.w3.org/2000/svg">${edges}${labels}</svg>`;
}
function syncRoadEdges(){
  const el=$("roadEdge"),previous=el.value;
  if(!el)return;
  el.innerHTML=(roadData.roads||[]).map((road,i)=>`<option value="${i}">${safe(road.from)} ↔ ${safe(road.to)} · ${road.blocked?'BLOCKED':'OPEN'}</option>`).join("");
  if([...el.options].some(o=>o.value===previous))el.value=previous;
  $("roadCount").textContent=(roadData.roads||[]).length;
  $("blockedCount").textContent=roadData.closedCount||0;
}
async function loadRoadGraph(){
  roadData=await api("/api/graph");
  syncRouteSlotChoices();syncRoadEdges();
  if($("routeSlot").value)await runRoute(false);
  else renderRoadGraph();
}
async function runRoute(updateGraph=true){
  const slot=$("routeSlot").value;
  if(!slot)return;
  routeData=await api("/api/route?slot="+encodeURIComponent(slot));
  if(routeData.reachable){
    $("routeResult").innerHTML=`<strong>Route found — ${routeData.edges} road segments</strong><p>${routeData.path.map(safe).join(" &rarr; ")}</p><small>DFS also confirms this slot is reachable. BFS minimizes the number of edges, not distance in metres.</small>`;
  }else{
    $("routeResult").innerHTML=`<strong style="color:#ff8896">No accessible route</strong><p>${safe(routeData.message)}</p><small>DFS reachable: ${routeData.dfsReachable?'Yes':'No'}. Reopen a blocked road and recalculate.</small>`;
  }
  renderRoadGraph();
}
async function submitRoadRule(e){
  e.preventDefault();
  const road=(roadData.roads||[])[Number($("roadEdge").value)];
  if(!road)return toast("Select a road connection.",true);
  try{
    await api("/api/road-block",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({from:road.from,to:road.to,blocked:$("roadMode").value==="1"})});
    toast(`Road ${road.from} ↔ ${road.to} updated.`);
    await refresh();await loadRoadGraph();
  }catch(error){toast(error.message,true);}
}

function renderRates(info) {
  activeRates=info.rates||{};
  $("rates").innerHTML=info.types.map(type=>`<tr>
    <td><strong>${safe(type)}</strong></td>
    <td>Rs. ${safeNumber(info.rates[type].hourly)} / hour</td>
    <td><button class="ghost rate-edit" type="button" data-rate-type="${safe(type)}"><i class="fa-solid fa-pen"></i> Edit rate</button></td>
  </tr>`).join('');
  $("rates").querySelectorAll("[data-rate-type]").forEach(button=>button.addEventListener("click",async()=>{
    if(rateBusy)return;
    const vehicleType=button.dataset.rateType;
    const current=Number(activeRates[vehicleType]?.hourly||0);
    const entered=window.prompt(`Hourly rate for ${vehicleType} (PKR)`,String(current));
    if(entered===null)return;
    const hourly=Number(entered.trim());
    if(!/^\d+$/.test(entered.trim())||!Number.isInteger(hourly)||hourly<1||hourly>1000000)return toast("Enter a whole-number rate between 1 and 1,000,000.",true);
    rateBusy=true;button.disabled=true;
    try{
      const result=await api('/api/rates',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({vehicleType,hourly})});
      toast(result.message);
      await loadRates();
      updateSlotModal();
    }catch(error){toast(error.message,true);}
    finally{rateBusy=false;button.disabled=false;}
  }));
}
async function loadRates(){return renderRates(await api('/api/vehicle-types'));}

async function runAction(form, callback) {
  const button = form.querySelector("button[type='submit']");
  button.disabled = true;
  try { await callback(); }
  catch (error) { toast(error.message || "Could not contact the server.", true); }
  finally { button.disabled = false; }
}

function setupForms() {
  $("entryForm").addEventListener("submit", async event => {
    event.preventDefault();
    const form = event.currentTarget;
    await runAction(form, async () => {
      const result = await api("/api/park", {method:"POST", headers:{"Content-Type":"application/json"}, body:JSON.stringify({
        vehicleNumber:$("number").value, ownerName:$("owner").value,
        vehicleType:$("type").value, priority:$("priority").checked,
        preferredBuilding:$("entryBuilding").value, preferredFloor:Number($("entryFloor").value),
      })});
      form.reset();
      toast(result.message);
      $("allocationPreview").textContent=result.explanation||result.message;
      if (result.queued) {
        $("entryTicket").hidden = true;
        go("queue");
      } else {
        ticket(result.vehicle,result.route);
      }
      await refresh();
    });
  });
  $("exitForm").addEventListener("submit", async event => {
    event.preventDefault();
    const form = event.currentTarget;
    await runAction(form, async () => {
      const result = await api("/api/exit", {method:"POST", headers:{"Content-Type":"application/json"}, body:JSON.stringify({vehicleNumber:$("exitNumber").value})});
      renderPhase7Receipt(result);
      form.reset();
      toast(result.message);
      await refresh();
    });
  });
}

async function search() {
  const q = $("query").value.trim();
  if (!q) return toast("Enter a vehicle number, ticket ID, owner, type or slot.", true);
  try {
    const found = await api("/api/search?q=" + encodeURIComponent(q));
    $("results").innerHTML = found.results.length
      ? found.results.map((v,i) => `<div class="panel result"><div class="result-main"><span class="result-icon"><i class="${vehicleIcon(v.vehicleType)}"></i></span><div><b>${safe(v.vehicleNumber)}</b><p>${safe(v.vehicleType)} · ${safe(v.ownerName)} · ${safe(v.parkingSlot || "Waiting")}</p></div></div>${v.parkingSlot ? `<button class="primary small-btn" data-locate="${safe(v.parkingSlot)}">Locate</button>` : ""}</div>`).join("")
      : `<div class="panel empty-state"><i class="fa-solid fa-magnifying-glass"></i><b>No vehicle found</b><span>Try another number, ticket ID, owner, type or slot.</span></div>`;
    document.querySelectorAll("#results [data-locate]").forEach(b => b.addEventListener("click", () => locate(b.dataset.locate)));
  } catch (error) { toast(error.message, true); }
}

async function reset() {
  if (!confirm("Reset SmartPark parking data INCLUDING weekly/monthly history? C++ and SQLite backups will be created.")) return;
  try {
    await api("/api/reset", {method:"POST"});
    $("entryTicket").hidden = true;
    $("bill").textContent = "No active bill.";
    $("results").innerHTML = "";
    // Parking details now appear in an accessible modal.
    toast("Parking state reset. Historical backups saved in data/.");
    await refresh();
  } catch (error) { toast(error.message, true); }
}

async function configureLayout(event) {
  event.preventDefault();
  const floors=Number($("layoutFloors").value),slots=Number($("layoutSlots").value);
  if (!Number.isInteger(floors)||!Number.isInteger(slots)||floors<1||floors>4||slots<3||slots>20) return toast("Use 1–4 floors and 3–20 slots per floor.",true);
  if (!confirm(`Set all 3 buildings to ${floors} floors with ${slots} slots per floor? All parking and queue records must be empty.`)) return;
  try {
    await api("/api/layout",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({floorsPerBuilding:floors,slotsPerFloor:slots})});
    toast("Building layout updated and saved."); await refresh();
  } catch(error) {toast(error.message,true);}
}
function clock() { $("clock").textContent = new Date().toLocaleString([], {weekday:"short",day:"2-digit",month:"short",hour:"2-digit",minute:"2-digit"}); }

document.addEventListener("DOMContentLoaded", () => {
  nav(); setupForms();
  $("phase9MenuBtn").addEventListener('click',phase9ToggleMenu);
  $("phase9Backdrop").addEventListener('click',phase9CloseMenu);
  document.addEventListener('keydown', e => {if(e.key==='Escape'){closeSlotInfo();phase9CloseMenu();}});
  $("queueAdminForm").addEventListener("submit", submitQueueAdmin);
  $("layoutForm").addEventListener("submit",configureLayout);
  $("slotRuleForm").addEventListener("submit",submitSlotRule);
  $("roadRuleForm").addEventListener("submit",submitRoadRule);
  $("runBfs").addEventListener("click",()=>runRoute().catch(error=>toast(error.message,true)));
  $("routeSlot").addEventListener("change",()=>runRoute().catch(error=>toast(error.message,true)));
  $("previewAllocation").addEventListener("click",previewAllocation);
  $("closeSlotModal").addEventListener("click",closeSlotInfo);
  document.querySelectorAll("[data-close-slot]").forEach(el=>el.addEventListener("click",closeSlotInfo));
  setInterval(updateSlotModal, 1000);
  $("ruleSlot").addEventListener("change",()=>{
    const s=(data.slots||[]).find(x=>x.id===$("ruleSlot").value);
    if(s){$("ruleMode").value=String(s.restriction||0);$("ruleNote").value=s.restrictionNote||"";}
  });
  ["filterBuilding","filterFloor","filterSize"].forEach(id=>$(id).addEventListener("change",renderParkingMap));
  $("reset").addEventListener("click", reset);
  $("settingsReset").addEventListener("click", reset);
  $("refreshNow").addEventListener("click", refresh);
  $("searchBtn").addEventListener("click", search);
  $("query").addEventListener("keydown", e => {if (e.key === "Enter") search();});
  loadRates().catch(() => { $("rates").innerHTML = '<tr><td colspan="3">Rates unavailable. Refresh to retry.</td></tr>'; });
  clock(); refresh();
  setInterval(clock, 30000);
  setInterval(() => {if(!document.hidden) refresh();}, 12000);
});

let lastPhase7Bill = null;
function phase7Duration(seconds) {
  const n = Number(seconds) || 0;
  return `${Math.floor(n / 3600)}h ${Math.floor((n % 3600) / 60)}m ${n % 60}s`;
}
function phase7ReceiptText(b) {
  return [`SMARTPARK TRACE - PARKING RECEIPT`, `Receipt: ${b.receiptId}`,
    `Ticket: ${b.ticketId}`, `Vehicle: ${b.vehicleNumber} (${b.vehicleType})`,
    `Owner: ${b.ownerName}`, `Slot: ${b.slot}`, `Entry: ${b.entryTime}`,
    `Exit: ${b.exitTime}`, `Actual duration: ${phase7Duration(b.durationSeconds)}`,
    `Started hours: ${b.hours}`, `Hourly rate: Rs. ${b.hourlyRate}`,
    `TOTAL: Rs. ${b.fee}`, `Thank you for parking with SmartPark TRACE.`].join("\n");
}
function renderPhase7Receipt(response) {
  const b = response.bill;
  lastPhase7Bill = b;
  const queue = response.assignedFromQueue;
  $("bill").innerHTML = `<div class="phase7-receipt">
    <div class="receipt-icon"><i class="fa-solid fa-circle-check"></i></div>
    <small>SMARTPARK TRACE · VERIFIED C++ BILL</small>
    <h3>Parking Receipt</h3><p class="phase7-receipt-id">${safe(b.receiptId)}</p>
    <div class="phase7-row"><span>Vehicle</span><b>${safe(b.vehicleNumber)} · ${safe(b.vehicleType)}</b></div>
    <div class="phase7-row"><span>Ticket</span><b>${safe(b.ticketId)}</b></div>
    <div class="phase7-row"><span>Owner</span><b>${safe(b.ownerName)}</b></div>
    <div class="phase7-row"><span>Location</span><b>${safe(b.slot)}</b></div>
    <div class="phase7-row"><span>Entry</span><b>${safe(b.entryTime)}</b></div>
    <div class="phase7-row"><span>Exit</span><b>${safe(b.exitTime)}</b></div>
    <div class="phase7-row"><span>Actual stay</span><b>${phase7Duration(b.durationSeconds)}</b></div>
    <div class="phase7-row"><span>Billed hours</span><b>${safeNumber(b.hours)} started hour(s)</b></div>
    <div class="phase7-row"><span>Hourly rate</span><b>Rs. ${safeNumber(b.hourlyRate)}</b></div>
    <div class="phase7-total"><span>Total due</span><strong>Rs. ${safeNumber(b.fee)}</strong></div>
    <p class="phase7-fineprint">Each started hour is charged in full.</p>
    <div class="phase7-actions"><button class="ghost" id="phase7Download" type="button"><i class="fa-solid fa-download"></i> Download</button><button class="primary" id="phase7Print" type="button"><i class="fa-solid fa-print"></i> Print</button></div>
    ${queue ? `<p class="queue-assigned">${safe(queue.vehicleNumber)} assigned after exit.</p>` : ""}
  </div>`;
  $("phase7Download").addEventListener("click", () => {
    const url = URL.createObjectURL(new Blob([phase7ReceiptText(b)], {type:"text/plain;charset=utf-8"}));
    const link = document.createElement("a"); link.href = url;
    link.download = `${b.receiptId.replace(/[^a-zA-Z0-9-]/g, "")}.txt`;
    document.body.appendChild(link); link.click(); link.remove();
    URL.revokeObjectURL(url);
  });
  $("phase7Print").addEventListener("click", () => window.print());
}
