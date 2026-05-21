#pragma once

#include <iostream>

using namespace std;

class Chef;
class Scooter;
class Table;

class Order
{
public:
    enum Type
    {
        TYPE_ODG,
        TYPE_ODN,
        TYPE_OT,
        TYPE_OVC,
        TYPE_OVG,
        TYPE_OVN,
        TYPE_OC
    };

private:
    int ID;
    int requestTimeStep;
    int size;
    double price;
    Type type;

    int requiredSeats;
    int duration;
    bool canShare;

    int distance;

    Chef* assignedChef;
    Scooter* assignedScooter;
    Table* assignedTable;
    Chef* assignedChefs[4];
    Scooter* assignedScooters[4];
    int assignedChefCount;
    int assignedScooterCount;
    int comboChefCount;
    int comboScooterCount;

    int assignmentTimeStep;
    int readyTimeStep;
    int inServiceTimeStep;
    int finishTimeStep;
    int cancelTimeStep;
    bool overwait;
    bool rescueUsed;

    static const char* TypeToString(Type orderType)
    {
        switch (orderType)
        {
        case TYPE_ODG: return "ODG";
        case TYPE_ODN: return "ODN";
        case TYPE_OT:  return "OT";
        case TYPE_OVC: return "OVC";
        case TYPE_OVG: return "OVG";
        case TYPE_OVN: return "OVN";
        case TYPE_OC:  return "OC";
        default:       return "UNKNOWN";
        }
    }

public:
    Order(int id = 0,
        int requestTimestep = 0,
        int orderSize = 0,
        double orderPrice = 0.0,
        Type orderType = TYPE_ODN,
        int seatsNeeded = 0,
        int orderDuration = 0,
        bool shareAllowed = false,
        int orderDistance = 0,
        int chefsNeeded = 1,
        int scootersNeeded = 1)
        : ID(id),
        requestTimeStep(requestTimestep),
        size(orderSize),
        price(orderPrice),
        type(orderType),
        requiredSeats(seatsNeeded),
        duration(orderDuration),
        canShare(shareAllowed),
        distance(orderDistance),
        assignedChef(nullptr),
        assignedScooter(nullptr),
        assignedTable(nullptr),
        assignedChefCount(0),
        assignedScooterCount(0),
        comboChefCount(chefsNeeded),
        comboScooterCount(scootersNeeded),
        assignmentTimeStep(-1),
        readyTimeStep(-1),
        inServiceTimeStep(-1),
        finishTimeStep(-1),
        cancelTimeStep(-1),
        overwait(false),
        rescueUsed(false)
    {
        for (int i = 0; i < 4; ++i)
        {
            assignedChefs[i] = nullptr;
            assignedScooters[i] = nullptr;
        }

        if (type == TYPE_OC)
        {
            if (comboChefCount < 2)
            {
                comboChefCount = 2;
            }

            if (comboChefCount > 4)
            {
                comboChefCount = 4;
            }

            if (comboScooterCount < 2)
            {
                comboScooterCount = 2;
            }

            if (comboScooterCount > 4)
            {
                comboScooterCount = 4;
            }
        }
        else
        {
            comboChefCount = 1;
            comboScooterCount = 1;
        }
    }

    int GetID() const
    {
        return ID;
    }

    int GetRequestTimeStep() const
    {
        return requestTimeStep;
    }

    int GetSize() const
    {
        return size;
    }

    double GetPrice() const
    {
        return price;
    }

    Type GetType() const
    {
        return type;
    }

    const char* GetTypeString() const
    {
        return TypeToString(type);
    }

    int GetRequiredSeats() const
    {
        return requiredSeats;
    }

    int GetDuration() const
    {
        return duration;
    }

    bool CanShare() const
    {
        return canShare;
    }

    int GetDistance() const
    {
        return distance;
    }

    bool IsDineIn() const
    {
        return type == TYPE_ODG || type == TYPE_ODN;
    }

    bool IsTakeaway() const
    {
        return type == TYPE_OT;
    }

    bool IsDelivery() const
    {
        return type == TYPE_OVC || type == TYPE_OVG || type == TYPE_OVN || type == TYPE_OC;
    }

    bool IsCombo() const
    {
        return type == TYPE_OC;
    }

    Chef* GetAssignedChef() const
    {
        return assignedChef;
    }

    Scooter* GetAssignedScooter() const
    {
        return assignedScooter;
    }

    Table* GetAssignedTable() const
    {
        return assignedTable;
    }

    int GetAssignedChefCount() const
    {
        return assignedChefCount;
    }

    int GetAssignedScooterCount() const
    {
        return assignedScooterCount;
    }

    Chef* GetAssignedChefAt(int index) const
    {
        if (index < 0 || index >= assignedChefCount)
        {
            return nullptr;
        }

        return assignedChefs[index];
    }

    Scooter* GetAssignedScooterAt(int index) const
    {
        if (index < 0 || index >= assignedScooterCount)
        {
            return nullptr;
        }

        return assignedScooters[index];
    }

    int GetComboChefCount() const
    {
        return comboChefCount;
    }

    int GetComboScooterCount() const
    {
        return comboScooterCount;
    }

    void SetAssignedChef(Chef* chef)
    {
        assignedChef = chef;
        assignedChefCount = 0;

        for (int i = 0; i < 4; ++i)
        {
            assignedChefs[i] = nullptr;
        }

        if (chef)
        {
            assignedChefs[0] = chef;
            assignedChefCount = 1;
        }
    }

    void SetAssignedScooter(Scooter* scooter)
    {
        assignedScooter = scooter;
        assignedScooterCount = 0;

        for (int i = 0; i < 4; ++i)
        {
            assignedScooters[i] = nullptr;
        }

        if (scooter)
        {
            assignedScooters[0] = scooter;
            assignedScooterCount = 1;
        }
    }

    void SetAssignedTable(Table* table)
    {
        assignedTable = table;
    }

    int GetAssignmentTimeStep() const
    {
        return assignmentTimeStep;
    }

    int GetReadyTimeStep() const
    {
        return readyTimeStep;
    }

    int GetInServiceTimeStep() const
    {
        return inServiceTimeStep;
    }

    int GetFinishTimeStep() const
    {
        return finishTimeStep;
    }

    int GetCancelTimeStep() const
    {
        return cancelTimeStep;
    }

    bool HasRescue() const
    {
        return rescueUsed;
    }

    bool IsOverwait() const
    {
        return overwait;
    }

    void SetAssignmentTimeStep(int t)
    {
        assignmentTimeStep = t;
    }

    void SetReadyTimeStep(int t)
    {
        readyTimeStep = t;
    }

    void SetInServiceTimeStep(int t)
    {
        inServiceTimeStep = t;
    }

    void SetFinishTimeStep(int t)
    {
        finishTimeStep = t;
    }

    void SetCancelTimeStep(int t)
    {
        cancelTimeStep = t;
    }

    void SetOverwait(bool value)
    {
        overwait = value;
    }

    void SetRescueUsed(bool value)
    {
        rescueUsed = value;
    }

    void ClearAssignedChefs()
    {
        assignedChef = nullptr;
        assignedChefCount = 0;

        for (int i = 0; i < 4; ++i)
        {
            assignedChefs[i] = nullptr;
        }
    }

    bool AddAssignedChef(Chef* chef)
    {
        if (!chef || assignedChefCount >= 4)
        {
            return false;
        }

        assignedChefs[assignedChefCount] = chef;
        assignedChefCount++;
        assignedChef = assignedChefs[0];
        return true;
    }

    void ClearAssignedScooters()
    {
        assignedScooter = nullptr;
        assignedScooterCount = 0;

        for (int i = 0; i < 4; ++i)
        {
            assignedScooters[i] = nullptr;
        }
    }

    bool AddAssignedScooter(Scooter* scooter)
    {
        if (!scooter || assignedScooterCount >= 4)
        {
            return false;
        }

        assignedScooters[assignedScooterCount] = scooter;
        assignedScooterCount++;
        assignedScooter = assignedScooters[0];
        return true;
    }

    void SetAssignedScooterAt(int index, Scooter* scooter)
    {
        if (index < 0 || index >= assignedScooterCount)
        {
            return;
        }

        assignedScooters[index] = scooter;
        assignedScooter = assignedScooters[0];
    }

    void Print() const
    {
        cout << *this;
    }

    friend ostream& operator<<(ostream& out, const Order& order)
    {
        out << order.GetTypeString() << "#" << order.ID
            << " T=" << order.requestTimeStep
            << " Size=" << order.size
            << " Price=" << order.price;

        if (order.IsDineIn())
        {
            out << " Seats=" << order.requiredSeats
                << " Duration=" << order.duration
                << " Share=" << (order.canShare ? "Y" : "N");
        }
        else if (order.IsDelivery())
        {
            out << " Dist=" << order.distance;

            if (order.IsCombo())
            {
                out << " Chefs=" << order.comboChefCount
                    << " Scooters=" << order.comboScooterCount;
            }
        }

        return out;
    }
};
