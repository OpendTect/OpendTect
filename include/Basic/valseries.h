#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril & Kris Tingdahl
 Date:          Mar 2005
________________________________________________________________________

-*/

#include "basicmod.h"
#include "odmemory.h"
#include "paralleltask.h"


/*!
\brief Interface to a series of values.

  If the values are in contiguous memory, arr() should return non-null.
*/

template <class T>
mClass(Basic) ValueSeries
{
public:

    virtual		~ValueSeries()		{}

    void		getValues(ValueSeries<T>&,od_int64 nrvals) const;
    void		getValues(T*,od_int64 nrvals) const;

    virtual ValueSeries<T>* clone() const			= 0;
    virtual bool	isOK() const			{ return true; }

    virtual T		value(od_int64) const		= 0;
    virtual bool	writable() const		{ return false; }
    virtual void	setValue(od_int64,T)		{}

    virtual bool	canSetAll() const		{ return false; }
    virtual void	setAll(T)			{}

    virtual bool	selfSufficient() const		{ return false; }
			/*!<\returns true if not depending on other objects */
    virtual bool	reSizeable() const		{ return false; }
    virtual bool	setSize(od_int64)		{ return false; }

    virtual T*		arr()				{ return nullptr; }
    virtual const T*	arr() const			{ return nullptr; }

    virtual od_int64	size() const			= 0;
    virtual char	bytesPerItem() const		{ return sizeof(T); }

    inline T		operator[](od_int64 idx) const	{ return value(idx); }
};


/*!\brief Gets ValueSeries.*/

template <class T>
mClass(Basic) ValueSeriesGetAll : public ParallelTask
{
public:

ValueSeriesGetAll(const ValueSeries<T>& from,
		  ValueSeries<T>& to, od_int64 nrelements )
    : from_( from )
    , to_( &to )
    , toptr_( nullptr )
    , nrelements_( nrelements )
{
}

ValueSeriesGetAll(const ValueSeries<T>& from, T* to,
		  od_int64 nrelements	)
    : from_( from )
    , toptr_( to )
    , to_( nullptr )
    , nrelements_( nrelements )
{
}

od_int64 nrIterations() const
{
    return nrelements_;
}


bool doWork( od_int64 start, od_int64 stop, int )
{
    od_int64 nrleft = stop-start+1;
    const T* fromarr = from_.arr();
    T* toarr = toptr_ ? toptr_ : to_->arr();
    if ( toarr && fromarr )
	OD::sysMemCopy( toarr+start, fromarr+start,
		(size_t) (nrleft*from_.bytesPerItem()) );
    else if ( toarr )
    {
	toarr += start;
	for ( od_int64 idx=start; idx<=stop; idx++, toarr++ )
	    *toarr = from_.value( idx );
    }
    else if ( fromarr )
    {
	fromarr += start;
	for ( od_int64 idx=start; idx<=stop; idx++, fromarr++ )
	    to_->setValue(idx, *fromarr );
    }
    else
    {
	for ( od_int64 idx=start; idx<=stop; idx++ )
	    to_->setValue(idx,from_.value(idx));
    }

    return true;
}

protected:

    od_int64			nrelements_;
    const ValueSeries<T>&	from_;
    ValueSeries<T>*		to_;
    T*				toptr_;

};


template <class T> inline
void ValueSeries<T>::getValues( ValueSeries<T>& to, od_int64 nrvals ) const
{
    ValueSeriesGetAll<T> setter( *this, to, nrvals );
    setter.execute();
}


template <class T> inline
void ValueSeries<T>::getValues( T* to, od_int64 nrvals ) const
{
    ValueSeriesGetAll<T> setter( *this, to, nrvals );
    setter.execute();
}
