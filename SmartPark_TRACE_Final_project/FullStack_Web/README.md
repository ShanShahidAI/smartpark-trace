# SmartPark TRACE — DSA + Complex Computing

**Riphah International University, Islamabad** · **Instructor: Sir Mueed Ahmad**  
**Members:** Muhammad Salman (72373), Shaheryar Khan (72775), Shan (72387)

Two independent deliverables (no shared runtime parking data):

- `Standalone_CPP/SmartPark.cpp`: one complete C++17 source file, menu-based admin login; **Menu 20: Smart Safety & Groups**. The source contains the raw CCP data structures inline; Windows launch `run_cpp.bat`.
- `FullStack_Web/`: separate Flask admin website, C++ bridge, SQLite reporting, premium frontend and **Smart Safety & Groups** page. Run `setup_website.bat` and `run_website.bat` on Windows, then visit `http://127.0.0.1:5000`.

The new C++ functionality includes plate-AVL walking search, raw BFS evacuation routing, VIP provisional reservation + rollback, 3/4-car convoy sliding windows, linked group queue and emergency admission lock. Existing 1–9 phase functionality remains included (slot rules, building/floor plans, roads, queue, billing, history, rates, admin login).

Files included:
- `Documentation/CCP_IMPLEMENTATION_GUIDE.md` — algorithms, method signatures, persistence, limitations, complexity, and viva notes.
- `Documentation/Complex_Computing_Proposal_Paragraph.txt` — copyable CCP proposal paragraph.
- `Testing/run_ccp_tests.py` — automated C++ bridge regression checks.

**Test execution (optional development tooling):** `python Testing/run_ccp_tests.py --bridge /path/to/smartpark_bridge`

**Important:** The new CCP algorithms use raw arrays/pointers; some older approved DSA/reporting modules retain STL. This update is *not* a full STL removal from the pre-existing project. A software evacuation graph does not replace physical safety procedures.
