#ifndef odmemory_h
#define odmemory_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2009
 RCS:		$Id$
________________________________________________________________________

*/

#include "task.h"

class IOPar;

namespace OD
{
    enum		PtrPolicy { UsePtr, CopyPtr, TakeOverPtr };
};

template <class T> class ValueSeries;

// 1M operations min per thread
#define mMemMinThreadSize 1048576


/*!Sets large amounts of values to a constant using multiple threads. */
template <class T>
class MemSetter : public ParallelTask
{
public:
    		MemSetter();
    		MemSetter(T*,T val,od_int64 sz);
    		MemSetter(ValueSeries<T>&,T val,od_int64 sz);

    void	setValue( const T& val )	{ val_ = val; }
    void	setTarget( T* ptr )		{ ptr_ = ptr; vs_ = 0; }
    void	setTarget( ValueSeries<T>& vs )	{ ptr_ = vs.arr(); vs_ = &vs; }
    void	setSize( od_int64 sz )		{ sz_ = sz; }

    bool	doPrepare(int);
    bool	doWork(od_int64,od_int64,int);
    od_int64	nrIterations() const		{ return sz_; }
    int		minThreadSize() const		{ return mMemMinThreadSize; }

protected:

    bool		setPtr(od_int64 start,od_int64 size);

    ValueSeries<T>*	vs_;
    T*			ptr_;
    od_int64		sz_;
    T			val_;
};


template <class T>
class MemCopier : public ParallelTask
{
public:

    		MemCopier();
    		MemCopier(T*,const T*,od_int64 sz);
    		MemCopier(T*,const ValueSeries<T>&,od_int64 sz);
    		MemCopier(ValueSeries<T>&,const T*,od_int64 sz);
    		MemCopier(ValueSeries<T>&,const ValueSeries<T>&,od_int64 sz);

    void	setInput( const T* ptr )	{ inptr_ = ptr; invs_ = 0; }
    void	setInput( const ValueSeries<T>& vs ) {	inptr_ = vs.arr();
							invs_ = &vs; }
    void	setOutput( T* ptr )		{ outptr_ = ptr; outvs_ = 0; }
    void	setOutput( ValueSeries<T>& vs )	{ outptr_ = vs.arr();
						  outvs_ = &vs; }
    void	setSize(od_int64 sz)		{ sz_ = sz; }

    bool	doPrepare(int);
    bool	doWork(od_int64,od_int64,int);
    od_int64	nrIterations() const		{ return sz_; }
    int		minThreadSize() const		{ return mMemMinThreadSize; }

protected:

    inline bool		setPtr(od_int64 start,od_int64 size);

    const T*		inptr_;
    const ValueSeries<T>* invs_;
    T*			outptr_;
    ValueSeries<T>*	outvs_;
    od_int64		sz_;
};


/*!Goes through some mem or a valseries and replaces one value with another */

template <class T>
class MemValReplacer : public ParallelTask
{
public:
		MemValReplacer();
		MemValReplacer(T*,const T& fromval,const T& toval,od_int64 sz);
		MemValReplacer(ValueSeries<T>&,const T& from,const T& to,
			       od_int64 sz);

    void        setFromValue(const T& val)	{ fromval_ = val; }
    void        setToValue(const T& val)	{ toval_ = val; }
    void        setPtr(T* ptr)			{ ptr_ = ptr; vs_ = 0; }
    void        setPtr(ValueSeries<T>& vs)	{ ptr_ = vs.arr(); vs_ = &vs; }
    void        setSize(od_int64 sz)		{ sz_ = sz; }

    bool        doPrepare(int);
    bool        doWork(od_int64,od_int64,int);
    od_int64    nrIterations() const            { return sz_; }
    int         minThreadSize() const           { return mMemMinThreadSize; }

protected:

    bool                setPtr(od_int64 start,od_int64 size);

    ValueSeries<T>*     vs_;
    T*                  ptr_;
    od_int64            sz_;
    T                   toval_;
    T                   fromval_;
};


#include "valseries.h"


template <class T> inline
MemSetter<T>::MemSetter()
    : ptr_( 0 )
    , vs_( 0 )
    , sz_( -1 )
{} 


template <class T> inline
MemSetter<T>::MemSetter( T* ptr, T val, od_int64 sz )
    : ptr_( ptr )
    , vs_( 0 )
    , val_( val )
    , sz_( sz )
{} 


template <class T> inline
MemSetter<T>::MemSetter( ValueSeries<T>& vs, T val, od_int64 sz )
    : ptr_( vs.arr() )
    , vs_( &vs )
    , val_( val )
    , sz_( sz )
{}


template <class T> inline
bool MemSetter<T>::doPrepare( int )
{ return ptr_ || vs_; }


template <class T> inline
bool MemSetter<T>::doWork( od_int64 start, od_int64 stop, int )
{
    if ( ptr_ )
	return setPtr( start, stop-start+1 );

    for ( od_int64 idx=start; idx<=stop; idx++ )
	vs_->setValue( idx, val_ );

    return true;
}


template <> inline
bool MemSetter<char>::setPtr( od_int64 start, od_int64 size )
{
    memset( ptr_+start, (int)val_, (size_t) size );
    return true;
}


template <> inline
bool MemSetter<unsigned char>::setPtr( od_int64 start, od_int64 size )
{
    memset( ptr_+start, (int)val_, (size_t) size );
    return true;
}


template <> inline
bool MemSetter<bool>::setPtr( od_int64 start, od_int64 size )
{
    memset( ptr_+start, (int)val_, (size_t) size );
    return true;
}


#define mSetterFullImpl(Type) \
    Type* ptr = ptr_ + start; \
    const Type* stopptr = ptr + size; \
    while ( ptr != stopptr ) \
	{ *ptr = val_; ptr++; } \
 \
    return true


#define mSpecialImpl( Type ) \
template <> inline \
bool MemSetter<Type>::setPtr( od_int64 start, od_int64 size ) \
{ \
    if ( val_==0 ) \
    { \
	memset( ptr_+start, 0, (size_t) (size*sizeof(Type)) ); \
	return true; \
    } \
 \
    mSetterFullImpl(Type); \
}


mSpecialImpl( float );
mSpecialImpl( double );
mSpecialImpl( int );
mSpecialImpl( unsigned int );
mSpecialImpl( short );
mSpecialImpl( unsigned short );
mSpecialImpl( od_int64 );
mSpecialImpl( od_uint64 );



template <class T> inline
bool MemSetter<T>::setPtr( od_int64 start, od_int64 size )
{
    mSetterFullImpl(T);
}

#undef mSpecialImpl
#undef mSetterFullImpl


template <class T> inline
MemCopier<T>::MemCopier()
    : sz_(0), inptr_(0), invs_(0), outptr_(0), outvs_(0)		{}

template <class T> inline
MemCopier<T>::MemCopier( T* o, const T* i, od_int64 sz )
    : sz_(sz), inptr_(i), invs_(0), outptr_(o), outvs_(0)		{}

template <class T> inline
MemCopier<T>::MemCopier( ValueSeries<T>& o, const T* i, od_int64 sz )
    : sz_(sz), inptr_(i), invs_(0), outptr_(o.arr()), outvs_(&o)	{}

template <class T> inline
MemCopier<T>::MemCopier( T* o, const ValueSeries<T>& i, od_int64 sz )
    : sz_(sz), inptr_(i.arr()), invs_(&i), outptr_(o), outvs_(0)	{}

template <class T> inline
MemCopier<T>::MemCopier( ValueSeries<T>& o, const ValueSeries<T>& i,od_int64 sz)
    : sz_(sz), inptr_(i.arr), invs_(&i), outptr_(o.arr()), outvs_(&o)	{}


template <class T> inline
bool MemCopier<T>::doPrepare( int )
{ return (inptr_ || invs_) && (outptr_ || outvs_); }


template <class T> inline
bool MemCopier<T>::doWork( od_int64 start, od_int64 stop, int )
{
    if ( inptr_ && outptr_ )
	return setPtr( start, stop-start+1 );

    if ( outptr_ )
    {
	for ( od_int64 idx=start; idx<=stop; idx++ )
	    outptr_[idx] = invs_->value( idx );
    }
    else if ( inptr_ )
    {
	for ( od_int64 idx=start; idx<=stop; idx++ )
	    outvs_->setValue( idx, inptr_[idx] );
    }
    else
    {
	for ( od_int64 idx=start; idx<=stop; idx++ )
	    outvs_->setValue( idx, invs_->value(idx) );
    }
    return true;
}


template <class T> inline
bool MemCopier<T>::setPtr( od_int64 start, od_int64 size )
{
    memcpy( outptr_ + start, inptr_ + start, (size_t) (size * sizeof(T)) );
    return true;
}


template <class T> inline
MemValReplacer<T>::MemValReplacer( T* ptr, const T& fromval, const T& toval,
				   od_int64 sz )
    : ptr_( ptr )
    , vs_( 0 )
    , fromval_( fromval )
    , toval_( toval )
    , sz_( sz )
{}


template <class T> inline
MemValReplacer<T>::MemValReplacer(ValueSeries<T>& vs, const T& fromval,
				  const T& toval,
				  od_int64 sz)
    : ptr_( vs.arr() )
    , vs_( &vs )
    , toval_( toval )
    , fromval_( fromval )
    , sz_( sz )
{}


template <class T> inline
bool MemValReplacer<T>::doPrepare( int )
{ return ptr_ || vs_; }


template <class T> inline
bool MemValReplacer<T>::doWork( od_int64 start, od_int64 stop, int )
{
    if ( ptr_ )
	return setPtr( start, stop-start+1 );

    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	if ( vs_->value(idx)==fromval_ )
	    vs_->setValue( idx, toval_ );
    }

    return true;
}

template <class T> inline
bool MemValReplacer<T>::setPtr( od_int64 start, od_int64 size )
{
    T* ptr = ptr_ + start;
    const T* stopptr = ptr + size;
    while ( ptr != stopptr )
    {
	if ( *ptr==fromval_ )
	    *ptr = toval_;

	ptr++;
    }

    return true;
}

#endif
