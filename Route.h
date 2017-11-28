#include <vector>

using namespace std;

class Route {
public:
    vector<int> stations;

    void add_station(int station){
        stations.push_back(station);
    }

    void print() {
        for(auto station: stations) cout << station << '-';
    }
};