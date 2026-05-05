#include <iostream>
#include <limits>

#include "../Restaurant/Restaurant.h"

using namespace std;

int main()
{
    cout << "Select program mode: (1) Interactive  (2) Silent: ";

    int mode = 2;
    cin >> mode;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    const bool interactive = (mode == 1);

    if (!interactive)
    {
        cout << "Simulation Starts in Silent mode .." << endl;
    }

    Restaurant restaurant;

    char inputFilePath[260] = "";
    char outputFilePath[260] = "";

    cout << "Enter input file path: ";
    cin.getline(inputFilePath, 260);

    cout << "Enter output file path: ";
    cin.getline(outputFilePath, 260);

    if (outputFilePath[0] == '\0')
    {
        restaurant.RunSimulation(inputFilePath, "phase2_output.txt", interactive);
    }
    else
    {
        restaurant.RunSimulation(inputFilePath, outputFilePath, interactive);
    }

    if (!interactive)
    {
        cout << "Simulation ends, Output file created" << endl;
    }

    return 0;
}
