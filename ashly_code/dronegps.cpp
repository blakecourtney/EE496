#include <iostream>
#include <vector>
#include <cmath>

// add config file

using namespace std;

double EARTH_RADIUS = 6371000.0; // meters
double drone_radius = 200.0; // meters
double drone_diameter = drone_radius * 2; // meters

struct Location {
    double lat;
    double lon;
};

double deg2rad(double deg){
    return deg * M_PI / 180;
}

double rad2deg(double rad){
    return rad * 180.0 / M_PI;
}

// Calculate distance between search destination (b) and home base (a)
double dist(const Location& a, const Location& b){
    double lat1 = deg2rad(a.lat);
    double lon1 = deg2rad(a.lon);
    double lat2 = deg2rad(b.lat);
    double lon2 = deg2rad(b.lon);

    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;

    double h = sin(dlat/2)*sin(dlat/2)+cos(lat1)*cos(lat2)*sin(dlon/2)*sin(dlon/2);

    return 2 * EARTH_RADIUS * atan2(sqrt(h),sqrt(1-h));
}

// Converts points to 3D coordinates on Earth (sphere), moves along arx, converts back to lat/lon
Location interpolate(const Location& a, const Location& b, double fraction) {
    double lat1 = deg2rad(a.lat);
    double lon1 = deg2rad(a.lon);
    double lat2 = deg2rad(b.lat);
    double lon2 = deg2rad(b.lon);

    double d = dist(a, b) / EARTH_RADIUS;

    if (d == 0) return a;

    double A = sin((1 - fraction) * d) / sin(d);
    double B = sin(fraction * d) / sin(d);

    double x = A * cos(lat1) * cos(lon1) + B * cos(lat2) * cos(lon2);
    double y = A * cos(lat1) * sin(lon1) + B * cos(lat2) * sin(lon2);
    double z = A * sin(lat1) + B * sin(lat2);

    double lat = atan2(z, sqrt(x*x + y*y));
    double lon = atan2(y, x);

    return { rad2deg(lat), rad2deg(lon) };
}

vector<Location> create_backbone(Location home, Location dest){
    double distance = dist(home, dest);
    int num_segments = ceil(distance / drone_diameter);
    int num_drones = max(0, num_segments - 1);

    vector<Location> drone_locations;

    for(int i = 1; i <= num_drones; i++){
        double fraction = static_cast<double>(i) / num_segments;
        drone_locations.push_back(interpolate(home, dest, fraction));
    }

    return drone_locations;
}

// Number of backbone drones required = create_backbone.size()

