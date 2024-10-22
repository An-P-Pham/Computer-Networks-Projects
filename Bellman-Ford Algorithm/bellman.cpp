#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <limits> //std::numeric_limits<int>::max(); -> inf
#include <algorithm>
#include <iterator>
#include <map>

//Graph Structure AdjMatrix
struct Graph
{
	//contains the edge weights of the nodes 
	//size of the graph is the #of nodes 
	std::vector<std::vector<int>> mEdges;

	//output data
	int iterations;
	std::map<int,int> distances;
	std::map<int,int> path; //only 1 predecessor
	bool term; //used for early termination
	bool neg;
	int negPred; //used to track a negative cycle
};

void BellmanFord(Graph& graph){

	//base-case
	for(int i=0; i<graph.mEdges.size(); i++){
		if(i == 0){
			graph.distances.insert({i,0});
			graph.path.insert({i,-1});
			graph.term = true;
			graph.neg = false;
			graph.iterations = 0;
		}
		else
		{
			graph.distances.insert({i,std::numeric_limits<int>::max()});
			graph.path.insert({i,i});
		}
	}


	//main alg
	for(int i= 1; i<graph.mEdges.size(); i++){ //run k-1 times
		graph.iterations++;
		if(graph.term){
			graph.term = false;
				for(int j=0; j<graph.mEdges.size(); j++) {
					for(int k=0; k<graph.mEdges[j].size(); k++){
						if((graph.mEdges[j][k] != 0) && (graph.mEdges[j][k] != std::numeric_limits<int>::max()) ){ //checks for same node
							if(graph.distances[j] != std::numeric_limits<int>::max()) //prevent int overflow
							{
								if(graph.distances[j] + graph.mEdges[j][k] < graph.distances[k])
								{
									graph.path[k] = j;
									graph.distances[k] = graph.distances[j] + graph.mEdges[j][k];
									graph.term = true;
								}
							}
							
						}
					}
			}
		}
		else
		{
			return; //bellman terminates
		}
		
	}

	//checking cycles
	for(int i=0; i<graph.mEdges.size(); i++){
		for(int j=0; j<graph.mEdges[i].size(); j++){
			if((graph.mEdges[i][j] != 0) && (graph.mEdges[i][j] != std::numeric_limits<int>::max()) ){
				if(graph.distances[i] + graph.mEdges[i][j] < graph.distances[j]){
					graph.distances[j] = graph.distances[i] + graph.mEdges[i][j];
					graph.path[j] = i;
					graph.negPred = j;
					graph.neg = true;
				}
			}
		}
	}
	
}

int main(int argc, char* argv[])
{
	std::string filename = argv[1];
	std::ifstream input(filename);
	std::string outputFile = "output-" + filename;
	std::ofstream results(outputFile);

	if(input.fail())
	{
		std::cout << "File does not exist" << std::endl;
		return 1;
	}

	Graph myGraph;

	//parsing the input file & build graph
	std::string line; //used to parse the file
	while(std::getline(input, line)){ //outer loop takes all rows

		std::stringstream ss(line);
		
		std::string temp;
		std::vector<int> inputRow;
		while(std::getline(ss, temp, ' ')){//inner loop takes all columns
			//fill in data structure 
			if(temp[0] == 'i'){ 
			 	inputRow.push_back(std::numeric_limits<int>::max());
			}
			else if(temp[0] == '['){ 
				int sum = 0;
				std::string adder;
				temp.erase(0,1);
				std::stringstream ss2(temp);
				int i = 0;
				while(std::getline(ss2, adder, ','))
				{
					if(i < 2)
						sum += std::stoi(adder);
					else
					{
						adder.erase(adder.length()-1,1);
						sum += std::stoi(adder);
						i = 0;
					}
					i++;
				} 
				inputRow.push_back(sum);
			}
			else{
				inputRow.push_back(std::stoi(temp));
			}
		}
		myGraph.mEdges.push_back(inputRow);
		
	}

	BellmanFord(myGraph);
	
	//checks if path exists
	for(int i=0; i<myGraph.distances.size(); i++)
	{
		if(myGraph.distances[i] == std::numeric_limits<int>::max()) //node not reachable?
		{
			results << "No path exists" << std::endl;
			return 0;
		}
	}
		//output
		if(!myGraph.neg)
		{
			//distances
			for(int i=0; i<myGraph.distances.size(); i++){
				results << myGraph.distances[i];
				if(i < myGraph.distances.size()-1)
				{
					results << ",";
				}
				else
					results << std::endl;
			}

			//paths
			for(int i=0; i<myGraph.path.size(); i++){
				
				int current = i;
				std::vector<int> way; //empty list
				while(current != -1){
				   way.push_back(current);
				   current = myGraph.path[current];
				}

				for(int j=way.size()-1; j>=0; j--){
					results << way[j];
					if(j != 0)
					{
						results << "->";
					}
					else
						results << std::endl;
				}
			}

			//Iterations
			results << "Iterations:" << myGraph.iterations << std::endl;

		}
		else
		{
			results << "Input:" << std::endl << std::endl;
			for(int i=0; i<myGraph.mEdges.size(); i++){
				for(int j=0; j<myGraph.mEdges.size(); j++) {
					if(myGraph.mEdges[i][j] == std::numeric_limits<int>::max())
					{
						results << "inf ";
					}
					else
					{
						results << myGraph.mEdges[i][j] << " ";
					}
				}
					results << std::endl;
			}

			results << std::endl;
			results << "Output:" << std::endl;
			results << "Negative Loop Detected" << std::endl;
			
			for(int i=0; i<myGraph.mEdges.size(); i++){
				myGraph.negPred = myGraph.path[myGraph.negPred];
			}
			std::vector<int> negCycle;
			for(int i = myGraph.negPred;; i = myGraph.path[i]){
				negCycle.push_back(i);
				if(i == myGraph.negPred && negCycle.size() > 1)
					break;
			}
			
			for(int j=negCycle.size()-1; j>=0; j--){
					results << negCycle[j];
					if(j != 0)
					{
						results << "->";
					}
					else
						results << std::endl;
			}

		}
	

	return 0;
}
