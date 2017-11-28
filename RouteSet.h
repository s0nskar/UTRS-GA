#include <memory>
#include <vector>

#include <Route.h>

using namespace std;

class RouteSet {
public:
    vector<shared_ptr<Route>> routes;
    float TOTFIT = -1;
    float d0, d1, d2, dun, ATT;

    bool operator <(const RouteSet& rs) {
        if (TOTFIT < 0 || rs.TOTFIT < 0) {
            LOGF << "Comparision without TOTFIT";
            exit(0);
        } // Check speed without this
        return TOTFIT < rs.TOTFIT;
    }

    void add_route(shared_ptr<Route> route) {
        routes.push_back(route);
    }

    void print() {
        cout << "*" << TOTFIT << "* ";
        for(auto route: routes) {
            route->print();
            cout << '|';
        }
        cout << endl;
    }

    RouteSet() = default;

    RouteSet(const shared_ptr<RouteSet> route_set) {
        routes = route_set->routes;
    }

    void EVAL() {
        TOTFIT = rand()%50;
    }
};
