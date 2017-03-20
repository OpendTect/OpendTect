#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril & Kris Tingdahl
 Date:          Mar 2005
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"


/*!\brief Interface to a series of values.

  If the values are in contiguous memory, arr() should return non-null.
*/

template <class T>
mClass(Basic) ValueSeries
{
public:

    typedef T		ValueType;

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

    virtual T*		arr()				{ return 0; }
    virtual const T*	arr() const			{ return 0; }

    virtual char	bytesPerItem() const		{ return sizeof(T); }

    inline T		operator[](od_int64 idx) const	{ return value(idx); }
};
