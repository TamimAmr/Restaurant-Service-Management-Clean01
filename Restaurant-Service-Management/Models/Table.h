#pragma once

#include <iostream>

using namespace std;

class Table
{
private:
    int ID;
    int capacity;
    int freeSeats;
    int busyOrders;
    bool sharingAllowed;

public:
    Table(int id = 0, int seatCount = 0)
        : ID(id), capacity(seatCount), freeSeats(seatCount), busyOrders(0), sharingAllowed(true)
    {
    }

    int GetID() const
    {
        return ID;
    }

    int GetCapacity() const
    {
        return capacity;
    }

    int GetSeats() const
    {
        return freeSeats;
    }

    int GetFreeSeats() const
    {
        return freeSeats;
    }

    int GetBusyOrders() const
    {
        return busyOrders;
    }

    bool IsFree() const
    {
        return busyOrders == 0;
    }

    bool CanShareMore() const
    {
        return busyOrders > 0 && sharingAllowed && freeSeats > 0;
    }

    void SetCapacity(int seatCount)
    {
        capacity = seatCount;
        freeSeats = seatCount;
        busyOrders = 0;
        sharingAllowed = true;
    }

    void SetSeats(int seatCount)
    {
        freeSeats = seatCount;
    }

    bool OccupySeats(int seatsNeeded, bool canShare)
    {
        if (seatsNeeded <= 0)
        {
            seatsNeeded = 1;
        }

        if (seatsNeeded > freeSeats)
        {
            return false;
        }

        freeSeats -= seatsNeeded;
        busyOrders++;

        if (!canShare)
        {
            sharingAllowed = false;
        }

        return true;
    }

    void ReleaseSeats(int seats)
    {
        if (seats <= 0)
        {
            seats = 1;
        }

        freeSeats += seats;

        if (freeSeats > capacity)
        {
            freeSeats = capacity;
        }

        if (busyOrders > 0)
        {
            busyOrders--;
        }

        if (busyOrders == 0)
        {
            freeSeats = capacity;
            sharingAllowed = true;
        }
    }

    void Print() const
    {
        cout << *this;
    }

    friend ostream& operator<<(ostream& out, const Table& table)
    {
        out << "T#" << table.ID << " Cap=" << table.capacity << " Free=" << table.freeSeats;
        return out;
    }
};
