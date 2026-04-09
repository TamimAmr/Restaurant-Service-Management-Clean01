#pragma once

#include <iostream>

class Restaurant;

class Action
{
protected:
    int actionTime;
    int orderID;

public:
    Action(int time = 0, int id = 0);
    virtual ~Action() {}

    int GetActionTime() const;
    int GetOrderID() const;

    virtual void Act(Restaurant* pRest) = 0;

    virtual void DescribeForUI(std::ostream& os) const;
};
