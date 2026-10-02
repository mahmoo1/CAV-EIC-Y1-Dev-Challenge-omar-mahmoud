//
// Created by dusan on 9/15/26.
//

#include "../include/antworld.h"
#include <iostream>

/** @brief this is where you as the applicant will make use of the above functions to develop your solution.
 * here are some existing examples of how calling these functions works to help get you started!
 */
static bool initialCycle = true;
static std::vector<int> searchers;
static std::vector<int> collectors;
static std::vector<Coord> scanTargets;
static std::vector<Coord> knownFood;
static int forageCycle = 0;


void AntWorld::forage() {
if (initialCycle)
{
std::vector<std::pair<int, int>> energyRank; //create a vector to store the energies of the ant colony, before sorting them
for (int i = 0; i< this->ants.size(); i++)
{
    energyRank.push_back({ants[i].energy, i}); //we use the type pair in order to use the energy value for the sorting while also maintaining the original index identity
}
  std::sort(
    energyRank.begin(), energyRank.end(), [](const std::pair<int, int>& a, const std::pair<int, int>& b){return a.first > b.first;} //we sort them in descending order of energy starting at the begining of thhe vector and stopping at the end
  );
int rows = this->terrainMap.size();
int cols = this->terrainMap[0].size();
int numberOfSearchers = (2 * this->ants.size()) / 3;    // We split the ants into searchers and collectors, this division of labour increases the efficiency of the colony
for (int i = 0; i < energyRank.size(); i++)
{
    if (i < numberOfSearchers)
    {
        searchers.push_back(energyRank[i].second);   
    }
    else
    {
        collectors.push_back(energyRank[i].second);
    }
}



// Get the food scanning radius and calculate the full scan width
int radius = this->ants[0].foodRadius;
int scanWidth = 2 * radius + 1;
int rowZones = (rows + scanWidth - 1) / scanWidth;
int colZones = (cols + scanWidth - 1) / scanWidth;

std::vector<int> scanRows;
std::vector<int> scanCols;

for (int i = 0; i < rowZones; i++)
{
    int start = i * rows / rowZones;
    int end = ((i + 1) * rows / rowZones) - 1;

    scanRows.push_back((start + end) / 2);
}

for (int i = 0; i < colZones; i++)
{
    int start = i * cols / colZones;
    int end = ((i + 1) * cols / colZones) - 1;

    scanCols.push_back((start + end) / 2);
}
for (int row : scanRows)
{
    for (int col : scanCols)
    {
        scanTargets.push_back({row, col});
    }
}

std::vector<std::pair<int, Coord>> targetCosts; 

//Now before starting the search we calculate the energy cost of each path so we assign each ant a scan position

for (int row : scanRows)
{
    for (int col : scanCols)
    {
        Coord target = {row, col};

        std::vector<Coord> path =
            shortestPath(this->terrainMap, this->homeCoordinates, target);

        int cost = calculatePathCost(this->terrainMap, path);

        targetCosts.push_back({cost, target});
    }
}

std::sort(
    targetCosts.begin(),
    targetCosts.end(),
    [](const std::pair<int, Coord>& a,
       const std::pair<int, Coord>& b)
    {
        return a.first > b.first;       //sort the paths the same way we sorted the ants
    }
);

for (int i = 0; i < searchers.size(); i++)
{
    int antIndex = searchers[i];

    int targetIndex = i % targetCosts.size();

    Coord target = targetCosts[targetIndex].second;

    this->ants[antIndex].move(
        this->terrainMap,
        target,
        this->foodMap
    );
    
    
    std::vector<Coord> foundFood =
    this->ants[antIndex].foodScan(this->foodMap);

    for (Coord food : foundFood)
{
    bool alreadyKnown = false;

    for (Coord known : knownFood)
    {
        if (food == known)
        {
            alreadyKnown = true;
            break;
        }
    }

    if (!alreadyKnown)
    {
        knownFood.push_back(food);
    }
}
}
std::cout << "\n--- AFTER INITIAL DEPLOYMENT ---\n";

for (int i = 0; i < this->ants.size(); i++)
{
    std::cout << "Ant " << i
              << " | Energy: " << this->ants[i].energy;

    bool isSearcher = false;

    for (int searcherIndex : searchers)
    {
        if (i == searcherIndex)
        {
            isSearcher = true;
            break;
        }
    }

    if (isSearcher)
        std::cout << " | SEARCHER";
    else
        std::cout << " | COLLECTOR";

    std::cout << std::endl;
}

std::cout << "Known food: "
          << knownFood.size() << std::endl;
initialCycle = false;
return;
}
//std::cout << "Known food after initial scan: "
  //        << knownFood.size() << std::endl;



forageCycle++;

if (forageCycle <= 5)
{
    std::cout << "\n--- FORAGE CYCLE " << forageCycle << " ---\n";
    std::cout << "Living ants: " << this->ants.size() << std::endl;
    std::cout << "Known food: " << knownFood.size() << std::endl;
    std::cout << "Score: " << this->score << std::endl;
}
//RETREIVAL SECTION POST INITIALIZATION 
std::vector<std::pair<int, int>> currentEnergyRank;

for (int i = 0; i < this->ants.size(); i++)
{
    currentEnergyRank.push_back(
        {this->ants[i].energy, i}
    );
}

std::sort(
    currentEnergyRank.begin(),
    currentEnergyRank.end(),
    [](const std::pair<int, int>& a,
       const std::pair<int, int>& b)
    {
        return a.first > b.first;
    }
);

for (const std::pair<int, int>& rankedAnt : currentEnergyRank)
{
    int antIndex = rankedAnt.second;
    Ant& ant = this->ants[antIndex];

    if (ant.carryingFood)
{
    ant.returnHome(
        this->terrainMap,
        this->foodMap
    );

    continue;
}
if (knownFood.empty())
{
    std::cout << "Known food available: "
          << knownFood.size() << std::endl;
    continue;
}
int bestFoodIndex = -1;
int bestCost = -1;
for (int i = 0; i < knownFood.size(); i++)
{
    Coord foodTarget = knownFood[i];

    std::vector<Coord> pathToFood =
        shortestPath(
            this->terrainMap,
            ant.position,
            foodTarget
        );

    int costToFood =
        calculatePathCost(
            this->terrainMap,
            pathToFood
        );

    std::vector<Coord> pathHome =
        shortestPath(
            this->terrainMap,
            foodTarget,
            this->homeCoordinates
        );

    int costHome =
        calculatePathCost(
            this->terrainMap,
            pathHome
        );

    int totalCost = costToFood + costHome;

    if (totalCost <= ant.energy)
    {
        if (bestFoodIndex == -1 || totalCost < bestCost)
        {
            bestFoodIndex = i;
            bestCost = totalCost;
        }
    }
}
if (bestFoodIndex != -1)
{
    Coord foodTarget = knownFood[bestFoodIndex];

    if (forageCycle <= 5)
    {
        std::cout << "Ant " << antIndex
                  << " energy=" << ant.energy
                  << " retrieving (" << foodTarget.first
                  << "," << foodTarget.second << ")"
                  << " bestCost=" << bestCost
                  << std::endl;
    }

    ant.move(
        this->terrainMap,
        foodTarget,
        this->foodMap
    );

    ant.returnHome(
        this->terrainMap,
        this->foodMap
    );

    knownFood.erase(
        knownFood.begin() + bestFoodIndex
    );
}
}
}

/** You may insert any custom functions below **/
