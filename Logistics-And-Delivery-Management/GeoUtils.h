#pragma once
#include <string>
#include <utility>
#include <vector>

class GeoUtils {
public:
    static constexpr double DEFAULT_BATCH_RADIUS_KM = 1.5;
    static constexpr double TEN_METERS_KM = 0.01;

    static std::vector<std::pair<std::string, std::pair<double,double>>> knownPlaces();
    static std::string normalize(const std::string& s);
    static std::string matchArea(const std::string& query);
    static std::pair<double,double> coordinatesOf(const std::string& area);
    static double haversineKm(double lat1, double lon1, double lat2, double lon2);
    static double distanceKm(const std::string& a, const std::string& b);
};
