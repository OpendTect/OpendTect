#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2001 / Mar 2016
 Contents:	PickSet base classes
________________________________________________________________________

-*/

#include "monitoredobject.h"


/*!\brief base class for MonitoredObject iterators. Inherit from one of its
	    subclasses.

  Needs a next() before a valid item is reached.

  It is your responisbility to keep the MonitoredObject alive. The iterator
  cannot itself be used in a Multi-Threaded way.

  If you pass a negative startidx or stopidx, then the iterator will be empty
  (no matter the value of the other idx).

  */

template <class ITyp>
mClass(Basic) MonitorableIterBase
{
public:

    typedef ITyp	idx_type;
    typedef idx_type	size_type;
    enum Direction	{ Forward, Backward };

    inline		MonitorableIterBase(const MonitoredObject&,
					    idx_type start,idx_type stop);
    inline		MonitorableIterBase(const MonitorableIterBase&);
    inline virtual	~MonitorableIterBase()	{ retire(); }
    inline const MonitoredObject& monitored() const	{ return obj_; }

    inline bool		isEmpty() const		{ return size() < 1; }
    inline size_type	size() const;

    inline bool		next();
    inline bool		isForward() const	{ return dir_ == Forward; }

    inline bool		isValid() const		{ return isPresent(curidx_); }
    inline bool		atFirst() const		{ return curidx_ == startidx_; }
    inline bool		atLast() const		{ return curidx_ == stopidx_; }
    inline idx_type	curIdx() const		{ return curidx_; }
    inline bool		isPresent(idx_type) const;

    virtual void	retire()		{}
    virtual void	reInit();

protected:

    const MonitoredObject&	obj_;
    const Direction	dir_;
    const idx_type	startidx_;
    const idx_type	stopidx_;

    idx_type		curidx_;

    MonitorableIterBase& operator=( const MonitorableIterBase& ) = delete;

};



/*!\brief base class for const MonitoredObject iterator.

  Will MonitorLock, so when done before going out of
  scope, calling retire() will lift the lock early (this is what you want).

  */

template <class ITyp>
mClass(Basic) MonitorableIter4Read : public MonitorableIterBase<ITyp>
{
public:

    inline		MonitorableIter4Read(const MonitoredObject&,
					     ITyp startidx,ITyp stopidx);
    inline		MonitorableIter4Read(const MonitorableIter4Read&);

    void		reInit() override;
    void		retire() override;

protected:

    MonitorLock		ml_;

    MonitorableIter4Read& operator=( const MonitorableIter4Read& ) = delete;

};


/*!\brief base class for non-const MonitoredObject iterator.

  Will not MonitorLock, so do not use this on objects that are shared.
  For that, use the copy -> change copy -> assign approach.

  */

template <class ITyp>
mClass(Basic) MonitorableIter4Write : public MonitorableIterBase<ITyp>
{
public:

    inline		MonitorableIter4Write(MonitoredObject&,
					      ITyp startidx,ITyp stopidx);
    inline		MonitorableIter4Write(const MonitorableIter4Write&);

    inline MonitoredObject&	edited()
			{ return mNonConst( mSelf().monitored() ); }

protected:

    inline void		insertedAtCurrent();
    inline void		currentRemoved();

    MonitorableIter4Write& operator=( const MonitorableIter4Write& ) = delete;

};


template <class ITyp> inline
MonitorableIterBase<ITyp>::MonitorableIterBase( const MonitoredObject& obj,
					ITyp startidx, ITyp stopidx )
    : obj_(obj)
    , startidx_(startidx)
    , stopidx_(stopidx)
    , dir_(startidx<=stopidx ? Forward : Backward)
{
    if ( startidx_ < 0 || stopidx_ < 0 )
    {
	// empty. make this a standard situation:
	mNonConst( startidx_ ) = 0;
	mNonConst( stopidx_ ) = -1;
	mNonConst( dir_ ) = Forward;
    }
    reInit();
}


template <class ITyp> inline
MonitorableIterBase<ITyp>::MonitorableIterBase( const MonitorableIterBase& oth )
    : obj_(oth.monitored())
    , startidx_(oth.startidx_)
    , stopidx_(oth.stopidx_)
    , dir_(oth.dir_)
    , curidx_(oth.curidx_)
{
}


template <class ITyp> inline
bool MonitorableIterBase<ITyp>::next()
{
    if ( dir_ == Forward )
	{ curidx_++; return curidx_ <= stopidx_; }
    else
	{ curidx_--; return curidx_ >= stopidx_; }
}


template <class ITyp> inline
bool MonitorableIterBase<ITyp>::isPresent( idx_type idx ) const
{
    if ( dir_ == Forward )
	return idx >= startidx_ && idx <= stopidx_;
    else
	return idx <= startidx_ && idx >= stopidx_;
}


template <class ITyp> inline typename
MonitorableIterBase<ITyp>::size_type MonitorableIterBase<ITyp>::size() const
{
    return dir_ == Forward ? stopidx_-startidx_+1 : startidx_-stopidx_+1;
}


template <class ITyp> inline
void MonitorableIterBase<ITyp>::reInit()
{
    curidx_ = dir_ == Forward ? startidx_ - 1 : startidx_ + 1;
}


template <class ITyp> inline
MonitorableIter4Read<ITyp>::MonitorableIter4Read( const MonitoredObject& obj,
						  ITyp startidx, ITyp stopidx )
    : MonitorableIterBase<ITyp>(obj,startidx,stopidx)
    , ml_(obj)
{
}


template <class ITyp> inline
MonitorableIter4Read<ITyp>::MonitorableIter4Read(
				const MonitorableIter4Read& oth )
    : MonitorableIterBase<ITyp>(oth)
    , ml_(oth.obj_)
{
}


template <class ITyp> inline
void MonitorableIter4Read<ITyp>::retire()
{
    ml_.unlockNow();
}


template <class ITyp> inline
void MonitorableIter4Read<ITyp>::reInit()
{
    ml_.reLock();
    MonitorableIterBase<ITyp>::reInit();
}


template <class ITyp> inline
MonitorableIter4Write<ITyp>::MonitorableIter4Write( MonitoredObject& obj,
						  ITyp startidx, ITyp stopidx )
    : MonitorableIterBase<ITyp>(obj,startidx,stopidx)
{
}


template <class ITyp> inline
MonitorableIter4Write<ITyp>::MonitorableIter4Write(
				const MonitorableIter4Write& oth )
    : MonitorableIterBase<ITyp>(oth)
{
}


template <class ITyp> inline
void MonitorableIter4Write<ITyp>::insertedAtCurrent()
{
    if ( this->dir_ == MonitorableIterBase<ITyp>::Backward )
	mNonConst(this->startidx_)++;
    else
    {
	mNonConst(this->stopidx_)++;
	this->curidx_++;
    }
}


template <class ITyp> inline
void MonitorableIter4Write<ITyp>::currentRemoved()
{
    if ( this->dir_ == MonitorableIterBase<ITyp>::Backward )
	mNonConst(this->startidx_)--;
    else
    {
	mNonConst(this->stopidx_)--;
	this->curidx_--;
    }
}
