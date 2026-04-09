#include <iostream>

#include "../DataStructures/ArrayStack.h"
#include "../DataStructures/Fit_Tables.h"
#include "../DataStructures/Pend_OVC.h"
#include "../DataStructures/RDY_OV.h"
#include "../DataStructures/priQueue.h"

using namespace std;

int main()
{
    Pend_OVC pendingOVC;

    Order* o1 = new Order(101);
    Order* o2 = new Order(102);
    Order* o3 = new Order(103);

    pendingOVC.enqueue(o1);
    pendingOVC.enqueue(o2);
    pendingOVC.enqueue(o3);

    cout << "Pending queue count = " << pendingOVC.getCount() << endl;
    pendingOVC.Print();

    Order* removed = nullptr;
    pendingOVC.CancelOrder(102, removed);
    cout << "Pending queue after cancel = ";
    pendingOVC.Print();
    cout << endl;

    RDY_OV readyOV;
    readyOV.enqueue(new Order(201));
    readyOV.enqueue(new Order(202));
    readyOV.enqueue(new Order(203));
    readyOV.CancelOrder(202, removed);
    readyOV.Print();

    priQueue<int> priorityQueue;
    priorityQueue.enqueue(10, 2);
    priorityQueue.enqueue(20, 5);
    priorityQueue.enqueue(30, 1);

    cout << "Priority queue count = " << priorityQueue.getCount() << endl;
    priorityQueue.Print();
    cout << endl;

    ArrayStack<int> stack;
    stack.push(7);
    stack.push(8);
    stack.push(9);

    cout << "Stack count = " << stack.getCount() << endl;
    stack.Print();
    cout << endl;

    Fit_Tables tables;
    Table* t1 = new Table(1, 2);
    Table* t2 = new Table(2, 4);
    Table* t3 = new Table(3, 6);

    tables.enqueue(t1, -t1->GetSeats());
    tables.enqueue(t2, -t2->GetSeats());
    tables.enqueue(t3, -t3->GetSeats());

    Table* chosenTable = nullptr;
    if (tables.getBest(3, chosenTable))
    {
        cout << "Best table for 3 seats = ";
        chosenTable->Print();
        cout << endl;
    }

    tables.Print();

    return 0;
}
