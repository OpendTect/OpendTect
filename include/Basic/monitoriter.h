#ifndef monitoriter_h
#define monitoriter_h

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
mExpClass(General) MonitorableIter
{
public:

    typedef ITyp	IdxType;

    inline		MonitorableIter(const Monitorable&,IdxType startidx);
    inline		MonitorableIter(const MonitorableIter&);
    inline virtual	~MonitorableIter()	{ retire(); }
    inline const Monitorable& monitored() const	{ return obj_; }

    virtual IdxType	size() const		= 0;

    inline bool		next();
    inline bool		prev();

    inline bool		isValid() const;
    inline bool		atFirst() const		{ return curidx_ == 0; }
    inline bool		atLast() const;

    inline void		retire();
    inline void		reInit(bool toend=false);

protected:

    const Monitorable&	obj_;
    IdxType		curidx_;
    MonitorLock		ml_;

    inline MonitorableIter& operator =(const MonitorableIter&); // pErrMsg

};


template <class ITyp> inline
MonitorableIter<ITyp>::MonitorableIter( const Monitorable& obj, ITyp startidx )
    : obj_(obj)
    , curidx_(startidx)
    , ml_(obj)
{
}


template <class ITyp> inline
MonitorableIter<ITyp>::MonitorableIter( const MonitorableIter& oth )
    : obj_(oth.monitored())
    , curidx_(oth.curidx_)
    , ml_(oth.monitored())
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
    return curidx_ < size();
}


template <class ITyp> inline
bool MonitorableIter<ITyp>::prev()
{
    curidx_--;
    return curidx_ >= 0 ;
}


template <class ITyp> inline
bool MonitorableIter<ITyp>::isValid() const
{
    return curidx_ >= 0 && curidx_ < size();
}


template <class ITyp> inline
bool MonitorableIter<ITyp>::atLast() const
{
    return curidx_ == size() - 1;
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
    curidx_ = toend ? size() : -1;
}


#endif
