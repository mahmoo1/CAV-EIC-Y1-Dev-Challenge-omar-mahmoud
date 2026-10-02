//
// Created by dusan on 9/15/26.
//

#include "../include/antworld.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream>

/**
 * @brief Applicant solution for coordinating the ant colony.
 *
 * The strategy uses an initial exploration phase to discover food,
 * followed by energy-efficient food retrieval. If no food can be
 * retrieved directly, a relay strategy is used to move food closer
 * to home.
 */

static bool initialCycle = true;
static bool pendingRelay = false;

static std::vector<int> searchers;
static std::vector<int> collectors;
static std::vector<Coord> knownFood;

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


// -----------------------------------------------------------------------------
// Helper function declarations
// -----------------------------------------------------------------------------

static bool containsFood(
    const std::vector<Coord>& foodList,
    Coord target);

static int pathCost(
    const MapTemplate& terrainMap,
    Coord start,
    Coord destination);

static bool canRelayCleanly(
    const MapTemplate& terrainMap,
    Coord foodTarget,
    Coord home,
    int energyAfterPickup);


// -----------------------------------------------------------------------------
// Main foraging algorithm
// -----------------------------------------------------------------------------

void AntWorld::forage()
{
    // No work can be performed if the colony has no remaining ants.
    if (this->ants.empty())
    {
        return;
    }


    // =========================================================================
    // STAGE 1: INITIAL EXPLORATION
    // =========================================================================

    if (initialCycle)
    {
        // Rank ants from highest to lowest energy while preserving
        // their original indices.
        std::vector<std::pair<int, int>> energyRank;

        for (int i = 0; i < static_cast<int>(this->ants.size()); i++)
        {
            energyRank.push_back({this->ants[i].energy, i});
        }

        std::sort(
            energyRank.begin(),
            energyRank.end(),
            [](const std::pair<int, int>& a,
               const std::pair<int, int>& b)
            {
                return a.first > b.first;
            });


        // Divide the colony evenly between searchers and collectors.
        // The highest-energy ants are selected as searchers.
        int numberOfSearchers =
            static_cast<int>(this->ants.size()) / 2;

        for (int i = 0; i < static_cast<int>(energyRank.size()); i++)
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


        // Divide the map into scanning regions based on the ants'
        // food detection radius.
        int rows = static_cast<int>(this->terrainMap.size());
        int cols = static_cast<int>(this->terrainMap[0].size());

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


        // Calculate the energy cost of reaching each scanning location.
        std::vector<std::pair<int, Coord>> targetCosts;

        for (int row : scanRows)
        {
            for (int col : scanCols)
            {
                Coord target = {row, col};

                int cost = pathCost(
                    this->terrainMap,
                    this->homeCoordinates,
                    target);

                targetCosts.push_back({cost, target});
            }
        }


        // Prioritize inexpensive scanning locations so searchers preserve
        // as much energy as possible for later food retrieval.
        std::sort(
            targetCosts.begin(),
            targetCosts.end(),
            [](const std::pair<int, Coord>& a,
               const std::pair<int, Coord>& b)
            {
                return a.first < b.first;
            });


        if (targetCosts.empty())
        {
            initialCycle = false;
            return;
        }


        // Send each searcher to a scanning location and record all food
        // discovered within its detection radius.
        for (int i = 0; i < static_cast<int>(searchers.size()); i++)
        {
            int antIndex = searchers[i];
            int targetIndex =
                i % static_cast<int>(targetCosts.size());

            Coord target = targetCosts[targetIndex].second;

            this->ants[antIndex].move(
                this->terrainMap,
                target,
                this->foodMap);

            std::vector<Coord> foundFood =
                this->ants[antIndex].foodScan(this->foodMap);

            for (Coord food : foundFood)
            {
                if (!containsFood(knownFood, food))
                {
                    knownFood.push_back(food);
                }
            }
        }

        initialCycle = false;
        return;
    }


    // =========================================================================
    // STAGE 2: PROCESS A PREVIOUS RELAY
    // =========================================================================

    // A relay ant with zero energy is removed by updateWorld(), which drops
    // its carried food at its final position. Add that new location back to
    // the colony's known food list.
    if (pendingRelay)
    {
        if (!containsFood(knownFood, pendingRelayFood))
        {
            knownFood.push_back(pendingRelayFood);
        }

        pendingRelay = false;
    }


    // =========================================================================
    // STAGE 3: RETURN FOOD ALREADY BEING CARRIED
    // =========================================================================

    // Before assigning new work, allow any ant already carrying food to
    // continue toward home.
    for (Ant& ant : this->ants)
    {
        if (ant.carryingFood)
        {
            ant.returnHome(
                this->terrainMap,
                this->foodMap);
        }
    }


    // =========================================================================
    // STAGE 4: BUILD ALL FEASIBLE DIRECT RETRIEVALS
    // =========================================================================

    std::vector<FoodAssignment> possibleAssignments;

    for (int antIndex = 0;
         antIndex < static_cast<int>(this->ants.size());
         antIndex++)
    {
        Ant& ant = this->ants[antIndex];

        // An ant can only carry one food item at a time.
        if (ant.carryingFood)
        {
            continue;
        }

        for (int foodIndex = 0;
             foodIndex < static_cast<int>(knownFood.size());
             foodIndex++)
        {
            Coord foodTarget = knownFood[foodIndex];

            int costToFood = pathCost(
                this->terrainMap,
                ant.position,
                foodTarget);

            int costHome = pathCost(
                this->terrainMap,
                foodTarget,
                this->homeCoordinates);

            int totalCost = costToFood + costHome;

            // Only consider assignments where the ant has enough energy
            // to collect the food and complete the return trip.
            if (ant.energy >= totalCost)
            {
                possibleAssignments.push_back(
                    {antIndex, foodIndex, totalCost});
            }
        }
    }


    // =========================================================================
    // STAGE 5: GLOBAL GREEDY ASSIGNMENT
    // =========================================================================

    // Prioritize the lowest-energy retrievals across the entire colony.
    std::sort(
        possibleAssignments.begin(),
        possibleAssignments.end(),
        [](const FoodAssignment& a,
           const FoodAssignment& b)
        {
            return a.cost < b.cost;
        });

    std::vector<bool> antAssigned(
        this->ants.size(),
        false);

    std::vector<bool> foodAssigned(
        knownFood.size(),
        false);

    std::vector<FoodAssignment> selectedAssignments;


    // Greedily select assignments while ensuring that each ant and each
    // food location can only be selected once during the cycle.
    for (const FoodAssignment& assignment : possibleAssignments)
    {
        int antIndex = assignment.antIndex;
        int foodIndex = assignment.foodIndex;

        if (!antAssigned[antIndex] &&
            !foodAssigned[foodIndex])
        {
            selectedAssignments.push_back(assignment);

            antAssigned[antIndex] = true;
            foodAssigned[foodIndex] = true;
        }
    }


    // =========================================================================
    // STAGE 6: EXECUTE DIRECT RETRIEVALS
    // =========================================================================

    for (const FoodAssignment& assignment : selectedAssignments)
    {
        Ant& ant =
            this->ants[assignment.antIndex];

        Coord foodTarget =
            knownFood[assignment.foodIndex];

        ant.move(
            this->terrainMap,
            foodTarget,
            this->foodMap);

        ant.returnHome(
            this->terrainMap,
            this->foodMap);
    }


    // =========================================================================
    // STAGE 7: RELAY MODE
    // =========================================================================

    // Relay mode is only necessary when no food can be collected and
    // returned directly during this cycle.
    if (selectedAssignments.empty() && !knownFood.empty())
    {
        std::vector<RelayAssignment> relayOptions;

        for (int antIndex = 0;
             antIndex < static_cast<int>(this->ants.size());
             antIndex++)
        {
            Ant& ant = this->ants[antIndex];

            if (ant.carryingFood)
            {
                continue;
            }

            for (int foodIndex = 0;
                 foodIndex < static_cast<int>(knownFood.size());
                 foodIndex++)
            {
                Coord foodTarget = knownFood[foodIndex];

                int costToFood = pathCost(
                    this->terrainMap,
                    ant.position,
                    foodTarget);

                // A relay candidate only needs enough energy to reach
                // and pick up the food.
                if (costToFood > ant.energy)
                {
                    continue;
                }

                int energyAfterPickup =
                    ant.energy - costToFood;


                // Only use a relay if its remaining energy can be exhausted
                // exactly while travelling toward home. This prevents an ant
                // from becoming permanently stranded while carrying food.
                if (!canRelayCleanly(
                        this->terrainMap,
                        foodTarget,
                        this->homeCoordinates,
                        energyAfterPickup))
                {
                    continue;
                }

                relayOptions.push_back(
                    {
                        antIndex,
                        foodIndex,
                        costToFood,
                        energyAfterPickup
                    });
            }
        }


        // Prefer the relay that retains the greatest amount of energy
        // after reaching the food, allowing it to carry the food farther
        // toward home before dropping it.
        std::sort(
            relayOptions.begin(),
            relayOptions.end(),
            [](const RelayAssignment& a,
               const RelayAssignment& b)
            {
                return a.energyAfterPickup >
                       b.energyAfterPickup;
            });


        if (!relayOptions.empty())
        {
            RelayAssignment relay = relayOptions.front();

            Ant& ant = this->ants[relay.antIndex];
            Coord foodTarget = knownFood[relay.foodIndex];

            ant.move(
                this->terrainMap,
                foodTarget,
                this->foodMap);

            Coord finalPosition =
                ant.returnHome(
                    this->terrainMap,
                    this->foodMap);


            // updateWorld() will remove an ant at zero energy and place
            // its carried food onto foodMap at its final coordinate.
            if (ant.energy == 0)
            {
                pendingRelayFood = finalPosition;
                pendingRelay = true;

                knownFood.erase(
                    knownFood.begin() + relay.foodIndex);
            }
        }
    }


    // =========================================================================
    // STAGE 8: UPDATE KNOWN FOOD
    // =========================================================================

    // Remove food locations assigned during this cycle. Iterate backwards
    // so erasing elements does not invalidate the remaining indices.
    for (int i = static_cast<int>(foodAssigned.size()) - 1;
         i >= 0;
         i--)
    {
        if (foodAssigned[i])
        {
            knownFood.erase(
                knownFood.begin() + i);
        }
    }
}


// =============================================================================
// Helper functions
// =============================================================================

/**
 * @brief Checks whether a food coordinate is already known to the colony.
 */
static bool containsFood(
    const std::vector<Coord>& foodList,
    Coord target)
{
    for (Coord food : foodList)
    {
        if (food == target)
        {
            return true;
        }
    }

    return false;
}


/**
 * @brief Calculates the shortest-path energy cost between two coordinates.
 */
static int pathCost(
    const MapTemplate& terrainMap,
    Coord start,
    Coord destination)
{
    std::vector<Coord> path =
        shortestPath(
            terrainMap,
            start,
            destination);

    return calculatePathCost(
        terrainMap,
        path);
}


/**
 * @brief Determines whether a relay ant will exhaust its remaining energy
 *        exactly while travelling from the food toward home.
 *
 * A relay is only useful if the ant eventually reaches zero energy. When
 * updateWorld() removes that ant, the carried food is dropped at its final
 * position so another ant can retrieve it during a later forage cycle.
 */
static bool canRelayCleanly(
    const MapTemplate& terrainMap,
    Coord foodTarget,
    Coord home,
    int energyAfterPickup)
{
    std::vector<Coord> path =
        shortestPath(
            terrainMap,
            foodTarget,
            home);

    int remainingEnergy = energyAfterPickup;

    for (int i = 1;
         i < static_cast<int>(path.size());
         i++)
    {
        auto [r1, c1] = path[i - 1];
        auto [r2, c2] = path[i];

        int stepCost =
            1 + std::abs(
                terrainMap[r1][c1] -
                terrainMap[r2][c2]);

        if (stepCost > remainingEnergy)
        {
            return false;
        }

        remainingEnergy -= stepCost;

        if (remainingEnergy == 0)
        {
            return true;
        }
    }

    return false;
}