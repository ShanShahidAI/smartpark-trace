/* interactive BST/AVL visualizers driven by real C++ snapshots. */
(() => {
  'use strict';
  const el = id => document.getElementById(id);
  const escapeText = value => String(value ?? '').replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const current = {mode:'live', type:'avl', live:null, practice:null, frames:[], frame:0,
    message:'', rotations:[], lastEventSignature:'', highlight:[], focus:null, busy:false};

  async function getJSON(url, options) {
    const opts = { ...(options || {}) };
  if (opts.method && !['GET','HEAD'].includes(String(opts.method).toUpperCase())) {
    opts.headers = {...(opts.headers || {}), 'X-CSRF-Token': window.SMARTPARK_CSRF || ''};
  }
  const response = await fetch(url, opts);
    let payload;
    try { payload = await response.json(); } catch (_) { throw Error('Server returned invalid JSON.'); }
    if (!response.ok) throw Error(payload.error || 'Server operation failed.');
    return payload;
  }
  const currentTrees = () => current.mode === 'live' ? current.live : current.practice;
  const activeTree = () => currentTrees()?.[current.type] || null;
  const showMessage = message => { current.message = message; el('treeOperationLog').textContent = message; };
  const isTreesOpen = () => el('trees')?.classList.contains('active');

  async function loadTrees(force = false) {
    if (!force && !isTreesOpen()) return;
    try {
      const info = await getJSON('/api/trees');
      current.live = info;
      const event = info.lastEvent;
      // Refresh should not erase the step the student is currently reviewing.
      if (current.mode === 'live' && event) {
        const fingerprint = JSON.stringify(event);
        if (fingerprint !== current.lastEventSignature) {
          current.lastEventSignature = fingerprint;
          const label = event.action === 'park' ? 'Vehicle parked' : 'Vehicle exited';
          setFrames(event.before?.[current.type], event.after?.[current.type],
                    label, event.rotations || [], null);
          return;
        }
      }
      if (!event && current.lastEventSignature) {
        current.lastEventSignature = '';
        current.frames = [];
        current.rotations = [];
        current.message = 'Live parking data refreshed.';
      }
      if (!current.frames.length || force) {
        current.frames = [];
        current.highlight = [];
        current.focus = null;
      }
      draw();
    } catch (error) {
      if (current.mode === 'live') showMessage(error.message);
    }
  }
  async function loadPractice() {
    const res = await getJSON('/api/tree/practice');
    current.practice = res.trees;
    draw();
  }

  function derivePath(tree, key) {
    if (!tree || tree.root == null) return [];
    const index = new Map(tree.nodes.map(n => [n.key, n]));
    const result = [];
    let node = index.get(tree.root);
    while (node && result.length <= tree.nodes.length) {
      result.push(node.key);
      if (node.key === key) break;
      node = index.get(key < node.key ? node.left : node.right);
    }
    return result;
  }
  function setFrames(before, after, action, rotations = [], key = null) {
    const frames = [];
    const path = key != null ? derivePath(before, key) : [];
    if (before) {
      if (path.length > 0) {
        path.forEach((_, index) => frames.push({tree:before, path:path.slice(0,index+1),
          label:`Step ${index+1}: Compare with node ${path[index]}`, focus:path[index]}));
      } else frames.push({tree:before, path:[], label:'Before operation', focus:null});
    }
    if (after) frames.push({tree:after, path:[],label:'After operation',focus:key});
    current.frames = frames;
    current.frame = 0;
    current.rotations = rotations;
    current.message = action;
    current.highlight = [];
    current.focus = key;
    draw();
  }
  function currentFrame() { return current.frames.length ? current.frames[current.frame] : null; }
  function selectedTree() { return currentFrame()?.tree || activeTree(); }
  function renderGraph(tree, path = [], focus = null) {
    if (!tree || !tree.nodes?.length) return `<div class="tree-empty"><div><i class="fa-solid fa-tree" style="font-size:36px;color:#75689b"></i><h3>This tree is empty</h3><p>Park a vehicle in Live Mode, or insert values in Practice Mode.</p></div></div>`;
    const lookup = new Map(tree.nodes.map(node => [node.key, node]));
    const coords = new Map();
    let visited = 0;
    function place(key, depth) {
      const node = lookup.get(key);
      if (!node || coords.has(key) || depth > 150) return;
      if (node.left !== null) place(node.left, depth+1);
      coords.set(key, {x:88 + visited++*104, y:70 + depth*123, depth});
      if (node.right !== null) place(node.right, depth+1);
    }
    place(tree.root, 0);
    const width = Math.max(700, (visited+1)*104);
    const height = Math.max(395, ...[...coords.values()].map(n => n.y+70));
    const edges = [], circles=[];
    for (const node of tree.nodes) {
      const pos = coords.get(node.key);
      if (!pos) continue;
      for (const childKey of [node.left,node.right]) {
        const child=coords.get(childKey);
        if(child) edges.push(`<path class="tree-edge" d="M${pos.x} ${pos.y+26} L${child.x} ${child.y-26}"/>`);
      }
      const klass = ['tree-node',node.key === tree.root ? 'root' : '',path.includes(node.key) ? 'on-path' : '',node.key === focus ? 'focus' : ''].filter(Boolean).join(' ');
      circles.push(`<g class="${klass}" transform="translate(${pos.x} ${pos.y})">
        <title>${escapeText(`Key ${node.key}; Height ${node.height}; Balance ${node.bf}${node.vehicleNumber ? '; Vehicle '+node.vehicleNumber : ''}`)}</title>
        <circle r="27"></circle><text class="node-value" text-anchor="middle" y="5" font-size="${String(node.key).length > 6 ? 11 : 14}">${node.key}</text>
        <text class="node-meta" text-anchor="middle" y="44">h:${node.height} · BF:${node.bf > 0 ? '+' : ''}${node.bf}</text>
        ${node.vehicleNumber ? `<text class="node-plate" text-anchor="middle" y="57">${escapeText(node.vehicleNumber.slice(0,18))}</text>` : ''}
      </g>`);
    }
    return `<svg xmlns="http://www.w3.org/2000/svg" role="img" aria-label="${escapeText(current.type.toUpperCase())} tree with ${tree.nodeCount} nodes" width="${width}" height="${height}" viewBox="0 0 ${width} ${height}"><g>${edges.join('')}</g><g>${circles.join('')}</g></svg>`;
  }
  function draw() {
    const tree = selectedTree();
    const frame = currentFrame();
    el('treeTitle').textContent = `${current.mode === 'live' ? 'Live' : 'Practice'} ${current.type.toUpperCase()} Tree`;
    el('treeCaption').textContent = current.mode === 'live'
      ? 'Actual C++ indexes of parked vehicle ticket IDs' : 'Independent C++ practice tree — no real parking records affected';
    el('treeNodes').textContent = tree?.nodeCount ?? 0;
    el('treeHeight').textContent = tree?.height ?? 0;
    el('treeRoot').textContent = tree?.root ?? '—';
    el('treeLeaves').textContent = tree?.leafCount ?? 0;
    el('treeBalance').textContent = current.type==='avl' ? (tree?.valid ? 'Balanced' : 'Check') : 'BST';
    el('treeViewport').innerHTML = renderGraph(tree, frame?.path || current.highlight, frame ? frame.focus : current.focus);
    for (const kind of ['inorder','preorder','postorder']) el('tree'+kind[0].toUpperCase()+kind.slice(1)).textContent = tree?.[kind]?.join(' → ') || '—';
    for (const [id, value] of [['treeModeLive',current.mode==='live'],['treeModePractice',current.mode==='practice'],['treeTypeAvl',current.type==='avl'],['treeTypeBst',current.type==='bst']]) el(id).classList.toggle('selected',value);
    const practice=current.mode==='practice';
    for(const id of ['treeInsert','treeDelete','treeClear','treeDemoLL','treeDemoRR','treeDemoLR','treeDemoRL'])el(id).disabled=!practice||current.busy;
    el('treeSearch').disabled=current.busy;
    el('treeActionTip').textContent = practice ? 'Insert, delete and search any numeric keys using real C++ tree functions.' : 'Use a ticket number from a parked vehicle (e.g. SP-00000001 → enter 1).';
    el('treePrevious').disabled = !current.frames.length || current.frame === 0;
    el('treeNext').disabled = !current.frames.length || current.frame === current.frames.length-1;
    el('treeFrameTitle').textContent = frame ? `${current.frame+1}/${current.frames.length} · ${frame.label}` : 'Current C++ tree';
    const log=el('treeOperationLog');
    log.innerHTML = `<div>${escapeText(current.message || (practice ? 'Use Insert to add a practice node.' : 'Park a vehicle to populate the live tree.'))}</div>`+
      (current.rotations || []).map(v => `<span class="rotation"><i class="fa-solid fa-rotate"></i> ${escapeText(v)}</span>`).join('') +
      (tree ? `<small style="color:#a3bbce">${tree.valid ? '✓ Structure verified by C++' : '⚠ Structure verification failed'}</small>` : '');
  }
  async function setMode(mode) {
    if (current.busy || current.mode===mode) return;
    current.mode=mode; current.frames=[]; current.highlight=[];current.focus=null;current.message='';current.rotations=[];
    if(mode==='practice'&&!current.practice){try{await loadPractice();}catch(e){showMessage(e.message);}}
    else if(mode==='live') await loadTrees(true);
    draw();
  }
  function setType(type) {
    if(current.busy)return;
    current.type=type;current.frames=[];current.highlight=[];current.focus=null;current.rotations=[];
    current.message = `${type.toUpperCase()} tree selected.`;
    draw();
  }
  function parseKey() {
    const input=el('treeKeyInput');
    const key=Number(input.value);
    if(!/^[0-9]+$/.test(input.value.trim()) || !Number.isSafeInteger(key) || key < 1 || key > 99999999)throw Error('Enter a whole number from 1 to 99999999.');
    return key;
  }
  async function action(verb, key) {
    if(current.busy)return;
    if(current.mode==='live' && ['insert','delete','clear'].includes(verb))return;
    current.busy=true;draw();
    try {
      if(current.mode==='live') {
        const tree = activeTree();
        const path=derivePath(tree,key);
        current.frames=path.map((node,i)=>({tree,path:path.slice(0,i+1),label:`Compare ${key} with ${node}`,focus:node}));
        if (!current.frames.length) current.frames=[{tree,path:[],label:'Empty tree',focus:null}];
        current.frame=0;
        current.rotations=[];
        current.message=tree?.nodes?.some(n=>n.key===key) ? `Ticket ${key} found in live ${current.type.toUpperCase()}.` : `Ticket ${key} not found.`;
      } else {
        const res=await getJSON('/api/tree/practice',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({tree:current.type,action:verb,key})});
        current.practice=res.trees;
        const oldTree=res.before?.[current.type], newTree=res.trees?.[current.type];
        const msg = verb === 'search' ? (res.found?`Found ${key}.`:`${key} not found.`) : verb === 'clear' ? 'Practice tree cleared.' : res.changed ? `${verb==='insert'?'Inserted':'Deleted'} ${key} in ${current.type.toUpperCase()}.` : `No change — ${key} ${verb==='insert'?'already exists':'was not found'}.`;
        setFrames(oldTree,newTree,msg,res.rotations||[],key??null);
        if(verb==='search') {const path=derivePath(newTree,key); current.frames=path.map((node,i)=>({tree:newTree,path:path.slice(0,i+1),label:`Compare ${key} with ${node}`,focus:node}));current.frame=0;draw();}
      }
    }catch(error){current.frames=[];current.rotations=[];current.message=error.message;}
    finally{current.busy=false;draw();}
  }
  async function rotationDemo(name) {
    if(current.busy)return;
    const examples={LL:[30,20,10],RR:[10,20,30],LR:[30,10,20],RL:[10,30,20]};
    current.busy=true;current.mode='practice';current.type='avl';draw();
    try{
      await getJSON('/api/tree/practice',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({tree:'avl',action:'clear'})});
      const frames=[];let finalRes=null;
      for(const [i,key] of examples[name].entries()){
        const res=await getJSON('/api/tree/practice',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({tree:'avl',action:'insert',key})});
        if(i===0)frames.push({tree:res.before.avl,path:[],label:'Empty AVL tree',focus:null});
        frames.push({tree:res.trees.avl,path:[],label:`Insert ${key}${res.rotations.length?' · '+name+' rotation':''}`,focus:key});
        finalRes=res;
      }
      current.practice=finalRes.trees;current.frames=frames;current.frame=0;
      current.rotations=finalRes.rotations;
      current.message=`${name} example: ${examples[name].join(' → ')}. Use Next to follow each insertion and rotation.`;
    }catch(error){current.message=error.message;current.frames=[];}
    finally{current.busy=false;draw();}
  }

  // Algorithm practice visualization (only educational steps run in JS).
  const sort={steps:[],index:0};
  function buildSort(){
    const text=el('sortValues').value;
    const arr=text.split(',').map(s=>s.trim());
    if(arr.length<2||arr.length>12||arr.some(s=>!/^\d{1,3}$/.test(s))) {
      el('sortStepLabel').textContent='Enter 2–12 integers between 0 and 999.';return;
    }
    const a=arr.map(Number),steps=[{values:[...a],compare:[],sorted:[],label:'Initial values'}];
    const mode=el('sortAlgorithm').value;
    if(mode==='bubble') {
      for(let i=0;i<a.length-1;i++) {
        for(let j=0;j<a.length-1-i;j++){
          steps.push({values:[...a],compare:[j,j+1],sorted:a.map((_,k)=>k).filter(k=>k>=a.length-i),label:`Compare ${a[j]} and ${a[j+1]}`});
          if(a[j]>a[j+1]){[a[j],a[j+1]]=[a[j+1],a[j]];steps.push({values:[...a],compare:[j,j+1],sorted:[],label:`Swap positions ${j+1} and ${j+2}`});}
        }
      }
    } else {
      for(let i=0;i<a.length-1;i++){
        let min=i;
        for(let j=i+1;j<a.length;j++){
          steps.push({values:[...a],compare:[min,j],sorted:a.map((_,k)=>k).filter(k=>k<i),label:`Compare ${a[min]} and ${a[j]}`});
          if(a[j]<a[min])min=j;
        }
        if(min!==i){[a[i],a[min]]=[a[min],a[i]];steps.push({values:[...a],compare:[i,min],sorted:[],label:`Place minimum at position ${i+1}`});}
      }
    }
    steps.push({values:[...a],compare:[],sorted:a.map((_,k)=>k),label:'Sorting complete ✓'});
    sort.steps=steps;sort.index=0;drawSort();
  }
  function drawSort(){
    const s=sort.steps[sort.index];if(!s)return;
    const max=Math.max(...s.values,1);
    el('sortBars').innerHTML=s.values.map((v,i)=>`<div class="sort-bar ${s.compare.includes(i)?'compare':''} ${s.sorted.includes(i)?'sorted':''}" style="height:${Math.max(40,Math.round(v/max*145))}px">${v}</div>`).join('');
    el('sortStepLabel').textContent=`${sort.index+1}/${sort.steps.length} · ${s.label}`;
    el('sortPrev').disabled=sort.index===0;el('sortNext').disabled=sort.index===sort.steps.length-1;
  }
  document.addEventListener('DOMContentLoaded',()=>{
    for(const b of document.querySelectorAll('[data-page="trees"]'))b.addEventListener('click',()=>loadTrees());
    el('treeModeLive').addEventListener('click',()=>setMode('live'));
    el('treeModePractice').addEventListener('click',()=>setMode('practice'));
    el('treeTypeAvl').addEventListener('click',()=>setType('avl'));
    el('treeTypeBst').addEventListener('click',()=>setType('bst'));
    el('treeRefresh').addEventListener('click',()=>current.mode==='live'?loadTrees(true):loadPractice().catch(e=>showMessage(e.message)));
    el('treeInsert').addEventListener('click',()=>{try{action('insert',parseKey());}catch(e){showMessage(e.message);}});
    el('treeDelete').addEventListener('click',()=>{try{action('delete',parseKey());}catch(e){showMessage(e.message);}});
    el('treeSearch').addEventListener('click',()=>{try{action('search',parseKey());}catch(e){showMessage(e.message);}});
    el('treeClear').addEventListener('click',()=>{if(confirm('Clear the selected practice tree?'))action('clear');});
    for(const kind of ['LL','RR','LR','RL'])el('treeDemo'+kind).addEventListener('click',()=>rotationDemo(kind));
    el('treeKeyInput').addEventListener('keydown',e=>{if(e.key==='Enter'){e.preventDefault();try{action('search',parseKey());}catch(err){showMessage(err.message);}}});
    el('treePrevious').addEventListener('click',()=>{if(current.frame>0){current.frame--;draw();}});
    el('treeNext').addEventListener('click',()=>{if(current.frame+1<current.frames.length){current.frame++;draw();}});
    el('sortBuild').addEventListener('click',buildSort);
    el('sortValues').addEventListener('keydown',e=>{if(e.key==='Enter'){e.preventDefault();buildSort();}});
    el('sortAlgorithm').addEventListener('change',buildSort);
    el('sortPrev').addEventListener('click',()=>{sort.index=Math.max(0,sort.index-1);drawSort();});
    el('sortNext').addEventListener('click',()=>{sort.index=Math.min(sort.steps.length-1,sort.index+1);drawSort();});
    buildSort();draw();
    setInterval(()=>{if(current.mode==='live' && isTreesOpen())loadTrees();},12000);
  });
})();
