/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2016
________________________________________________________________________

-*/


#include "refcount.h"

namespace RefCount
{

static od_int32 sStartRefCount()	{ return -2; }
static od_int32 sInvalidRefCount()	{ return -1; }


bool Referenced::isSane( const Referenced* ptr )
{
    return ptr && ptr->magicnumber_ == 0x123456789abcdef;
}


Referenced::Referenced( const Referenced& oth )
    : magicnumber_(oth.magicnumber_)
{
}


Referenced& Referenced::operator =( const Referenced& oth )
{
    return *this;
}


Referenced::~Referenced()
{
    const od_int32 count = refcount_.count();
    if ( count!=sStartRefCount() && refcount_.count()!=sInvalidRefCount() )
    {
	pErrMsg("It seems I'm deleted without an unref on an object "
		"that has been referenced." );
    }
}


void Referenced::ref() const
{
#ifdef __debug__
    /*If the object passed in ptr is truly 'Referenced' (and not just any
    class casted to Referenced, the magicnumber_ will
    be initialized. If it is not, well, then we should crash
    to let the developer figure out what is up. The check is only
    run on ref, as all pointers go through ref before unref.*/
    if ( !isSane(this) )
    {
	pErrMsg("Invalid this pointer.");
	DBG::forceCrash(false);
    }
#endif
    refcount_.ref();
    refNotify();
}


bool Referenced::refIfReffed() const
{
    if ( !refcount_.refIfReffed() )
	return false;

    refNotify();
    return true;
}


void Referenced::unRef() const
{
    unRefNotify();
    if ( refcount_.unRef() )
    {
	refcount_.clearAllObservers();
	const_cast<Referenced*>(this)->prepareForDelete();
	delete this;
    }

    return;
}


void Referenced::unRefNoDelete() const
{
    unRefNoDeleteNotify();
    refcount_.unRefDontInvalidate();
}


int Referenced::nrRefs() const
{
    return refcount_.count();
}


bool Referenced::tryRef() const
{
    if ( refcount_.tryRef() )
    {
	refNotify();
	return true;
    }

    return false;
}


void Referenced::addObserver( WeakPtrBase* obs )
{
    refcount_.addObserver( obs );
}


void Referenced::removeObserver(WeakPtrBase* obs)
{
    refcount_.removeObserver( obs );
}

#ifdef __win__
# define mDeclareCounters	od_int32 oldcount = count_.load(), newcount = 0
#else
# define mDeclareCounters	od_int32 oldcount = count_.load(), newcount;
#endif


// Counter
Counter::Counter()
    : count_( sStartRefCount() )
{}


Counter::Counter( const Counter& a )
    : count_( sStartRefCount() )
{}


od_int32 Counter::cInvalidRefCount()
{ return sInvalidRefCount(); }


od_int32 Counter::cStartRefCount()
{ return sStartRefCount(); }


void Counter::ref()
{
    mDeclareCounters;

    do
    {
	if ( oldcount==sInvalidRefCount() )
	{
	    pErrMsg("Invalid ref");
#ifdef __debug__
	    DBG::forceCrash(false);
#endif
	    newcount = 1; //Hoping for the best
	}
	else
	    newcount = oldcount==sStartRefCount() ? 1 : oldcount+1;

    } while ( !count_.setIfValueIs( oldcount, newcount, &oldcount ) );
}


bool Counter::tryRef()
{
    mDeclareCounters;

    do
    {
	if ( oldcount==sInvalidRefCount() )
	{
	    return false;
	}
	else if ( oldcount==sStartRefCount() )
	{
	    newcount = 1;
	}
	else
	{
	    newcount = oldcount+1;
	}

    } while ( !count_.setIfValueIs( oldcount, newcount, &oldcount ) );

    return true;
}


bool Counter::unRef()
{
    mDeclareCounters;

    do
    {
	if ( oldcount==sInvalidRefCount() )
	{
	    pErrMsg("Invalid reference.");
#ifdef __debug__
	    DBG::forceCrash(false);
	    newcount = 0; //To fool unitialized code warning
#else
	    return false;
#endif
	}
	else if ( oldcount==1 )
	    newcount = sInvalidRefCount();
	else
	    newcount = oldcount-1;

    } while ( !count_.setIfValueIs(oldcount,newcount, &oldcount ) );

    return newcount==sInvalidRefCount();
}


bool Counter::refIfReffed()
{
    mDeclareCounters;

    do
    {
	if ( oldcount==sInvalidRefCount() )
	{
	    pErrMsg("Invalid ref");
#ifdef __debug__
	    DBG::forceCrash(false);
#else
	    return false; //Hoping for the best
#endif
	}
	else if ( oldcount==sStartRefCount() || !oldcount)
	    //It can be zero only if unRefDontInvalidate has been called
	{
	    return false;
	}

	newcount = oldcount+1;

    } while ( !count_.setIfValueIs( oldcount, newcount, &oldcount ) );

    return true;
}


void Counter::unRefDontInvalidate()
{
    mDeclareCounters;

    do
    {
	if ( oldcount==sInvalidRefCount() )
	{
	    pErrMsg("Invalid reference.");
#ifdef __debug__
	    DBG::forceCrash(false);
	    newcount = 0; //Fool the unitialized warning
#else
	    newcount = 0; //Hope for the best
#endif
	}
	else
	    newcount = oldcount-1;

    } while ( !count_.setIfValueIs( oldcount, newcount, &oldcount ) );
}

#undef mDeclareCounters


void Counter::clearAllObservers()
{
    observerslock_.lock();

    for ( auto observer : observers_ )
	observer->clearPtr();
    observers_.erase();

    observerslock_.unLock();
}


void Counter::addObserver( WeakPtrBase* obj )
{
    observerslock_.lock();
    observers_.addIfNew( obj );
    observerslock_.unLock();
}


void Counter::removeObserver( WeakPtrBase* obj )
{
    observerslock_.lock();
    observers_ -= obj;
    observerslock_.unLock();
}


// WeakPtrBase
WeakPtrBase::WeakPtrBase()
    : ptr_( 0 )
{}

void WeakPtrBase::clearPtr()
{
    lock_.lock();
    ptr_ = 0;
    lock_.unLock();
}


WeakPtrBase::operator bool() const
{
    lock_.lock();
    bool res = ptr_;
    lock_.unLock();
    return res;
}


bool WeakPtrBase::operator!() const
{
    lock_.lock();
    bool res = ptr_;
    lock_.unLock();
    return !res;
}


void WeakPtrBase::set( Referenced* p )
{
    lock_.lock();

    while ( ptr_ )
    {
	if ( !ptr_->tryRef() )
	{
	    //Object is closing down
	    lock_.unLock();

	    //Sleep to let the closing refcount call clear();
	    Threads::sleep( 0.001 );

	    lock_.lock();

	    continue;
	}

#ifdef __debug__
	if ( ptr_->nrRefs()==1 )
	{
	    pErrMsg("Observing object that is not reffed. "
		    "This will always fail");
	}
#endif

	Referenced* tmpptr = ptr_;

	tmpptr->removeObserver(this);
	ptr_ = 0;
	lock_.unLock();
	tmpptr->unRef(); //We may not be locked during unref, for safety
	lock_.lock();
    }

    if ( p && p->tryRef() )
    {
#ifdef __debug__
	if ( p->nrRefs()==1 )
	{
	    pErrMsg("I am the only reffer - not good" );
	}
#endif

	p->addObserver(this);
	ptr_ = p;

	//We may not be locked during unref as clear may be called
	lock_.unLock();
	p->unRef();
    }
    else
	lock_.unLock();
}



void WeakPtrSetBase::blockCleanup()
{
    while ( true )
    {
        int prevval = blockcleanup_;
        if ( prevval==-1 ) //cleanup in session
            continue;

        if ( blockcleanup_.setIfValueIs( prevval, prevval+1 ) )
            break;
    }
}


void WeakPtrSetBase::unblockCleanup()
{
    blockcleanup_--;
}

} // namespace RefCount
