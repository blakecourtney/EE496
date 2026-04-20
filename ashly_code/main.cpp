// *** TEMPORARY TESTING *** //
#include "dronegps.h"
#include "gndstation.h"
#include <iostream>

using namespace std;

// Setup serial port (Linux)
int open_serial(const char* port) {
    int fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    struct termios tty{};
    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr");
        close(fd);
        return -1;
    }

    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag = 0;
    tty.c_oflag = 0;
    tty.c_lflag = 0;

    tty.c_cflag |= (CLOCAL | CREAD);

    tcsetattr(fd, TCSANOW, &tty);

    return fd;
}

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

    const char* port = "/dev/ttyUSB0";  // <-- CHANGE THIS
    int fd = open_serial(port);

    if (fd < 0) {
        std::cerr << "Failed to open serial\n";
        return 1;
    }

    std::cout << "Serial opened\n";

    return 0;
}
