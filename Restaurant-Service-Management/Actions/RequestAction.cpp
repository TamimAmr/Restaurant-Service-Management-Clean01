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
    int orderDistance)
    : Action(time, id),
    size(orderSize),
    price(orderPrice),
    orderType(type),
    seats(neededSeats),
    duration(orderDuration),
    canShare(shareAllowed),
    distance(orderDistance)
{
}

void RequestAction::Act(Restaurant* pRest)
{
    if (!pRest)
    {
        return;
    }

    Order* newOrder = new Order(orderID, actionTime, size, price, orderType, seats, duration, canShare, distance);
    pRest->AddOrderToPendingList(newOrder);
}
