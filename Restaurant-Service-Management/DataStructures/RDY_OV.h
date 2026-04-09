#pragma once

#include "LinkedQueue.h"
#include "../Models/Order.h"

class RDY_OV : public LinkedQueue<Order*>
{
public:
    bool CancelOrder(int id, Order*& removedOrder);
    void Print() const;
};
