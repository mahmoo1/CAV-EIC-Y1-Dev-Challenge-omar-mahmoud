//
// Created by dusan on 9/15/26.
//

#include "../include/antworld.h"


/** @brief this is where you as the applicant will make use of the above functions to develop your solution.
 * here are some existing examples of how calling these functions works to help get you started!
 */
void AntWorld::forage() {
std::vector<std::pair<int, int>> energyRank; //create a vector to store the energies of the ant colony, before sorting them
for (int i = 0; i< this->ants.size(); i++)
{
    energyRank.push_back({ants[i].energy, i}); //we use the type pair in order to use the energy value for the sorting while also maintaining the original index identity
}
  std::sort(
    energyRank.begin(), energyRank.end(), [](const std::pair<int, int>& a, const std::pair<int, int>& b){return a.first > b.first;} //we sort them in descending order of energy starting at the begining of thhe vector and stopping at the end
  );
int quadrant;
int rows = this->terrainMap.size();
int cols = this->terrainMap[0].size();
if(homeCoordinates.second < cols/2)
{
    if(homeCoordinates.first < rows/2)
    {
        quadrant = 0; //top left
    }
    else 
    {
        quadrant = 1; //bottom left
    }
}
else 
{
    if (homeCoordinates.first < rows/2)
    {
        quadrant = 3; //top right
    }
    else{quadrant = 2;} //bottom right
        
}
}

/** You may insert any custom functions below **/
