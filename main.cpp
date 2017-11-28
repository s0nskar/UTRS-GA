#define INF 10000

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <queue>
#include <map>
#include <set>

#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>

#include <Network.h>
#include <Generation.h>

using namespace std;

static int N;
static Network city;
static Generation population;

const int NUM_GEN = 5;
const int P = 200;
const int K = 14;
const int M = 10;
const int L = 35;
const int U = 5;
const int PER_GEN_RETAINED = 20;
const int m = 5;

const int w1 = 1;
const int w2 = 1;
const int w3 = 1;

const int K1 = 10;
const int b1 = 0.3;
const float xm = 25;

const int K2 = 10;
const int b2 = 20;
const float a = 0.70;
const float b = 0.25;
const float c = 0.5;

const int K3 = 10;
const int b3 = 0.4;


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

void EVAL(shared_ptr<RouteSet> rs) {
    map<string, vector<bool>> edge_routes;

    for(int route=1;route<=city.R;route++) {
        shared_ptr<Route> r = rs->routes[route-1];
        for(int station=1;station<r->stations.size();station++) {
            string key1 = to_string(r->stations[station-1]) + "-" + to_string(r->stations[station]);
            string key2 = to_string(r->stations[station]) + "-" + to_string(r->stations[station-1]);

            if (edge_routes.find(key1) != edge_routes.end()) {
                edge_routes[key1][route] = true;
                edge_routes[key2][route] = true;
            } else {
                edge_routes[key1] = vector<bool>(city.R+1, false);
                edge_routes[key2] = vector<bool>(city.R+1, false);
                edge_routes[key1][route] = true;
                edge_routes[key2][route] = true;
            }
        }
    }


    vector<vector<int>> time_matrix(N, vector<int>(N, INF));
    vector<vector<int>> change_matrix(N, vector<int>(N, 0));
    priority_queue<Edge, vector<Edge>> pq;

    for(int i=0;i<N;i++) {
        time_matrix[i][i] = 0;
        change_matrix[i][i] = 0;

        for(int route=1;route<=city.R;route++)
            pq.push(Edge{0, 0, route, i});

        while (!pq.empty()) {
            Edge front = pq.top(); pq.pop();

            if (front.distance > time_matrix[i][front.node]) continue;

            for(auto v: city.stations[front.node]) {
                string key = to_string(front.node) + "-" + to_string(v.first);
                if (edge_routes.find(key) == edge_routes.end()) continue;

                vector<bool> temp = edge_routes[key];
                for(int route=1;route<=city.R;route++) {
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

    float d0, d1, d2, dun, ATT;
    d0 = d1 = d2 = dun = ATT = 0;

    float F1 = 0;

    for(int i=0;i<N;i++) {
        for(int j=0;j<N;j++) {
            if (time_matrix[i][j] == INF or change_matrix[i][j] > 2) {
                dun += city.demand_matrix[i][j];
                continue;
            }
            if (change_matrix[i][j] == 0) {
                d0 += city.demand_matrix[i][j];
            } else if (change_matrix[i][j] == 1) {
                d1 += city.demand_matrix[i][j];
            } else if (change_matrix[i][j] == 2) {
                d2 += city.demand_matrix[i][j];
            }

            ATT += time_matrix[i][j]*city.demand_matrix[i][j];

            int x = time_matrix[i][j] - city.time_matrix[i][j];
            float f = (x < xm) ? - (K1 - b1)*(x/xm)*(x/xm) - b1*(x/xm) + K1 : 0;

            F1 += city.demand_matrix[i][j]*f;
        }
    }

    F1 = F1/city.total_demand;

    d0 = d0/city.total_demand;
    d1 = d1/city.total_demand;
    d2 = d2/city.total_demand;
    dun = dun/city.total_demand;
    ATT = ATT/city.total_demand;

    rs->d0 = d0;
    rs->d1 = d1;
    rs->d2 = d2;
    rs->dun = dun;
    rs->ATT = ATT;

    // cout << d0 << ' ' << d1 << ' ' << d2 << ' ' << dun << ' ';

    float dt = a*d0 + b*d1 + c*d2;
    float F2 = ((K2 - b2*a)/(a*a))*dt*dt + b2*dt;

    float F3 = - (K3 - b3)*dun*dun - b3*dun + K3;

    rs->TOTFIT = w1*F1 + w2*F2 + w3*F3;
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

            rs->add_route(r);
        }

        EVAL(rs);
        population.add_member(rs);
    }

    LOGV << "Generation 1 created";
}

void interCrossover(shared_ptr<RouteSet> rs1, shared_ptr<RouteSet> rs2) {
    int num_iteration = rand()%city.R;

    if (rand()%2) {
        for(int i=0;i<num_iteration;i++) {
            int m = rand()%city.R, n = rand()%city.R;
            swap(rs1->routes[m], rs2->routes[n]);
        }
    }
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
        EVAL(rs1); EVAL(rs2);
        temp_route_sets.push_back(rs1);
        temp_route_sets.push_back(rs2);
    }

    sort(population.route_sets.rbegin(), population.route_sets.rend());
    sort(temp_route_sets.rbegin(), temp_route_sets.rend());

    copy(temp_route_sets.begin(), temp_route_sets.begin()+P-RETAINED, population.route_sets.begin()+RETAINED);
}

void mutation(shared_ptr<RouteSet> rs) {
    shared_ptr<Route> r = make_shared<Route>();
    vector<bool> bset(N, false);
    int num_nodes = 1, len_route = 0;

    int cur_node = rand()%N;

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
    rs->routes[rand()%city.R] = r;
    EVAL(rs);
}

void selection(Generation& population) {
    vector<shared_ptr<RouteSet>> new_population, sel_population;
    new_population.resize(P*m);
    for(int i=0;i<m;i++) {
        copy(population.route_sets.begin(), population.route_sets.end(), new_population.begin()+i*P);
    }

    int rnd = rand()%P;
    while(rnd--) {
        int x = rand()%(P*m);
        mutation(new_population[x]);
    }

    random_shuffle(new_population.begin(), new_population.end());

    int max_index = -1;
    int sec_max_index = -1;
    int max_value = 0;
    int sec_max_value = 0;

    for(int i=0;i<P*m;i++) {
        if (i%(2*m) == 0) {
            if (i != 0) {
                sel_population.push_back(new_population[max_index]);
                sel_population.push_back(new_population[sec_max_index]);
            }
            max_index = sec_max_index = i;
            max_value = sec_max_value = new_population[i]->TOTFIT;
            continue;
        }

        if (new_population[i]->TOTFIT >= max_value) {
            sec_max_index = max_index;
            sec_max_value = max_value;
            max_value = new_population[i]->TOTFIT;
            max_index = i;
        }
        else if (new_population[i]->TOTFIT > sec_max_value) {
            sec_max_index = i;
            sec_max_value = new_population[i]->TOTFIT;
        }
    }
    sel_population.push_back(new_population[max_index]);
    if (P%2 == 0) sel_population.push_back(new_population[sec_max_index]);

    copy(sel_population.begin(), sel_population.end(), population.route_sets.begin());
}


void MODIFY(Generation& population) {
    LOGV << "Modifying Generation " << population.level;

    crossover(population);
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
        for(int j=0;j<N;j++) {
            demandFile >> city.demand_matrix[i][j];
            city.total_demand += city.demand_matrix[i][j];
        }
    LOGV << "Done";


    IRSG();
    // population.print();

    for(int i=0;i<NUM_GEN;i++) {
        MODIFY(population);
        // population.print();
    }

    // population.print();

    sort(population.route_sets.rbegin(), population.route_sets.rend());

    shared_ptr<RouteSet> best_rs = population.route_sets[0];
    best_rs->print();
    cout << "d0: " << best_rs->d0 << endl;
    cout << "d1: " << best_rs->d1 << endl;
    cout << "d2: " << best_rs->d2 << endl;
    cout << "dun: " << best_rs->dun << endl;
    cout << "ATT: " << best_rs->ATT << endl;

    LOGV << "Program completed successfully";
}