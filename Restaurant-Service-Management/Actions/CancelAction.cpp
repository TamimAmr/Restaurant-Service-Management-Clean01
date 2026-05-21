#include "CancelAction.h"
#include "../Restaurant/Restaurant.h"

CancelAction::CancelAction(int time, int id) : Action(time, id)
{
}

void CancelAction::Act(Restaurant* pRest)
{
    if (!pRest)
    {
        return;
    }

    pRest->CancelOrder(orderID, GetActionTime());
}

void CancelAction::DescribeForUI(std::ostream& os) const
{
    os << "(X, " << GetActionTime() << ", " << GetOrderID() << ')';
}
