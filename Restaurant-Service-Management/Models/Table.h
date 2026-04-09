#pragma once

#include <iostream>

using namespace std;

class Table
{
private:
    int ID;
    int capacity;

public:
    Table(int id = 0, int seatCount = 0) : ID(id), capacity(seatCount) {}

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
        return capacity;
    }

    void SetCapacity(int seatCount)
    {
        capacity = seatCount;
    }

    void SetSeats(int seatCount)
    {
        capacity = seatCount;
    }

    void Print() const
    {
        cout << *this;
    }

    friend ostream& operator<<(ostream& out, const Table& table)
    {
        out << "T#" << table.ID << " Cap=" << table.capacity;
        return out;
    }
};
