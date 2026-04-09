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
        TYPE_OVN
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

    int assignmentTimeStep;
    int readyTimeStep;
    int inServiceTimeStep;
    int finishTimeStep;
    int cancelTimeStep;

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
        int orderDistance = 0)
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
        assignmentTimeStep(-1),
        readyTimeStep(-1),
        inServiceTimeStep(-1),
        finishTimeStep(-1),
        cancelTimeStep(-1)
    {
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
        return type == TYPE_OVC || type == TYPE_OVG || type == TYPE_OVN;
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

    void SetAssignedChef(Chef* chef)
    {
        assignedChef = chef;
    }

    void SetAssignedScooter(Scooter* scooter)
    {
        assignedScooter = scooter;
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
        }

        return out;
    }
};
