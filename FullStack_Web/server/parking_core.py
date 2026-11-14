"""SmartPark Phase 1 domain logic and local persistence (Python standard library only).

This module is deliberately independent of Flask so the parking rules can be unit
-tested. A later phase will connect these operations to the shared C++ DSA engine.
"""

from __future__ import annotations

from copy import deepcopy
from datetime import datetime
from pathlib import Path
from threading import RLock
import json
import os
import re
import tempfile
import uuid


SLOT_COUNT = 12
RATES = {
    "Car": (100, 100), "Motorcycle": (50, 50), "Van": (150, 150),
    "Emergency": (100, 100), "Sedan": (100, 100), "SUV": (130, 130),
    "Sports Car": (180, 180), "Pickup": (150, 150), "Bus": (220, 220),
    "Taxi": (100, 100), "Truck": (250, 250),
}
VEHICLE_TYPES = tuple(RATES)
PLATE_PATTERN = re.compile(r"^[A-Z0-9][A-Z0-9 -]{0,18}[A-Z0-9]$")


class ParkingError(ValueError):
    def __init__(self, message: str, status: int = 400):
        super().__init__(message)
        self.status = status


def current_time() -> str:
    return datetime.now().strftime("%d %b %Y, %I:%M %p")


def default_state() -> dict:
    return {
        "slots": [
            {"id": f"P{i:02d}", "occupied": False, "vehicle": None}
            for i in range(1, SLOT_COUNT + 1)
        ],
        "waiting": [],
        "history": [],
        "total_revenue": 0,
        "served": 0,
    }


def parking_fee(vehicle_type: str, hours: int) -> int:
    if vehicle_type not in RATES or not isinstance(hours, int) or not 1 <= hours <= 720:
        raise ParkingError("Invalid vehicle type or parking duration.")
    hourly_rate = RATES[vehicle_type][0]
    return hourly_rate * hours


def priority_level(vehicle: dict) -> int:
    """Emergency > VIP > normal; FIFO within each level."""
    if vehicle.get("vehicleType") == "Emergency":
        return 3
    if vehicle.get("priority") is True:
        return 2
    return 1


def _is_valid_state(state: object) -> bool:
    if not isinstance(state, dict):
        return False
    slots = state.get("slots")
    if not isinstance(slots, list) or len(slots) != SLOT_COUNT:
        return False
    if not isinstance(state.get("waiting"), list) or not isinstance(state.get("history"), list):
        return False
    if type(state.get("served")) is not int or state["served"] < 0:
        return False
    if type(state.get("total_revenue")) not in (int, float) or state["total_revenue"] < 0:
        return False
    present = set()
    for i, slot in enumerate(slots, start=1):
        if not isinstance(slot, dict) or slot.get("id") != f"P{i:02d}":
            return False
        occupied, vehicle = slot.get("occupied"), slot.get("vehicle")
        if type(occupied) is not bool or ((vehicle is not None) != occupied):
            return False
        if occupied:
            if not _valid_vehicle(vehicle) or vehicle.get("parkingSlot") != slot["id"]:
                return False
            number = vehicle["vehicleNumber"].upper()
            if number in present:
                return False
            present.add(number)
    for vehicle in state["waiting"]:
        if not _valid_vehicle(vehicle) or vehicle.get("parkingSlot"):
            return False
        number = vehicle["vehicleNumber"].upper()
        if number in present:
            return False
        present.add(number)
    return all(_valid_vehicle(vehicle) for vehicle in state["history"])


def _valid_vehicle(value: object) -> bool:
    return (
        isinstance(value, dict)
        and isinstance(value.get("vehicleNumber"), str)
        and isinstance(value.get("ownerName"), str)
        and value.get("vehicleType") in RATES
        and isinstance(value.get("parkingSlot"), str)
    )


def _atomic_write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="w", encoding="utf-8", dir=path.parent,
            prefix=f".{path.name}.", suffix=".tmp", delete=False
        ) as handle:
            tmp = Path(handle.name)
            handle.write(content)
            handle.flush()
            os.fsync(handle.fileno())
        os.replace(tmp, path)
    finally:
        if tmp and tmp.exists():
            tmp.unlink()


class ParkingService:
    def __init__(self, data_dir: str | Path):
        self.data_dir = Path(data_dir)
        self.data_dir.mkdir(parents=True, exist_ok=True)
        self.state_file = self.data_dir / "state.json"
        self.lock = RLock()
        self.state = self._load()
        if not self.state_file.exists():
            self._save()

    def _load(self) -> dict:
        if not self.state_file.exists():
            return default_state()
        try:
            saved = json.loads(self.state_file.read_text(encoding="utf-8"))
            if not _is_valid_state(saved):
                raise ValueError("Invalid state structure")
            return saved
        except (OSError, ValueError, TypeError):
            backup = self.data_dir / f"state-recovery-{datetime.now():%Y%m%d-%H%M%S-%f}.json"
            try:
                os.replace(self.state_file, backup)
                print(f"WARNING: damaged state preserved at {backup.name}; starting an empty demo.")
            except OSError:
                print("WARNING: could not load saved state; starting an empty demo.")
            return default_state()

    def _save(self) -> None:
        """Save primary state atomically; readable .txt exports are derived views."""
        _atomic_write(self.state_file, json.dumps(self.state, indent=2, ensure_ascii=False))
        rows = {
            "vehicles.txt": [
                f'{s["vehicle"]["vehicleNumber"]} | {s["vehicle"]["ownerName"]} | {s["id"]}'
                for s in self.state["slots"] if s["occupied"]
            ],
            "parking.txt": [
                f'{s["id"]} | {"Occupied" if s["occupied"] else "Available"}'
                for s in self.state["slots"]
            ],
            "history.txt": [
                f'{h["vehicleNumber"]} | Rs.{h.get("parkingFee", 0)} | {h.get("exitTime", "")}'
                for h in self.state["history"]
            ],
        }
        for name, lines in rows.items():
            _atomic_write(self.data_dir / name, "\n".join(lines))

    def snapshot(self) -> dict:
        with self.lock:
            occupied = sum(slot["occupied"] for slot in self.state["slots"])
            return deepcopy({
                "slots": self.state["slots"],
                "waiting": self.state["waiting"],
                "history": self.state["history"],
                "totalSlots": SLOT_COUNT,
                "available": SLOT_COUNT - occupied,
                "occupied": occupied,
                "waitingCount": len(self.state["waiting"]),
                "revenue": self.state["total_revenue"],
                "served": self.state["served"],
            })

    def park(self, payload: dict) -> dict:
        if not isinstance(payload, dict):
            raise ParkingError("Please send valid vehicle details.")
        number = str(payload.get("vehicleNumber") or "").strip().upper()
        number = re.sub(r"\s+", " ", number)
        owner = re.sub(r"\s+", " ", str(payload.get("ownerName") or "")).strip()
        typ = payload.get("vehicleType", "Car")
        priority = payload.get("priority") is True or typ == "Emergency"
        if not PLATE_PATTERN.fullmatch(number):
            raise ParkingError("Vehicle number must be 2-20 letters/numbers (spaces or hyphens allowed).")
        if not owner or len(owner) > 70 or any(ord(ch) < 32 for ch in owner):
            raise ParkingError("Owner name must be between 1 and 70 characters.")
        if typ not in VEHICLE_TYPES:
            raise ParkingError("Invalid vehicle type.")
        with self.lock:
            all_vehicles = [s["vehicle"] for s in self.state["slots"] if s["occupied"]]
            all_vehicles += self.state["waiting"]
            if any(v["vehicleNumber"].upper() == number for v in all_vehicles):
                raise ParkingError("This vehicle is already parked or waiting.", 409)
            vehicle = {
                "ticketId": "SP-" + uuid.uuid4().hex[:8].upper(),
                "vehicleNumber": number,
                "ownerName": owner,
                "vehicleType": typ,
                "priority": priority,
                "parkingSlot": "",
                "entryTime": current_time(),
                "parkingFee": 0,
            }
            slot = next((s for s in self.state["slots"] if not s["occupied"]), None)
            if slot is None:
                self.state["waiting"].append(vehicle)
                queued = True
                message = "Parking is full. Vehicle added to waiting queue."
            else:
                vehicle["parkingSlot"] = slot["id"]
                slot["occupied"] = True
                slot["vehicle"] = vehicle
                queued = False
                message = f'Vehicle parked in {slot["id"]}.'
            self._save()
            return {"queued": queued, "message": message,
                    "vehicle": deepcopy(vehicle), **self.snapshot()}

    def exit(self, payload: dict) -> dict:
        if not isinstance(payload, dict):
            raise ParkingError("Please send valid exit details.")
        number = str(payload.get("vehicleNumber") or "").strip().upper()
        raw_hours = payload.get("hours", 1)
        if isinstance(raw_hours, bool) or not str(raw_hours).isdigit():
            raise ParkingError("Parking duration must be a whole number.")
        hours = int(raw_hours)
        if not 1 <= hours <= 720:
            raise ParkingError("Parking duration must be between 1 and 720 hours.")
        with self.lock:
            slot = next((s for s in self.state["slots"]
                         if s["occupied"] and s["vehicle"]["vehicleNumber"].upper() == number), None)
            if slot is None:
                raise ParkingError("Vehicle not found in an occupied slot.", 404)
            record = deepcopy(slot["vehicle"])
            amount = parking_fee(record["vehicleType"], hours)
            record["parkingFee"] = amount
            record["exitTime"] = current_time()
            record["hours"] = hours
            self.state["history"].insert(0, record)
            self.state["total_revenue"] += amount
            self.state["served"] += 1
            freed = slot["id"]
            slot["occupied"] = False
            slot["vehicle"] = None
            assigned = None
            waiting = self.state["waiting"]
            if waiting:
                # Use earliest arrival to break ties: stable priority ordering.
                idx = max(range(len(waiting)), key=lambda i: priority_level(waiting[i]))
                assigned = waiting.pop(idx)
                assigned["parkingSlot"] = freed
                assigned["entryTime"] = current_time()
                slot["occupied"] = True
                slot["vehicle"] = assigned
            self._save()
            return {
                "message": f"{number} exited from {freed}.",
                "bill": {"vehicleNumber": number, "slot": freed, "hours": hours,
                         "fee": amount, "exitTime": record["exitTime"]},
                "assignedFromQueue": deepcopy(assigned), **self.snapshot(),
            }

    def search(self, query: str) -> dict:
        q = query.strip().lower()[:80]
        if not q:
            return {"results": []}
        with self.lock:
            vehicles = [s["vehicle"] for s in self.state["slots"] if s["occupied"]]
            vehicles += self.state["waiting"]
            fields = ("vehicleNumber", "ownerName", "vehicleType", "parkingSlot", "ticketId")
            matched = [v for v in vehicles if any(q in str(v.get(field, "")).lower() for field in fields)]
            return {"results": deepcopy(matched)}

    def dsa(self) -> dict:
        with self.lock:
            active = [s["vehicle"] for s in self.state["slots"] if s["occupied"]]
            waiting = self.state["waiting"]
            priority_items = sorted(
                enumerate(waiting), key=lambda p: (-priority_level(p[1]), p[0])
            )
            return deepcopy({
                "linkedList": active,
                "queue": waiting,
                "stack": self.state["history"][:8],
                "priorityQueue": [v for _, v in priority_items],
                "metrics": {
                    "linkedListNodes": len(active),
                    "queueNodes": len(waiting),
                    "stackRecords": len(self.state["history"]),
                    "priorityItems": len(priority_items),
                },
            })

    def reset(self) -> dict:
        with self.lock:
            if any(s["occupied"] for s in self.state["slots"]) or self.state["waiting"] or self.state["history"]:
                backup_dir = self.data_dir / "backups"
                name = f"before-reset-{datetime.now():%Y%m%d-%H%M%S-%f}.json"
                _atomic_write(backup_dir / name, json.dumps(self.state, indent=2, ensure_ascii=False))
            self.state = default_state()
            self._save()
            return self.snapshot()
