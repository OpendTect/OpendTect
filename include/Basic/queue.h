#ifndef queue_h
#define queue_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Lammertink
 Date:		March 2004
 RCS:		$Id: queue.h,v 1.1 2004-03-17 11:58:11 arend Exp $
________________________________________________________________________

-*/

#include "thread.h"

template <class T> class QueueEntry
{
public:
			QueueEntry( T* item )
			    : value( item_value ), next( 0 ) {}

  T*			value;
  QueueEntry<T>*	next;
};


template <class T> class Queue
{
public:
			Queue() : head(0), tail(0) {}

			//! item becomes MINE!
    void		add( T* item ) 
			{
			    mutex.lock();
			    if (tail == (QueueEntry*)0)
				head = tail = new QueueEntry<T>( item );
			    else
			    {
				tail->next = new QueueEntry<T>( item );
				tail = tail->next;
			    }
			    mutex.unlock();
			}

			//! becomes YOURS!
    T*			next() 
			{
			    if ( !head ) return 0;

			    mutex.lock();

			    T* value = head->value;

			    QueueEntry<T>* old = head;
			    head = head->next;
			    delete old;

			    if ( !head ) tail = 0;

			    mutex.unlock();

			    return value;
			}
protected:

    Threads::Mutex	mutex;

    QueueEntry<T>*	head;
    QueueEntry<T>*	tail;

};

#endif
