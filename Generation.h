#include <memory>
#include <vector>

#include <RouteSet.h>

using namespace std;

class Generation {
public:
    int level = 1;
    vector<shared_ptr<RouteSet>> route_sets;

    void add_member(shared_ptr<RouteSet> route_set) {
        route_sets.push_back(route_set);
    }

    void print() {
        for(auto route_set: route_sets) {
            route_set->print();
        }
        cout << endl;
    }
};
