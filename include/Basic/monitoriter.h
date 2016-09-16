#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2001 / Mar 2016
 Contents:	PickSet base classes
________________________________________________________________________

-*/

#include "monitor.h"



/*!\brief base class for const Monitorable iterator.
  Will MonitorLock, so when done before going out of
  scope, call retire().

  Needs a next() or prev() before a valid item is reached.

  It is your responisbility to keep the Monitorable alive. The iterator cannot
  itself be used in a Multi-Threaded way.

  */

template <class ITyp>
mClass(Basic) MonitorableIter
{
public:

    typedef ITyp	IdxType;
    typedef IdxType	size_type;

    inline		MonitorableIter(const Monitorable&,IdxType startidx);
    inline		MonitorableIter(const Monitorable&,IdxType startidx,
							   IdxType stopidx);

    inline		MonitorableIter(const MonitorableIter&);
    inline virtual	~MonitorableIter()	{ retire(); }
    inline const Monitorable& monitored() const	{ return obj_; }

    virtual size_type	size() const		= 0;

    inline bool		next();
    inline bool		prev();

    inline bool		isValid() const;
    inline bool		atFirst() const		{ return curidx_ == startidx_; }
    inline bool		atLast() const;

    inline void		retire();
    inline void		reInit(bool toend=false);

protected:

    const Monitorable&	obj_;
    IdxType		startidx_;
    IdxType		stopidx_;
    IdxType		curidx_;
    MonitorLock		ml_;

    inline MonitorableIter& operator =(const MonitorableIter&); // pErrMsg

};


template <class ITyp> inline
MonitorableIter<ITyp>::MonitorableIter( const Monitorable& obj, ITyp startidx )
    : obj_(obj)
    , ml_(obj)
    , startidx_(startidx)
    , stopidx_(-1)
    , curidx_(startidx)
{
}


template <class ITyp> inline
MonitorableIter<ITyp>::MonitorableIter( const Monitorable& obj,
					ITyp startidx,
					ITyp stopidx )
    : obj_(obj)
    , ml_(obj)
    , startidx_(startidx)
    , stopidx_(stopidx)
    , curidx_(startidx)
{
}


template <class ITyp> inline
MonitorableIter<ITyp>::MonitorableIter( const MonitorableIter& oth )
    : obj_(oth.monitored())
    , ml_(oth.monitored())
    , startidx_(oth.startidx_)
    , stopidx_(oth.stopidx_)
    , curidx_(oth.curidx_)

{
}


template <class ITyp> inline
MonitorableIter<ITyp>& MonitorableIter<ITyp>::operator =(
						const MonitorableIter& oth )
{
    pErrMsg( "No assignment" );
    return *this;
}


template <class ITyp> inline
bool MonitorableIter<ITyp>::next()
{
    curidx_++;
    return curidx_ < ( stopidx_ < 0 ? size() : stopidx_ );
}


template <class ITyp> inline
bool MonitorableIter<ITyp>::prev()
{
    curidx_--;
    return curidx_ >= startidx_+1;
}


template <class ITyp> inline
bool MonitorableIter<ITyp>::isValid() const
{
    return curidx_ >= startidx_+1 && curidx_ < ( stopidx_ < 0 ? size()
							      : stopidx_ );
}


template <class ITyp> inline
bool MonitorableIter<ITyp>::atLast() const
{
    return curidx_ == ( stopidx_ < 0 ? size() : stopidx_ );
}


template <class ITyp> inline
void MonitorableIter<ITyp>::retire()
{
    ml_.unlockNow();
}


template <class ITyp> inline
void MonitorableIter<ITyp>::reInit( bool toend )
{
    ml_.reLock();
    curidx_ = toend ? ( stopidx_ < 0 ? size() : stopidx_ ) : startidx_;
}


