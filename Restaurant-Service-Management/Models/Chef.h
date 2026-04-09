#pragma once

#include <iostream>

using namespace std;

class Chef
{
public:
    enum Type
    {
        TYPE_CS,
        TYPE_CN
    };

private:
    int ID;
    Type type;
    int speed;

    static const char* TypeToString(Type chefType)
    {
        switch (chefType)
        {
        case TYPE_CS: return "CS";
        case TYPE_CN: return "CN";
        default:      return "UNKNOWN";
        }
    }

public:
    Chef(int id = 0, Type chefType = TYPE_CN, int chefSpeed = 0)
        : ID(id), type(chefType), speed(chefSpeed)
    {
    }

    int GetID() const
    {
        return ID;
    }

    Type GetType() const
    {
        return type;
    }

    const char* GetTypeString() const
    {
        return TypeToString(type);
    }

    int GetSpeed() const
    {
        return speed;
    }

    bool IsSpecial() const
    {
        return type == TYPE_CS;
    }

    bool IsNormal() const
    {
        return type == TYPE_CN;
    }

    void Print() const
    {
        cout << *this;
    }

    friend ostream& operator<<(ostream& out, const Chef& chef)
    {
        out << chef.GetTypeString() << "#" << chef.ID
            << " Speed=" << chef.speed;
        return out;
    }
};
