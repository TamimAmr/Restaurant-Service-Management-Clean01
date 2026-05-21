#include "UI.h"

#include <cstring>
#include <iostream>
#include <limits>

using namespace std;

int UI::ReadMode()
{
    cout << "Select program mode: (1) Interactive  (2) Silent: ";

    int mode = 2;
    cin >> mode;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    return mode;
}

void UI::ReadFilePaths(char inputFilePath[], char outputFilePath[], int maxSize)
{
    cout << "Enter input file path: ";
    cin.getline(inputFilePath, maxSize);
    CleanFilePath(inputFilePath);

    cout << "Enter output file path: ";
    cin.getline(outputFilePath, maxSize);
    CleanFilePath(outputFilePath);
}

void UI::PrintSilentStart()
{
    cout << "Simulation Starts in Silent mode .." << endl;
}

void UI::PrintSilentEnd()
{
    cout << "Simulation ends, Output file created" << endl;
}

void UI::CleanFilePath(char path[])
{
    int len = static_cast<int>(strlen(path));

    while (len > 0 && (path[len - 1] == ' ' || path[len - 1] == '\t' || path[len - 1] == '"'))
    {
        path[len - 1] = '\0';
        len--;
    }

    int start = 0;

    while (path[start] == ' ' || path[start] == '\t' || path[start] == '"')
    {
        start++;
    }

    if (start > 0)
    {
        int i = 0;

        while (path[start] != '\0')
        {
            path[i] = path[start];
            i++;
            start++;
        }

        path[i] = '\0';
    }
}
