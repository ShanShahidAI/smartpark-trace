# SmartPark TRACE — One-Phase CCP Implementation QA

**Institution:** Riphah International University, Islamabad  
**Instructor:** Sir Mueed Ahmad  
**Team:** Muhammad Salman (72373), Shaheryar Khan (72775), Shan (72387)

## Confirmed checks

- Single-source standalone C++ compiled with Linux `g++ -std=c++17`.
- Website C++ bridge (17 C++ implementation files) compiled with Linux `g++ -std=c++17`.
- **18/18 automated C++ CCP integration tests passed** with newly compiled bridge.
- New raw structures: AVL plate search, raw BFS, route stack, rollback stack, group linked queue and fixed-array sliding window.
- VIP reservation / road-block rollback / fallback queue and persistence tested.
- Convoy contiguous 3/4, split 4 into 2+2, waiting group and restart handling tested.
- Evacuation flag, admission lock, blocked edges, no-route handling and restart state tested.
- Existing vehicle exit/billing and older V5 migration regression included.
- `ccp.js` JavaScript syntax, Flask Python source compilation passed.
- **Desktop & mobile isolated Chromium checks passed**, zero JS runtime errors and no mobile overflow for the new CCP page.
- Standalone C++ first-time admin login and menu-20 smoke test passed on Linux.
- Updated Word proposal, PDF report and PPTX presentation rendered and visually reviewed.

## Remaining manual verification

- Windows MSVC (`cl`) or Windows `g++` builds and `.bat` startup (not executable in this Linux environment).
- Real authenticated Flask browser workflow (Flask not installed in this execution environment); isolated UI tests use mocked API responses.
- Physical emergency safety: simulated road graph with staff-entered closures; not a certified emergency guidance system.

## Developer verification

`python Testing/run_ccp_tests.py --bridge <compiled web_bridge executable>`  
`python Testing/browser_smoke.py`

**Core DSA rule:** New CCP DSAs use raw arrays / pointers and linked nodes. Previously developed approved modules retain some STL adapters/containers.
