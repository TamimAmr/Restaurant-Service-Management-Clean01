#pragma once
#include "priNode.h"

#include <iostream>
using namespace std;

template <typename T>
class priQueue
{
protected:
    priNode<T>* head;
    int count;

    static void PrintItem(const T& item)
    {
        cout << item;
    }

    template <typename U>
    static void PrintItem(U* const& item)
    {
        if (item)
            item->Print();
        else
            cout << "NULL";
    }
public:
    priQueue() : head(nullptr), count(0) {}

    ~priQueue() {
        T tmp;
        int p;
        while (dequeue(tmp,p));
    }

    void enqueue(const T& data, int priority) {
        priNode<T>* newNode = new priNode<T>(data, priority);

        if (head == nullptr || priority > head->getPri()) {
            
            newNode->setNext(head);
            head = newNode;
            count++;
            return;
        }
       
        priNode<T>* current = head;        
        while (current->getNext() && priority <= current->getNext()->getPri()) {
            current = current->getNext();
        }
        newNode->setNext(current->getNext());
        current->setNext(newNode);
        count++;
    }

    bool dequeue(T& topEntry, int& pri) {
        if (isEmpty())
            return false;

        topEntry = head->getItem(pri);
        priNode<T>* temp = head;
        head = head->getNext();
        delete temp;
        count--;
        return true;
    }

    bool peek(T& topEntry, int& pri) {
        if (isEmpty())
            return false;

        topEntry = head->getItem(pri);
        return true;
    }

    bool isEmpty() const {
        return head == nullptr;
    }

    int getCount() const {
        return count;
    }

    void Print() const {
        cout << "Front[";

        priNode<T>* current = head;
        while (current) {
            int pri = 0;
            PrintItem(current->getItem(pri));
            cout << "(" << pri << ")";
            current = current->getNext();

            if (current)
                cout << ", ";
        }

        cout << "]Back";
    }
};
