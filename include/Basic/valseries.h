#ifndef valseries_h
#define valseries_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril & Kris Tingdahl
 Date:          Mar 2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"
#include "errh.h"
#include "odmemory.h"

#ifdef __debug__
#include "debug.h"
#endif


/*\brief Interface to a series of values

  If the values are in contiguous memory, arr() should return non-null.
 
*/


template <class T>
class ValueSeries
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
    virtual bool	setSize(od_int64) 		{ return false; }

    virtual T*		arr()				{ return 0; }
    virtual const T*	arr() const			{ return 0; }

    virtual char	bytesPerItem() const		{ return sizeof(T); }

    inline T		operator[](od_int64 idx) const	{ return value(idx); }
};


template <class T>
class OffsetValueSeries : public ValueSeries<T>
{
public:
    inline		OffsetValueSeries( ValueSeries<T>& src, od_int64 off );
    inline		OffsetValueSeries( const ValueSeries<T>& src,
	    				   od_int64 off);
    inline ValueSeries<T>* clone() const;

    inline T		value( od_int64 idx ) const;
    inline void		setValue( od_int64 idx, T v );
    inline T*		arr();
    inline const T*	arr() const;
    inline bool		writable() const;
    inline bool		canSetAll() const;
    inline void		setAll(T);

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

    ValueSeries<RT>*	clone() const;

    bool	isOK() const			{ return ptr_; }

    RT		value( od_int64 idx ) const;
    bool	writable() const		{ return true; }
    void	setValue( od_int64 idx, RT v );

    bool	canSetAll() const		{ return writable(); }
    void	setAll(RT);

    const RT*	arr() const			mImplArr;
    RT*		arr()				mImplArr;

    bool	selfSufficient() const		{ return mine_; }
    bool	reSizeable() const		{ return mine_; }
    inline bool	setSize(od_int64);
    od_int64	size() const			{ return cursize_; }
    char	bytesPerItem() const		{ return sizeof(AT); }

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
    		MultiArrayValueSeries(const MultiArrayValueSeries<RT, AT>&);
    		~MultiArrayValueSeries();

    ValueSeries<RT>*	clone() const;

    bool	isOK() const			{ return cursize_>=0; }

    RT		value( od_int64 idx ) const;
    bool	writable() const		{ return true; }
    void	setValue(od_int64 idx, RT v);

    bool	canSetAll() const		{ return writable(); }
    void	setAll(RT);

    const RT*	arr() const;
    RT*		arr();

    bool	selfSufficient() const		{ return true; }
    bool	reSizeable() const		{ return true; }
    inline bool	setSize(od_int64);
    od_int64	size() const			{ return cursize_; }
    char	bytesPerItem() const		{ return sizeof(AT); }

protected:
    ObjectSet<AT>	ptrs_;
    od_int64		cursize_;
    const unsigned int	chunksize_;
};

template <class T>
class ValueSeriesGetAll : public ParallelTask
{
public:
		ValueSeriesGetAll(const ValueSeries<T>& from,
				  ValueSeries<T>& to, od_int64 nriterations )
		    : from_( from )
		    , to_( &to )
		    , toptr_( 0 )
		    , nriterations_( nriterations )
		{}

		ValueSeriesGetAll(const ValueSeries<T>& from, T* to,
				  od_int64 nriterations	)
		    : from_( from )
		    , toptr_( to )
		    , to_( 0 )
		    , nriterations_( nriterations )
		{}

od_int64	nrIterations() const { return nriterations_; }
bool		doWork( od_int64 start, od_int64 stop, int )
		{
		    od_int64 nrleft = stop-start+1;
		    const T* fromarr = from_.arr();
		    T* toarr = toptr_ ? toptr_ : to_->arr(); 
		    if ( toarr && fromarr )
		    {
			memcpy( toarr+start, fromarr+start,
			        (size_t) (nrleft*from_.bytesPerItem()) );
		    }
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
od_int64		nriterations_;
const ValueSeries<T>&	from_;
ValueSeries<T>*		to_;
T*			toptr_;
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


template <class RT, class AT> inline
ValueSeries<RT>* MultiArrayValueSeries<RT,AT>::clone() const
{ return new MultiArrayValueSeries<RT,AT>( *this ); }


template <class T> inline
OffsetValueSeries<T>::OffsetValueSeries( ValueSeries<T>& src, od_int64 off )
    : src_( src ), off_( off ), writable_(true) 
{}


template <class T> inline
OffsetValueSeries<T>::OffsetValueSeries(const ValueSeries<T>& src,od_int64 off)
    : src_( const_cast<ValueSeries<T>& >(src) ), off_( off ), writable_(false) 
{}


template <class T> inline
ValueSeries<T>* OffsetValueSeries<T>::clone() const
{ return new OffsetValueSeries( src_, off_ ); }


template <class T> inline
T OffsetValueSeries<T>::value( od_int64 idx ) const
{ return src_.value(idx+off_); }

template <class T> inline
void OffsetValueSeries<T>::setValue( od_int64 idx, T v )
{
    if ( writable_ )
	src_.setValue(idx+off_,v);
    else
	pErrMsg("Attempting to write to write-protected array");
}


template <class T> inline
void OffsetValueSeries<T>::setAll( T v )
{
    if ( writable_ )
	src_.setAll( v );
    else
	pErrMsg("Attempting to write to write-protected array");
}



template <class T> inline
bool OffsetValueSeries<T>::canSetAll() const
{ return writable_ && src_.canSetAll(); }


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
    : ptr_( 0 ), mine_(true), cursize_( -1 )
{
    setSize( sz );
}


template <class RT, class AT>
ValueSeries<RT>* ArrayValueSeries<RT,AT>::clone() const
{
    AT* ptr = ptr_;
    if ( mine_ && cursize_>0 )
    {
	ptr = new AT[cursize_];
	memcpy( ptr, ptr_, sizeof(AT)*cursize_ );
    }

    return new ArrayValueSeries( ptr, mine_, cursize_ );
}


template <class RT, class AT>
RT ArrayValueSeries<RT,AT>::value( od_int64 idx ) const
{
#ifdef __debug__
    if ( idx<0 || (cursize_>=0 && idx>=cursize_ ) )
    {
	pErrMsg("Invalid access");
	DBG::forceCrash(true);
    }
#endif

    return ptr_[idx];
}


template <class RT, class AT>
void ArrayValueSeries<RT,AT>::setValue( od_int64 idx, RT v )
{
#ifdef __debug__
    if ( idx<0 || (cursize_>=0 && idx>=cursize_ ) )
    {
	pErrMsg("Invalid access");
	DBG::forceCrash(true);
    }
#endif

    ptr_[idx] = (AT) v;
}


template <class RT, class AT>
void ArrayValueSeries<RT,AT>::setAll( RT val )
{
    if ( cursize_<=0 )
    {
	MemSetter<AT> setter( ptr_, (AT) val, cursize_ );
	setter.execute();
    }
}


template <class RT,class AT> inline
bool ArrayValueSeries<RT,AT>::setSize( od_int64 sz )
{
    if ( cursize_!=-1 && cursize_==sz && ptr_ ) return true;
    if ( !mine_ ) return false;

    AT* oldptr = ptr_;
    if ( sz )
    {
	mTryAlloc( ptr_, AT[sz] );
    }
    else
	ptr_ = 0;

    const od_int64 copysize = mMIN(sz,cursize_);
    cursize_ = ptr_ ? sz : -1;
    if ( ptr_ && copysize>0 )
	memcpy( ptr_, oldptr, (size_t) (copysize*sizeof(AT)) );
    
    delete [] oldptr;
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
MultiArrayValueSeries<RT, AT>::MultiArrayValueSeries( 
				const MultiArrayValueSeries<RT, AT>& mavs )
    : ValueSeries<RT>( mavs )
    , cursize_( -1 )
    , chunksize_( mavs.chunksize_ )
{
    ptrs_.allowNull( true );
    if ( setSize( mavs.cursize_ ) && ptrs_.size() == mavs.ptrs_.size() )
    {
	MemCopier<AT> cpier;
	for ( int idx=0; idx<ptrs_.size(); idx++ )
	{
	    const od_int64 nextstart = ((od_int64) idx+1)*chunksize_;
	    od_int64 curchunksize = chunksize_;
	    if ( nextstart>cursize_ )
	    {
		od_int64 diff = nextstart-cursize_;
		curchunksize -= diff;
	    }

	    cpier.setInput( mavs.ptrs_[idx] );
	    cpier.setOutput( ptrs_[idx] );
	    cpier.setSize( curchunksize );
	    cpier.execute();
	}
    }
}


template <class RT, class AT> inline
MultiArrayValueSeries<RT,AT>::~MultiArrayValueSeries()
{
    deepEraseArr( ptrs_ );
}


template <class RT, class AT> inline
RT MultiArrayValueSeries<RT,AT>::value( od_int64 idx ) const
{
#ifdef __debug__
    if ( idx<0 || idx>=cursize_ )
    {
	pErrMsg("Invalid access");
	DBG::forceCrash(true);
    }
#endif
    const od_int64 arridx = idx/chunksize_;
    if ( arridx>=ptrs_.size() )
	return RT();

    idx -= arridx*chunksize_;
    return  ptrs_[mIdx(arridx)][mIdx(idx)];
}


template <class RT, class AT> inline
void MultiArrayValueSeries<RT,AT>::setValue( od_int64 idx, RT v )
{
#ifdef __debug__
    if ( idx<0 || idx>=cursize_ )
    {
	pErrMsg("Invalid access");
	DBG::forceCrash(true);
    }
#endif
    const od_int64 arridx = idx/chunksize_;
    if ( arridx>=ptrs_.size() )
	return;

    idx -= arridx*chunksize_;
    ptrs_[mIdx(arridx)][mIdx(idx)] = v;
}


template <class RT, class AT> inline
void MultiArrayValueSeries<RT,AT>::setAll( RT val )
{
    if ( cursize_<=0 )
	return;

    MemSetter<AT> memsetter;
    memsetter.setValue( (AT)val );

    for ( int idx=ptrs_.size()-1; idx>=0; idx-- )
    {
	const od_int64 nextstart = ((od_int64) idx+1)*chunksize_;
	od_int64 curchunksize = chunksize_;
	if ( nextstart>cursize_ )
	{
	    od_int64 diff = nextstart-cursize_;
	    curchunksize -= diff;
	}

	memsetter.setTarget( ptrs_[idx] );
	memsetter.setSize( curchunksize );
	memsetter.execute();
    }
}


template <class RT, class AT> inline
RT* MultiArrayValueSeries<RT,AT>::arr()
{
    return cursize_>0 && cursize_<=chunksize_ && typeid(RT)==typeid(AT)
	? (RT*) ptrs_[0] : (RT*) 0;
}


template <class RT, class AT> inlinef
const RT* MultiArrayValueSeries<RT,AT>::arr() const
{ return const_cast<MultiArrayValueSeries<RT,AT>*>( this )->arr(); }


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

