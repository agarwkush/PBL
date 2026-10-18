#include "GeoUtils.h"
#include <cmath>
#include <cctype>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

std::string GeoUtils::normalize(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (std::isalnum((unsigned char)c))
            out.push_back((char)std::tolower((unsigned char)c));
    }
    return out;
}

std::vector<std::pair<std::string, std::pair<double,double>>> GeoUtils::knownPlaces() {
    return {
        {"clock tower", {30.324323, 78.041863}},
        {"rajpur road", {30.385693, 78.090299}},
        {"mussoorie", {30.4569, 78.0783}},
        {"clement town", {30.26781, 78.00751}},
        {"isbt", {30.29385, 78.05373}},
        {"nehru colony", {30.3010, 78.0540}},
        {"patel nagar", {30.30736, 78.01702}},
        {"jolly grant airport", {30.195134, 78.192097}},
        {"saharanpur road", {30.28185, 77.992333}},
        {"raipur", {30.316706, 78.099994}},
        {"karanpur", {30.3200, 78.0400}},
        {"kishan nagar", {30.3100, 78.0200}},
        {"dalanwala", {30.3000, 78.0500}},
        {"cantonment", {30.351790, 78.029870}},
        {"haridwar road", {30.2500, 78.0000}},
        {"vikas nagar", {30.2800, 78.0300}},
        {"malsi", {30.3300, 78.0200}},
        {"ballupur", {30.3000, 78.0300}},
        {"survey chowk", {30.3300, 78.0400}},
        {"gandhi road", {30.3200, 78.0400}},
        {"prem nagar", {30.2900, 78.0600}},
        {"rajeev nagar", {30.3050, 78.0550}},
    };
}

std::string GeoUtils::matchArea(const std::string& query) {
    std::string q = normalize(query);
    std::string best;
    size_t bestLength = 0;
    for (const auto& place : knownPlaces()) {
        std::string key = normalize(place.first);
        if (!key.empty() && key.size() > bestLength && q.find(key) != std::string::npos) {
            best = place.first;
            bestLength = key.size();
        }
    }
    return best;
}

std::pair<double,double> GeoUtils::coordinatesOf(const std::string& area) {
    for (const auto& place : knownPlaces()) {
        if (place.first == area) return place.second;
    }
    return {0.0, 0.0};
}

double GeoUtils::haversineKm(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371.0;
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    double a = std::sin(dLat/2)*std::sin(dLat/2) +
               std::cos(lat1*M_PI/180.0)*std::cos(lat2*M_PI/180.0) *
               std::sin(dLon/2)*std::sin(dLon/2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    return R * c;
}

double GeoUtils::distanceKm(const std::string& a, const std::string& b) {
    std::string areaA = matchArea(a);
    std::string areaB = matchArea(b);
    if (areaA.empty() || areaB.empty()) return -1.0;
    if (areaA == areaB) return 0.0;
    auto pa = coordinatesOf(areaA);
    auto pb = coordinatesOf(areaB);
    return haversineKm(pa.first, pa.second, pb.first, pb.second);
}
