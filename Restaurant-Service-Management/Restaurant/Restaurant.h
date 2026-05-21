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
    LinkedQueue<Order*> PEND_OC;

    LinkedQueue<Chef*> Free_CS;
    LinkedQueue<Chef*> Free_CN;

    LinkedQueue<Order*> Cancelled_orders;
    ArrayStack<Order*> Finished_orders;
    priQueue<Order*> Cooking_Orders;

    LinkedQueue<Order*> RDY_OT;
    RDY_OV RDY_OV;
    LinkedQueue<Order*> RDY_OD;
    LinkedQueue<Order*> RDY_OC;
    priQueue<Order*> RDY_OVERWAIT_OVG;
    priQueue<Order*> InServ_Orders;

    priQueue<Scooter*> Free_Scooters;
    priQueue<Scooter*> Back_Scooters;
    LinkedQueue<Scooter*> Maint_Scooters;

    Fit_Tables Free_Tables;
    Fit_Tables Busy_Sharable;
    Fit_Tables Busy_No_Share;

    int Phase2TotalOrders;
    int Phase2OrderTypeCounts[7];
    int Phase2TotalChefs;
    int Phase2ChefsCS;
    int Phase2ChefsCN;
    int Phase2TotalScooters;
    int Phase2TotalTables;
    int Phase2OverwaitThreshold;
    int Phase2ScooterMaintenanceOrders;
    int Phase2OverwaitOrders;
    int Phase2RescueOrders;
    long long Phase2ChefBusySteps;
    long long Phase2ScooterBusySteps;
    int Phase2LastTimeStep;

public:
    Restaurant();

    bool LoadInputFile(const char* inputFilePath);
    void RunSimulation(const char* inputFilePath, const char* outputFilePath, bool interactiveMode);

    void AddAction(Action* pAction);
    bool ExecuteNextAction();
    int GetActionsCount() const;

    void AddOrderToPendingList(Order* pOrder);
    bool CancelOrder(int id);
    bool CancelOrder(int id, int currentTimestep);

    void PrintSummary() const;

    void RunPhase1RandomSimulation(unsigned randomSeed, bool interactiveMode, const char* outputFilePath);

private:
    void ResetPhase2Stats();
    void ExecuteActionsAtTimeStep(int currentTimestep);
    bool IsSimulationFinished() const;
    void CheckAvailableScooters(int currentTimestep);
    void CheckRescueScooters(int currentTimestep);
    void CheckFinishedDeliveryOrders(int currentTimestep);
    void CheckFinishedDineInOrders(int currentTimestep);
    void MoveCookingOrdersToReady(int currentTimestep);
    void AssignStage1(int currentTimestep);
    void AssignStage2(int currentTimestep);
    void FinalizeTakeawayOrders(int currentTimestep);
    void CollectPhase2Statistics(int currentTimestep);
    bool TakeChefForOrder(Order* order, Chef*& chef);
    void StartCookingOrder(Order* order, Chef* chef, int currentTimestep);
    bool AssignFromPendingQueue(LinkedQueue<Order*>& pendingList, int currentTimestep);
    bool AssignFromPendingOvg(int currentTimestep);
    bool AssignFromPendingCombo(int currentTimestep);
    bool TakeChefsForCombo(Order* order);
    void ReturnOrderChefs(Order* order);
    void StartComboCookingOrder(Order* order, int currentTimestep);
    int ComboCookingTime(Order* order) const;
    int CookingTime(Order* order, Chef* chef) const;
    int DeliveryTime(Order* order, Scooter* scooter) const;
    void MoveOverwaitOvgOrders(int currentTimestep);
    bool AssignOverwaitOrder(int currentTimestep);
    bool AssignComboScooters(Order* order, int currentTimestep);
    bool DequeueReadyDelivery(bool coldOnly, Order*& outOrder);
    bool AssignScooter(Order* order, int currentTimestep);
    bool AssignTable(Order* order, int currentTimestep);
    bool RemoveTableFromList(Fit_Tables& list, Table* table);
    void RemoveTableFromWaitingLists(Table* table);
    void PutTableInRightList(Table* table);

    void PrintProgramInterface(int currentTimestep);
    void WritePhase2OutputFile(const char* path,
        int totalOrders,
        const int orderTypeCounts[7],
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
