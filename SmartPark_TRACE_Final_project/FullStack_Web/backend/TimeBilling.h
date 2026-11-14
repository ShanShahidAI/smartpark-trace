#ifndef SMARTPARK_TIME_BILLING_H
#define SMARTPARK_TIME_BILLING_H
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

// Phase 7 clock helpers: elapsed seconds use a stable epoch, while receipt times
// are formatted for the local operator. Legacy V1-V4 minute timestamps migrate.
inline long long parkingNowEpoch() {
    return static_cast<long long>(std::time(nullptr));
}
inline std::string parkingFormatEpoch(long long seconds) {
    std::time_t when = static_cast<std::time_t>(seconds);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &when);
#else
    localtime_r(&when, &local);
#endif
    std::ostringstream text;
    text << std::put_time(&local, "%d %b %Y, %I:%M:%S %p");
    return text.str();
}
inline long long parkingParseStamp(const std::string& text) {
    if (text.empty()) return 0;
    for (const char* format : {"%d %b %Y, %I:%M:%S %p", "%d %b %Y, %I:%M %p"}) {
        std::tm parsed{};
        parsed.tm_isdst = -1;
        std::istringstream input(text);
        input >> std::get_time(&parsed, format);
        if (!input.fail()) {
            input >> std::ws;
            if (input.eof()) {
                std::time_t result = std::mktime(&parsed);
                if (result > 0) return static_cast<long long>(result);
            }
        }
    }
    return 0;
}
inline int parkingStartedHours(long long elapsedSeconds) {
    if (elapsedSeconds < 0) return 0; // Clock cannot precede entry.
    // Even zero elapsed seconds count as one started hour.
    return static_cast<int>(std::max(1LL, (elapsedSeconds + 3599LL) / 3600LL));
}

#endif
