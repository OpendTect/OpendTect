#ifndef queue_h
#define queue_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Lammertink
 Date:		March 2004
 RCS:		$Id$
________________*_______________________________________________________

-*/

#include "threadlock.h"


/*!
\brief One single queue entry.
*/

template <class T>
mClass(Basic) QueueEntry
{
public:
			QueueEntry( T item )
			    : value( item ), next( 0 ) {}

    T			value;
    QueueEntry<T>*	next;
};


/*!
\brief Queue of objects.
*/

template <class T>
mClass(Basic) ObjQueue
{
public:
			ObjQueue() : head(0), tail(0), lock_(true)	{}

			//! item becomes MINE!
    void		add( T* item ) 
			{
			    Threads::Locker lckr( lock_ );
			    if ( !tail )
				head = tail = new QueueEntry<T*>( item );
			    else
			    {
				tail->next = new QueueEntry<T*>( item );
				tail = tail->next;
			    }
			}

			//! becomes YOURS!
    T*			next() 
			{
			    if ( !head ) return 0;
			    Threads::Locker lckr( lock_ );

			    T* value = head->value;

			    QueueEntry<T*>* old = head;
			    head = head->next;
			    delete old;

			    if ( !head ) tail = 0;
			    return value;
			}
protected:

    Threads::Lock	lock_;

    QueueEntry<T*>*	head;
    QueueEntry<T*>*	tail;

};


/*!
\brief Queue of basic data types.
*/

template <class T>
mClass(Basic) TypeQueue
{
public:
			TypeQueue() : head(0), tail(0), lock_(true)	{}

    void		add( T item ) 
			{
			    Threads::Locker lckr( lock_ );
			    if ( !tail )
				head = tail = new QueueEntry<T>( item );
			    else
			    {
				tail->next = new QueueEntry<T>( item );
				tail = tail->next;
			    }
			}

    bool		empty()		{ return !head; }

    T			next() 
			{
			    if ( empty() ) return 0;
			    Threads::Locker lckr( lock_ );

			    T value = head->value;

			    QueueEntry<T>* old = head;
			    head = head->next;
			    delete old;

			    if ( !head ) tail = 0;

			    return value;
			}
protected:

    Threads::Lock	lock_;

    QueueEntry<T>*	head;
    QueueEntry<T>*	tail;

};


#endif
