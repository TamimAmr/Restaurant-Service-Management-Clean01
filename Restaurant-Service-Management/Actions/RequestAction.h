#pragma once

#include "Action.h"
#include "../Models/Order.h"

class RequestAction : public Action
{
private:
    int size;
    double price;
    Order::Type orderType;
    int seats;
    int duration;
    bool canShare;
    int distance;

public:
    RequestAction(int time,
        int id,
        int orderSize,
        double orderPrice,
        Order::Type type,
        int neededSeats = 0,
        int orderDuration = 0,
        bool shareAllowed = false,
        int orderDistance = 0);

    void Act(Restaurant* pRest);

    void DescribeForUI(std::ostream& os) const override;
};
