#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream> 
#include <unordered_map>
#include <vector>
#include <set> 
#include <iterator>
#include <map> 
using namespace std; 
#define INF -999

bool stableState(unordered_map<int, int> old_distance, unordered_map<int, int> new_distance){
	map<int, int> ordered_old(old_distance.begin(), old_distance.end());
	map<int, int> ordered_new(new_distance.begin(), new_distance.end());

	for (auto v: ordered_old){
		if (ordered_old[v.first] != ordered_new[v.first]){
			return false;
		}
	}

	return true;
}

void dp(unordered_map< int, unordered_map<int, int> > graph, int src, set<int> allNodes, 
	unordered_map<int, int> &distance, unordered_map<int, int> &forward, unordered_map<int, vector<int> > &res);

void writeForwardTableToFile(FILE *outputfile, unordered_map<int, unordered_map<int, int> > allForward,
	unordered_map<int, unordered_map<int, int> > allDistance, unordered_map<int, 
	unordered_map<int, vector<int> > > allPath){
	map<int, unordered_map<int, int> > ordered_allForward(allForward.begin(), allForward.end());
	int count = 0;
	for(auto x : ordered_allForward){
		map<int, int> ordered_x(x.second.begin(), x.second.end());
		if (count > 0){
			fwrite("\n", sizeof(char), strlen("\n"), outputfile);
		}
		for (auto y : ordered_x){
			if(y.second == -1 || allDistance[x.first][y.first] == INF)
				continue;
			char str[80];
			memset(str, '\0', sizeof(str));
			sprintf(str, "%d %d %d\n", y.first, y.second, allDistance[x.first][y.first]);
			fwrite(str, sizeof(char), strlen(str), outputfile);
		}
		count += 1;
	}
}

void writeMessageToFile(FILE *outputfile, map<int, int> src_map, map<int, int> des_map, 
	map<int, string> msg_map, unordered_map<int, unordered_map<int, int> > allDistance, 
	unordered_map<int, unordered_map<int, vector<int> > > allPath){
	int x_iter_count = 0;
	for(auto x : msg_map){
		vector<int> :: iterator vec_iter;
		if (x_iter_count == 0){
			fwrite("\n", sizeof(char), strlen("\n"), outputfile);
		}

		char writeBUF[2500];
		memset(writeBUF, '\0', sizeof(writeBUF));

		char from_to[50];
		memset(from_to, '\0', sizeof(from_to));
		sprintf(from_to, "from %d to %d ", src_map[x.first], des_map[x.first]);

		if(allPath[src_map[x.first]][des_map[x.first]].size() == 0){
			strcpy(writeBUF, from_to);
			strcat(writeBUF, "cost infinite hops unreachable message ");
			strcat(writeBUF, x.second.c_str());
			strcat(writeBUF, "\n");
			fwrite(writeBUF, sizeof(char), strlen(writeBUF), outputfile);
			continue;
		}

		char cost[50];
		memset(cost, '\0', sizeof(cost));
		sprintf(cost, "cost %d ", allDistance[src_map[x.first]][des_map[x.first]]);

		char hops[500];
		memset(hops, '\0', sizeof(hops));
		for(vec_iter = allPath[src_map[x.first]][des_map[x.first]].begin(); 
			vec_iter != allPath[src_map[x.first]][des_map[x.first]].end(); ++vec_iter){
			if (vec_iter == allPath[src_map[x.first]][des_map[x.first]].begin()){
				sprintf(hops, "hops %d", *vec_iter);
			} else {
				char temp[20];
				memset(temp, '\0', sizeof(temp));
				sprintf(temp, " %d", *vec_iter);
				strcat(hops, temp);
			}
		}

		char msgs[1000];
		memset(msgs, '\0', sizeof(msgs));
		sprintf(msgs, " message %s\n", x.second.c_str());

		strcpy(writeBUF, from_to);
		strcat(writeBUF, cost);
		strcat(writeBUF, hops);
		strcat(writeBUF, msgs);

		fwrite(writeBUF, sizeof(char), strlen(writeBUF), outputfile);
		x_iter_count += 1;
	}
	fwrite("\n", sizeof(char),strlen("\n"), outputfile);
}


// wrap what you got from dp() and write to the outputfile
void dp_wrapper(unordered_map< int, unordered_map<int, int> > graph, 
	FILE *outputfile, set<int> allNodes, map<int, int> src_map,
	map<int,int> des_map, map<int, string>msg_map){
	unordered_map<int, unordered_map<int, int> > allForward;
	unordered_map<int, unordered_map<int, int> > allDistance;
	unordered_map<int, unordered_map<int, vector<int> > > allPath;
	set<int> :: iterator set_iter;
	for (set_iter = allNodes.begin(); set_iter != allNodes.end(); ++set_iter){
		dp(graph, *set_iter, allNodes, allDistance[*set_iter], allForward[*set_iter], allPath[*set_iter]);
	}

	writeForwardTableToFile(outputfile, allForward, allDistance, allPath);

	writeMessageToFile(outputfile, src_map, des_map, msg_map, allDistance, allPath);
}

// generate the distance, forward table and path table for one single node: src;
void dp(unordered_map< int, unordered_map<int, int> > graph, int src, set<int> allNodes, 
	unordered_map<int, int> &distance, unordered_map<int, int> &forward, 
	unordered_map<int, vector<int> > &res){
	// distance[x] stands for the distance from src to x
	int allNumber = 0;
	unordered_map<int, int> old_distance;
	unordered_map<int, int> new_distance;

	// initialize some stuff...
	for (auto x : allNodes){
		allNumber += 1;
		if (x == src){
			distance[x] = 0;
			forward[x] = x;
			res[x].push_back(x);
		} else if (graph[src][x]) {
			distance[x] = graph[src][x];
			forward[x] = x;
			res[x].push_back(src);
		} else {
			distance[x] = INF;
			forward[x] = -1;
		}
	}

	while(true){
		old_distance = distance;
		for (auto x: allNodes){
			int minDis = distance[x];
			for (auto v: allNodes){
				if (distance[v] != INF && graph[v][x] && graph[v][x] != INF){
					if (minDis == INF || (minDis != INF && minDis > distance[v] + graph[v][x])){
						minDis = distance[v] + graph[v][x];
						forward[x] = forward[v];
						res[x] = res[v];
						res[x].push_back(v);
					}
				}
			}
			distance[x] = minDis;
		}
		new_distance = distance;
		if (stableState(old_distance, new_distance)){
			break;
		}
	}
}

int main(int argc, char** argv) {
    if (argc != 4) {
        printf("Usage: ./distvec topofile messagefile changesfile\n");
        return -1;
    }

    unordered_map< int, unordered_map<int, int> > graph; 
    set<int> allNodes; 
    FILE *outputfile = fopen("output.txt", "wb");

    // read something from topofile;
	FILE* topo = fopen(argv[1], "rb");
	char *line = NULL;
	size_t len = 0;
	while ((getline(&line, &len, topo)) != -1) {
		if (line[strlen(line)-1] == '\n')
	    	line[strlen(line)-1] = '\0';
	    int firstNode = atoi(strtok(line, " "));
	    int secondNode = atoi(strtok(NULL, " "));
	    int weight = atoi(strtok(NULL, " "));
	    graph[firstNode][secondNode] = weight;
	    graph[secondNode][firstNode] = weight;
	    allNodes.insert(firstNode);
	    allNodes.insert(secondNode);
	}
	if (line)
	    free(line);
	fclose(topo);

	// read something from messagefile;
	FILE *message = fopen(argv[2], "rb");
	char *line2 = NULL;
	size_t len2 = 0;
	unordered_map<int, int> unorder_src_map;
	unordered_map<int, int> unorder_des_map;
	unordered_map<int, string> unorder_msg_map;
	int iter_time = 1;
	while ((getline(&line2, &len2, message)) != -1) {
		int src = atoi(strtok(line2, " "));
		int des = atoi(strtok(NULL, " "));
		char *messageBUF = strtok(NULL, "\n");
		string str(messageBUF);
		unorder_src_map[iter_time] = src;
		unorder_des_map[iter_time] = des;
		unorder_msg_map[iter_time] = str;
		iter_time += 1;
	}
	map<int, int> src_map(unorder_src_map.begin(), unorder_src_map.end());
	map<int, int> des_map(unorder_des_map.begin(), unorder_des_map.end());
	map<int, string> msg_map(unorder_msg_map.begin(), unorder_msg_map.end());
	dp_wrapper(graph, outputfile, allNodes, src_map, des_map, msg_map);
	if (line2)
	    free(line2);
	fclose(message);

	//read something from changesfile;
	FILE *changes = fopen(argv[3], "rb");
	char *line3 = NULL;
	size_t len3 = 0;
	while ((getline(&line3, &len3, changes)) != -1){
		if (line3[strlen(line3)-1] == '\n')
			line3[strlen(line3)-1] = '\0';
	    int firstNode = atoi(strtok(line3, " "));
	    int secondNode = atoi(strtok(NULL, " "));
	    int weight = atoi(strtok(NULL, " "));
	    graph[firstNode][secondNode] = weight;
	    graph[secondNode][firstNode] = weight;

		FILE *message2 = fopen(argv[2], "rb");
		char *line4 = NULL;
		size_t len4 = 0;
		unordered_map<int, int> unorder_src_map2;
		unordered_map<int, int> unorder_des_map2;
		unordered_map<int, string> unorder_msg_map2;
		int iter_time2 = 1;
	    while ((getline(&line4, &len4, message2)) != -1){
	    	int src2 = atoi(strtok(line4, " "));
	    	int des2 = atoi(strtok(NULL, " "));
	    	char *messageBUF2 = strtok(NULL, "\n");
	    	string str2(messageBUF2);
			unorder_src_map2[iter_time2] = src2;
			unorder_des_map2[iter_time2] = des2;
			unorder_msg_map2[iter_time2] = str2;
			iter_time2 += 1;
	    }
		map<int, int> src_map2(unorder_src_map2.begin(), unorder_src_map2.end());
		map<int, int> des_map2(unorder_des_map2.begin(), unorder_des_map2.end());
		map<int, string> msg_map2(unorder_msg_map2.begin(), unorder_msg_map2.end());
		dp_wrapper(graph, outputfile, allNodes, src_map2, des_map2, msg_map2);
	    fclose(message2);
		if (line4)
			free(line4);
	}

	if (line3)
		free(line3);

	fclose(changes);	
	fclose(outputfile);
    return 0;
}

