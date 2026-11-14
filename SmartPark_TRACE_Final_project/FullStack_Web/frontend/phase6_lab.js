/* – live algorithm metrics from the SAME C++ engine, never client-estimated. */
(() => {
  const el=id=>document.getElementById(id);
  const safe=x=>String(x??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  async function read(path){
    const response=await fetch(path,{credentials:'same-origin'});
    const json=await response.json();
    if(!response.ok)throw new Error(json.error||`HTTP ${response.status}`);
    return json;
  }
  const trace=(label,x)=>`<div class="phase6-metric"><strong>${label}</strong><b>${x.comparisons}</b><small>${x.found?'FOUND':'NOT FOUND'}</small>${x.path?`<span>Path: ${safe(x.path.join(' → ')||'(empty)')}</span>`:''}</div>`;
  async function search(){
    const box=el('phase6TicketResults');
    const key=el('phase6TicketKey').value.trim();
    if(!/^[1-9][0-9]{0,7}$/.test(key)){box.textContent='Enter valid ticket number 1–99999999.';return;}
    box.textContent='Reading live C++ indexes…';
    try{
      const a=await read('/api/algorithms/ticket?key='+encodeURIComponent(key));
      box.innerHTML=`<div class="phase6-metrics">${trace('Linear Search',a.linear)}${trace('BST',a.bst)}${trace('AVL',a.avl)}</div>`+
         `<p><b>Ticket ${a.ticket}</b> · ${a.activeCount} parked · Indexes ${a.indexesValid?'valid':'INVALID'} · ${a.avl.plate?'Vehicle '+safe(a.avl.plate):'Ticket not in active parking'}</p>`;
    }catch(e){box.textContent=e.message;}
  }
  async function sorting(){
    const box=el('phase6SortResults'),source=el('phase6SortSource').value,field=el('phase6SortField').value;
    box.textContent='Counting comparisons inside C++…';
    try{
      const a=await read('/api/algorithms/sort?source='+encodeURIComponent(source)+'&field='+encodeURIComponent(field));
      const v=a.sorted.slice(0,12).map(x=>`<span>${safe(field==='fee'?`Rs. ${x.parkingFee}`:x.vehicleNumber)}</span>`).join('');
      box.innerHTML=`<div class="phase6-metrics">`+
        `<div class="phase6-metric"><strong>Bubble Sort</strong><b>${a.bubble.comparisons}</b><small>comparisons · ${a.bubble.swaps} swaps</small></div>`+
        `<div class="phase6-metric"><strong>Selection Sort</strong><b>${a.selection.comparisons}</b><small>comparisons · ${a.selection.swaps} swaps</small></div></div>`+
        `<p><b>${a.count} actual records</b> · Both algorithms ${a.resultsIdentical?'returned the same record order':'produced different tie orders'}</p>`+
        `<div class="phase6-values">${v||'<small>No records yet. Park a vehicle to compare.</small>'}</div>`;
    }catch(e){box.textContent=e.message;}
  }
  el('phase6TicketRun')?.addEventListener('click',search);
  el('phase6SortRun')?.addEventListener('click',sorting);
  el('phase6TicketKey')?.addEventListener('keydown',e=>{if(e.key==='Enter')search();});
})();
