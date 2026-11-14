#include "FeeCalculator.h"
#include <cstdlib>
#include <fstream>
#include <iomanip>

// Phase 7: one flat hourly rate per vehicle category (no cheaper extra hours).
// Current per-hour charges are read from the local admin rate file.
// Missing keys use the original default values.
double hourlyParkingRate(const string& type) {
    long long base = type == "Motorcycle" ? 50 :
        (type == "Van" || type == "Pickup") ? 150 :
        type == "SUV" ? 130 : type == "Sports Car" ? 180 :
        type == "Bus" ? 220 : type == "Truck" ? 250 : 100;
    const char* custom = std::getenv("SMARTPARK_RATE_FILE");
    const string path = custom && *custom ? custom : "data/parking_rates.txt";
    std::ifstream file(path);
    if (file) {
        string key; long long price = 0;
        while (file >> std::quoted(key) >> price) {
            if (key == type && price >= 1 && price <= 1000000) return static_cast<double>(price);
        }
    }
    return static_cast<double>(base);
}
double calculateParkingFee(const string& type, int startedHours) {
    if (startedHours < 1) startedHours = 1;
    return hourlyParkingRate(type) * startedHours;
}
