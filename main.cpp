#define INF 10000

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <queue>
#include <map>

#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>

#include <Network.h>
#include <Generation.h>

using namespace std;

static int N;
static Network city;
static Generation population;

const int R = 4;
const int P = 5;
const int K = 14;
const int M = 10;
const int L = 35;
const int U = 5;
const int PER_GEN_RETAINED = 20;


const int RETAINED = P * (float)PER_GEN_RETAINED/100;

///////////////////////////////////////////////////////////////////////////////////

struct Edge {
    int distance;
    int change;
    int route;
    int node;
};

bool operator< (const Edge e1, const Edge e2) {
    return e1.distance < e2.distance;
}


bool operator< (const shared_ptr<RouteSet> rs1, const shared_ptr<RouteSet> rs2) {
    return rs1->TOTFIT < rs2->TOTFIT;
}

int EVAL(shared_ptr<RouteSet> rs) {
    map<string, vector<bool>> edge_routes;

    for(int route=1;route<=R;route++) {
        shared_ptr<Route> r = rs->routes[route-1];
        for(int station=1;station<r->stations.size();station++) {
            string key1 = to_string(r->stations[station-1]) + "-" + to_string(r->stations[station]);
            string key2 = to_string(r->stations[station]) + "-" + to_string(r->stations[station-1]);

            if (edge_routes.find(key1) != edge_routes.end()) {
                edge_routes[key1][route] = true;
                edge_routes[key2][route] = true;
            } else {
                edge_routes[key1] = vector<bool>(R+1, false);
                edge_routes[key2] = vector<bool>(R+1, false);
                edge_routes[key1][route] = true;
                edge_routes[key2][route] = true;
            }
        }
    }

    // for(auto i: edge_routes){
    //     cout << i.first << '-';
    //     for(int j=0;j<=R;j++) {
    //         cout << i.second[j] << ' ';
    //     }
    //     cout << endl;
    // }

    vector<vector<int>> time_matrix(N, vector<int>(N, INF));
    vector<vector<int>> change_matrix(N, vector<int>(N, INF));
    priority_queue<Edge, vector<Edge>> pq;

    for(int i=0;i<N;i++) {
        time_matrix[i][i] = 0;
        change_matrix[i][i] = 0;

        for(int route=1;route<=R;route++)
            pq.push(Edge{0, 0, route, i});

        while (!pq.empty()) {
            Edge front = pq.top(); pq.pop();

            if (front.distance > time_matrix[i][front.node]) continue;

            for(auto v: city.stations[front.node]) {
                string key = to_string(front.node) + "-" + to_string(v.first);
                if (edge_routes.find(key) == edge_routes.end()) continue;

                vector<bool> temp = edge_routes[key];
                for(int route=1;route<=R;route++) {
                    if (!temp[route]) continue;

                    int distance = time_matrix[i][front.node] + v.second;
                    int change = front.change;
                    if (route != front.route) distance += U, change++;

                    if (distance < time_matrix[i][v.first]) {
                        time_matrix[i][v.first] = distance;
                        change_matrix[i][v.first] = change;
                        pq.push(Edge{distance, change, route, v.first});
                    }
                }
            }
        }
    }

    // for(auto i: time_matrix) {
    //     for(auto j: i) {
    //         cout << j << ' ';
    //     } cout << endl;
    // }

    for(int i=0;i<N;i++) {
        for(int j=0;j<N;j++) {
            
        }
    }

    return 0;
}


void IRSG() {
    LOGV << "Creating intial population";

    LOGV << "Determining Activity Levels";
    vector<pair<int, int>> activity_level(N);
    for(int i=0;i<N;i++) {
        activity_level[i] = {0, i};
    }

    for(int i=0;i<N;i++) {
        for(int j=0;j<N;j++) {
            activity_level[i].first += city.demand_matrix[i][j];
            activity_level[j].first += city.demand_matrix[i][j];
        }
    }

    sort(activity_level.begin(), activity_level.end(), [](auto &left, auto &right) {
        return left.first > right.first;
    });

    LOGV << "Filling Initial Population";
    for(int route_set=0;route_set<P;route_set++) {

        shared_ptr<RouteSet> rs = make_shared<RouteSet>();
        vector<pair<int, int>> INS(activity_level.begin(), activity_level.begin()+K);

        int total = accumulate(INS.begin(), INS.end(), 0, [](auto &a, auto &b) {
            return a + b.first;
        });

        for(int route=1;route<=city.R;route++) {
            shared_ptr<Route> r = make_shared<Route>();
            vector<bool> bset(N, false);
            int num_nodes = 1, len_route = 0;

            int rnd = rand()%total;
            int sum = 0, index = 0;
            while (sum < rnd) {
                sum += INS[index].first;
                index++;
            }
            int cur_node = INS[index-1].second;
            total -= INS[index-1].first;
            INS.erase(INS.begin()+index-1);


            r->add_station(cur_node);
            bset[cur_node] = true;

            int tries = 0, VNS = (int)city.stations[cur_node].size();
            while (tries < 2*VNS) {
                pair<int, int> next_node = city.stations[cur_node][rand()%VNS];
                if (bset[next_node.first]) {
                    tries++; continue;
                }

                if (num_nodes + 1 > M or len_route + next_node.second > L) break;

                cur_node = next_node.first;
                tries = 0;
                VNS = (int)city.stations[cur_node].size();
                bset[cur_node] = true;
                num_nodes++;
                len_route += next_node.second;
                r->add_station(cur_node);
            }

            // for(auto i: r->stations) cout << i << ' ';
            // cout << endl;
            rs->add_route(r);
        }
        // cout << endl;
        rs->EVAL();
        population.add_member(rs);
    }

    LOGV << "Generation 1 created";
}

void interCrossover(shared_ptr<RouteSet> rs1, shared_ptr<RouteSet> rs2) {
    int num_iteration = rand()%R;

    for(int i=0;i<num_iteration;i++) {
        int m = rand()%R, n = rand()%R;
        swap(rs1->routes[m], rs2->routes[n]);
    }
}

void intraCrossover(shared_ptr<RouteSet> rs) {
    //TODO
}

void crossover(Generation& population) {
    vector<shared_ptr<RouteSet>> temp_route_sets;

    // Inter Crossover
    int num_iteration = P;

    for(int i=0;i<num_iteration;i++) {
        int m = rand()%P, n = rand()%P;
        shared_ptr<RouteSet> rs1 = make_shared<RouteSet>(population.route_sets[m]);
        shared_ptr<RouteSet> rs2 = make_shared<RouteSet>(population.route_sets[n]);

        interCrossover(rs1, rs2);
        rs1->EVAL(); rs2->EVAL();
        temp_route_sets.push_back(rs1);
        temp_route_sets.push_back(rs2);
    }

    for(auto route_set: temp_route_sets) {
        route_set->print();
    }
    cout << endl;

    // //Intra Crossover
    // num_iteration = rand()%P;

    // for(int i=0;i<num_iteration;i++) {
    //     intraCrossover(population->route_sets[rand()%P]);
    // }

    sort(population.route_sets.rbegin(), population.route_sets.rend());
    sort(temp_route_sets.rbegin(), temp_route_sets.rend());

    // cout << RETAINED << endl;
    copy(temp_route_sets.begin(), temp_route_sets.begin()+P-RETAINED, population.route_sets.begin()+RETAINED);
}

void mutation(Generation& population) {

}

void selection(Generation& population) {

}


void MODIFY(Generation& population) {
    LOGV << "Modifying Generation " << population.level;

    crossover(population);
    mutation(population);
    selection(population);

    population.level++;
    LOGV << "Done";
    LOGV << "Created Evolved Population";
}


int main() {
    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::verbose, &consoleAppender);

    srand(time(NULL)); //Seeding

    LOGV << "Starting program...";

    fstream dataFile("data/network.dat", ios::in);
    if (!dataFile.is_open()) {
        LOGE << "Network file not found!!";
        exit(0);
    }

    LOGV << "Reading network data";
    LOGV << "Creating network";
    dataFile >> N;

    city = Network("Swiss Network", N);

    int node_A, node_B, weight;
    while(dataFile >> node_A >> node_B >> weight) {
        city.stations[node_A].push_back({node_B, weight});
        city.stations[node_B].push_back({node_A, weight});
    }
    LOGV << "Created network: " << city.name;

    LOGV << "Constructing time matrix";
    city.construct_time_matrix();
    LOGV << "Done";

    fstream demandFile("data/demand.dat", ios::in);
    if (!demandFile.is_open()) {
        LOGE << "Demand file not found!!";
        exit(0);
    }

    LOGV << "Filling demand data";
    for(int i=0;i<N;i++)
        for(int j=0;j<N;j++)
            demandFile >> city.demand_matrix[i][j];
    LOGV << "Done";

    for(auto i: city.time_matrix) {
        for(auto j: i) {
            cout << j << ' ';
        } cout << endl;
    }
    IRSG();
    population.print();

    EVAL(population.route_sets[0]);

    // int m = 2;
    // while (m--) {
    //     MODIFY(population);
    //     population.print();
    // }

    LOGV << "Program completed successfully";
}