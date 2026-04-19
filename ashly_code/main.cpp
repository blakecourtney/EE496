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

    


    return 0;
}
