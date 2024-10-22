#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <iterator>
#include <map>
#include <cmath>

struct TDM
{
	std::map<std::pair<int,int>, std::string> TDM_input;
	int order;
};

int main(int argc, char* argv[])
{

	std::vector<TDM> list_input;
	int inputs = 0; //start with A - then incrememnt 
	int tot_packet, tot_time;
	


	tot_time = 0;
	tot_packet = 0;

	//std::string filename = argv[1]; - no need for output file
	std::ifstream input(argv[1]);

	//parsing the input file for TDM inputs 
	std::string line; //used to parse the file
	while(std::getline(input, line)){ //outer loop takes all rows

		TDM newinput;
		newinput.order = inputs;

		std::stringstream ss(line);
		std::string temp;

		std::getline(ss, temp, ':'); //parse source name
		temp = "";

		while(std::getline(ss, temp, ',')) //parse the content;
		{
			//std::cout << temp << std::endl;
			std::stringstream ss2(temp);
			std::string temp2;
			int i=0;
			std::pair<int, int> time(0,0);
			std::string data; 

			while(std::getline(ss2,temp2, ' '))
			{
				
				if(i == 0){
					time.first = std::stoi(temp2);
					i++;
				}else if(i == 1){
					time.second = std::stoi(temp2);
					i++;
				}
				else
				{
					data = temp2;
					i = 0;
					tot_packet++;
				}
				
				if(tot_time < time.second)
					tot_time = time.second;
		
			}
			newinput.TDM_input.insert({time, data});
			
		}

		list_input.push_back(newinput);
		inputs++;
	}

	
	int avg_trans = ceil(((float)tot_packet/tot_time)); //rounded up - packets per seconds 
	float pack_dur = (float)avg_trans/inputs; //for slot numbers
	int in_rate = list_input[0].TDM_input.begin()->first.second - list_input[0].TDM_input.begin()->first.first;
	float current_time = in_rate;

	//current_time begins after the first transmission

	//output 
	std::cout << "SF" << std::endl;
	for(int i=0; i<tot_time; i+= in_rate)
	{

		if(list_input[0].TDM_input.find({i,i+in_rate}) != list_input[0].TDM_input.end()) //it exists
		{	

			if(current_time < float(i+in_rate))
			{
				current_time = i+in_rate;
				if((int)(current_time) % avg_trans == 0 && ((current_time) == int(current_time)) )
				{
					std::cout << "EF" << std::endl;
					std::cout << "SF" << std::endl;
				}
			}

			std::cout << "0, " << current_time << " " << current_time+pack_dur << " "<< list_input[0].TDM_input.find({i,i+in_rate})->second << std::endl;
			if((int)(current_time+pack_dur) % avg_trans == 0 && ((current_time+pack_dur) == int(current_time+pack_dur)) )
			{
				std::cout << "EF" << std::endl;
				std::cout << "SF" << std::endl;
			}

			current_time += pack_dur;
				
		}

		if(list_input[1].TDM_input.find({i,i+in_rate}) != list_input[1].TDM_input.end())
		{
			if(current_time < float(i+in_rate))
			{
				current_time = i+in_rate;
				if((int)(current_time) % avg_trans == 0 && ((current_time) == int(current_time)) )
				{
					std::cout << "EF" << std::endl;
					std::cout << "SF" << std::endl;
				}
			}
			
			std::cout << "1, " << current_time << " " << current_time+pack_dur << " " << list_input[1].TDM_input.find({i,i+in_rate})->second << std::endl;
			if((int)(current_time+pack_dur) % avg_trans == 0 && (current_time+pack_dur) == int(current_time+pack_dur) )
			{
				std::cout << "EF" << std::endl;
				std::cout << "SF" << std::endl;
			}

			current_time += pack_dur;
			
		}
	
		if(list_input[2].TDM_input.find({i,i+in_rate}) != list_input[2].TDM_input.end())
		{
			if(current_time < float(i+in_rate))
			{
				current_time = i+in_rate;
				if((int)(current_time) % avg_trans == 0 && ((current_time) == int(current_time)) )
				{
					std::cout << "EF" << std::endl;
					std::cout << "SF" << std::endl;
				}
			}

			std::cout << "2, " << current_time << " " << current_time+pack_dur << " " << list_input[2].TDM_input.find({i,i+in_rate})->second << std::endl;
			if((int)(current_time+pack_dur) % avg_trans == 0 && (current_time+pack_dur) == int(current_time+pack_dur) )
			{
				std::cout << "EF" << std::endl;
				std::cout << "SF" << std::endl;
			}

			current_time += pack_dur;
		}

		if(list_input[3].TDM_input.find({i,i+in_rate}) != list_input[3].TDM_input.end())
		{
			if(current_time < float(i+in_rate))
			{
				current_time = i+in_rate;
				if((int)(current_time) % avg_trans == 0 && ((current_time) == int(current_time)) )
				{
					std::cout << "EF" << std::endl;
					std::cout << "SF" << std::endl;
				}
			}
			std::cout << "3, " << current_time << " " << current_time+pack_dur << " " << list_input[3].TDM_input.find({i,i+in_rate})->second << std::endl;
			if((int)(current_time+pack_dur) % avg_trans == 0 && (current_time+pack_dur) == int(current_time+pack_dur) )
			{
				std::cout << "EF" << std::endl;
				std::cout << "SF" << std::endl;
			}

			current_time += pack_dur;
		}
		
	}
	std::cout << "EF" << std::endl;
	
}