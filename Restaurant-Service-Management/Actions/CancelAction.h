#pragma once

#include "Action.h"

class CancelAction : public Action
{
public:
    CancelAction(int time, int id);
    void Act(Restaurant* pRest);
};
