#include "Restaurant.h"

#include <iostream>
using namespace std;

namespace
{
    bool CancelCookingOV(priQueue<Order*>& cookingOrders, int id, Order*& removedOrder)
    {
        priQueue<Order*> temp;
        Order* current = nullptr;
        removedOrder = nullptr;
        bool found = false;
        int pri = 0;

        while (cookingOrders.dequeue(current, pri))
        {
            if (!found && current->GetID() == id && current->IsDelivery())
            {
                removedOrder = current;
                found = true;
            }
            else
            {
                temp.enqueue(current, pri);
            }
        }

        while (!temp.isEmpty())
        {
            temp.dequeue(current, pri);
            cookingOrders.enqueue(current, pri);
        }

        return found;
    }
}

Restaurant::Restaurant()
{
}

void Restaurant::AddAction(Action* pAction)
{
    ACTIONS_LIST.enqueue(pAction);
}

bool Restaurant::ExecuteNextAction()
{
    Action* pAction = nullptr;

    if (!ACTIONS_LIST.dequeue(pAction))
    {
        return false;
    }

    if (pAction)
    {
        pAction->Act(this);
    }

    return true;
}

int Restaurant::GetActionsCount() const
{
    return ACTIONS_LIST.getCount();
}

void Restaurant::AddOrderToPendingList(Order* pOrder)
{
    if (!pOrder)
    {
        return;
    }

    switch (pOrder->GetType())
    {
    case Order::TYPE_ODG:
        PEND_ODG.enqueue(pOrder);
        break;
    case Order::TYPE_ODN:
        PEND_ODN.enqueue(pOrder);
        break;
    case Order::TYPE_OT:
        PEND_OT.enqueue(pOrder);
        break;
    case Order::TYPE_OVC:
        PEND_OVC.enqueue(pOrder);
        break;
    case Order::TYPE_OVG:
        PEND_OVG.enqueue(pOrder, static_cast<int>(pOrder->GetPrice()) + pOrder->GetSize() + pOrder->GetDistance());
        break;
    case Order::TYPE_OVN:
        PEND_OVN.enqueue(pOrder);
        break;
    }
}

bool Restaurant::CancelOrder(int id)
{
    Order* cancelledOrder = nullptr;

    if (PEND_OVC.CancelOrder(id, cancelledOrder))
    {
        Cancelled_orders.enqueue(cancelledOrder);
        return true;
    }

    if (RDY_OV.CancelOrder(id, cancelledOrder))
    {
        Cancelled_orders.enqueue(cancelledOrder);
        return true;
    }

    if (CancelCookingOV(Cooking_Orders, id, cancelledOrder))
    {
        Chef* chef = cancelledOrder->GetAssignedChef();

        if (chef)
        {
            if (chef->IsSpecial())
            {
                Free_CS.enqueue(chef);
            }
            else
            {
                Free_CN.enqueue(chef);
            }

            cancelledOrder->SetAssignedChef(nullptr);
        }

        Cancelled_orders.enqueue(cancelledOrder);
        return true;
    }

    return false;
}

void Restaurant::PrintSummary() const
{
    cout << "Restaurant Lists Summary" << endl;
    cout << "Actions: " << ACTIONS_LIST.getCount() << endl;
    cout << "Pending ODG: " << PEND_ODG.getCount() << endl;
    cout << "Pending ODN: " << PEND_ODN.getCount() << endl;
    cout << "Pending OT: " << PEND_OT.getCount() << endl;
    cout << "Pending OVN: " << PEND_OVN.getCount() << endl;
    cout << "Pending OVC: " << PEND_OVC.getCount() << endl;
    cout << "Pending OVG: " << PEND_OVG.getCount() << endl;
    cout << "Free CS: " << Free_CS.getCount() << endl;
    cout << "Free CN: " << Free_CN.getCount() << endl;
    cout << "Cancelled orders: " << Cancelled_orders.getCount() << endl;
    cout << "Finished orders: " << Finished_orders.getCount() << endl;
    cout << "Cooking orders: " << Cooking_Orders.getCount() << endl;
    cout << "Ready OT: " << RDY_OT.getCount() << endl;
    cout << "Ready OV: " << RDY_OV.getCount() << endl;
    cout << "Ready OD: " << RDY_OD.getCount() << endl;
    cout << "In-service orders: " << InServ_Orders.getCount() << endl;
    cout << "Free scooters: " << Free_Scooters.getCount() << endl;
    cout << "Back scooters: " << Back_Scooters.getCount() << endl;
    cout << "Maint scooters: " << Maint_Scooters.getCount() << endl;
    cout << "Free tables: " << Free_Tables.getCount() << endl;
    cout << "Busy sharable tables: " << Busy_Sharable.getCount() << endl;
    cout << "Busy no-share tables: " << Busy_No_Share.getCount() << endl;
}
