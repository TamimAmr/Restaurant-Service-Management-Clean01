#include "../Restaurant/Restaurant.h"
#include "../UI/UI.h"

int main()
{
    UI ui;
    int mode = ui.ReadMode();
    const bool interactive = (mode == 1);

    if (!interactive)
    {
        ui.PrintSilentStart();
    }

    Restaurant restaurant;

    char inputFilePath[260] = "";
    char outputFilePath[260] = "";

    ui.ReadFilePaths(inputFilePath, outputFilePath, 260);

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
        ui.PrintSilentEnd();
    }

    return 0;
}
