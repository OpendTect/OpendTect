#ifndef queue_h
#define queue_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Lammertink
 Date:		March 2004
 RCS:		$Id: queue.h,v 1.3 2007-11-15 13:17:32 cvskris Exp $
________________*_______________________________________________________

-*/

#include "thread.h"

template <class T> class QueueEntry
{
public:
			QueueEntry( T item )
			    : value( item ), next( 0 ) {}

    T			value;
    QueueEntry<T>*	next;
};


template <class T> class ObjQueue
{
public:
			ObjQueue() : head(0), tail(0) {}

			//! item becomes MINE!
    void		add( T* item ) 
			{
			    mutex.lock();
			    if ( !tail )
				head = tail = new QueueEntry<T*>( item );
			    else
			    {
				tail->next = new QueueEntry<T*>( item );
				tail = tail->next;
			    }
			    mutex.unLock();
			}

			//! becomes YOURS!
    T*			next() 
			{
			    if ( !head ) return 0;

			    mutex.lock();

			    T* value = head->value;

			    QueueEntry<T*>* old = head;
			    head = head->next;
			    delete old;

			    if ( !head ) tail = 0;

			    mutex.unLock();

			    return value;
			}
protected:

    Threads::Mutex	mutex;

    QueueEntry<T*>*	head;
    QueueEntry<T*>*	tail;

};


template <class T> class TypeQueue
{
public:
			TypeQueue() : head(0), tail(0) {}

    void		add( T item ) 
			{
			    mutex.lock();
			    if ( !tail )
				head = tail = new QueueEntry<T>( item );
			    else
			    {
				tail->next = new QueueEntry<T>( item );
				tail = tail->next;
			    }
			    mutex.unLock();
			}

    bool		empty()		{ return !head; }

    T			next() 
			{
			    if ( empty() ) return 0;

			    mutex.lock();

			    T value = head->value;

			    QueueEntry<T>* old = head;
			    head = head->next;
			    delete old;

			    if ( !head ) tail = 0;

			    mutex.unLock();

			    return value;
			}
protected:

    Threads::Mutex	mutex;

    QueueEntry<T>*	head;
    QueueEntry<T>*	tail;

};

#endif
