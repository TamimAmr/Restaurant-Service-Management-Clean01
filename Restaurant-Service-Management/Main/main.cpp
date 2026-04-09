#include <ctime>
#include <iostream>
#include <limits>

#include "../Restaurant/Restaurant.h"

using namespace std;

void RunPhase1RandomSimulation(Restaurant& restaurant, unsigned randomSeed, bool interactiveMode, bool useActionDrivenInput, const char* outputFilePath)
{
    restaurant.RunPhase1RandomSimulation(randomSeed, interactiveMode, useActionDrivenInput, outputFilePath);
}

int main()
{
    cout << "Select program mode: (1) Interactive  (2) Silent: ";

    int mode = 2;
    cin >> mode;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    const bool interactive = (mode == 1);
    cout << "Use ACTIONS_LIST driven input? (1 = ON, 0 = OFF): ";
    int useActionsInput = 1;
    cin >> useActionsInput;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (!interactive)
    {
        cout << "Simulation Starts in Silent mode .." << endl;
    }

    Restaurant restaurant;

    const unsigned seed = static_cast<unsigned>(time(nullptr));

    if (interactive)
    {
        cout << "Random seed: " << seed << endl;
    }

    RunPhase1RandomSimulation(restaurant, seed, interactive, useActionsInput != 0, "phase2_output.txt");

    if (!interactive)
    {
        cout << "Simulation ends, Output file created" << endl;
    }

    return 0;
}
