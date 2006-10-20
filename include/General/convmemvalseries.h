#ifndef convmemvalseries_h
#define convmemvalseries_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kris Tingdahl
 Date:          Oct 2006
 RCS:           $Id: convmemvalseries.h,v 1.1 2006-10-20 21:35:27 cvskris Exp $
________________________________________________________________________

-*/

#include "valseries.h"
#include "datainterp.h"
#include "bindatadesc.h"


/*!ValueSeries that holds data in memory, but the memory may be of a different
format than T. I.e. a ValueSeries<float> can have it's values stored as 
chars. */


template <class T>
class ConvMemValueSeries : public ValueSeries<T>
{
public:

    		ConvMemValueSeries( int sz, const BinDataDesc& stortype )
		    : ptr_( 0 )
		    , size_( sz )
		    , interpreter_( DataCharacteristics(stortype) )
		{
		    T dummy;
		    rettype_ = BinDataDesc( dummy );
		    ptr_ = new char[sz*interpreter_.nrBytes()];
		}

		~ConvMemValueSeries() { delete [] ptr_; }

    T		value(int idx) const
    		{ return interpreter_.get( ptr_, idx ); }

    bool	writable() const		{ return true; }

    int		size() const 			{ return size_; }

    void	setValue( int idx, T v )
		{
		    interpreter_.put( ptr_, idx, v );
		}

    const T*	arr() const
    		{ return interpreter_.dataChar()==rettype_ ? (T*) ptr_ : 0; }
    T*		arr()
		{ return interpreter_.dataChar()==rettype_ ? (T*) ptr_ : 0; }

    char*	storArr()	{ return ptr_; }
    const char*	storArr() const	{ return ptr_; }

protected:

    DataInterpreter<T>	interpreter_;
    BinDataDesc		rettype_;

    char*		ptr_;
    int			size_;
};


#endif
