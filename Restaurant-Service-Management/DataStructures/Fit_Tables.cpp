#include "Fit_Tables.h"

bool Fit_Tables::getBest(int neededSeats, Table*& bestTable)
{
    priQueue<Table*> temp;
    Table* currentTable = nullptr;
    Table* selectedTable = nullptr;
    int currentPriority = 0;
    int selectedPriority = 0;

    bestTable = nullptr;

    while (dequeue(currentTable, currentPriority))
    {
        if (currentTable->GetSeats() >= neededSeats)
        {
            if (!selectedTable || currentTable->GetSeats() < selectedTable->GetSeats())
            {
                if (selectedTable)
                {
                    temp.enqueue(selectedTable, selectedPriority);
                }

                selectedTable = currentTable;
                selectedPriority = currentPriority;
            }
            else
            {
                temp.enqueue(currentTable, currentPriority);
            }
        }
        else
        {
            temp.enqueue(currentTable, currentPriority);
        }
    }

    while (!temp.isEmpty())
    {
        temp.dequeue(currentTable, currentPriority);
        enqueue(currentTable, currentPriority);
    }

    bestTable = selectedTable;
    return (bestTable != nullptr);
}

void Fit_Tables::Print() const
{
    cout << "FIT_TABLES: ";
    priQueue<Table*>::Print();
    cout << endl;
}
