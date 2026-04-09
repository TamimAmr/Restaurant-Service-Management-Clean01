#ifndef ARRAY_STACK_
#define ARRAY_STACK_

#include "StackADT.h"

#include <iostream>
using namespace std;

template <typename T>
class ArrayStack : public StackADT<T>
{
    enum { MAX_SIZE = 100 };

protected:
    T items[MAX_SIZE];
    int top;

    static void PrintItem(const T& item)
    {
        cout << item;
    }

    template <typename U>
    static void PrintItem(U* const& item)
    {
        if (item)
        {
            item->Print();
        }
        else
        {
            cout << "NULL";
        }
    }

public:
    ArrayStack()
    {
        top = -1;
    }

    bool isEmpty() const
    {
        return top == -1;
    }

    bool push(const T& newEntry)
    {
        if (top == MAX_SIZE - 1)
        {
            return false;
        }

        top++;
        items[top] = newEntry;
        return true;
    }

    bool pop(T& TopEntry)
    {
        if (isEmpty())
        {
            return false;
        }

        TopEntry = items[top];
        top--;
        return true;
    }

    bool peek(T& TopEntry) const
    {
        if (isEmpty())
        {
            return false;
        }

        TopEntry = items[top];
        return true;
    }

    int getCount() const
    {
        return top + 1;
    }

    void Print() const
    {
        cout << "Top[";

        for (int i = top; i >= 0; i--)
        {
            PrintItem(items[i]);
            if (i > 0)
            {
                cout << ", ";
            }
        }

        cout << "]Bottom";
    }
};

#endif
