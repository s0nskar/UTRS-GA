#include <vector>
#include <limits>
#include <queue>

using namespace std;

class Network {
public:
    string name;
    int N, R=4;
    vector<vector<pair<int, int>>> stations;
    vector<vector<int>> time_matrix;
    vector<vector<int>> demand_matrix;

    Network() = default;

    Network(string name, int N) : name(name), N(N) {
        stations.resize(N, vector<pair<int, int>>());
        demand_matrix.resize(N, vector<int>(N));
        time_matrix.assign(N, vector<int>(N, INF));
    }

    void construct_time_matrix() {
        priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>> > pq;

        for(int i=0;i<N;i++){
            time_matrix[i][i] = 0;
            pq.push({0, i});

            while (!pq.empty()) {
                pair<int, int> front = pq.top(); pq.pop();
                int d = front.first, u = front.second;

                if (d > time_matrix[i][u]) continue;

                for(auto v: stations[u]) {
                    if(time_matrix[i][u] + v.second < time_matrix[i][v.first]){
                        time_matrix[i][v.first] = time_matrix[i][u] + v.second;
                        pq.push({time_matrix[i][v.first], v.first});
                    }
                }
            }
        }
    }
};