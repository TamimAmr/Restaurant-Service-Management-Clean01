#include "RequestAction.h"
#include "../Restaurant/Restaurant.h"

RequestAction::RequestAction(int time,
    int id,
    int orderSize,
    double orderPrice,
    Order::Type type,
    int neededSeats,
    int orderDuration,
    bool shareAllowed,
    int orderDistance,
    int chefsNeeded,
    int scootersNeeded)
    : Action(time, id),
    size(orderSize),
    price(orderPrice),
    orderType(type),
    seats(neededSeats),
    duration(orderDuration),
    canShare(shareAllowed),
    distance(orderDistance),
    comboChefs(chefsNeeded),
    comboScooters(scootersNeeded)
{
}

void RequestAction::Act(Restaurant* pRest)
{
    if (!pRest)
    {
        return;
    }

    Order* newOrder = new Order(orderID, actionTime, size, price, orderType, seats, duration, canShare, distance, comboChefs, comboScooters);
    pRest->AddOrderToPendingList(newOrder);
}

namespace
{
    const char* OrderTypeUiName(Order::Type t)
    {
        switch (t)
        {
        case Order::TYPE_ODG: return "ODG";
        case Order::TYPE_ODN: return "ODN";
        case Order::TYPE_OT: return "OT";
        case Order::TYPE_OVC: return "OVC";
        case Order::TYPE_OVG: return "OVG";
        case Order::TYPE_OVN: return "OVN";
        case Order::TYPE_OC: return "OC";
        default: return "?";
        }
    }
}

void RequestAction::DescribeForUI(std::ostream& os) const
{
    os << '[' << OrderTypeUiName(orderType) << ", " << actionTime << ", " << orderID << ']';
}
