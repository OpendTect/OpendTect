#ifndef valseries_h
#define valseries_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril & Kris Tingdahl
 Date:          Mar 2005
 RCS:           $Id: valseries.h,v 1.1 2005-03-08 11:55:44 cvsbert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"

/*\brief Interface to a series of values

  If the values are in contiguous memory, arr() should return non-null.
 
 */

template <class T>
class ValueSeries
{
public:

    virtual		~ValueSeries()		{}

    virtual T		value(int) const		= 0;
    virtual bool	writable() const		{ return false; }
    virtual void	setValue(int,T)			{}

    virtual T*		arr()				{ return 0; }
    virtual const T*	arr() const			{ return 0; }

    inline T		operator[]( int idx ) const	{ return value(idx); }

};


/*\brief series of values from a pointer to some kind of array */

template <class T>
class ArrayValueSeries : public ValueSeries<T>
{
public:

    		ArrayValueSeries( T* ptr, bool memmine=false )
		    : ptr_(ptr), mine_(memmine)	{}
    		~ArrayValueSeries()		{ if ( mine_ ) delete [] ptr_; }

    T		value( int idx ) const		{ return ptr_[idx]; }
    bool	writable() const		{ return true; }
    void	setValue( int idx, T v )	{ ptr_[idx] = v; }

    const T*	arr() const			{ return ptr_; }
    T*		arr()				{ return ptr_; }

protected:

    T*		ptr_;
    bool	mine_;

};


#endif
