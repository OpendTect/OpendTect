#ifndef valseries_h
#define valseries_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril & Kris Tingdahl
 Date:          Mar 2005
 RCS:           $Id: valseries.h,v 1.14 2007-11-20 18:20:20 cvskris Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
#include "errh.h"

/*\brief Interface to a series of values

  If the values are in contiguous memory, arr() should return non-null.
 
 */

template <class T>
class ValueSeries
{
public:

    virtual		~ValueSeries()		{}

    virtual bool	isOK() const			{ return true; }

    virtual T		value(od_int64) const		= 0;
    virtual bool	writable() const		{ return false; }
    virtual void	setValue(od_int64,T)		{}

    virtual bool	reSizeable() const		{ return false; }
    virtual bool	setSize(od_int64) 		{ return false; }

    virtual T*		arr()				{ return 0; }
    virtual const T*	arr() const			{ return 0; }

    inline T		operator[](od_int64 idx) const	{ return value(idx); }

};


template <class T>
class OffsetValueSeries : public ValueSeries<T>
{
public:
    inline		OffsetValueSeries( ValueSeries<T>& src, od_int64 off );
    inline		OffsetValueSeries( const ValueSeries<T>& src,
	    				   od_int64 off);

    inline T		value( od_int64 idx ) const;
    inline void		setValue( od_int64 idx, T v ) const;
    inline T*		arr();
    inline const T*	arr() const;
    inline bool		writable() const;

    inline od_int64	getOffset() const;
    inline void		setOffset(od_int64 no);

    const ValueSeries<T>&	source() const { return src_; }

protected:
    ValueSeries<T>&	src_;
    od_int64		off_;
    bool		writable_;
};


#define mImplArr \
{ return typeid(RT)==typeid(AT) ? (RT*) ptr_ : (RT*) 0;}

/*\brief series of values from a pointer to some kind of array. If a more
         advanced conversion between the return type and the array type is
	 wanted, use ConvValueSeries instead. */

template <class RT, class AT>
class ArrayValueSeries : public ValueSeries<RT>
{
public:

    		ArrayValueSeries( AT* ptr, bool memmine, od_int64 sz=-1 );
    		ArrayValueSeries( od_int64 sz );
    		~ArrayValueSeries()		{ if ( mine_ ) delete [] ptr_; }

    bool	isOK() const			{ return ptr_; }

    RT		value( od_int64 idx ) const	{ return ptr_[idx]; }
    bool	writable() const		{ return true; }
    void	setValue( od_int64 idx, RT v )	{ ptr_[idx] = (AT) v; }

    const RT*	arr() const			mImplArr;
    RT*		arr()				mImplArr;

    bool	reSizeable() const		{ return mine_; }
    inline bool	setSize(od_int64);
    od_int64	size() const			{ return cursize_; }

protected:

    AT*		ptr_;
    bool	mine_;
    od_int64	cursize_;
};

#undef mImplArr

#define mChunkSize	0x20000000


/*!Valueseries that allocates its data in smaller chunks
   (default is 512MB). Bydoing this, it performs better in environments
   where the memory is fragmented (i.e. windows 32 bit). */

template <class RT, class AT>
class MultiArrayValueSeries : public ValueSeries<RT>
{
public:
    		MultiArrayValueSeries(od_int64);
    		~MultiArrayValueSeries();

    bool	isOK() const			{ return ptrs_.size(); }

    RT		value( od_int64 idx ) const;
    bool	writable() const		{ return true; }
    void	setValue(od_int64 idx, RT v);

    const RT*	arr() const;
    RT*		arr();

    bool	reSizeable() const		{ return true; }
    inline bool	setSize(od_int64);
    od_int64	size() const			{ return cursize_; }

protected:
    ObjectSet<AT>	ptrs_;
    od_int64		cursize_;
    const unsigned int	chunksize_;
};



template <class T> inline
OffsetValueSeries<T>::OffsetValueSeries( ValueSeries<T>& src, od_int64 off )
    : src_( src ), off_( off ), writable_(true) 
{}


template <class T> inline
OffsetValueSeries<T>::OffsetValueSeries(const ValueSeries<T>& src,od_int64 off)
    : src_( const_cast<ValueSeries<T>& >(src) ), off_( off ), writable_(false) 
{}


template <class T> inline
T OffsetValueSeries<T>::value( od_int64 idx ) const
{ return src_.value(idx+off_); }

template <class T> inline
void OffsetValueSeries<T>::setValue( od_int64 idx, T v ) const
{
    if ( writable_ )
	src_.setValue(idx+off_,v);
    else
	pErrMsg("Attempting to write to write-protected array");
}


template <class T> inline
T* OffsetValueSeries<T>::arr()
{ T* p = src_.arr(); return p ? p+off_ : 0; }


template <class T> inline
const T* OffsetValueSeries<T>::arr() const
{ T* p = src_.arr(); return p ? p+off_ : 0; }


template <class T> inline
od_int64 OffsetValueSeries<T>::getOffset() const
{ return off_; }


template <class T> inline
void OffsetValueSeries<T>::setOffset(od_int64 no)
{ off_ = no; }


template <class T> inline
bool OffsetValueSeries<T>::writable() const
{ return writable_; }


template <class RT, class AT>
ArrayValueSeries<RT,AT>::ArrayValueSeries(AT* ptr, bool memmine,od_int64 cursz )
    : ptr_(ptr), mine_(memmine), cursize_( cursz )
{}


template <class RT, class AT>
ArrayValueSeries<RT,AT>::ArrayValueSeries( od_int64 sz )
    : mine_(true), cursize_( -1 )
{
    setSize( sz );
}


template <class RT,class AT> inline
bool ArrayValueSeries<RT,AT>::setSize( od_int64 sz )
{
    if ( cursize_!=-1 && cursize_==sz && ptr_ ) return true;
    if ( !mine_ ) return false;

    delete [] ptr_;
    mTryAlloc( ptr_, AT[sz] )
    cursize_ = sz;
    return ptr_;
}


template <class RT, class AT> inline
MultiArrayValueSeries<RT,AT>::MultiArrayValueSeries( od_int64 sz )
    : cursize_( -1 )
    , chunksize_( mChunkSize/sizeof(AT) )
{
    ptrs_.allowNull( true );
    setSize( sz );
}


template <class RT, class AT> inline
MultiArrayValueSeries<RT,AT>::~MultiArrayValueSeries()
{
    deepEraseArr( ptrs_ );
}


template <class RT, class AT> inline
RT MultiArrayValueSeries<RT,AT>::value( od_int64 idx ) const
{
    const od_int64 arridx = idx/chunksize_;
    if ( arridx>=ptrs_.size() )
	return RT();

    idx -= arridx*chunksize_;
    return ptrs_[arridx][idx];
}


template <class RT, class AT> inline
void MultiArrayValueSeries<RT,AT>::setValue( od_int64 idx, RT v )
{
    const od_int64 arridx = idx/chunksize_;
    if ( arridx>=ptrs_.size() )
	return;

    idx -= arridx*chunksize_;
    ptrs_[arridx][idx] = v;
}


template <class RT, class AT> inline
RT* MultiArrayValueSeries<RT,AT>::arr()
{
    return typeid(RT)==typeid(AT) && cursize_<=chunksize_
	? (RT*) ptrs_[0] : (RT*) 0;
}


template <class RT, class AT> inline
const RT* MultiArrayValueSeries<RT,AT>::arr() const
{
    return typeid(RT)==typeid(AT) && cursize_<chunksize_
	? (RT*) ptrs_[0] : (RT*) 0;
}


template <class RT, class AT> inline
bool MultiArrayValueSeries<RT,AT>::setSize( od_int64 sz )
{
    if ( cursize_==sz )
	return true;

    od_int64 lefttoalloc = sz;
    deepEraseArr( ptrs_ );

    while ( lefttoalloc>0 )
    {
	const od_int64 allocsize = lefttoalloc>=chunksize_
	    ? chunksize_ : lefttoalloc;

	AT* ptr;
	mTryAlloc( ptr, AT[allocsize] );
	if ( !ptr )
	{
	    cursize_ = -1;
	    deepEraseArr( ptrs_ );
	    return false;
	}

	ptrs_ += ptr;

	lefttoalloc -= allocsize;
    }

    cursize_ = sz;
    return true;
}

#undef mChunkSize

#endif
