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
    Phase2ChefBusySteps = 0;
    Phase2ScooterBusySteps = 0;
    Phase2LastTimeStep = 0;

    for (int i = 0; i < 6; ++i)
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

            const bool canShare = (shareChar == 'Y' || shareChar == 'y');
            AddAction(new RequestAction(requestTime, orderId, size, price, orderType, seats, duration, canShare, distance));

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
        && Cooking_Orders.isEmpty()
        && RDY_OT.isEmpty()
        && RDY_OV.isEmpty()
        && RDY_OD.isEmpty()
        && InServ_Orders.isEmpty()
        && Back_Scooters.isEmpty()
        && Maint_Scooters.isEmpty();
}

void Restaurant::CheckAvailableScooters(int currentTimestep)
{
    (void)currentTimestep;
}

void Restaurant::CheckFinishedDeliveryOrders(int currentTimestep)
{
    (void)currentTimestep;
}

void Restaurant::CheckFinishedDineInOrders(int currentTimestep)
{
    (void)currentTimestep;
}

void Restaurant::AssignStage1(int currentTimestep)
{
    AssignPendingOrdersToChefs(currentTimestep);
    CheckFinishedCookingOrders(currentTimestep);
}

void Restaurant::AssignStage2(int currentTimestep)
{
    (void)currentTimestep;
}

void Restaurant::FinalizeTakeawayOrders(int currentTimestep)
{
    (void)currentTimestep;
}

void Restaurant::AssignPendingOrdersToChefs(int timestep)
{
    while (!Free_CS.isEmpty() || !Free_CN.isEmpty())
    {
        Order* order = nullptr;

        if (DequeueFirstOrderReady(PEND_ODG, timestep, order)
            || DequeueFirstOrderReady(PEND_ODN, timestep, order)
            || DequeueFirstOrderReady(PEND_OT, timestep, order)
            || DequeueFirstOrderReady(PEND_OVC, timestep, order)
            || DequeueFirstOrderReadyOvg(PEND_OVG, timestep, order)
            || DequeueFirstOrderReady(PEND_OVN, timestep, order))
        {
            Chef* chef = nullptr;
            if (!TryTakeFreeChef(chef) || !chef)
            {
                AddOrderToPendingList(order);
                return;
            }

            order->SetAssignedChef(chef);
            order->SetAssignmentTimeStep(timestep);
            Cooking_Orders.enqueue(order, CookingPriority(order));
            continue;
        }

        return;
    }
}

void Restaurant::CheckFinishedCookingOrders(int timestep)
{
    priQueue<Order*> stillCooking;
    Order* order = nullptr;
    int pri = 0;

    while (Cooking_Orders.dequeue(order, pri))
    {
        Chef* chef = order ? order->GetAssignedChef() : nullptr;
        const int finishTime = CalculateCookingFinishTime(order, chef);

        if (finishTime > 0 && finishTime <= timestep)
        {
            order->SetReadyTimeStep(finishTime);
            order->SetAssignedChef(nullptr);
            ReturnChefToFreeList(chef);
            RouteOrderToReadyList(order);
        }
        else
        {
            stillCooking.enqueue(order, pri);
        }
    }

    while (stillCooking.dequeue(order, pri))
    {
        Cooking_Orders.enqueue(order, pri);
    }
}

int Restaurant::CalculateCookingFinishTime(Order* order, Chef* chef)
{
    if (!order || !chef)
    {
        return -1;
    }

    int speed = chef->GetSpeed();
    if (speed <= 0)
    {
        speed = 1;
    }

    int size = order->GetSize();
    if (size <= 0)
    {
        size = 1;
    }

    const int assignment = order->GetAssignmentTimeStep();
    if (assignment < 0)
    {
        return -1;
    }

    const int cookingSteps = (size + speed - 1) / speed;
    return assignment + cookingSteps;
}

void Restaurant::CollectPhase2Statistics(int currentTimestep)
{
    Phase2LastTimeStep = currentTimestep;
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
        CheckFinishedDeliveryOrders(currentTimestep);
        CheckFinishedDineInOrders(currentTimestep);
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
    return s ? (s->GetSpeed() + s->GetID()) : 0;
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
            cout << "[T" << table->GetID() << ", " << cap << ", " << cap << "] ";
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

    cout << "------------ Available scooters IDs ------------" << endl;
    PrintScooterIdsPri(Free_Scooters);

    cout << "------------ Available tables [ID, capacity, free seats] ------------" << endl;
    PrintTableListLine("", Free_Tables);

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
    const int orderTypeCounts[6],
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
    Order* arr[1024];
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

    out << "Finished orders (Tf, ID, Te, TA, TR, Ts, Tw, Tserv)" << endl;

    for (int i = 0; i < nFin; ++i)
    {
        order = arr[i];
        const int te = order->GetRequestTimeStep();
        const int ta = order->GetAssignmentTimeStep();
        const int tr = order->GetReadyTimeStep();
        const int ts = order->GetInServiceTimeStep();
        const int tf = order->GetFinishTimeStep();

        int tw = 0;

        if (ta >= 0)
        {
            tw = ta - te;
        }

        int tserv = 0;

        if (tf >= 0 && ts >= 0)
        {
            tserv = tf - ts;
        }
        else if (tf >= 0 && tr >= 0)
        {
            tserv = tf - tr;
        }

        out << tf << ' ' << order->GetID() << ' ' << te << ' ' << ta << ' ' << tr << ' ' << ts << ' ' << tw << ' ' << tserv << endl;
    }

    const int nCancel = Cancelled_orders.getCount();
    const int denomOrders = (totalOrders > 0) ? totalOrders : 1;
    const double pctFinished = 100.0 * static_cast<double>(nFin) / static_cast<double>(denomOrders);
    const double pctCancelled = 100.0 * static_cast<double>(nCancel) / static_cast<double>(denomOrders);

    int overwait = 0;

    for (int i = 0; i < nFin; ++i)
    {
        order = arr[i];
        const int te = order->GetRequestTimeStep();
        const int tf = order->GetFinishTimeStep();
        const int limit = 40 + order->GetSize() * 8;

        if (tf >= 0 && (tf - te) > limit)
        {
            ++overwait;
        }
    }

    const double pctOverwait = (nFin > 0) ? (100.0 * static_cast<double>(overwait) / static_cast<double>(nFin)) : 0.0;

    double sumTf = 0.0;
    double sumTe = 0.0;
    double sumTer = 0.0;

    for (int i = 0; i < nFin; ++i)
    {
        order = arr[i];
        const int te = order->GetRequestTimeStep();
        const int tf = order->GetFinishTimeStep();
        sumTf += static_cast<double>(tf);
        sumTe += static_cast<double>(te);
        sumTer += static_cast<double>(tf - te);
    }

    const double avgTf = (nFin > 0) ? sumTf / nFin : 0.0;
    const double avgTe = (nFin > 0) ? sumTe / nFin : 0.0;
    const double avgTer = (nFin > 0) ? sumTer / nFin : 0.0;

    const long long chefDenom = static_cast<long long>(totalChefs) * static_cast<long long>(simTimeSteps > 0 ? simTimeSteps : 1);
    const long long scooterDenom = static_cast<long long>(totalScooters) * static_cast<long long>(simTimeSteps > 0 ? simTimeSteps : 1);
    const double chefUtilPct = 100.0 * static_cast<double>(chefBusySteps) / static_cast<double>(chefDenom > 0 ? chefDenom : 1);
    const double scooterUtilPct = 100.0 * static_cast<double>(scooterBusySteps) / static_cast<double>(scooterDenom > 0 ? scooterDenom : 1);

    out << endl << "--- Statistics ---" << endl;
    out << "Total orders placed: " << totalOrders << endl;
    out << "Counts by type - ODG: " << orderTypeCounts[0] << " ODN: " << orderTypeCounts[1] << " OT: " << orderTypeCounts[2]
        << " OVC: " << orderTypeCounts[3] << " OVG: " << orderTypeCounts[4] << " OVN: " << orderTypeCounts[5] << endl;
    out << "Total chefs: " << totalChefs << " (CS: " << chefsCS << ", CN: " << chefsCN << ')' << endl;
    out << "Total scooters: " << totalScooters << " (single class in this build)" << endl;
    out << "Total tables (initial free pool): " << totalTables << endl;
    out << "Percentage finished: " << pctFinished << '%' << endl;
    out << "Percentage cancelled: " << pctCancelled << '%' << endl;
    out << "Percentage overwait (finished only; threshold Tf-Te > 40+8*size): " << pctOverwait << '%' << endl;
    out << "Average Tf (finished): " << avgTf << endl;
    out << "Average Te (finished): " << avgTe << endl;
    out << "Average Ter = Tf-Te (finished): " << avgTer << endl;
    out << "Chefs utilization % (busy = orders in cooking / chefs / timesteps): " << chefUtilPct << '%' << endl;
    out << "Scooters utilization % (busy = not free and not maintenance / scooters / timesteps): " << scooterUtilPct << '%' << endl;
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

    int orderTypeCounts[6] = { 0, 0, 0, 0, 0, 0 };
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
