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

    LinkedQueue<Order*> PEND_ODG;
    LinkedQueue<Order*> PEND_ODN;
    LinkedQueue<Order*> PEND_OT;
    LinkedQueue<Order*> PEND_OVN;
    Pend_OVC PEND_OVC;
    priQueue<Order*> PEND_OVG;

    LinkedQueue<Chef*> Free_CS;
    LinkedQueue<Chef*> Free_CN;

    LinkedQueue<Order*> Cancelled_orders;
    ArrayStack<Order*> Finished_orders;
    priQueue<Order*> Cooking_Orders;

    LinkedQueue<Order*> RDY_OT;
    RDY_OV RDY_OV;
    LinkedQueue<Order*> RDY_OD;
    priQueue<Order*> InServ_Orders;

    priQueue<Scooter*> Free_Scooters;
    priQueue<Scooter*> Back_Scooters;
    LinkedQueue<Scooter*> Maint_Scooters;

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

    void RunPhase1RandomSimulation(unsigned randomSeed, bool interactiveMode, bool useActionDrivenInput, const char* outputFilePath);

private:
    void PrintProgramInterface(int currentTimestep);
    void WritePhase2OutputFile(const char* path,
        int totalOrders,
        const int orderTypeCounts[6],
        int totalChefs,
        int chefsCS,
        int chefsCN,
        int totalScooters,
        int totalTables,
        long long chefBusySteps,
        long long scooterBusySteps,
        int simTimeSteps);

    static int CookingPriority(const Order* order);
    static int InServicePriority(const Order* order);
    bool TryDequeueRandomPendingOrder(Order*& outOrder, int currentTimestep);
    bool TryTakeFreeChef(Chef*& outChef);
    void ReturnChefToFreeList(Chef* chef);
    void RouteOrderToReadyList(Order* order);
    bool TryDequeueRandomReadyOrder(Order*& outOrder);
    bool TryAssignScooterForDelivery(Order* order, int currentTimestep);
    bool TryAssignTableForDineIn(Order* order, int currentTimestep);
    static int ScooterFreePriority(const Scooter* s);
    static int TableFitPriority(const Table* t);
};
