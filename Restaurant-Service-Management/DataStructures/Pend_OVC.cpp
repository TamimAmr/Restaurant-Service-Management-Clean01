#include "Pend_OVC.h"

bool Pend_OVC::CancelOrder(int id, Order*& removedOrder)
{
    LinkedQueue<Order*> temp;
    Order* current = nullptr;
    removedOrder = nullptr;
    bool found = false;

    while (!isEmpty())
    {
        dequeue(current);

        if (!found && current->GetID() == id)
        {
            removedOrder = current;
            found = true;
        }
        else
        {
            temp.enqueue(current);
        }
    }

    while (!temp.isEmpty())
    {
        temp.dequeue(current);
        enqueue(current);
    }

    return found;
}

void Pend_OVC::Print() const
{
    cout << "PEND_OVC: ";
    LinkedQueue<Order*>::Print();
    cout << endl;
}
