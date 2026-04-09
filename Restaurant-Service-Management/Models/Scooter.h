#pragma once

#include <iostream>

using namespace std;

class Scooter
{
private:
    int ID;
    int speed;
    int maintenanceDuration;
    int assignedOrdersCount;

public:
    Scooter(int id = 0, int scooterSpeed = 0, int mainDur = 0)
        : ID(id), speed(scooterSpeed), maintenanceDuration(mainDur), assignedOrdersCount(0)
    {
    }

    int GetID() const
    {
        return ID;
    }

    int GetSpeed() const
    {
        return speed;
    }

    int GetMaintenanceDuration() const
    {
        return maintenanceDuration;
    }

    int GetAssignedOrdersCount() const
    {
        return assignedOrdersCount;
    }

    void IncrementAssignedOrders()
    {
        assignedOrdersCount++;
    }

    void ResetAssignedOrders()
    {
        assignedOrdersCount = 0;
    }

    bool NeedsMaintenance(int mainOrdersLimit) const
    {
        return assignedOrdersCount >= mainOrdersLimit;
    }

    void Print() const
    {
        cout << *this;
    }

    friend ostream& operator<<(ostream& out, const Scooter& scooter)
    {
        out << "S#" << scooter.ID
            << " Speed=" << scooter.speed
            << " MainDur=" << scooter.maintenanceDuration
            << " Orders=" << scooter.assignedOrdersCount;
        return out;
    }
};
