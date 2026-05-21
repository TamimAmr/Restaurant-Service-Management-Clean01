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
    int totalDistance;
    int returnTimeStep;
    int maintenanceFinishTimeStep;
    bool broken;

public:
    Scooter(int id = 0, int scooterSpeed = 0, int mainDur = 0)
        : ID(id),
        speed(scooterSpeed),
        maintenanceDuration(mainDur),
        assignedOrdersCount(0),
        totalDistance(0),
        returnTimeStep(0),
        maintenanceFinishTimeStep(0),
        broken(false)
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

    int GetTotalDistance() const
    {
        return totalDistance;
    }

    int GetReturnTimeStep() const
    {
        return returnTimeStep;
    }

    int GetMaintenanceFinishTimeStep() const
    {
        return maintenanceFinishTimeStep;
    }

    bool IsBroken() const
    {
        return broken;
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
        return mainOrdersLimit > 0 && assignedOrdersCount >= mainOrdersLimit;
    }

    void AddDistance(int distance)
    {
        totalDistance += distance;
    }

    void SetReturnTimeStep(int t)
    {
        returnTimeStep = t;
    }

    void SetMaintenanceFinishTimeStep(int t)
    {
        maintenanceFinishTimeStep = t;
    }

    void SetBroken(bool value)
    {
        broken = value;
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
            << " Orders=" << scooter.assignedOrdersCount
            << " Dist=" << scooter.totalDistance;
        return out;
    }
};
