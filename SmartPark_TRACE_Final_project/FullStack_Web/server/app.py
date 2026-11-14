"""SmartPark TRACE — Flask HTTP adapter for the real C++ DSA engine.

The C++ ParkingSystem handles entry/exit, waiting priority, persistence, BST and AVL.
Flask only validates HTTP input, calls the C++ bridge and returns JSON to the UI.
"""
from pathlib import Path
from threading import RLock
import shlex
import tempfile
import json
import os
import re
import socket
import subprocess

from flask import Flask, jsonify, render_template, request
from parking_core import RATES, VEHICLE_TYPES  # Static fee table, tested against C++.
from auth import install_auth
from history_db import HistoryDatabase, HistoryError

ROOT = Path(__file__).resolve().parent.parent
DATA = Path(os.environ.get("SMARTPARK_DATA_DIR", str(ROOT / "data"))).resolve()
DATA.mkdir(parents=True, exist_ok=True)
ENGINE = Path(os.environ.get("SMARTPARK_CPP_BRIDGE", str(ROOT / "build" / ("smartpark_bridge.exe" if os.name == "nt" else "smartpark_bridge"))))
WEB_STATE = DATA / "web_cpp_state.txt"
PRACTICE_STATE = DATA / "web_tree_practice.txt"
LOCK = RLock()
last_event = None
HISTORY = HistoryDatabase(DATA / "smartpark_history.sqlite3")
RATE_FILE = DATA / "parking_rates.txt"
os.environ["SMARTPARK_RATE_FILE"] = str(RATE_FILE)

app = Flask(__name__, template_folder=str(ROOT / "frontend"),
            static_folder=str(ROOT / "frontend"), static_url_path="/static")
app.json.ensure_ascii = False
app.config["MAX_CONTENT_LENGTH"] = 8 * 1024
install_auth(app, DATA)


class BridgeError(Exception):
    def __init__(self, message, status=400):
        super().__init__(message)
        self.status = status


def invoke(command, *args):
    """Run a single C++ operation with no shell interpolation."""
    if not ENGINE.is_file():
        raise BridgeError("C++ engine not compiled. Run compile_bridge.bat, then restart SmartPark.", 503)
    try:
        result = subprocess.run([str(ENGINE), str(WEB_STATE), command, *map(str, args)],
                                cwd=ROOT, capture_output=True, text=True,
                                encoding="utf-8", errors="replace", timeout=12,
                                check=False)
    except (OSError, subprocess.TimeoutExpired) as exc:
        raise BridgeError("Cannot start the C++ engine. Check the build and terminal logs.", 503) from exc
    try:
        payload = json.loads(result.stdout)
    except ValueError as exc:
        raise BridgeError("C++ engine returned an invalid response. See terminal logs.", 502) from exc
    if result.returncode != 0:
        message = payload.get("error", "C++ operation failed")
        status = 404 if "not found" in message.lower() else 400
        raise BridgeError(message, status)
    return payload



def mutation(command, *args):
    result = invoke(command, *args)
    HISTORY.sync(invoke("state"))
    if command in ("queue-cancel", "queue-vip", "road-block", "slot-rule", "configure"):
        HISTORY.log(command.upper().replace("-", "_"), " ".join(map(str, args)))
    return result

@app.errorhandler(HistoryError)
def handle_history_error(error):
    return jsonify(error=str(error)), 400

@app.errorhandler(BridgeError)
def handle_bridge_error(error):
    return jsonify(error=str(error)), error.status


@app.errorhandler(413)
def handle_big_request(_error):
    return jsonify(error="Request is too large."), 413


@app.get("/")
def index():
    from flask import session
    return render_template("index.html", admin_username=session["admin"], csrf=session["csrf_token"])


@app.get("/api/health")
def health():
    with LOCK:
        return jsonify(invoke("health") | {"version": "1.0", "webState": "C++"})


@app.get("/api/state")
def state():
    with LOCK:
        snapshot = invoke("state")
        HISTORY.sync(snapshot)
        return jsonify(snapshot)


def current_rates():
    amounts = {name: int(costs[0]) for name, costs in RATES.items()}
    if RATE_FILE.is_file():
        for line in RATE_FILE.read_text(encoding="utf-8").splitlines():
            try:
                parts = shlex.split(line)
                if len(parts) == 2 and parts[0] in amounts:
                    price = int(parts[1])
                    if 1 <= price <= 1000000:
                        amounts[parts[0]] = price
            except (ValueError, UnicodeError):
                continue
    return amounts


@app.get("/api/vehicle-types")
def vehicle_types():
    with LOCK:
        amounts = current_rates()
        return jsonify(types=VEHICLE_TYPES, rates={
            kind: {"hourly": amounts[kind], "first": amounts[kind], "additional": amounts[kind]}
            for kind in VEHICLE_TYPES
        })


@app.post("/api/rates")
def update_rate():
    payload = request.get_json(silent=True)
    if not isinstance(payload, dict) or payload.get("vehicleType") not in VEHICLE_TYPES or type(payload.get("hourly")) is not int:
        raise BridgeError("Choose a valid vehicle type and whole-number hourly rate.")
    rate = payload["hourly"]
    if not 1 <= rate <= 1000000:
        raise BridgeError("Hourly rate must be between Rs. 1 and Rs. 1,000,000.")
    with LOCK:
        amounts = current_rates()
        amounts[payload["vehicleType"]] = rate
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", dir=DATA, prefix="rates_", suffix=".tmp", delete=False) as handle:
            temporary = Path(handle.name)
            for kind in VEHICLE_TYPES:
                handle.write(json.dumps(kind, ensure_ascii=False) + " " + str(amounts[kind]) + "\n")
            handle.flush()
            os.fsync(handle.fileno())
        try:
            os.replace(temporary, RATE_FILE)
        finally:
            temporary.unlink(missing_ok=True)
        HISTORY.log("RATE_CHANGED", f"{payload['vehicleType']}: Rs. {rate} per started hour")
        return jsonify(message="Hourly rate saved.", vehicleType=payload["vehicleType"], hourly=rate)


@app.post("/api/park")
def park():
    global last_event
    p = request.get_json(silent=True)
    if not isinstance(p, dict):
        raise BridgeError("Please provide valid vehicle details.")
    plate = str(p.get("vehicleNumber") or "").strip()
    owner = str(p.get("ownerName") or "").strip()
    kind = str(p.get("vehicleType") or "")
    vip = p.get("priority") is True
    building = str(p.get("preferredBuilding") or "").strip().upper()
    floor = p.get("preferredFloor", 0)
    if building not in ("", "A", "B", "C") or type(floor) is not int or not 0 <= floor <= 4:
        raise BridgeError("Choose a valid building and floor.")
    if len(plate) > 32 or len(owner) > 100 or kind not in VEHICLE_TYPES:
        raise BridgeError("Invalid vehicle number, owner, or type.")
    with LOCK:
        result = invoke("park", plate, kind, owner, "1" if vip else "0", building, str(floor))
        last_event = result.pop("treeEvent", None)
        HISTORY.sync(invoke("state"))
        return jsonify(result)


@app.post("/api/exit")
def exit_vehicle():
    global last_event
    p = request.get_json(silent=True)
    if not isinstance(p, dict):
        raise BridgeError("Please provide valid exit details.")
    plate = str(p.get("vehicleNumber") or "").strip()
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9 -]{0,18}[A-Za-z0-9]", plate):
        raise BridgeError("Enter a valid parked vehicle number.")
    # No client-provided hours/timestamps: C++ takes the trusted server clock.
    with LOCK:
        result = invoke("exit", plate)
        last_event = result.pop("treeEvent", None)
        HISTORY.sync(invoke("state"))
        return jsonify(result)


@app.get("/api/search")
def search():
    query = request.args.get("q", "")[:80]
    with LOCK:
        return jsonify(invoke("search", query))


@app.get("/api/dsa")
def dsa():
    with LOCK:
        return jsonify(invoke("dsa"))


@app.get("/api/queue")
def queue_state():
    with LOCK:
        return jsonify(invoke("queue-state"))


@app.post("/api/queue/cancel")
def queue_cancel():
    payload = request.get_json(silent=True)
    if not isinstance(payload, dict) or not isinstance(payload.get("vehicleNumber"), str):
        raise BridgeError("Provide a waiting vehicle number.")
    plate = payload["vehicleNumber"].strip().upper()
    if not re.fullmatch(r"[A-Z0-9][A-Z0-9 -]{0,18}[A-Z0-9]", plate):
        raise BridgeError("Invalid vehicle number.")
    with LOCK:
        return jsonify(mutation("queue-cancel", plate))


@app.post("/api/queue/priority")
def queue_priority():
    payload = request.get_json(silent=True)
    if not isinstance(payload, dict) or not isinstance(payload.get("vehicleNumber"), str) or type(payload.get("vip")) is not bool:
        raise BridgeError("Provide a waiting vehicle number and VIP true/false.")
    plate = payload["vehicleNumber"].strip().upper()
    if not re.fullmatch(r"[A-Z0-9][A-Z0-9 -]{0,18}[A-Z0-9]", plate):
        raise BridgeError("Invalid vehicle number.")
    with LOCK:
        return jsonify(mutation("queue-vip", plate, "1" if payload["vip"] else "0"))


# Read-only analyses computed by actual C++ nodes and live records.
@app.get("/api/algorithms/ticket")
def algorithm_ticket():
    key = request.args.get("key", "")
    if not re.fullmatch(r"[1-9][0-9]{0,7}", key):
        raise BridgeError("Enter a numeric ticket ID (1-99999999).")
    with LOCK:
        return jsonify(invoke("ticket-audit", key))


@app.get("/api/algorithms/sort")
def algorithm_sort():
    source = request.args.get("source", "active")
    field = request.args.get("field", "number")
    if source not in ("active", "history") or field not in ("number", "fee"):
        raise BridgeError("Choose active/history and number/fee.")
    with LOCK:
        return jsonify(invoke("sort-audit", source, field))


@app.get("/api/trees")
def trees():
    with LOCK:
        tree_data = invoke("trees")
        tree_data["lastEvent"] = last_event
        return jsonify(tree_data)


@app.route("/api/tree/practice", methods=["GET", "POST"])
def practice():
    if request.method == "GET":
        tree, action, key = "avl", "view", None
    else:
        p = request.get_json(silent=True)
        if not isinstance(p, dict):
            raise BridgeError("Please enter a valid practice operation.")
        tree, action, key = p.get("tree"), p.get("action"), p.get("key")
    if tree not in ("bst", "avl") or action not in ("view", "insert", "delete", "search", "clear"):
        raise BridgeError("Choose BST/AVL and a valid action.")
    params = [str(PRACTICE_STATE), tree, action]
    if action in ("insert", "delete", "search"):
        if type(key) is not int or not 1 <= key <= 99999999:
            raise BridgeError("Enter a whole number from 1 to 99999999.")
        params.append(str(key))
    with LOCK:
        return jsonify(invoke("practice", *params))


# Complex-computing UI: all pathfinding, AVL lookups, group decisions and
# admission locks are computed in C++; Flask performs validation/auth only.
@app.get("/api/find-my-car")
def find_my_car():
    plate = (request.args.get("plate") or "").strip().upper()
    if not re.fullmatch(r"[A-Z0-9][A-Z0-9 -]{0,18}[A-Z0-9]", plate):
        raise BridgeError("Enter a valid plate number.")
    with LOCK:
        return jsonify(invoke("find-my-car", plate))


@app.get("/api/ccp/state")
def ccp_state():
    with LOCK:
        return jsonify(invoke("ccp-state"))


@app.post("/api/ccp/evacuation")
def ccp_evacuation():
    body = request.get_json(silent=True)
    if not isinstance(body, dict) or type(body.get("active")) is not bool:
        raise BridgeError("Select evacuation on or off.")
    building = body.get("building")
    if body["active"] and building not in ("A", "B", "C"):
        raise BridgeError("Choose Building A, B or C.")
    with LOCK:
        return jsonify(mutation("evacuate", building) if body["active"] else mutation("evac-stop"))


@app.post("/api/ccp/vip-reserve")
def ccp_vip_reserve():
    body = request.get_json(silent=True)
    if not isinstance(body, dict):
        raise BridgeError("Provide VIP reservation fields.")
    plate = str(body.get("plate") or "").strip().upper()
    kind = body.get("type")
    owner = str(body.get("owner") or "").strip()
    building = body.get("building") or ""
    floor = body.get("floor")
    if not re.fullmatch(r"[A-Z0-9][A-Z0-9 -]{0,18}[A-Z0-9]", plate) or \
       kind not in VEHICLE_TYPES or not 1 <= len(owner) <= 70 or \
       building not in ("", "A", "B", "C") or type(floor) is not int or not 0 <= floor <= 4:
        raise BridgeError("Invalid reservation fields.")
    with LOCK:
        return jsonify(mutation("vip-reserve", plate, kind, owner, building, floor))


@app.post("/api/ccp/vip-confirm")
def ccp_vip_confirm():
    body = request.get_json(silent=True)
    plate = str(body.get("plate") or "").strip().upper() if isinstance(body,dict) else ""
    if not re.fullmatch(r"[A-Z0-9][A-Z0-9 -]{0,18}[A-Z0-9]", plate):
        raise BridgeError("Invalid plate.")
    with LOCK:
        return jsonify(mutation("vip-confirm", plate))


@app.post("/api/ccp/convoy")
def ccp_convoy():
    body = request.get_json(silent=True)
    if not isinstance(body, dict) or not isinstance(body.get("vehicles"), list):
        raise BridgeError("Provide convoy details.")
    vehicles=body["vehicles"]
    building=body.get("building") or ""
    floor=body.get("floor")
    vip=body.get("vip")
    if len(vehicles) not in (3,4) or building not in ("", "A", "B", "C") or \
       type(floor) is not int or not 0 <= floor <= 4 or type(vip) is not bool:
        raise BridgeError("Convoy requires 3–4 vehicles, a valid building, floor and priority.")
    args=[len(vehicles),building,floor,"1" if vip else "0"]
    plates=set()
    for vehicle in vehicles:
        if not isinstance(vehicle,dict):
            raise BridgeError("Invalid convoy record.")
        plate=str(vehicle.get("plate") or "").strip().upper()
        owner=str(vehicle.get("owner") or "").strip()
        kind=vehicle.get("type")
        if not re.fullmatch(r"[A-Z0-9][A-Z0-9 -]{0,18}[A-Z0-9]", plate) or \
           plate in plates or kind not in VEHICLE_TYPES or not 1 <= len(owner) <= 70:
            raise BridgeError("Invalid or duplicate convoy vehicle.")
        plates.add(plate)
        args.extend([plate,kind,owner])
    with LOCK:
        return jsonify(mutation("convoy", *args))


@app.get("/api/graph")
def graph_state():
    with LOCK:
        return jsonify(invoke("graph"))


@app.get("/api/route")
def route_to_slot():
    slot = (request.args.get("slot") or "").strip().upper()
    if not re.fullmatch(r"[ABC]-F[1-4]-S[0-9]{2}", slot):
        raise BridgeError("Choose a valid parking slot.")
    with LOCK:
        return jsonify(invoke("route", slot))


@app.post("/api/road-block")
def road_restriction():
    body = request.get_json(silent=True)
    if not isinstance(body, dict) or type(body.get("blocked")) is not bool:
        raise BridgeError("Provide road endpoints and blocked true/false.")
    from_node, to_node = str(body.get("from") or "").strip().upper(), str(body.get("to") or "").strip().upper()
    if len(from_node) > 18 or len(to_node) > 18 or not from_node or not to_node:
        raise BridgeError("Invalid road vertices.")
    with LOCK:
        return jsonify(mutation("road-block", from_node, to_node, "1" if body["blocked"] else "0"))


@app.post("/api/slot-rule")
def change_slot_rule():
    p = request.get_json(silent=True)
    if not isinstance(p, dict) or not isinstance(p.get("slotId"), str) or type(p.get("mode")) is not int:
        raise BridgeError("Supply slot ID and rule (normal / reserved / disputed).")
    slot_id = p["slotId"].strip()
    mode = p["mode"]
    note = str(p.get("note") or "").strip()
    if not re.fullmatch(r"[ABC]-F[1-4]-S[0-9]{2}",slot_id) or mode not in (0,1,2) or len(note)>90 or "\n" in note or "\r" in note:
        raise BridgeError("Invalid slot, rule or note.")
    with LOCK:
        result = mutation("slot-rule", slot_id, mode, note)
        found = next((s for s in result.get("slots", []) if s.get("id") == slot_id), None)
        if found is None or found.get("restriction") != mode:
            raise BridgeError("Parking rule could not be verified after saving.", 503)
        return jsonify(message="Slot rule saved.", slot=found, state=result)


@app.get("/api/recommend")
def recommendation():
    kind = str(request.args.get("type") or "")
    building = str(request.args.get("building") or "").upper()
    floor = request.args.get("floor", "0")
    vip = request.args.get("vip", "0")
    if kind not in VEHICLE_TYPES or building not in ("","A","B","C") or not floor.isdigit() or not 0 <= int(floor) <= 4 or vip not in ("0","1"):
        raise BridgeError("Invalid recommendation parameters.")
    with LOCK:
        return jsonify(invoke("recommend",kind,vip,building,floor))


@app.post("/api/layout")
def configure_layout():
    body = request.get_json(silent=True)
    if not isinstance(body, dict) or type(body.get("floorsPerBuilding")) is not int or type(body.get("slotsPerFloor")) is not int:
        raise BridgeError("Enter valid layout numbers.")
    floors, slots = body["floorsPerBuilding"], body["slotsPerFloor"]
    if not (1 <= floors <= 4 and 3 <= slots <= 20):
        raise BridgeError("Allowed range: 1-4 floors and 3-20 slots per floor.")
    with LOCK:
        return jsonify(mutation("configure", floors, slots))


@app.post("/api/reset")
def reset():
    global last_event
    with LOCK:
        result = invoke("reset")
        HISTORY.reset()
        last_event = None
        return jsonify(result)



# PHASE 8: authenticated history, weekly/monthly reports and replayed slot occupancy.
@app.get("/api/history/report")
def phase8_report():
    period=request.args.get("period", "week")
    date=request.args.get("date", "")
    building=request.args.get("building", "").strip().upper()
    slot=request.args.get("slot", "").strip().upper()
    plate=request.args.get("plate", "").strip().upper()
    try:
        floor=int(request.args.get("floor", "0"))
        limit=int(request.args.get("limit", "250"))
        offset=int(request.args.get("offset", "0"))
    except ValueError as exc:raise HistoryError("Invalid number in report filters.") from exc
    with LOCK:
        HISTORY.sync(invoke("state"))
        return jsonify(HISTORY.report(period,date,building,floor,slot,plate,limit,offset))

@app.get("/api/history/slot-at")
def phase8_slot_at():
    slot=request.args.get("slot", "").strip().upper()
    at=request.args.get("at", "").strip()
    with LOCK:
        HISTORY.sync(invoke("state"))
        return jsonify(HISTORY.slot_at(slot,at))

@app.get("/api/history/audit")
def phase8_audit():
    with LOCK:
        HISTORY.sync(invoke("state"))
        return jsonify(events=HISTORY.audit())

def get_local_ip():
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.connect(("8.8.8.8", 80))
            return sock.getsockname()[0]
    except OSError:
        return "127.0.0.1"


if __name__ == "__main__":
    lan_mode = os.getenv("SMARTPARK_LAN", "0") == "1"
    print("\n" + "=" * 54)
    print(" SMARTPARK TRACE PHASE 9 REVIEW — INTEGRATION & QA")
    print("=" * 54)
    print(" Localhost: http://127.0.0.1:5000")
    print(" C++ bridge:", ENGINE)
    if lan_mode:
        print(" LAN:", "http://" + get_local_ip() + ":5000")
        print(" WARNING: LAN only on trusted classroom networks.")
    print(" Stop: Ctrl+C\n")
    app.run(host="0.0.0.0" if lan_mode else "127.0.0.1", port=5000,
            debug=False, use_reloader=False)
