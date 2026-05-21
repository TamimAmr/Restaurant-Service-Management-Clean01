#include "RDY_OV.h"

bool RDY_OV::CancelOrder(int id, Order*& removedOrder)
{
    LinkedQueue<Order*> temp;
    Order* current = nullptr;
    removedOrder = nullptr;
    bool found = false;

    while (!isEmpty())
    {
        dequeue(current);

        if (!found && current->GetID() == id && current->GetType() == Order::TYPE_OVC)
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

void RDY_OV::Print() const
{
    cout << "RDY_OV: ";
    LinkedQueue<Order*>::Print();
    cout << endl;
}
