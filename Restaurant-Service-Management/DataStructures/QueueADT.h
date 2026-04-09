/*
	This is an updated version of code originally
	created by Frank M. Carrano and Timothy M. Henry.
	Copyright (c) 2017 Pearson Education, Hoboken, New Jersey.
*/

/** @file QueueADT.h */
#ifndef QUEUE_ADT_
#define QUEUE_ADT_

template<class T>
class QueueADT
{
public:
   virtual bool isEmpty() const = 0;
   virtual bool enqueue(const T& newEntry) = 0;
   virtual bool dequeue(T& FrontEntry) = 0;
   virtual bool peek(T& FrontEntry) const = 0;
   virtual ~QueueADT() { }
};
#endif
