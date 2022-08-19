#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "valseries.h"
#include "datainterp.h"
#include "datachar.h"
#include "scaler.h"
#include "undefarray.h"


/*!ValueSeries that holds data in memory, but the memory may be of a different
format than T. I.e. a ValueSeries<float> can have it's values stored as
chars. */


template <class T>
mClass(General) ConvMemValueSeries : public ValueSeries<T>
{
public:

    inline	    	ConvMemValueSeries(od_int64 sz,
	    				   const BinDataDesc& stortype,
					   bool doundef=true,
					   const Scaler* scaler=0);

    inline		~ConvMemValueSeries();
    inline bool		isOK() const override;

    ValueSeries<T>*	clone() const override;

    inline od_int64	size() const override;
    inline bool		writable() const override;
    inline T		value(od_int64 idx) const override;
    inline void		setValue( od_int64 idx, T v ) override;

    inline bool		selfSufficient() const override { return true; }
    inline bool		reSizeable() const override	{ return true; }
    inline bool		setSize(od_int64) override;

    inline const T*	arr() const override;
    inline T*		arr() override;

    inline char*	storArr();
    inline const char*	storArr() const;

    inline BinDataDesc	dataDesc() const;
    bool		handlesUndef() const { return undefhandler_; }

protected:

    UndefArrayHandler*	undefhandler_;
    DataInterpreter<T>	interpreter_;
    BinDataDesc		rettype_;
    const Scaler*	scaler_;

    char*		ptr_;
    od_int64		size_;
};


template <class T> inline
ConvMemValueSeries<T>::ConvMemValueSeries( od_int64 sz,
					   const BinDataDesc& stortype,
					   bool doundef,
					   const Scaler* scaler )
    : ptr_( 0 )
    , scaler_( scaler ? scaler->clone() : 0 )
    , size_( -1 )
    , interpreter_( DataCharacteristics(stortype) )
    , undefhandler_( doundef ? new UndefArrayHandler(stortype) : 0 )
{
    if ( undefhandler_ && !undefhandler_->isOK() )
    { delete undefhandler_; undefhandler_ = 0; }

    T dummy;
    rettype_ = BinDataDesc( dummy );
    setSize( sz );
}


template <class T> inline
ConvMemValueSeries<T>::~ConvMemValueSeries()
{
    delete [] ptr_;
    delete scaler_;
    delete undefhandler_;
}


template <class T> inline
ValueSeries<T>*	ConvMemValueSeries<T>::clone() const
{
    ConvMemValueSeries<T>* res = new ConvMemValueSeries( size_,
	    interpreter_.dataChar(), undefhandler_, scaler_ );
    if ( storArr() )
	OD::memCopy( res->storArr(), storArr(), size_ * interpreter_.nrBytes());

    return res;
}



template <class T> inline
bool ConvMemValueSeries<T>::isOK() const
{
    if ( undefhandler_ && !undefhandler_->isOK() )
	return false;

    return ptr_;
}


template <class T> inline
T ConvMemValueSeries<T>::value(od_int64 idx) const
{
    if ( undefhandler_ && undefhandler_->isUdf( ptr_,idx ) )
	return mUdf(T);

    const T val = interpreter_.get( ptr_, idx );
    return mCast( T, scaler_ ? scaler_->unScale(val) : val );
}


template <class T> inline
bool ConvMemValueSeries<T>::setSize( od_int64 sz )
{
    if ( sz==size_ ) return true;

    delete [] ptr_;
    mTryAlloc( ptr_, char[sz*interpreter_.nrBytes()] );
    return ptr_;
}


template <class T> inline
bool ConvMemValueSeries<T>::writable() const
{ return true; }


template <class T> inline
od_int64 ConvMemValueSeries<T>::size() const
{ return size_; }


template <class T> inline
void ConvMemValueSeries<T>::setValue( od_int64 idx, T v )
{
    if ( undefhandler_ && mIsUdf(v) )
	undefhandler_->setUdf( ptr_, idx );
    else
    {
	const T val = mCast( T, scaler_ ? scaler_->scale(v) : v );
	interpreter_.put( ptr_, idx, val );
	if ( undefhandler_ ) undefhandler_->unSetUdf( ptr_, idx );
    }
}


template <class T> inline
const T* ConvMemValueSeries<T>::arr() const
{
    return interpreter_.dataChar() == rettype_
	 ? reinterpret_cast<const T*>(ptr_) : 0;
}

template <class T> inline
T* ConvMemValueSeries<T>::arr()
{ return interpreter_.dataChar()==rettype_ ? reinterpret_cast<T*>(ptr_) : 0; }


template <class T> inline
char* ConvMemValueSeries<T>::storArr()
{ return ptr_; }


template <class T> inline
const char* ConvMemValueSeries<T>::storArr() const
{ return ptr_; }


template <class T> inline
BinDataDesc ConvMemValueSeries<T>::dataDesc() const
{ return interpreter_.dataChar(); }
