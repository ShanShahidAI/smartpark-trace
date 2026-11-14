# SmartPark TRACE — Explainable Parking Management

**Data Structures & Algorithms (DSA) semester project**  
**University:** Riphah International University, Islamabad  
**Instructor:** Mueed Ahmad  
**Team:** Muhammad Salman (72373), Shaheryar Khan (72775), Shan (72387)

SmartPark TRACE includes **two independent implementations** of a parking allocation system:

1. A standalone, menu-driven **C++17** application.
2. A separate **Flask + C++ web application**, with a frontend dashboard and SQLite-backed reporting.

They implement related features but **do not share live parking data** between versions.

## Repository map

| Path | Contents |
| --- | --- |
| [Standalone_CPP/SmartPark.cpp](Standalone_CPP/SmartPark.cpp) | Standalone C++17 console application with administrator login |
| [FullStack_Web/](FullStack_Web/) | Website, Flask API, C++ bridge, frontend, compiler and launch scripts |
| [Documentation/](Documentation/) | Approved proposal, updated CCP proposal, full report, slides, AI-usage report, implementation guide |
| [Testing/](Testing/) | Automated C++ bridge regression tests and browser smoke-test scripts |
| [Previews/](Previews/) | Desktop and mobile preview screenshots |
| [TEST_REPORT.md](TEST_REPORT.md) | Historical verification results supplied with the project |
| [run_cpp.bat](run_cpp.bat) | Compile and run independent Windows C++ console version |
| [setup_website.bat](setup_website.bat) | Prepare Python environment for web version on Windows |
| [run_website.bat](run_website.bat) | Compile bridge and launch web app on Windows |

## Quick start — Windows

**Requirements:** Python 3 and a C++17-capable compiler (`g++` or Visual Studio C++ Build Tools).

### Standalone C++

Run `run_cpp.bat` from the repository root. It compiles `Standalone_CPP/SmartPark.cpp` and starts the menu-driven console application.

### Web application

1. Run `setup_website.bat` (creates a local `.venv` and installs Flask requirements).
2. Run `run_website.bat` (builds the C++ bridge, then starts the Flask server).
3. Open **http://127.0.0.1:5000**.

Alternatively, follow the [web-specific guide](FullStack_Web/README.md).

## Implemented academic concepts

- Arrays and linked structures: singly, doubly and circular linked lists
- Stack, vehicle queue and waiting-priority policies
- BST and AVL indexing and search demonstrations
- Graph traversal (BFS) and road-blocked/evacuation route logic
- Searching and sorting, scheduling and allocation rules
- Parking slot and vehicle compatibility, billing and historical reporting
- Complex Computing Problem extension: VIP/ambulance-related constraints, plate search, convoy groups and rollback logic

## Reports, tests and limitations

The supplied [TEST_REPORT.md](TEST_REPORT.md) documents earlier Linux C++ compilation and regression tests, while noting outstanding Windows and real Flask workflow checks. **This repository reorganization moved files using their original Git blobs and did not rerun build, browser or integration tests.**

Do not use this course demonstration as a certified physical traffic management or emergency response system.

## File preservation

The manually uploaded repository had an outer `SmartPark_TRACE_Final_project/` wrapper. That wrapper was removed so the project's original relative Windows batch paths now resolve from the **repository root**. Subfolders and actual C++/Python/JS/HTML source have not been rewritten.

- The uploaded original project README is preserved in [Documentation/ORIGINAL_PROJECT_README.md](Documentation/ORIGINAL_PROJECT_README.md).
- The uploaded original web README is preserved in [FullStack_Web/ORIGINAL_README.md](FullStack_Web/ORIGINAL_README.md).
- Every uploaded source/document file is recoverable on `backup/before-dsa-projects-reorganization-2026-11-14`.
- [Original file mapping](Documentation/SOURCE_FILE_MAP.csv) records original paths and blob checksums.

## Related course repositories

[DSA Labs and Assignments](https://github.com/ShanShahidAI/Shan_72387_Data-Structures) · [Library Management Mini Project](https://github.com/ShanShahidAI/Library_Management_System_Mini_Project-DSA)
