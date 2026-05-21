#include "Restaurant.h"

#include "../Actions/CancelAction.h"
#include "../Actions/RequestAction.h"

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
using namespace std;

namespace
{
    bool DequeueFirstOrderReady(LinkedQueue<Order*>& q, int currentTimestep, Order*& outOrder)
    {
        const int n = q.getCount();

        for (int i = 0; i < n; ++i)
        {
            Order* order = nullptr;

            if (!q.dequeue(order))
            {
                return false;
            }

            if (order->GetRequestTimeStep() < currentTimestep)
            {
                outOrder = order;
                return true;
            }

            q.enqueue(order);
        }

        outOrder = nullptr;
        return false;
    }

    bool DequeueFirstOrderReadyOvg(priQueue<Order*>& pq, int currentTimestep, Order*& outOrder)
    {
        priQueue<Order*> temp;
        Order* order = nullptr;
        int pri = 0;
        bool found = false;

        while (pq.dequeue(order, pri))
        {
            if (!found && order->GetRequestTimeStep() < currentTimestep)
            {
                outOrder = order;
                found = true;
            }
            else
            {
                temp.enqueue(order, pri);
            }
        }

        while (temp.dequeue(order, pri))
        {
            pq.enqueue(order, pri);
        }

        return found;
    }

    bool ParseOrderType(const char* typeText, Order::Type& orderType)
    {
        if (strcmp(typeText, "ODG") == 0)
        {
            orderType = Order::TYPE_ODG;
            return true;
        }

        if (strcmp(typeText, "ODN") == 0)
        {
            orderType = Order::TYPE_ODN;
            return true;
        }

        if (strcmp(typeText, "OT") == 0)
        {
            orderType = Order::TYPE_OT;
            return true;
        }

        if (strcmp(typeText, "OVC") == 0)
        {
            orderType = Order::TYPE_OVC;
            return true;
        }

        if (strcmp(typeText, "OVG") == 0)
        {
            orderType = Order::TYPE_OVG;
            return true;
        }

        if (strcmp(typeText, "OVN") == 0)
        {
            orderType = Order::TYPE_OVN;
            return true;
        }

        if (strcmp(typeText, "OC") == 0)
        {
            orderType = Order::TYPE_OC;
            return true;
        }

        return false;
    }

    int OrderTypeIndex(Order::Type orderType)
    {
        return static_cast<int>(orderType);
    }

    bool CancelCookingOV(priQueue<Order*>& cookingOrders, int id, Order*& removedOrder)
    {
        priQueue<Order*> temp;
        Order* current = nullptr;
        removedOrder = nullptr;
        bool found = false;
        int pri = 0;

        while (cookingOrders.dequeue(current, pri))
        {
            if (!found && current->GetID() == id && current->GetType() == Order::TYPE_OVC)
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
    ResetPhase2Stats();
}

void Restaurant::ResetPhase2Stats()
{
    Phase2TotalOrders = 0;
    Phase2TotalChefs = 0;
    Phase2ChefsCS = 0;
    Phase2ChefsCN = 0;
    Phase2TotalScooters = 0;
    Phase2TotalTables = 0;
    Phase2OverwaitThreshold = 0;
    Phase2ScooterMaintenanceOrders = 0;
    Phase2OverwaitOrders = 0;
    Phase2RescueOrders = 0;
    Phase2ChefBusySteps = 0;
    Phase2ScooterBusySteps = 0;
    Phase2LastTimeStep = 0;

    for (int i = 0; i < 7; ++i)
    {
        Phase2OrderTypeCounts[i] = 0;
    }
}

bool Restaurant::LoadInputFile(const char* inputFilePath)
{
    ifstream input(inputFilePath);

    if (!input)
    {
        cout << "Error: could not open input file: " << inputFilePath << endl;
        return false;
    }

    ResetPhase2Stats();

    int cnCount = 0;
    int csCount = 0;
    int cnSpeed = 0;
    int csSpeed = 0;
    int scooterCount = 0;
    int scooterSpeed = 0;
    int scooterMaintenanceOrders = 0;
    int scooterMaintenanceDuration = 0;
    int totalTables = 0;

    input >> cnCount >> csCount;
    input >> cnSpeed >> csSpeed;
    input >> scooterCount >> scooterSpeed;
    input >> scooterMaintenanceOrders >> scooterMaintenanceDuration;
    input >> totalTables;

    Phase2ChefsCS = csCount;
    Phase2ChefsCN = cnCount;
    Phase2TotalChefs = csCount + cnCount;
    Phase2TotalScooters = scooterCount;
    Phase2TotalTables = totalTables;
    Phase2ScooterMaintenanceOrders = scooterMaintenanceOrders;

    int nextChefId = 1;

    for (int i = 0; i < csCount; ++i)
    {
        Free_CS.enqueue(new Chef(nextChefId++, Chef::TYPE_CS, csSpeed));
    }

    for (int i = 0; i < cnCount; ++i)
    {
        Free_CN.enqueue(new Chef(nextChefId++, Chef::TYPE_CN, cnSpeed));
    }

    for (int i = 1; i <= scooterCount; ++i)
    {
        Scooter* scooter = new Scooter(i, scooterSpeed, scooterMaintenanceDuration);
        Free_Scooters.enqueue(scooter, ScooterFreePriority(scooter));
    }

    int createdTables = 0;
    int nextTableId = 1;

    while (createdTables < totalTables && input)
    {
        int tableCount = 0;
        int tableSeats = 0;

        input >> tableCount >> tableSeats;

        if (!input)
        {
            cout << "Error: invalid table data in input file." << endl;
            return false;
        }

        for (int i = 0; i < tableCount && createdTables < totalTables; ++i)
        {
            Table* table = new Table(nextTableId++, tableSeats);
            Free_Tables.enqueue(table, TableFitPriority(table));
            ++createdTables;
        }
    }

    input >> Phase2OverwaitThreshold;

    if (!input)
    {
        cout << "Error: missing overwait threshold in input file." << endl;
        return false;
    }

    int actionsCount = 0;
    input >> actionsCount;

    if (!input)
    {
        cout << "Error: missing actions count in input file." << endl;
        return false;
    }

    for (int i = 0; i < actionsCount && input; ++i)
    {
        char actionType = '\0';
        input >> actionType;

        if (actionType == 'Q')
        {
            char typeText[8] = "";
            int requestTime = 0;
            int orderId = 0;
            int size = 0;
            double price = 0.0;
            int seats = 0;
            int duration = 0;
            char shareChar = 'N';
            int distance = 0;
            int comboChefs = 1;
            int comboScooters = 1;

            input >> typeText >> requestTime >> orderId >> size >> price;

            Order::Type orderType = Order::TYPE_ODN;

            if (!ParseOrderType(typeText, orderType))
            {
                cout << "Error: unknown order type '" << typeText << "' in input file." << endl;
                return false;
            }

            if (orderType == Order::TYPE_ODG || orderType == Order::TYPE_ODN)
            {
                input >> seats >> duration >> shareChar;
            }
            else if (orderType == Order::TYPE_OVC || orderType == Order::TYPE_OVG || orderType == Order::TYPE_OVN)
            {
                input >> distance;
            }
            else if (orderType == Order::TYPE_OC)
            {
                input >> distance >> comboChefs >> comboScooters;
            }

            const bool canShare = (shareChar == 'Y' || shareChar == 'y');
            AddAction(new RequestAction(requestTime, orderId, size, price, orderType, seats, duration, canShare, distance, comboChefs, comboScooters));

            ++Phase2TotalOrders;
            ++Phase2OrderTypeCounts[OrderTypeIndex(orderType)];
        }
        else if (actionType == 'X')
        {
            int cancelTime = 0;
            int orderId = 0;

            input >> cancelTime >> orderId;
            AddAction(new CancelAction(cancelTime, orderId));
        }
        else
        {
            cout << "Error: unknown action type '" << actionType << "' in input file." << endl;
            return false;
        }
    }

    return true;
}

void Restaurant::ExecuteActionsAtTimeStep(int currentTimestep)
{
    Action* action = nullptr;

    while (ACTIONS_LIST.peek(action) && action && action->GetActionTime() <= currentTimestep)
    {
        ACTIONS_LIST.dequeue(action);
        action->Act(this);
    }
}

bool Restaurant::IsSimulationFinished() const
{
    return ACTIONS_LIST.isEmpty()
        && PEND_ODG.isEmpty()
        && PEND_ODN.isEmpty()
        && PEND_OT.isEmpty()
        && PEND_OVN.isEmpty()
        && PEND_OVC.isEmpty()
        && PEND_OVG.isEmpty()
        && PEND_OC.isEmpty()
        && Cooking_Orders.isEmpty()
        && RDY_OT.isEmpty()
        && RDY_OV.isEmpty()
        && RDY_OD.isEmpty()
        && RDY_OC.isEmpty()
        && RDY_OVERWAIT_OVG.isEmpty()
        && InServ_Orders.isEmpty()
        && Back_Scooters.isEmpty();
}

void Restaurant::CheckAvailableScooters(int currentTimestep)
{
    priQueue<Scooter*> tempBack;
    Scooter* scooter = nullptr;
    int pri = 0;

    while (Back_Scooters.dequeue(scooter, pri))
    {
        if (scooter->GetReturnTimeStep() <= currentTimestep)
        {
            if (scooter->IsBroken() || scooter->NeedsMaintenance(Phase2ScooterMaintenanceOrders))
            {
                scooter->SetBroken(false);
                scooter->ResetAssignedOrders();
                scooter->SetMaintenanceFinishTimeStep(currentTimestep + scooter->GetMaintenanceDuration());
                Maint_Scooters.enqueue(scooter);
            }
            else
            {
                Free_Scooters.enqueue(scooter, ScooterFreePriority(scooter));
            }
        }
        else
        {
            tempBack.enqueue(scooter, pri);
        }
    }

    while (tempBack.dequeue(scooter, pri))
    {
        Back_Scooters.enqueue(scooter, pri);
    }

    const int maintCount = Maint_Scooters.getCount();

    for (int i = 0; i < maintCount; ++i)
    {
        Maint_Scooters.dequeue(scooter);

        if (scooter->GetMaintenanceFinishTimeStep() <= currentTimestep)
        {
            Free_Scooters.enqueue(scooter, ScooterFreePriority(scooter));
        }
        else
        {
            Maint_Scooters.enqueue(scooter);
        }
    }
}

void Restaurant::CheckRescueScooters(int currentTimestep)
{
    priQueue<Order*> temp;
    Order* order = nullptr;
    int pri = 0;

    while (InServ_Orders.dequeue(order, pri))
    {
        if (order->IsDelivery()
            && !order->HasRescue()
            && order->GetAssignedScooterCount() > 0
            && order->GetFinishTimeStep() > currentTimestep
            && !Free_Scooters.isEmpty()
            && ((currentTimestep + order->GetID()) % 50 == 0))
        {
            Scooter* failedScooter = order->GetAssignedScooterAt(0);
            Scooter* rescueScooter = nullptr;
            int scooterPri = 0;

            if (failedScooter && Free_Scooters.dequeue(rescueScooter, scooterPri))
            {
                const int returnTime = currentTimestep + 1;
                failedScooter->SetBroken(true);
                failedScooter->SetReturnTimeStep(returnTime);
                Back_Scooters.enqueue(failedScooter, -returnTime);

                rescueScooter->IncrementAssignedOrders();
                rescueScooter->AddDistance(order->GetDistance() * 2);
                order->SetAssignedScooterAt(0, rescueScooter);
                order->SetRescueUsed(true);
                order->SetFinishTimeStep(order->GetFinishTimeStep() + DeliveryTime(order, rescueScooter));
                Phase2RescueOrders++;
            }
        }

        temp.enqueue(order, -order->GetFinishTimeStep());
    }

    while (temp.dequeue(order, pri))
    {
        InServ_Orders.enqueue(order, pri);
    }
}

void Restaurant::CheckFinishedDeliveryOrders(int currentTimestep)
{
    priQueue<Order*> temp;
    Order* order = nullptr;
    int pri = 0;

    while (InServ_Orders.dequeue(order, pri))
    {
        if (order->IsDelivery() && order->GetFinishTimeStep() <= currentTimestep)
        {
            const int scooterCount = order->GetAssignedScooterCount();

            for (int i = 0; i < scooterCount; ++i)
            {
                Scooter* scooter = order->GetAssignedScooterAt(i);

                if (scooter)
                {
                    const int returnTime = currentTimestep + DeliveryTime(order, scooter);
                    scooter->SetReturnTimeStep(returnTime);
                    Back_Scooters.enqueue(scooter, -returnTime);
                }
            }

            order->ClearAssignedScooters();
            Finished_orders.push(order);
        }
        else
        {
            temp.enqueue(order, pri);
        }
    }

    while (temp.dequeue(order, pri))
    {
        InServ_Orders.enqueue(order, pri);
    }
}

void Restaurant::CheckFinishedDineInOrders(int currentTimestep)
{
    priQueue<Order*> temp;
    Order* order = nullptr;
    int pri = 0;

    while (InServ_Orders.dequeue(order, pri))
    {
        if (order->IsDineIn() && order->GetFinishTimeStep() <= currentTimestep)
        {
            Table* table = order->GetAssignedTable();

            if (table)
            {
                RemoveTableFromWaitingLists(table);
                table->ReleaseSeats(order->GetRequiredSeats());
                PutTableInRightList(table);
                order->SetAssignedTable(nullptr);
            }

            Finished_orders.push(order);
        }
        else
        {
            temp.enqueue(order, pri);
        }
    }

    while (temp.dequeue(order, pri))
    {
        InServ_Orders.enqueue(order, pri);
    }
}

void Restaurant::MoveCookingOrdersToReady(int currentTimestep)
{
    priQueue<Order*> temp;
    Order* order = nullptr;
    int pri = 0;

    while (Cooking_Orders.dequeue(order, pri))
    {
        if (order->GetReadyTimeStep() <= currentTimestep)
        {
            ReturnOrderChefs(order);
            RouteOrderToReadyList(order);
        }
        else
        {
            temp.enqueue(order, pri);
        }
    }

    while (temp.dequeue(order, pri))
    {
        Cooking_Orders.enqueue(order, pri);
    }
}

void Restaurant::AssignStage1(int currentTimestep)
{
    AssignFromPendingCombo(currentTimestep);
    AssignFromPendingQueue(PEND_ODG, currentTimestep);
    AssignFromPendingQueue(PEND_ODN, currentTimestep);
    AssignFromPendingQueue(PEND_OT, currentTimestep);
    AssignFromPendingOvg(currentTimestep);
    AssignFromPendingQueue(PEND_OVC, currentTimestep);
    AssignFromPendingQueue(PEND_OVN, currentTimestep);
}

void Restaurant::AssignStage2(int currentTimestep)
{
    const int comboCount = RDY_OC.getCount();
    const int dineCount = RDY_OD.getCount();
    Order* order = nullptr;

    for (int i = 0; i < comboCount; ++i)
    {
        RDY_OC.dequeue(order);

        if (!AssignComboScooters(order, currentTimestep))
        {
            RDY_OC.enqueue(order);
        }
    }

    for (int i = 0; i < dineCount; ++i)
    {
        RDY_OD.dequeue(order);

        if (!AssignTable(order, currentTimestep))
        {
            RDY_OD.enqueue(order);
        }
    }

    MoveOverwaitOvgOrders(currentTimestep);

    while (!Free_Scooters.isEmpty())
    {
        if (!AssignOverwaitOrder(currentTimestep))
        {
            break;
        }
    }

    while (!Free_Scooters.isEmpty())
    {
        if (!DequeueReadyDelivery(true, order))
        {
            break;
        }

        if (!AssignScooter(order, currentTimestep))
        {
            RDY_OV.enqueue(order);
            break;
        }
    }

    while (!Free_Scooters.isEmpty())
    {
        if (!DequeueReadyDelivery(false, order))
        {
            break;
        }

        if (!AssignScooter(order, currentTimestep))
        {
            RDY_OV.enqueue(order);
            break;
        }
    }
}

void Restaurant::FinalizeTakeawayOrders(int currentTimestep)
{
    const int readyCount = RDY_OT.getCount();
    Order* order = nullptr;

    for (int i = 0; i < readyCount; ++i)
    {
        RDY_OT.dequeue(order);

        if (order->GetReadyTimeStep() < currentTimestep)
        {
            order->SetInServiceTimeStep(currentTimestep);
            order->SetFinishTimeStep(currentTimestep);
            Finished_orders.push(order);
        }
        else
        {
            RDY_OT.enqueue(order);
        }
    }
}

void Restaurant::CollectPhase2Statistics(int currentTimestep)
{
    Phase2LastTimeStep = currentTimestep;
    Phase2ChefBusySteps += Cooking_Orders.getCount();
    Phase2ScooterBusySteps += Phase2TotalScooters - Free_Scooters.getCount() - Maint_Scooters.getCount();
}

bool Restaurant::TakeChefForOrder(Order* order, Chef*& chef)
{
    chef = nullptr;

    if (!order)
    {
        return false;
    }

    switch (order->GetType())
    {
    case Order::TYPE_ODG:
    case Order::TYPE_OVG:
        return Free_CS.dequeue(chef);

    case Order::TYPE_ODN:
    case Order::TYPE_OVC:
        if (Free_CN.dequeue(chef))
        {
            return true;
        }

        return Free_CS.dequeue(chef);

    case Order::TYPE_OT:
    case Order::TYPE_OVN:
        return Free_CN.dequeue(chef);

    default:
        return false;
    }
}

int Restaurant::CookingTime(Order* order, Chef* chef) const
{
    if (!order || !chef || chef->GetSpeed() <= 0)
    {
        return 1;
    }

    int result = order->GetSize() / chef->GetSpeed();

    if (order->GetSize() % chef->GetSpeed() != 0)
    {
        result++;
    }

    if (result <= 0)
    {
        result = 1;
    }

    return result;
}

void Restaurant::StartCookingOrder(Order* order, Chef* chef, int currentTimestep)
{
    const int readyTime = currentTimestep + CookingTime(order, chef);

    order->SetAssignedChef(chef);
    order->SetAssignmentTimeStep(currentTimestep);
    order->SetReadyTimeStep(readyTime);
    Cooking_Orders.enqueue(order, -readyTime);
}

bool Restaurant::AssignFromPendingQueue(LinkedQueue<Order*>& pendingList, int currentTimestep)
{
    bool assigned = false;
    Order* order = nullptr;
    Chef* chef = nullptr;

    while (DequeueFirstOrderReady(pendingList, currentTimestep, order))
    {
        if (!TakeChefForOrder(order, chef))
        {
            pendingList.enqueue(order);
            return assigned;
        }

        StartCookingOrder(order, chef, currentTimestep);
        assigned = true;
    }

    return assigned;
}

bool Restaurant::AssignFromPendingOvg(int currentTimestep)
{
    bool assigned = false;
    Order* order = nullptr;
    Chef* chef = nullptr;

    while (DequeueFirstOrderReadyOvg(PEND_OVG, currentTimestep, order))
    {
        if (!TakeChefForOrder(order, chef))
        {
            PEND_OVG.enqueue(order, CookingPriority(order));
            return assigned;
        }

        StartCookingOrder(order, chef, currentTimestep);
        assigned = true;
    }

    return assigned;
}

bool Restaurant::AssignFromPendingCombo(int currentTimestep)
{
    bool assigned = false;
    Order* order = nullptr;

    while (DequeueFirstOrderReady(PEND_OC, currentTimestep, order))
    {
        if (!TakeChefsForCombo(order))
        {
            PEND_OC.enqueue(order);
            return assigned;
        }

        StartComboCookingOrder(order, currentTimestep);
        assigned = true;
    }

    return assigned;
}

bool Restaurant::TakeChefsForCombo(Order* order)
{
    if (!order)
    {
        return false;
    }

    Chef* taken[4];
    int takenCount = 0;
    Chef* chef = nullptr;

    for (int i = 0; i < 4; ++i)
    {
        taken[i] = nullptr;
    }

    if (!Free_CS.dequeue(chef))
    {
        return false;
    }

    taken[takenCount++] = chef;

    while (takenCount < order->GetComboChefCount() && Free_CN.dequeue(chef))
    {
        taken[takenCount++] = chef;
    }

    while (takenCount < order->GetComboChefCount() && Free_CS.dequeue(chef))
    {
        taken[takenCount++] = chef;
    }

    if (takenCount < order->GetComboChefCount())
    {
        for (int i = 0; i < takenCount; ++i)
        {
            ReturnChefToFreeList(taken[i]);
        }

        return false;
    }

    order->ClearAssignedChefs();

    for (int i = 0; i < takenCount; ++i)
    {
        order->AddAssignedChef(taken[i]);
    }

    return true;
}

void Restaurant::ReturnOrderChefs(Order* order)
{
    if (!order)
    {
        return;
    }

    const int chefCount = order->GetAssignedChefCount();

    for (int i = 0; i < chefCount; ++i)
    {
        ReturnChefToFreeList(order->GetAssignedChefAt(i));
    }

    order->ClearAssignedChefs();
}

int Restaurant::ComboCookingTime(Order* order) const
{
    if (!order)
    {
        return 1;
    }

    int totalSpeed = 0;

    for (int i = 0; i < order->GetAssignedChefCount(); ++i)
    {
        Chef* chef = order->GetAssignedChefAt(i);

        if (chef)
        {
            totalSpeed += chef->GetSpeed();
        }
    }

    if (totalSpeed <= 0)
    {
        return 1;
    }

    int result = order->GetSize() / totalSpeed;

    if (order->GetSize() % totalSpeed != 0)
    {
        result++;
    }

    if (result <= 0)
    {
        result = 1;
    }

    return result;
}

void Restaurant::StartComboCookingOrder(Order* order, int currentTimestep)
{
    const int readyTime = currentTimestep + ComboCookingTime(order);

    order->SetAssignmentTimeStep(currentTimestep);
    order->SetReadyTimeStep(readyTime);
    Cooking_Orders.enqueue(order, -readyTime);
}

int Restaurant::DeliveryTime(Order* order, Scooter* scooter) const
{
    if (!order || !scooter || scooter->GetSpeed() <= 0)
    {
        return 1;
    }

    int result = order->GetDistance() / scooter->GetSpeed();

    if (order->GetDistance() % scooter->GetSpeed() != 0)
    {
        result++;
    }

    if (result <= 0)
    {
        result = 1;
    }

    return result;
}

void Restaurant::MoveOverwaitOvgOrders(int currentTimestep)
{
    const int count = RDY_OV.getCount();
    Order* order = nullptr;

    for (int i = 0; i < count; ++i)
    {
        RDY_OV.dequeue(order);

        if (order->GetType() == Order::TYPE_OVG
            && Phase2OverwaitThreshold > 0
            && currentTimestep - order->GetReadyTimeStep() > Phase2OverwaitThreshold)
        {
            if (!order->IsOverwait())
            {
                order->SetOverwait(true);
                Phase2OverwaitOrders++;
            }

            RDY_OVERWAIT_OVG.enqueue(order, currentTimestep - order->GetRequestTimeStep());
        }
        else
        {
            RDY_OV.enqueue(order);
        }
    }
}

bool Restaurant::AssignOverwaitOrder(int currentTimestep)
{
    Order* order = nullptr;
    int pri = 0;

    if (!RDY_OVERWAIT_OVG.dequeue(order, pri))
    {
        return false;
    }

    if (!AssignScooter(order, currentTimestep))
    {
        RDY_OVERWAIT_OVG.enqueue(order, pri);
        return false;
    }

    return true;
}

bool Restaurant::AssignComboScooters(Order* order, int currentTimestep)
{
    if (!order)
    {
        return false;
    }

    const int needed = order->GetComboScooterCount();

    if (Free_Scooters.getCount() < needed)
    {
        return false;
    }

    Scooter* scooters[4];
    int pri = 0;

    for (int i = 0; i < 4; ++i)
    {
        scooters[i] = nullptr;
    }

    order->ClearAssignedScooters();

    for (int i = 0; i < needed; ++i)
    {
        Free_Scooters.dequeue(scooters[i], pri);
        order->AddAssignedScooter(scooters[i]);
    }

    int slowestSpeed = scooters[0]->GetSpeed();

    for (int i = 1; i < needed; ++i)
    {
        if (scooters[i]->GetSpeed() < slowestSpeed)
        {
            slowestSpeed = scooters[i]->GetSpeed();
        }
    }

    if (slowestSpeed <= 0)
    {
        slowestSpeed = 1;
    }

    int serviceTime = order->GetDistance() / slowestSpeed;

    if (order->GetDistance() % slowestSpeed != 0)
    {
        serviceTime++;
    }

    if (serviceTime <= 0)
    {
        serviceTime = 1;
    }

    const int finishTime = currentTimestep + serviceTime;

    for (int i = 0; i < needed; ++i)
    {
        scooters[i]->IncrementAssignedOrders();
        scooters[i]->AddDistance(order->GetDistance() * 2);
    }

    order->SetInServiceTimeStep(currentTimestep);
    order->SetFinishTimeStep(finishTime);
    InServ_Orders.enqueue(order, -finishTime);
    return true;
}

bool Restaurant::DequeueReadyDelivery(bool coldOnly, Order*& outOrder)
{
    const int count = RDY_OV.getCount();
    Order* order = nullptr;
    outOrder = nullptr;

    for (int i = 0; i < count; ++i)
    {
        RDY_OV.dequeue(order);

        const bool isCold = order->GetType() == Order::TYPE_OVC;

        if ((!outOrder) && ((coldOnly && isCold) || (!coldOnly && !isCold)))
        {
            outOrder = order;
        }
        else
        {
            RDY_OV.enqueue(order);
        }
    }

    return outOrder != nullptr;
}

bool Restaurant::AssignScooter(Order* order, int currentTimestep)
{
    if (!order)
    {
        return false;
    }

    Scooter* scooter = nullptr;
    int pri = 0;

    if (!Free_Scooters.dequeue(scooter, pri))
    {
        return false;
    }

    const int serviceTime = DeliveryTime(order, scooter);
    const int finishTime = currentTimestep + serviceTime;

    if (order->GetType() == Order::TYPE_OVG
        && Phase2OverwaitThreshold > 0
        && currentTimestep - order->GetReadyTimeStep() > Phase2OverwaitThreshold
        && !order->IsOverwait())
    {
        order->SetOverwait(true);
        Phase2OverwaitOrders++;
    }

    scooter->IncrementAssignedOrders();
    scooter->AddDistance(order->GetDistance() * 2);
    order->SetAssignedScooter(scooter);
    order->SetInServiceTimeStep(currentTimestep);
    order->SetFinishTimeStep(finishTime);
    InServ_Orders.enqueue(order, -finishTime);
    return true;
}

bool Restaurant::AssignTable(Order* order, int currentTimestep)
{
    if (!order)
    {
        return false;
    }

    int seats = order->GetRequiredSeats();

    if (seats <= 0)
    {
        seats = 1;
    }

    Table* table = nullptr;

    if (order->CanShare())
    {
        Busy_Sharable.getBest(seats, table);
    }

    if (!table)
    {
        Free_Tables.getBest(seats, table);
    }

    if (!table)
    {
        return false;
    }

    if (!table->OccupySeats(seats, order->CanShare()))
    {
        PutTableInRightList(table);
        return false;
    }

    int duration = order->GetDuration();

    if (duration <= 0)
    {
        duration = 1;
    }

    const int finishTime = currentTimestep + duration;
    order->SetAssignedTable(table);
    order->SetInServiceTimeStep(currentTimestep);
    order->SetFinishTimeStep(finishTime);
    InServ_Orders.enqueue(order, -finishTime);
    PutTableInRightList(table);
    return true;
}

bool Restaurant::RemoveTableFromList(Fit_Tables& list, Table* table)
{
    priQueue<Table*> temp;
    Table* current = nullptr;
    int pri = 0;
    bool found = false;

    while (list.dequeue(current, pri))
    {
        if (!found && current == table)
        {
            found = true;
        }
        else
        {
            temp.enqueue(current, pri);
        }
    }

    while (temp.dequeue(current, pri))
    {
        list.enqueue(current, pri);
    }

    return found;
}

void Restaurant::RemoveTableFromWaitingLists(Table* table)
{
    RemoveTableFromList(Free_Tables, table);
    RemoveTableFromList(Busy_Sharable, table);
    RemoveTableFromList(Busy_No_Share, table);
}

void Restaurant::PutTableInRightList(Table* table)
{
    if (!table)
    {
        return;
    }

    if (table->IsFree())
    {
        Free_Tables.enqueue(table, TableFitPriority(table));
    }
    else if (table->CanShareMore())
    {
        Busy_Sharable.enqueue(table, TableFitPriority(table));
    }
    else
    {
        Busy_No_Share.enqueue(table, TableFitPriority(table));
    }
}

void Restaurant::RunSimulation(const char* inputFilePath, const char* outputFilePath, bool interactiveMode)
{
    if (!LoadInputFile(inputFilePath))
    {
        return;
    }

    const int maxTimeSteps = 200000;
    int currentTimestep = 1;

    while (!IsSimulationFinished() && currentTimestep <= maxTimeSteps)
    {
        ExecuteActionsAtTimeStep(currentTimestep);
        CheckAvailableScooters(currentTimestep);
        CheckRescueScooters(currentTimestep);
        CheckFinishedDeliveryOrders(currentTimestep);
        CheckFinishedDineInOrders(currentTimestep);
        MoveCookingOrdersToReady(currentTimestep);
        AssignStage1(currentTimestep);
        AssignStage2(currentTimestep);
        FinalizeTakeawayOrders(currentTimestep);
        CollectPhase2Statistics(currentTimestep);

        if (interactiveMode)
        {
            PrintProgramInterface(currentTimestep);
            cout << "PRESS ANY KEY TO MOVE TO NEXT STEP!" << endl;
            cin.get();
        }

        ++currentTimestep;
    }

    if (currentTimestep > maxTimeSteps)
    {
        cout << "Warning: stopped before all orders were finished or cancelled (step cap)." << endl;
    }

    WritePhase2OutputFile(
        outputFilePath,
        Phase2TotalOrders,
        Phase2OrderTypeCounts,
        Phase2TotalChefs,
        Phase2ChefsCS,
        Phase2ChefsCN,
        Phase2TotalScooters,
        Phase2TotalTables,
        Phase2ChefBusySteps,
        Phase2ScooterBusySteps,
        Phase2LastTimeStep);
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
    case Order::TYPE_OC:
        PEND_OC.enqueue(pOrder);
        break;
    }
}

bool Restaurant::CancelOrder(int id)
{
    return CancelOrder(id, -1);
}

bool Restaurant::CancelOrder(int id, int currentTimestep)
{
    Order* cancelledOrder = nullptr;

    if (PEND_OVC.CancelOrder(id, cancelledOrder))
    {
        cancelledOrder->SetCancelTimeStep(currentTimestep);
        Cancelled_orders.enqueue(cancelledOrder);
        return true;
    }

    if (RDY_OV.CancelOrder(id, cancelledOrder))
    {
        cancelledOrder->SetCancelTimeStep(currentTimestep);
        Cancelled_orders.enqueue(cancelledOrder);
        return true;
    }

    if (CancelCookingOV(Cooking_Orders, id, cancelledOrder))
    {
        ReturnOrderChefs(cancelledOrder);

        cancelledOrder->SetCancelTimeStep(currentTimestep);
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
    cout << "Pending OC: " << PEND_OC.getCount() << endl;
    cout << "Free CS: " << Free_CS.getCount() << endl;
    cout << "Free CN: " << Free_CN.getCount() << endl;
    cout << "Cancelled orders: " << Cancelled_orders.getCount() << endl;
    cout << "Finished orders: " << Finished_orders.getCount() << endl;
    cout << "Cooking orders: " << Cooking_Orders.getCount() << endl;
    cout << "Ready OT: " << RDY_OT.getCount() << endl;
    cout << "Ready OV: " << RDY_OV.getCount() << endl;
    cout << "Ready OD: " << RDY_OD.getCount() << endl;
    cout << "Ready OC: " << RDY_OC.getCount() << endl;
    cout << "Overwait OVG: " << RDY_OVERWAIT_OVG.getCount() << endl;
    cout << "In-service orders: " << InServ_Orders.getCount() << endl;
    cout << "Free scooters: " << Free_Scooters.getCount() << endl;
    cout << "Back scooters: " << Back_Scooters.getCount() << endl;
    cout << "Maint scooters: " << Maint_Scooters.getCount() << endl;
    cout << "Free tables: " << Free_Tables.getCount() << endl;
    cout << "Busy sharable tables: " << Busy_Sharable.getCount() << endl;
    cout << "Busy no-share tables: " << Busy_No_Share.getCount() << endl;
}

int Restaurant::CookingPriority(const Order* order)
{
    if (!order)
    {
        return 0;
    }

    if (order->IsDelivery())
    {
        return static_cast<int>(order->GetPrice()) + order->GetSize() + order->GetDistance();
    }

    return static_cast<int>(order->GetPrice()) + order->GetSize() + order->GetRequiredSeats();
}

int Restaurant::InServicePriority(const Order* order)
{
    return CookingPriority(order);
}

int Restaurant::ScooterFreePriority(const Scooter* s)
{
    return s ? -s->GetTotalDistance() : 0;
}

int Restaurant::TableFitPriority(const Table* t)
{
    return t ? (-t->GetSeats()) : 0;
}

bool Restaurant::TryDequeueRandomPendingOrder(Order*& outOrder, int currentTimestep)
{
    int visitOrder[6] = { 0, 1, 2, 3, 4, 5 };

    for (int i = 5; i > 0; --i)
    {
        const int j = rand() % (i + 1);
        const int tmp = visitOrder[i];
        visitOrder[i] = visitOrder[j];
        visitOrder[j] = tmp;
    }

    for (int k = 0; k < 6; ++k)
    {
        switch (visitOrder[k])
        {
        case 0:
            if (DequeueFirstOrderReady(PEND_ODG, currentTimestep, outOrder))
            {
                return true;
            }
            break;
        case 1:
            if (DequeueFirstOrderReady(PEND_ODN, currentTimestep, outOrder))
            {
                return true;
            }
            break;
        case 2:
            if (DequeueFirstOrderReady(PEND_OT, currentTimestep, outOrder))
            {
                return true;
            }
            break;
        case 3:
            if (DequeueFirstOrderReady(PEND_OVN, currentTimestep, outOrder))
            {
                return true;
            }
            break;
        case 4:
            if (DequeueFirstOrderReady(PEND_OVC, currentTimestep, outOrder))
            {
                return true;
            }
            break;
        case 5:
            if (DequeueFirstOrderReadyOvg(PEND_OVG, currentTimestep, outOrder))
            {
                return true;
            }
            break;
        default:
            break;
        }
    }

    outOrder = nullptr;
    return false;
}

bool Restaurant::TryDequeueRandomReadyOrder(Order*& outOrder)
{
    int visitOrder[3] = { 0, 1, 2 };

    for (int i = 2; i > 0; --i)
    {
        const int j = rand() % (i + 1);
        const int tmp = visitOrder[i];
        visitOrder[i] = visitOrder[j];
        visitOrder[j] = tmp;
    }

    for (int k = 0; k < 3; ++k)
    {
        switch (visitOrder[k])
        {
        case 0:
            if (!RDY_OT.isEmpty())
            {
                RDY_OT.dequeue(outOrder);
                return true;
            }
            break;
        case 1:
            if (!RDY_OV.isEmpty())
            {
                RDY_OV.dequeue(outOrder);
                return true;
            }
            break;
        case 2:
            if (!RDY_OD.isEmpty())
            {
                RDY_OD.dequeue(outOrder);
                return true;
            }
            break;
        default:
            break;
        }
    }

    outOrder = nullptr;
    return false;
}

bool Restaurant::TryTakeFreeChef(Chef*& outChef)
{
    const bool hasCs = !Free_CS.isEmpty();
    const bool hasCn = !Free_CN.isEmpty();

    if (!hasCs && !hasCn)
    {
        outChef = nullptr;
        return false;
    }

    if (hasCs && hasCn)
    {
        if (rand() % 2 == 0)
        {
            Free_CS.dequeue(outChef);
        }
        else
        {
            Free_CN.dequeue(outChef);
        }
        return true;
    }

    if (hasCs)
    {
        Free_CS.dequeue(outChef);
    }
    else
    {
        Free_CN.dequeue(outChef);
    }

    return true;
}

void Restaurant::ReturnChefToFreeList(Chef* chef)
{
    if (!chef)
    {
        return;
    }

    if (chef->IsSpecial())
    {
        Free_CS.enqueue(chef);
    }
    else
    {
        Free_CN.enqueue(chef);
    }
}

void Restaurant::RouteOrderToReadyList(Order* order)
{
    if (!order)
    {
        return;
    }

    switch (order->GetType())
    {
    case Order::TYPE_OT:
        RDY_OT.enqueue(order);
        break;
    case Order::TYPE_OVC:
    case Order::TYPE_OVG:
    case Order::TYPE_OVN:
        RDY_OV.enqueue(order);
        break;
    case Order::TYPE_OC:
        RDY_OC.enqueue(order);
        break;
    case Order::TYPE_ODG:
    case Order::TYPE_ODN:
        RDY_OD.enqueue(order);
        break;
    default:
        break;
    }
}

bool Restaurant::TryAssignScooterForDelivery(Order* order, int currentTimestep)
{
    if (!order || Free_Scooters.isEmpty())
    {
        return false;
    }

    Scooter* scooter = nullptr;
    int pri = 0;

    if (!Free_Scooters.dequeue(scooter, pri))
    {
        return false;
    }

    order->SetAssignedScooter(scooter);
    scooter->IncrementAssignedOrders();
    order->SetInServiceTimeStep(currentTimestep);
    InServ_Orders.enqueue(order, InServicePriority(order));
    return true;
}

bool Restaurant::TryAssignTableForDineIn(Order* order, int currentTimestep)
{
    if (!order)
    {
        return false;
    }

    int seats = order->GetRequiredSeats();
    if (seats <= 0)
    {
        seats = 2;
    }

    int perm[3] = { 0, 1, 2 };

    for (int i = 2; i > 0; --i)
    {
        const int j = rand() % (i + 1);
        const int tmp = perm[i];
        perm[i] = perm[j];
        perm[j] = tmp;
    }

    Table* table = nullptr;

    for (int k = 0; k < 3; ++k)
    {
        Fit_Tables* list = nullptr;

        switch (perm[k])
        {
        case 0:
            list = &Free_Tables;
            break;
        case 1:
            list = &Busy_Sharable;
            break;
        case 2:
            list = &Busy_No_Share;
            break;
        default:
            list = nullptr;
            break;
        }

        if (list && list->getBest(seats, table))
        {
            order->SetAssignedTable(table);
            order->SetInServiceTimeStep(currentTimestep);
            InServ_Orders.enqueue(order, InServicePriority(order));
            return true;
        }

        table = nullptr;
    }

    RDY_OD.enqueue(order);
    return true;
}

namespace
{
    void PrintOrderQueueIds(const char* orderType, LinkedQueue<Order*>& q)
    {
        LinkedQueue<Order*> temp;
        Order* order = nullptr;

        cout << q.getCount() << ' ' << orderType << ": ";

        while (q.dequeue(order))
        {
            cout << order->GetID() << ' ';
            temp.enqueue(order);
        }

        while (temp.dequeue(order))
        {
            q.enqueue(order);
        }

        cout << endl;
    }

    void PrintPendingOvgIds(priQueue<Order*>& pq)
    {
        priQueue<Order*> temp;
        Order* order = nullptr;
        int pri = 0;

        cout << pq.getCount() << " OVG: ";

        while (pq.dequeue(order, pri))
        {
            cout << order->GetID() << ' ';
            temp.enqueue(order, pri);
        }

        while (temp.dequeue(order, pri))
        {
            pq.enqueue(order, pri);
        }

        cout << endl;
    }

    void PrintChefIdsLine(const char* typeLabel, LinkedQueue<Chef*>& q)
    {
        LinkedQueue<Chef*> temp;
        Chef* chef = nullptr;

        cout << q.getCount() << ' ' << typeLabel << " : ";

        while (q.dequeue(chef))
        {
            cout << chef->GetID() << ' ';
            temp.enqueue(chef);
        }

        while (temp.dequeue(chef))
        {
            q.enqueue(chef);
        }

        cout << endl;
    }

    void PrintScooterIdsPri(priQueue<Scooter*>& pq)
    {
        priQueue<Scooter*> temp;
        Scooter* scooter = nullptr;
        int pri = 0;

        cout << pq.getCount() << " Scooters: ";

        while (pq.dequeue(scooter, pri))
        {
            cout << scooter->GetID() << ' ';
            temp.enqueue(scooter, pri);
        }

        while (temp.dequeue(scooter, pri))
        {
            pq.enqueue(scooter, pri);
        }

        cout << endl;
    }

    void PrintTableListLine(const char* title, Fit_Tables& ft)
    {
        priQueue<Table*> temp;
        Table* table = nullptr;
        int pri = 0;

        cout << title << ft.getCount() << " tables: ";

        while (ft.dequeue(table, pri))
        {
            const int cap = table->GetCapacity();
            cout << "[T" << table->GetID() << ", " << cap << ", " << table->GetFreeSeats() << "] ";
            temp.enqueue(table, pri);
        }

        while (temp.dequeue(table, pri))
        {
            ft.enqueue(table, pri);
        }

        cout << endl;
    }

    void SwapOrderPtr(Order*& a, Order*& b)
    {
        Order* t = a;
        a = b;
        b = t;
    }
}

void Restaurant::PrintProgramInterface(int currentTimestep)
{
    cout << "\nCurrent Timestep: " << currentTimestep << endl;
    cout << "------------ Actions List ------------" << endl;
    cout << "For reQuest action: print [Order Type, Trequest, order ID], For cancel print (X, Tcancel, order ID)" << endl;
    {
        const int remaining = ACTIONS_LIST.getCount();
        LinkedQueue<Action*> temp;
        Action* action = nullptr;
        int shown = 0;

        cout << remaining << " actions remaining: ";

        while (ACTIONS_LIST.dequeue(action))
        {
            if (shown < 10)
            {
                if (shown > 0)
                {
                    cout << ", ";
                }

                action->DescribeForUI(cout);
                ++shown;
            }

            temp.enqueue(action);
        }

        while (temp.dequeue(action))
        {
            ACTIONS_LIST.enqueue(action);
        }

        cout << endl;
        cout << "-> Print ONLY the first 10 actions currently in the actions list" << endl;
    }

    cout << "------------ Pending Orders IDs ------------" << endl;
    cout << "For each pending list print" << endl;
    cout << "List count, order type, IDs of all orders in the list" << endl;
    PrintOrderQueueIds("ODG", PEND_ODG);
    PrintOrderQueueIds("ODN", PEND_ODN);
    PrintOrderQueueIds("OT", PEND_OT);
    PrintOrderQueueIds("OVN", PEND_OVN);
    PrintOrderQueueIds("OVC", PEND_OVC);
    PrintPendingOvgIds(PEND_OVG);
    PrintOrderQueueIds("OC", PEND_OC);

    cout << "------------ Available chefs IDs ------------" << endl;
    PrintChefIdsLine("CS", Free_CS);
    PrintChefIdsLine("CN", Free_CN);

    cout << "------------ Cooking orders [Orders ID, chef ID] ------------" << endl;
    {
        priQueue<Order*> temp;
        Order* order = nullptr;
        int pri = 0;

        cout << Cooking_Orders.getCount() << " cooking orders: ";

        while (Cooking_Orders.dequeue(order, pri))
        {
            Chef* ch = order->GetAssignedChef();
            cout << '[' << order->GetID() << ", " << (ch ? ch->GetID() : -1) << "] ";
            temp.enqueue(order, pri);
        }

        while (temp.dequeue(order, pri))
        {
            Cooking_Orders.enqueue(order, pri);
        }

        cout << endl;
    }

    cout << "------------ Ready Orders IDs ------------" << endl;
    cout << "For each Ready list print" << endl;
    cout << "List count, order type, IDs of all orders in the list" << endl;
    PrintOrderQueueIds("OT", RDY_OT);
    PrintOrderQueueIds("OV", RDY_OV);
    PrintOrderQueueIds("OD", RDY_OD);
    PrintOrderQueueIds("OC", RDY_OC);

    cout << "------------ Available scooters IDs ------------" << endl;
    PrintScooterIdsPri(Free_Scooters);

    cout << "------------ Available tables [ID, capacity, free seats] ------------" << endl;
    PrintTableListLine("Free ", Free_Tables);
    PrintTableListLine("Sharable busy ", Busy_Sharable);

    cout << "------------ In-service orders [order ID, scooter/Table ID] ------------" << endl;
    {
        priQueue<Order*> temp;
        Order* order = nullptr;
        int pri = 0;

        cout << InServ_Orders.getCount() << " Orders: ";

        while (InServ_Orders.dequeue(order, pri))
        {
            if (order->IsDelivery())
            {
                Scooter* sc = order->GetAssignedScooter();
                cout << '[' << order->GetID() << ", S" << (sc ? sc->GetID() : -1) << "] ";
            }

            temp.enqueue(order, pri);
        }

        while (temp.dequeue(order, pri))
        {
            InServ_Orders.enqueue(order, pri);
        }

        while (InServ_Orders.dequeue(order, pri))
        {
            if (order->IsDineIn())
            {
                Table* tb = order->GetAssignedTable();
                cout << '[' << order->GetID() << ", T" << (tb ? tb->GetID() : -1) << "] ";
            }

            temp.enqueue(order, pri);
        }

        while (temp.dequeue(order, pri))
        {
            InServ_Orders.enqueue(order, pri);
        }

        cout << endl;
    }

    cout << "------------ In-maintainance scooters IDs ------------" << endl;
    {
        LinkedQueue<Scooter*> temp;
        Scooter* scooter = nullptr;

        cout << Maint_Scooters.getCount() << " scooters: ";

        while (Maint_Scooters.dequeue(scooter))
        {
            cout << scooter->GetID() << ' ';
            temp.enqueue(scooter);
        }

        while (temp.dequeue(scooter))
        {
            Maint_Scooters.enqueue(scooter);
        }

        cout << endl;
    }

    cout << "------------ Scooters Back to Restaurant IDs ------------" << endl;
    PrintScooterIdsPri(Back_Scooters);

    cout << "------------ Cancelled Orders IDs ------------" << endl;
    {
        LinkedQueue<Order*> temp;
        Order* order = nullptr;

        cout << Cancelled_orders.getCount() << " cancelled: ";

        while (Cancelled_orders.dequeue(order))
        {
            cout << order->GetID() << ' ';
            temp.enqueue(order);
        }

        while (temp.dequeue(order))
        {
            Cancelled_orders.enqueue(order);
        }

        cout << endl;
    }

    cout << "------------ Finished orders IDs ------------" << endl;
    {
        const int n = Finished_orders.getCount();

        if (n <= 0)
        {
            cout << "0 Orders" << endl;
        }
        else
        {
            Order* arr[1024];
            Order* order = nullptr;

            for (int i = 0; i < n; ++i)
            {
                Finished_orders.pop(order);
                arr[i] = order;
            }

            for (int i = 0; i < n; ++i)
            {
                for (int j = i + 1; j < n; ++j)
                {
                    if (arr[j]->GetFinishTimeStep() > arr[i]->GetFinishTimeStep())
                    {
                        SwapOrderPtr(arr[i], arr[j]);
                    }
                }
            }

            cout << n << " Orders: ";

            for (int i = 0; i < n; ++i)
            {
                if (i > 0)
                {
                    cout << ", ";
                }

                cout << arr[i]->GetID();
            }

            cout << endl;

            for (int i = n - 1; i >= 0; --i)
            {
                Finished_orders.push(arr[i]);
            }
        }
    }

    cout << "(print IDs of all finished orders in descending order of finish time)" << endl;
    cout << "================================================================" << endl;
}

void Restaurant::WritePhase2OutputFile(const char* path,
    int totalOrders,
    const int orderTypeCounts[7],
    int totalChefs,
    int chefsCS,
    int chefsCN,
    int totalScooters,
    int totalTables,
    long long chefBusySteps,
    long long scooterBusySteps,
    int simTimeSteps)
{
    ofstream out(path);

    if (!out)
    {
        cout << "Error: could not open output file: " << path << endl;
        return;
    }

    const int nFin = Finished_orders.getCount();
    Order* arr[5000];
    Order* order = nullptr;

    for (int i = 0; i < nFin; ++i)
    {
        Finished_orders.pop(order);
        arr[i] = order;
    }

    for (int i = 0; i < nFin; ++i)
    {
        for (int j = i + 1; j < nFin; ++j)
        {
            if (arr[j]->GetFinishTimeStep() > arr[i]->GetFinishTimeStep())
            {
                SwapOrderPtr(arr[i], arr[j]);
            }
        }
    }

    out << "TF ID TQ TA TR Ts T1 Tc Tw Tserv" << endl;

    long long sumT1 = 0;
    long long sumTc = 0;
    long long sumTw = 0;
    long long sumTserv = 0;

    for (int i = 0; i < nFin; ++i)
    {
        order = arr[i];
        const int tq = order->GetRequestTimeStep();
        const int ta = order->GetAssignmentTimeStep();
        const int tr = order->GetReadyTimeStep();
        const int ts = order->GetInServiceTimeStep();
        const int tf = order->GetFinishTimeStep();

        int t1 = 0;
        int tc = 0;
        int tw = 0;
        int tserv = 0;

        if (ta >= 0 && tq >= 0)
        {
            t1 += ta - tq;
        }

        if (ts >= 0 && tr >= 0)
        {
            t1 += ts - tr;
        }

        if (tr >= 0 && ta >= 0)
        {
            tc = tr - ta;
        }

        if (tf >= 0 && ts >= 0)
        {
            tserv = tf - ts;
        }

        tw = t1 + tc;

        sumT1 += t1;
        sumTc += tc;
        sumTw += tw;
        sumTserv += tserv;

        out << tf << ' '
            << order->GetID() << ' '
            << tq << ' '
            << ta << ' '
            << tr << ' '
            << ts << ' '
            << t1 << ' '
            << tc << ' '
            << tw << ' '
            << tserv << endl;
    }

    const int nCancel = Cancelled_orders.getCount();
    const int denomOrders = (totalOrders > 0) ? totalOrders : 1;
    const double pctFinished = 100.0 * static_cast<double>(nFin) / static_cast<double>(denomOrders);
    const double pctCancelled = 100.0 * static_cast<double>(nCancel) / static_cast<double>(denomOrders);
    const double pctOverwait = (nFin > 0) ? (100.0 * static_cast<double>(Phase2OverwaitOrders) / static_cast<double>(nFin)) : 0.0;
    const double avgT1 = (nFin > 0) ? static_cast<double>(sumT1) / nFin : 0.0;
    const double avgTc = (nFin > 0) ? static_cast<double>(sumTc) / nFin : 0.0;
    const double avgTw = (nFin > 0) ? static_cast<double>(sumTw) / nFin : 0.0;
    const double avgTserv = (nFin > 0) ? static_cast<double>(sumTserv) / nFin : 0.0;

    const long long chefDenom = static_cast<long long>(totalChefs) * static_cast<long long>(simTimeSteps > 0 ? simTimeSteps : 1);
    const long long scooterDenom = static_cast<long long>(totalScooters) * static_cast<long long>(simTimeSteps > 0 ? simTimeSteps : 1);
    const double chefUtilPct = 100.0 * static_cast<double>(chefBusySteps) / static_cast<double>(chefDenom > 0 ? chefDenom : 1);
    const double scooterUtilPct = 100.0 * static_cast<double>(scooterBusySteps) / static_cast<double>(scooterDenom > 0 ? scooterDenom : 1);

    out << endl << "--- Statistics ---" << endl;
    out << "Total orders placed: " << totalOrders << endl;
    out << "Counts by type - ODG: " << orderTypeCounts[0] << " ODN: " << orderTypeCounts[1] << " OT: " << orderTypeCounts[2]
        << " OVC: " << orderTypeCounts[3] << " OVG: " << orderTypeCounts[4] << " OVN: " << orderTypeCounts[5] << " OC: " << orderTypeCounts[6] << endl;
    out << "Total chefs: " << totalChefs << " (CS: " << chefsCS << ", CN: " << chefsCN << ')' << endl;
    out << "Total scooters: " << totalScooters << endl;
    out << "Total tables: " << totalTables << endl;
    out << "Percentage finished: " << pctFinished << '%' << endl;
    out << "Percentage cancelled: " << pctCancelled << '%' << endl;
    out << "Percentage overwait orders: " << pctOverwait << '%' << endl;
    out << "Rescue scooter cases: " << Phase2RescueOrders << endl;
    out << "Average T1: " << avgT1 << endl;
    out << "Average Tc: " << avgTc << endl;
    out << "Average Tw: " << avgTw << endl;
    out << "Average Tserv: " << avgTserv << endl;
    out << "Scooters utilization % (busy = not free and not maintenance / scooters / timesteps): " << scooterUtilPct << '%' << endl;
    out << "Chefs utilization % (busy = preparing an order): " << chefUtilPct << '%' << endl;

    for (int i = nFin - 1; i >= 0; --i)
    {
        Finished_orders.push(arr[i]);
    }
}

void Restaurant::RunPhase1RandomSimulation(unsigned randomSeed, bool interactiveMode, const char* outputFilePath)
{
    srand(randomSeed);

    const int totalSimOrders = 500;
    const int maxTimeSteps = 200000;
    const int totalChefsCount = 30;
    const int chefsCSCount = 10;
    const int chefsCNCount = 20;
    const int totalScootersCount = 15;
    const int totalTablesCount = 10;

    int orderTypeCounts[7] = { 0, 0, 0, 0, 0, 0, 0 };
    long long chefBusySteps = 0;
    long long scooterBusySteps = 0;

    const char* outPath = outputFilePath ? outputFilePath : "phase2_output.txt";

    for (int i = 1; i <= 10; ++i)
    {
        Chef* cs = new Chef(1000 + i, Chef::TYPE_CS, 5 + (i % 3));
        Free_CS.enqueue(cs);
    }

    for (int i = 1; i <= 20; ++i)
    {
        Chef* cn = new Chef(2000 + i, Chef::TYPE_CN, 3 + (i % 4));
        Free_CN.enqueue(cn);
    }

    for (int i = 1; i <= 15; ++i)
    {
        Scooter* sc = new Scooter(3000 + i, 20 + (i % 10), 2);
        Free_Scooters.enqueue(sc, ScooterFreePriority(sc));
    }

    const int tableCaps[10] = { 2, 2, 4, 4, 6, 6, 6, 8, 8, 10 };

    for (int i = 0; i < 10; ++i)
    {
        Table* tb = new Table(4000 + i + 1, tableCaps[i]);
        Free_Tables.enqueue(tb, TableFitPriority(tb));
    }

    for (int n = 1; n <= totalSimOrders; ++n)
    {
        const int r = rand() % 6;
        const Order::Type orderType = static_cast<Order::Type>(r);
        ++orderTypeCounts[r];

        const int requestTs = rand() % 80;
        const int orderSize = 1 + (rand() % 5);
        const double orderPrice = 10.0 + static_cast<double>(rand() % 200);
        const int seats = 2 + 2 * (rand() % 4);
        const int duration = 15 + (rand() % 60);
        const bool share = (rand() % 2) == 0;
        const int distance = 1 + (rand() % 20);

        Order* newOrder = new Order(
            n,
            requestTs,
            orderSize,
            orderPrice,
            orderType,
            seats,
            duration,
            share,
            distance);

        AddOrderToPendingList(newOrder);
    }

    int timeStep = 0;

    while (Finished_orders.getCount() + Cancelled_orders.getCount() < totalSimOrders
        && timeStep < maxTimeSteps)
    {
        ++timeStep;

        const int scooterBusyAtStepStart = totalScootersCount - Free_Scooters.getCount() - Maint_Scooters.getCount();

        for (int i = 0; i < 30; ++i)
        {
            Order* order = nullptr;

            if (!TryDequeueRandomPendingOrder(order, timeStep))
            {
                continue;
            }

            Chef* chef = nullptr;

            if (!TryTakeFreeChef(chef))
            {
                AddOrderToPendingList(order);
                continue;
            }

            order->SetAssignedChef(chef);
            order->SetAssignmentTimeStep(timeStep);
            Cooking_Orders.enqueue(order, CookingPriority(order));
        }

        const int cookingAfterAssign = Cooking_Orders.getCount();

        for (int i = 0; i < 15; ++i)
        {
            if (rand() % 4 == 0)
            {
                continue;
            }

            if (Cooking_Orders.isEmpty())
            {
                continue;
            }

            Order* cooked = nullptr;
            int cookPri = 0;

            if (!Cooking_Orders.dequeue(cooked, cookPri))
            {
                continue;
            }

            Chef* chef = cooked->GetAssignedChef();
            ReturnChefToFreeList(chef);
            cooked->SetAssignedChef(nullptr);
            cooked->SetReadyTimeStep(timeStep);
            RouteOrderToReadyList(cooked);
        }

        const int cookingAfterReadyBatch = Cooking_Orders.getCount();

        chefBusySteps += static_cast<long long>(cookingAfterAssign + cookingAfterReadyBatch) / 2;

        for (int i = 0; i < 10; ++i)
        {
            Order* readyOrder = nullptr;

            if (!TryDequeueRandomReadyOrder(readyOrder))
            {
                continue;
            }

            if (readyOrder->IsTakeaway())
            {
                readyOrder->SetInServiceTimeStep(timeStep);
                readyOrder->SetFinishTimeStep(timeStep);
                Finished_orders.push(readyOrder);
                continue;
            }

            if (readyOrder->IsDelivery())
            {
                if (!TryAssignScooterForDelivery(readyOrder, timeStep))
                {
                    RouteOrderToReadyList(readyOrder);
                }
                continue;
            }

            if (readyOrder->IsDineIn())
            {
                TryAssignTableForDineIn(readyOrder, timeStep);
                continue;
            }
        }

        {
            const int cancelId = 1 + (rand() % 5000);
            Order* removed = nullptr;

            if (PEND_OVC.CancelOrder(cancelId, removed))
            {
                removed->SetCancelTimeStep(timeStep);
                Cancelled_orders.enqueue(removed);
            }
        }

        {
            const int cancelId = 1 + (rand() % 5000);
            Order* removed = nullptr;

            if (RDY_OV.CancelOrder(cancelId, removed))
            {
                removed->SetCancelTimeStep(timeStep);
                Cancelled_orders.enqueue(removed);
            }
        }

        {
            const int cancelId = 1 + (rand() % 5000);
            Order* removed = nullptr;

            if (CancelCookingOV(Cooking_Orders, cancelId, removed))
            {
                Chef* chef = removed->GetAssignedChef();
                ReturnChefToFreeList(chef);
                removed->SetAssignedChef(nullptr);
                removed->SetCancelTimeStep(timeStep);
                Cancelled_orders.enqueue(removed);
            }
        }

        if (rand() % 4 == 0 && !InServ_Orders.isEmpty())
        {
            Order* inServ = nullptr;
            int inPri = 0;

            if (InServ_Orders.dequeue(inServ, inPri))
            {
                inServ->SetFinishTimeStep(timeStep);
                Finished_orders.push(inServ);

                if (inServ->IsDelivery())
                {
                    Scooter* scooter = inServ->GetAssignedScooter();

                    if (scooter)
                    {
                        inServ->SetAssignedScooter(nullptr);
                        Back_Scooters.enqueue(scooter, ScooterFreePriority(scooter));
                    }
                }
                else if (inServ->IsDineIn())
                {
                    Table* table = inServ->GetAssignedTable();

                    if (table)
                    {
                        inServ->SetAssignedTable(nullptr);
                        Free_Tables.enqueue(table, TableFitPriority(table));
                    }
                }
            }
        }

        if (rand() % 2 == 0 && !Back_Scooters.isEmpty())
        {
            Scooter* scooter = nullptr;
            int sp = 0;

            if (Back_Scooters.dequeue(scooter, sp))
            {
                if (rand() % 2 == 0)
                {
                    Free_Scooters.enqueue(scooter, ScooterFreePriority(scooter));
                }
                else
                {
                    Maint_Scooters.enqueue(scooter);
                }
            }
        }

        if (rand() % 2 == 0 && !Maint_Scooters.isEmpty())
        {
            Scooter* scooter = nullptr;

            if (Maint_Scooters.dequeue(scooter))
            {
                Free_Scooters.enqueue(scooter, ScooterFreePriority(scooter));
            }
        }

        const int scooterBusyAtStepEnd = totalScootersCount - Free_Scooters.getCount() - Maint_Scooters.getCount();

        scooterBusySteps += static_cast<long long>(scooterBusyAtStepStart + scooterBusyAtStepEnd) / 2;

        if (interactiveMode)
        {
            PrintProgramInterface(timeStep);
            cout << "PRESS ANY KEY TO MOVE TO NEXT STEP!" << endl;
            cin.get();
        }
    }

    if (interactiveMode)
    {
        cout << "Simulation finished after " << timeStep << " time steps." << endl;
        cout << "Finished: " << Finished_orders.getCount()
            << "  Cancelled: " << Cancelled_orders.getCount()
            << "  (target " << totalSimOrders << ")" << endl;

        if (Finished_orders.getCount() + Cancelled_orders.getCount() < totalSimOrders)
        {
            cout << "Warning: stopped before all orders were finished or cancelled (step cap)." << endl;
        }
    }

    WritePhase2OutputFile(
        outPath,
        totalSimOrders,
        orderTypeCounts,
        totalChefsCount,
        chefsCSCount,
        chefsCNCount,
        totalScootersCount,
        totalTablesCount,
        chefBusySteps,
        scooterBusySteps,
        timeStep);
}
