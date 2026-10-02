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

static std::vector<Coord> knownFood;
static int forageCycle = 0;
static bool pendingRelay = false;
static Coord pendingRelayFood = {-1, -1};

struct FoodAssignment
{
    int antIndex;
    int foodIndex;
    int cost;
};
struct RelayAssignment
{
    int antIndex;
    int foodIndex;
    int costToFood;
    int energyAfterPickup;
};


void AntWorld::forage() {

if (this->ants.empty())
{
    return;
}

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

if (targetCosts.empty())
{
    initialCycle = false;
    return;
}

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

if (forageCycle <= 10)
{
    std::cout << "\n--- FORAGE CYCLE " << forageCycle << " ---\n";
    std::cout << "Living ants: " << this->ants.size() << std::endl;
    std::cout << "Known food: " << knownFood.size() << std::endl;
    std::cout << "Score: " << this->score << std::endl;
}
//RETREIVAL SECTION POST INITIALIZATION 

if (pendingRelay)
{
    std::cout << "Adding relayed food at ("
              << pendingRelayFood.first << ","
              << pendingRelayFood.second << ")"
              << std::endl;
    bool alreadyKnown = false;

    for (Coord food : knownFood)
    {
        if (food == pendingRelayFood)
        {
            alreadyKnown = true;
            break;
        }
    }

    if (!alreadyKnown)
    {
        knownFood.push_back(pendingRelayFood);
    }

    pendingRelay = false;
    // Any ant already carrying food should prioritize returning it home
for (int i = 0; i < this->ants.size(); i++)
{
    if (this->ants[i].carryingFood)
    {
        this->ants[i].returnHome(
            this->terrainMap,
            this->foodMap
        );
    }
}
    std::cout << "Known food after relay update: "
          << knownFood.size() << std::endl;
}


std::vector<FoodAssignment> possibleAssignments;

for (int antIndex = 0; antIndex < this->ants.size(); antIndex++)
{
    Ant& ant = this->ants[antIndex];
    if (ant.carryingFood)
    {
    continue;
    }

    for (int foodIndex = 0; foodIndex < knownFood.size(); foodIndex++)
    {
        Coord foodTarget = knownFood[foodIndex];

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

        if (ant.energy >= totalCost)
        {
            possibleAssignments.push_back(
                {antIndex, foodIndex, totalCost}
            );
        }
    }
}
std::sort(
    possibleAssignments.begin(),
    possibleAssignments.end(),
    [](const FoodAssignment& a, const FoodAssignment& b)
    {
        return a.cost < b.cost;
    }
);

std::vector<bool> antAssigned(this->ants.size(), false);
std::vector<bool> foodAssigned(knownFood.size(), false);

std::vector<FoodAssignment> selectedAssignments;

for (const FoodAssignment& assignment : possibleAssignments)
{
    int antIndex = assignment.antIndex;
    int foodIndex = assignment.foodIndex;

    if (!antAssigned[antIndex] && !foodAssigned[foodIndex])
    {
        selectedAssignments.push_back(assignment);

        antAssigned[antIndex] = true;
        foodAssigned[foodIndex] = true;
    }
}

for (const FoodAssignment& assignment : selectedAssignments)
{
    int antIndex = assignment.antIndex;
    int foodIndex = assignment.foodIndex;

    Ant& ant = this->ants[antIndex];
    Coord foodTarget = knownFood[foodIndex];

    if (forageCycle <= 10)
    {
        std::cout << "Ant " << antIndex
                  << " energy=" << ant.energy
                  << " retrieving (" << foodTarget.first
                  << "," << foodTarget.second << ")"
                  << " globalCost=" << assignment.cost
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
}

if (selectedAssignments.empty() && !knownFood.empty())
{
    std::vector<RelayAssignment> relayOptions;
        for (int antIndex = 0; antIndex < this->ants.size(); antIndex++)
    {
        Ant& ant = this->ants[antIndex];

        if (ant.carryingFood)
        {
            continue;
        }

        for (int foodIndex = 0; foodIndex < knownFood.size(); foodIndex++)
        {
            Coord foodTarget = knownFood[foodIndex];

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

            // The ant must at least be able to REACH the food.
            if (costToFood <= ant.energy)
            {
                int energyAfterPickup =
                    ant.energy - costToFood;

                // Must have energy left to actually move the food.
                if (energyAfterPickup > 0)
                {
                    relayOptions.push_back(
                        {
                            antIndex,
                            foodIndex,
                            costToFood,
                            energyAfterPickup
                        }
                    );
                }
            }
        }
    }

    if (relayOptions.empty() && forageCycle <= 10)
{
    std::cout << "\n--- NO RELAY POSSIBLE ---\n";

    for (int antIndex = 0; antIndex < this->ants.size(); antIndex++)
    {
        Ant& ant = this->ants[antIndex];

        int cheapestFoodCost = -1;
        Coord cheapestFood = {-1, -1};

        for (int foodIndex = 0; foodIndex < knownFood.size(); foodIndex++)
        {
            std::vector<Coord> path =
                shortestPath(
                    this->terrainMap,
                    ant.position,
                    knownFood[foodIndex]
                );

            int cost =
                calculatePathCost(
                    this->terrainMap,
                    path
                );

            if (cheapestFoodCost == -1 || cost < cheapestFoodCost)
            {
                cheapestFoodCost = cost;
                cheapestFood = knownFood[foodIndex];
            }
        }

        std::cout << "Ant " << antIndex
                  << " | energy=" << ant.energy
                  << " | cheapest reachable food cost="
                  << cheapestFoodCost
                  << " at (" << cheapestFood.first
                  << "," << cheapestFood.second << ")"
                  << " | carryingFood=" << ant.carryingFood
                  << std::endl;
    }
}
    std::sort(
        relayOptions.begin(),
        relayOptions.end(),
        [](const RelayAssignment& a,
           const RelayAssignment& b)
        {
            return a.energyAfterPickup > b.energyAfterPickup;
        }
    );

    if (!relayOptions.empty())
    {
        RelayAssignment relay = relayOptions[0];

        Ant& ant = this->ants[relay.antIndex];
        Coord foodTarget = knownFood[relay.foodIndex];

        std::cout << "\n--- RELAY MODE ---\n";
        std::cout << "Sacrificing Ant " << relay.antIndex
                  << " | Energy: " << ant.energy
                  << " | Target: (" << foodTarget.first
                  << "," << foodTarget.second << ")"
                  << " | Cost to food: " << relay.costToFood
                  << " | Carry energy: " << relay.energyAfterPickup
                  << std::endl;

        ant.move(
            this->terrainMap,
            foodTarget,
            this->foodMap
        );

        Coord finalPosition =
            ant.returnHome(
                this->terrainMap,
                this->foodMap
            );
        pendingRelayFood = finalPosition;
        pendingRelay = true;

        std::cout << "Relay ended at: ("
                  << finalPosition.first << ","
                  << finalPosition.second << ")"
                  << std::endl;

        knownFood.erase(
            knownFood.begin() + relay.foodIndex
        );
    }
}


for (int i = static_cast<int>(foodAssigned.size()) - 1; i >= 0; i--)
{
    if (foodAssigned[i])
    {
        knownFood.erase(knownFood.begin() + i);
    }
}


}


/** You may insert any custom functions below **/
