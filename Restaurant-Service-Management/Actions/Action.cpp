#include "Action.h"

Action::Action(int time, int id) : actionTime(time), orderID(id)
{
}

int Action::GetActionTime() const
{
    return actionTime;
}

int Action::GetOrderID() const
{
    return orderID;
}
