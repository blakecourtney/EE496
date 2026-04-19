// *** TEMPORARY TESTING *** //
#include "dronegps.h"
#include "gndstation.h"
#include <iostream>

using namespace std;

int main(){
    cout << "testing" << endl;
    
    Location A{34.025017, -118.287173};   // top-left
    Location B{34.021807, -118.280111};  // top-right
    Location C{34.018506, -118.280702};  // bottom-right
    Location D{34.018274, -118.291419};   // bottom-left
    Location home{34.018380, -118.308697}; 

    SearchResult result = relay_search_drones(A, B, C, D, home);
    cout << " relay drones: ";
    for (int i = 0; i < result.relay_drones.size(); i++) {
        cout << result.relay_drones[0].lat;
        cout << " " << result.relay_drones[0].lon;
        cout << endl;
    }

    cout << " dest: " << result.search_drone.lat << " " << result.search_drone.lon << endl;
    cout << " flag " << result.update_backbone << endl;

    
// 1. Open serial port (CHANGE THIS to your port)
int fd = open("/dev/cu.usbserial-0001");

if (fd < 0) {
    cout << "Failed to open serial port\n";
    return 1;
}
else {
    cout << "Succeeded to open serial port\n";
}

    // // 2. Configure serial (115200 baud like your Python)
    // struct termios tty;
    // memset(&tty, 0, sizeof tty);

    // tcgetattr(fd, &tty);
    // cfsetospeed(&tty, B115200);
    // cfsetispeed(&tty, B115200);

    // tty.c_cflag |= (CLOCAL | CREAD);
    // tty.c_cflag &= ~CSIZE;
    // tty.c_cflag |= CS8;     // 8-bit
    // tty.c_cflag &= ~PARENB; // no parity
    // tty.c_cflag &= ~CSTOPB; // 1 stop bit

    // tcsetattr(fd, TCSANOW, &tty);

    // // 3. Build packet
    // auto packet = build_waypoint_packet(
    //     1,              // drone_id
    //     34.0522f,      // lat
    //     -118.2437f,    // lon
    //     50.0f          // alt
    // );

    // // 4. Send it
    // write(fd, packet.data(), packet.size());

    // std::cout << "Sent waypoint packet\n";

    // // 5. Close
    // close(fd);


    return 0;
}
