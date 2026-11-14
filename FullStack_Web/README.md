# SmartPark TRACE — Full-Stack Web Version

This directory contains the **separate website implementation** of SmartPark TRACE. Its frontend talks to a Flask server, which uses the compiled C++ bridge.

## Structure

- `backend/` — C++ data structures, models, allocation engine, and bridge
- `server/` — Flask API, admin authentication and history/database logic
- `frontend/` — dashboard interface and visualizations
- `vendor/` — third-party licensing notices
- `requirements.txt` — Python requirements
- `compile_bridge.bat`, `setup_windows.bat`, `run_smartpark.bat` — Windows build, setup and launch scripts

## Windows setup

From the **repository root**, first run `setup_website.bat`, then `run_website.bat`. These scripts change into `FullStack_Web/` and execute the corresponding scripts here.

Or, within this directory, run `setup_windows.bat` once, followed by `run_smartpark.bat`. Python 3 and a working C++17 compiler are required.

Open **http://127.0.0.1:5000** after the Flask server starts.

## Important notes

- This is not the same running process or storage as the standalone C++ version.
- Runtime admin credentials, SQLite databases, session secrets and generated executables should be local, not committed to GitHub.
- Previous project test results are documented in [../TEST_REPORT.md](../TEST_REPORT.md); **these tests have not been rerun during folder reorganization**.
- [Original uploaded web README](ORIGINAL_README.md) retained for historical reference.

[Repository overview](../README.md) · [Documentation](../Documentation/)
