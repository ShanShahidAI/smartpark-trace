// =========== SMARTPARK TRACE HISTORICAL DASHBOARD ===========
let phase8Current = null;
let phase8Offset = 0;
const phase8PageSize = 100;
function phase8Local(when) {
  if(!when) return 'Still parked';
  return new Date(Number(when)*1000).toLocaleString([], {dateStyle:'medium',timeStyle:'short'});
}
function phase8FormQuery() {
  const p = new URLSearchParams({
    period:document.getElementById('phase8Period').value,
    date:document.getElementById('phase8Date').value,
    building:document.getElementById('phase8Building').value,
    floor:document.getElementById('phase8Floor').value,
    slot:document.getElementById('phase8Slot').value.trim().toUpperCase(),
    plate:document.getElementById('phase8Plate').value.trim().toUpperCase(),
    limit:String(phase8PageSize),offset:String(phase8Offset)
  });
  return p.toString();
}
function phase8Bars(target, list, label, value, suffix) {
  const root=document.getElementById(target);
  if(!list.length){root.innerHTML='<p class="phase8-empty">No activity in this period.</p>';return;}
  const maximum=Math.max(1,...list.map(x=>Number(x[value]||0)));
  root.innerHTML=list.map(x=>`<div class="phase8-bar-row"><div><strong>${safe(x[label])}</strong><span>${safeNumber(x[value])} ${safe(suffix)}</span></div><div class="phase8-track"><span style="width:${Math.max(2,Math.round(100*Number(x[value]||0)/maximum))}%"></span></div></div>`).join('');
}
async function loadPhase8History() {
  const tbody=document.getElementById('phase8ReportBody');
  if(!tbody)return;
  tbody.innerHTML='<tr><td colspan="8">Loading saved history…</td></tr>';
  try {
    const report=await api('/api/history/report?'+phase8FormQuery());
    phase8Current=report;
    const summary=report.summary;
    document.getElementById('phase8PeriodLabel').textContent=report.label+' · Reports include sessions overlapping the period; revenue from completed exits only.';
    document.getElementById('phase8Arrivals').textContent=safeNumber(summary.arrivals);
    document.getElementById('phase8Departures').textContent=safeNumber(summary.departures);
    document.getElementById('phase8Revenue').textContent='Rs. '+safeNumber(summary.revenue);
    document.getElementById('phase8Minutes').textContent=safeNumber(summary.occupiedMinutes);
    document.getElementById('phase8RecordCount').textContent=`${safeNumber(report.totalMatching)} matching sessions`;
    tbody.innerHTML=report.rows.length ? report.rows.map(v=>`<tr><td>${safe(v.ticketId)}</td><td><strong>${safe(v.vehicleNumber)}</strong><small>${safe(v.vehicleType)}</small></td><td>Building ${safe(v.building)} · Floor ${Number(v.floor)}</td><td>${safe(v.parkingSlot)}</td><td>${safe(phase8Local(v.entryEpoch))}</td><td>${safe(phase8Local(v.exitEpoch))}</td><td>${Number(v.hours)}</td><td>Rs. ${safeNumber(v.parkingFee)}</td></tr>`).join('')
      :'<tr><td colspan="8">No matching parking sessions for this period.</td></tr>';
    const page=Math.floor(phase8Offset/phase8PageSize)+1;
    document.getElementById('phase8Paging').textContent=`Page ${page} · ${Math.min(phase8Offset+1,report.totalMatching)}–${Math.min(phase8Offset+report.rows.length,report.totalMatching)} of ${report.totalMatching}`;
    document.getElementById('phase8Prev').disabled=phase8Offset===0;
    document.getElementById('phase8Next').disabled=phase8Offset+report.rows.length>=report.totalMatching;
    phase8Bars('phase8BuildingBars',summary.buildings,'building','revenue','PKR');
    phase8Bars('phase8TopSlots',summary.topSlots,'slot','occupiedMinutes','min');
    loadPhase8Audit().catch(error=>toast(error.message,true));
  } catch(error) {tbody.innerHTML='<tr><td colspan="8">'+safe(error.message)+'</td></tr>';toast(error.message,true);}
}
async function loadPhase8Audit() {
  const result=await api('/api/history/audit');
  document.getElementById('phase8Audit').innerHTML=result.events.length
    ? result.events.slice(0,25).map(e=>`<div class="phase8-audit-row"><b>${safe(e.event)}</b><span>${safe(e.plate||e.slot||e.detail||'Admin operation')}</span><small>${safe(phase8Local(e.moment))}</small></div>`).join('')
    :'<p class="phase8-empty">No events recorded.</p>';
}
async function phase8LookUp(event) {
  event.preventDefault();
  const slot=document.getElementById('phase8LookupSlot').value.trim().toUpperCase();
  const at=document.getElementById('phase8At').value;
  try {
    const result=await api('/api/history/slot-at?'+new URLSearchParams({slot,at}).toString());
    const el=document.getElementById('phase8LookupResult');
    el.innerHTML=result.occupied ? `<strong>OCCUPIED</strong><p>${safe(result.vehicle.vehicleNumber)} · ${safe(result.vehicle.ticketId)}</p><small>Entered ${safe(phase8Local(result.vehicle.entryEpoch))}; exited ${safe(phase8Local(result.vehicle.exitEpoch))}</small>`
      :'<strong>NO RECORDED OCCUPANCY</strong><p>No saved vehicle record matches this slot and time. This does not prove the physical slot was free.</p>';
  } catch(error) {toast(error.message,true);}
}
async function phase8DownloadCsv(){
  if(!phase8Current){toast('Generate a report first.',true);return;}
  const button=document.getElementById('phase8Csv');
  button.disabled=true;
  const original=button.textContent;
  button.textContent='Preparing full filtered CSV…';
  try{
    const requested=phase8Current.totalMatching;
    const results=[];
    for(let offset=0;offset<requested;offset+=500){
      // Explicitly request ALL matching sessions, page by page; never export just the visible 100.
      const p=new URLSearchParams(phase8FormQuery());
      p.set('offset',String(offset));p.set('limit','500');
      const portion=await api('/api/history/report?'+p.toString());
      results.push(...portion.rows);
      button.textContent=`Exporting ${results.length}/${requested}…`;
      if(!portion.rows.length) break;
    }
    const header=['Ticket','Plate','Owner','Type','Building','Floor','Slot','Entry','Exit','Started Hours','Fee'];
    const escCell=x=>`"${String(x??'').replace(/^[=+@-]/,'\'$&').replace(/"/g,'""')}"`;
    const rows=results.map(v=>[v.ticketId,v.vehicleNumber,v.ownerName,v.vehicleType,v.building,v.floor,v.parkingSlot,phase8Local(v.entryEpoch),v.exitEpoch?phase8Local(v.exitEpoch):'',v.hours,v.parkingFee]);
    const csv=[header,...rows].map(row=>row.map(escCell).join(',')).join('\r\n');
    const url=URL.createObjectURL(new Blob(['\uFEFF'+csv],{type:'text/csv;charset=utf-8'}));
    const a=document.createElement('a');a.href=url;
    a.download='smartpark-history-'+phase8Current.period+'-'+document.getElementById('phase8Date').value+'.csv';
    document.body.appendChild(a);a.click();a.remove();
    setTimeout(()=>URL.revokeObjectURL(url),1000);
    toast(`Exported ${results.length} matching parking sessions.`);
  }catch(error){toast(error.message,true);}
  finally{button.disabled=false;button.textContent=original;}
}
document.addEventListener('DOMContentLoaded',()=>{
  const now=new Date(), pad=n=>String(n).padStart(2,'0');
  const today=`${now.getFullYear()}-${pad(now.getMonth()+1)}-${pad(now.getDate())}`;
  document.getElementById('phase8Date').value=today;
  document.getElementById('phase8At').value=`${today}T${pad(now.getHours())}:${pad(now.getMinutes())}`;
  document.getElementById('phase8Filters').addEventListener('submit',event=>{
    event.preventDefault();phase8Offset=0;loadPhase8History();
  });
  document.getElementById('phase8Lookup').addEventListener('submit',phase8LookUp);
  document.getElementById('phase8Csv').addEventListener('click',phase8DownloadCsv);
  document.getElementById('phase8Prev').addEventListener('click',()=>{phase8Offset=Math.max(0,phase8Offset-phase8PageSize);loadPhase8History();});
  document.getElementById('phase8Next').addEventListener('click',()=>{phase8Offset+=phase8PageSize;loadPhase8History();});
  document.getElementById('phase8RefreshAudit').addEventListener('click',()=>loadPhase8Audit().catch(e=>toast(e.message,true)));
});
window.loadPhase8History=loadPhase8History;
