#pragma once

#include "priQueue.h"
#include "../Models/Table.h"

class Fit_Tables : public priQueue<Table*>
{
public:
    bool getBest(int neededSeats, Table*& bestTable);
    void Print() const;
};
