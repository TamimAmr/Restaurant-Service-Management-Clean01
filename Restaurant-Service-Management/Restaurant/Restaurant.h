#pragma once

#include "../Actions/Action.h"
#include "../DataStructures/ArrayStack.h"
#include "../DataStructures/Fit_Tables.h"
#include "../DataStructures/LinkedQueue.h"
#include "../DataStructures/Pend_OVC.h"
#include "../DataStructures/RDY_OV.h"
#include "../DataStructures/priQueue.h"
#include "../Models/Chef.h"
#include "../Models/Order.h"
#include "../Models/Scooter.h"
#include "../Models/Table.h"

class Restaurant
{
private:
    LinkedQueue<Action*> ACTIONS_LIST;

    // Pending orders
    LinkedQueue<Order*> PEND_ODG;
    LinkedQueue<Order*> PEND_ODN;
    LinkedQueue<Order*> PEND_OT;
    LinkedQueue<Order*> PEND_OVN;
    Pend_OVC PEND_OVC;
    priQueue<Order*> PEND_OVG;

    // Free chefs
    LinkedQueue<Chef*> Free_CS;
    LinkedQueue<Chef*> Free_CN;

    // Order history and active processing
    LinkedQueue<Order*> Cancelled_orders;
    ArrayStack<Order*> Finished_orders;
    priQueue<Order*> Cooking_Orders;

    // Ready and in-service orders
    LinkedQueue<Order*> RDY_OT;
    RDY_OV RDY_OV;
    LinkedQueue<Order*> RDY_OD;
    priQueue<Order*> InServ_Orders;

    // Scooter lists
    priQueue<Scooter*> Free_Scooters;
    priQueue<Scooter*> Back_Scooters;
    LinkedQueue<Scooter*> Maint_Scooters;

    // Table lists
    Fit_Tables Free_Tables;
    Fit_Tables Busy_Sharable;
    Fit_Tables Busy_No_Share;

public:
    Restaurant();

    void AddAction(Action* pAction);
    bool ExecuteNextAction();
    int GetActionsCount() const;

    void AddOrderToPendingList(Order* pOrder);
    bool CancelOrder(int id);

    void PrintSummary() const;
};
