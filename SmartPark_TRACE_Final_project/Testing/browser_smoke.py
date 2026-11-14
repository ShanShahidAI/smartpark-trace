"""Isolated frontend smoke test with intercepted JSON API. No Flask server required.
Run: python Testing/browser_smoke.py  (requires Playwright + Chromium)
This validates DOM, browser JS and form wiring, not live Flask authentication.
"""
import asyncio
from pathlib import Path
import json
import re
import tempfile
from playwright.async_api import async_playwright

ROOT = Path(__file__).resolve().parents[1]
HTML = ROOT / 'FullStack_Web' / 'frontend' / 'index.html'
PREVIEWS = ROOT / 'Previews'
PREVIEWS.mkdir(exist_ok=True)

async def run():
    src=HTML.read_text(encoding='utf8')
    folder=ROOT / 'FullStack_Web' / 'frontend'
    src=src.replace('<link rel="stylesheet" href="/static/style.css">', '<style>'+ (folder/'style.css').read_text(encoding='utf8')+'</style>')
    for filename in ['icons.js','script.js','premium_nav.js','visualizers.js','phase6_lab.js','phase8_history.js','ccp.js']:
        src=src.replace('<script src="/static/'+filename+'"></script>',
                        '<script>'+ (folder/filename).read_text(encoding='utf8')+'</script>')
    src=src.replace('{{ csrf|tojson }}','"mock-csrf"').replace('{{ csrf }}','mock-csrf')
    with tempfile.NamedTemporaryFile('w', suffix='.html',delete=False) as out:
        out.write(src)
        page_file=Path(out.name)
    dummy='''window.fetch = async (url, opts={}) => {
      const u=String(url), method=(opts.method||'GET').toUpperCase();
      const payload = u.includes('find-my-car') ? {found:true, reachable:true, slot:'A-F1-S02',comparisons:2,edges:4,steps:'1. Pedestrian Gate → Entry\\n2. Entry → G-A\\n3. G-A → A-F1\\n4. A-F1 → A-F1-S02',message:'Found using AVL + BFS'}
      : u.includes('/api/ccp/state') ? {active:false,building:'',lastRecovery:'',holds:[],groups:[],evacuationRoutes:[]}
      : u.includes('/api/ccp/evacuation') ? {active:true, building:'B',evacuationRoutes:[],lastRecovery:'',holds:[],groups:[]}
      : u.includes('/api/ccp/convoy') ? {groupId:'CV-1',message:'Contiguous allocation',queued:false,split:false,slots:['A-F1-S02','A-F1-S03','A-F1-S04','A-F1-S05']}
      : u.includes('/api/ccp/vip-reserve') ? {message:'Hold created',slot:'A-F1-S02',route:'ENTRY -> G-A -> A-F1-S02'}
      : u.includes('/api/ccp/vip-confirm') ? {message:'VIP confirmed',slot:'A-F1-S02'}
      : u.includes('/api/vehicle-types') ? {types:['Car','SUV']}
      : u.includes('/api/rates') ? {rates:{Car:100,SUV:130}}
      : u.includes('/api/state') ? {totalSlots:36,occupied:0,available:36,waitingCount:0,revenue:0,served:0,waiting:[],history:[],slots:[], buildings:[{id:'A',total:12,occupied:0,available:12},{id:'B',total:12,occupied:0,available:12},{id:'C',total:12,occupied:0,available:12}],layout:{floorsPerBuilding:2,slotsPerFloor:6,buildingCount:3}}
      : u.includes('/api/health') ? {status:'ok',engine:'C++',treesValid:true}
      : u.includes('/api/graph') ? {roads:[],closedCount:0}
      : u.includes('/api/queue') ? {arrival:[],dispatch:[],counts:{emergency:0,vip:0,normal:0}}
      : u.includes('/api/dsa') ? {linkedList:[],queue:[],priorityQueue:[],doublyForward:[],doublyBackward:[],stack:[],metrics:{}}
      : u.includes('/api/trees') ? {bst:{nodes:[],root:null,height:0,inorder:[],preorder:[],postorder:[]},avl:{nodes:[],root:null,height:0,inorder:[],preorder:[],postorder:[]}}
      : u.includes('/api/history') ? {rows:[],summary:{sessions:0,departures:0,revenue:0}}
      : {};
      return {ok:true,status:200,json:async()=>payload};
    };'''
    async with async_playwright() as p:
        browser=await p.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--disable-dev-shm-usage','--no-sandbox'])
        results=[]
        for width,height,name in ((1440,920,'desktop_ccp.png'),(390,844,'mobile_ccp.png')):
            context=await browser.new_context(viewport={'width':width,'height':height})
            page=await context.new_page()
            issues=[]
            page.on('pageerror',lambda err:issues.append(str(err)))
            await page.add_init_script(dummy)
            await page.set_content(src,wait_until='domcontentloaded')
            await page.evaluate(dummy)
            await page.wait_for_timeout(1700)
            if width<500:
                await page.locator('#phase9MenuBtn').click(timeout=4500)
            await page.locator('[data-page="ccp"]').first.click(timeout=4500)
            await page.wait_for_timeout(500)
            assert await page.locator('#ccp').is_visible()
            await page.locator('#ccpFindPlate').fill('ABC-123')
            await page.locator('#findMyCarForm button[type=submit]').click()
            await page.wait_for_timeout(400)
            print('FIND TEXT', (await page.locator('#ccpFindResult').inner_text())[:300], 'ERRORS', issues[:4])
            assert 'A-F1-S02' in (await page.locator('#ccpFindResult').inner_text())
            if width<500:
                doc_width=await page.evaluate('document.documentElement.scrollWidth')
                assert doc_width<=width+2, f'horizontal overflow: {doc_width} vs {width}'
            await page.screenshot(path=str(PREVIEWS/name),full_page=False,animations='disabled',timeout=7000)
            results.append((name,issues))
            await context.close()
        await browser.close()
    page_file.unlink(missing_ok=True)
    for name,issues in results:
        print(name,'JavaScript errors:',len(issues),issues[:2])
    assert not any(issues for _,issues in results), 'browser JS errors'
    print('Browser smoke checks passed.')

if __name__=='__main__':asyncio.run(run())
