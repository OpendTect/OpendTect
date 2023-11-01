#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "valseries.h"

#include "samplingdata.h"

#ifdef __debug__
#include "debug.h"
#endif

/*!\brief ValueSeries of offsets.*/

template <class T>
mClass(Basic) OffsetValueSeries : public ValueSeries<T>
{
public:
    inline		OffsetValueSeries( ValueSeries<T>& src, od_int64 off,
					   od_int64 sz );
    inline		OffsetValueSeries( const ValueSeries<T>& src,
					   od_int64 off, od_int64 sz );
    inline ValueSeries<T>* clone() const override;

    inline T		value( od_int64 idx ) const override;
    inline void		setValue( od_int64 idx, T v ) override;
    inline T*		arr() override;
    inline const T*	arr() const override;
    inline bool		writable() const override;
    inline bool		canSetAll() const override;
    inline void		setAll(T) override;
    od_int64		size() const override	{ return cursize_; }

    inline od_int64	getOffset() const;
    inline void		setOffset(od_int64 no);
    inline bool		setSize(od_int64 sz) override;

    const ValueSeries<T>&	source() const { return src_; }

protected:
    ValueSeries<T>&	src_;
    od_int64		off_;
    od_int64		cursize_ = 0;
    bool		writable_;
};


#include <typeinfo>

#define mImplArr \
{ return typeid(RT)==typeid(AT) ? (RT*) ptr_ : (RT*) 0;}

/*!
\brief Series of values from a pointer to some kind of array.
  If a more advanced conversion between the return type and the array type is
  wanted, use ConvMemValueSeries instead.
*/

template <class RT, class AT>
mClass(Basic) ArrayValueSeries : public ValueSeries<RT>
{
public:

		ArrayValueSeries( AT* ptr, bool memmine, od_int64 sz=-1 );
		ArrayValueSeries( od_int64 sz );
		~ArrayValueSeries() override
		{ if ( mine_ ) delete [] ptr_; }

    bool	operator ==(const ArrayValueSeries<RT,AT>&) const;
    bool	operator !=(const ArrayValueSeries<RT,AT>&) const;

    ValueSeries<RT>*	clone() const override;

    bool	isOK() const override			{ return ptr_; }

    RT		value( od_int64 idx ) const override;
    bool	writable() const override		{ return true; }
    void	setValue( od_int64 idx, RT v ) override;

    bool	canSetAll() const override		{ return writable(); }
    void	setAll(RT) override;

    const RT*	arr() const override			mImplArr;
    RT*		arr() override				mImplArr;

    const AT*	storArr() const				{ return ptr_; }
    AT*		storArr()				{ return ptr_; }

    bool	reSizeable() const override		{ return mine_; }
    inline bool	setSize(od_int64) override;
    od_int64	size() const override			{ return cursize_; }
    char	bytesPerItem() const override		{ return sizeof(AT); }

protected:

    AT*		ptr_;
    bool	mine_;
    od_int64	cursize_;
};

#undef mImplArr

#define mChunkSize mMaxContiguousMemSize

/*!
\brief Valueseries that allocates its data in smaller chunks. By doing this,
it performs better in environments where the memory is fragmented
(i.e. windows 32 bit).
Default chunk size for windows 32 bit is 512MB and for all other platforms
default is 32 GB.
*/

template <class RT, class AT>
mClass(Basic) MultiArrayValueSeries : public ValueSeries<RT>
{
public:
		MultiArrayValueSeries(od_int64);
		MultiArrayValueSeries(const MultiArrayValueSeries<RT, AT>&);
		~MultiArrayValueSeries() override;

    ValueSeries<RT>*	clone() const override;

    bool	isOK() const override			{ return cursize_>=0; }

    RT		value( od_int64 idx ) const override;
    bool	writable() const override		{ return true; }
    void	setValue(od_int64 idx, RT v) override;

    bool	canSetAll() const override		{ return writable(); }
    void	setAll(RT) override;

    const RT*	arr() const override;
    RT*		arr() override;

    bool	selfSufficient() const override		{ return true; }
    bool	reSizeable() const override		{ return true; }
    inline bool	setSize(od_int64) override;
    od_int64	size() const override			{ return cursize_; }
    char	bytesPerItem() const override		{ return sizeof(AT); }

protected:
    ObjectSet<AT>	ptrs_;
    od_int64		cursize_;
    const od_int64	chunksize_;
};


/*!
\brief Series of values from a SamplingData object.
       Never writable, only a quick way of providing all values
       without storing them in an array
*/

template <class T>
mClass(Basic) SamplingValues : public virtual ValueSeries<T>
{
public:
				SamplingValues(const SamplingData<T>&,
					       od_int64 sz);
				SamplingValues(const StepInterval<T>&);
				SamplingValues(const SamplingValues&);
				~SamplingValues();

    ValueSeries<T>&		operator =(const SamplingValues&);
    bool			operator ==(const SamplingValues<T>&) const;
    bool			operator !=(const SamplingValues<T>&) const;

    ValueSeries<T>*		clone() const override;
    bool			isOK() const override;

    T				value(od_int64) const override;
    bool			reSizeable() const override { return true; }
    bool			setSize(od_int64) override;
    void			setSampling(const SamplingData<T>&,
					    od_int64 sz=-1);
    void			setSampling(const StepInterval<T>&);

    od_int64			size() const override	{ return sz_; }
    char			bytesPerItem() const override	{ return 0; }

    const SamplingData<T>&	getSampling() const	{ return sd_; }

protected:

    SamplingData<T>		sd_;
    od_int64			sz_;
};


template <class RT, class AT> inline
ValueSeries<RT>* MultiArrayValueSeries<RT,AT>::clone() const
{ return new MultiArrayValueSeries<RT,AT>( *this ); }


template <class T> inline
OffsetValueSeries<T>::OffsetValueSeries( ValueSeries<T>& src, od_int64 off,
					 od_int64 sz )
    : src_(src), off_(off), cursize_(sz)
    , writable_(true)
{}


template <class T> inline
OffsetValueSeries<T>::OffsetValueSeries( const ValueSeries<T>& src,
					 od_int64 off, od_int64 sz )
    : src_( const_cast<ValueSeries<T>& >(src) ), off_(off), cursize_(sz)
    , writable_(false)
{}


template <class T> inline
ValueSeries<T>* OffsetValueSeries<T>::clone() const
{ return new OffsetValueSeries( src_, off_, cursize_ ); }


template <class T> inline
T OffsetValueSeries<T>::value( od_int64 idx ) const
{ return src_.value(idx+off_); }

template <class T> inline
void OffsetValueSeries<T>::setValue( od_int64 idx, T v )
{
    if ( writable_ )
	src_.setValue(idx+off_,v);
    else
	{ pErrMsg("Attempting to write to write-protected array"); }
}


template <class T> inline
void OffsetValueSeries<T>::setAll( T v )
{
    if ( writable_ )
    {
        od_int64 lastidx = off_+cursize_-1;
        if ( lastidx >= src_.size() )
            lastidx = size()-1;
        od_int64 nrsamps = lastidx - off_ + 1;
        T* arrvals = arr();
        if ( arrvals )
        {
            OD::sysMemValueSet( arrvals, v, nrsamps );
            return;
        }

        for ( od_int64 idx=off_; idx<lastidx; idx++ )
            src_.setValue( idx, v );
    }
    else
	{ pErrMsg("Attempting to write to write-protected array"); }
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
bool OffsetValueSeries<T>::setSize( od_int64 sz )
{
    if ( off_+sz >= src_.size() )
        return false;
    cursize_ = sz; return true;
}


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
bool ArrayValueSeries<RT,AT>::operator ==(
				   const ArrayValueSeries<RT,AT>& oth ) const
{
    if ( &oth == this )
	return true;

    return oth.cursize_ == cursize_ && oth.mine_ == mine_ && oth.ptr_ == ptr_;
}


template <class RT, class AT>
bool ArrayValueSeries<RT,AT>::operator !=(
				   const ArrayValueSeries<RT,AT>& oth ) const
{
    return !(oth == *this);
}


template <class RT, class AT>
ValueSeries<RT>* ArrayValueSeries<RT,AT>::clone() const
{
    AT* ptr = ptr_;
    if ( mine_ && cursize_>0 )
    {
	ptr = new AT[cursize_];
	OD::memCopy( ptr, ptr_, sizeof(AT)*cursize_ );
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

    return (RT) ptr_[idx];
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
    OD::memValueSet( ptr_, (AT)val, cursize_ );
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
	OD::memCopy( ptr_, oldptr, (size_t) (copysize*sizeof(AT)) );

    delete [] oldptr;
    return ptr_;
}


template <class RT, class AT> inline
MultiArrayValueSeries<RT,AT>::MultiArrayValueSeries( od_int64 sz )
    : cursize_( -1 )
    , chunksize_( mChunkSize/sizeof(AT) )
{
    ptrs_.setNullAllowed();
    setSize( sz );
}


template <class RT, class AT> inline
MultiArrayValueSeries<RT, AT>::MultiArrayValueSeries(
				const MultiArrayValueSeries<RT, AT>& mavs )
    : ValueSeries<RT>( mavs )
    , cursize_( -1 )
    , chunksize_( mavs.chunksize_ )
{
    ptrs_.setNullAllowed();
    if ( setSize( mavs.cursize_ ) && ptrs_.size() == mavs.ptrs_.size() )
    {
	for ( int idx=0; idx<ptrs_.size(); idx++ )
	{
	    const od_int64 nextstart = ((od_int64) idx+1)*chunksize_;
	    od_int64 curchunksize = chunksize_;
	    if ( nextstart>cursize_ )
	    {
		od_int64 diff = nextstart-cursize_;
		curchunksize -= diff;
	    }
	    OD::memCopy( ptrs_[idx], mavs.ptrs_[idx], curchunksize*sizeof(AT) );
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
    if ( !ptrs_.validIdx(arridx) )
	return RT();

    idx -= arridx*chunksize_;
    return  ptrs_[mCast(int,arridx)][idx];
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
    ptrs_[mCast(int,arridx)][idx] = v;
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


template <class RT, class AT> inline
const RT* MultiArrayValueSeries<RT,AT>::arr() const
{ return const_cast<MultiArrayValueSeries<RT,AT>*>( this )->arr(); }


template <class RT, class AT> inline
bool MultiArrayValueSeries<RT,AT>::setSize( od_int64 sz )
{
    if ( cursize_==sz )
	return true;

    od_uint64 lefttoalloc = sz > 0 ? (od_uint64)sz : 0;
    deepEraseArr( ptrs_ );

    while ( lefttoalloc )
    {
	const od_uint64 allocsize = lefttoalloc>=chunksize_
	    ? (od_uint64)chunksize_ : lefttoalloc;

	AT* ptr;
	mTryAlloc( ptr, AT[allocsize] );
	if ( !ptr )
	{
	    cursize_ = -1;
	    deepEraseArr( ptrs_ );
	    return false;
	}

	ptrs_ += ptr;

	if ( lefttoalloc > allocsize )
	    lefttoalloc -= allocsize;
	else
	    lefttoalloc = 0;
    }

    cursize_ = sz;
    return true;
}

#undef mChunkSize


template <class T> inline
SamplingValues<T>::SamplingValues( const SamplingData<T>& sd, od_int64 sz )
    : sd_(sd)
    , sz_(sz)
{}


template <class T> inline
SamplingValues<T>::SamplingValues( const StepInterval<T>& rg )
{
    setSampling( rg );
}


template <class T> inline
SamplingValues<T>::SamplingValues( const SamplingValues<T>& oth )
    : SamplingValues<T>(oth.sd_,oth.sz_)
{}


template <class T> inline
SamplingValues<T>::~SamplingValues()
{}


template <class T> inline
ValueSeries<T>& SamplingValues<T>::operator =( const SamplingValues<T>& oth )
{
    if ( &oth == this )
	return *this;

    sd_ = oth.sd_;
    sz_ = oth.sz_;

    return *this;
}


template <class T> inline
bool SamplingValues<T>::operator ==( const SamplingValues<T>& oth ) const
{
    if ( &oth == this )
	return true;

    return oth.sz_ == sz_ && oth.sd_ == sd_;
}


template <class T> inline
bool SamplingValues<T>::operator !=( const SamplingValues<T>& oth ) const
{
    return !(oth == *this);
}


template <class T> inline
ValueSeries<T>* SamplingValues<T>::clone() const
{
    return new SamplingValues<T>( sd_, sz_ );
}


template <class T> inline
bool SamplingValues<T>::isOK() const
{
    return sz_ >=0 && !sd_.isUdf();
}


template <class T> inline
T SamplingValues<T>::value( od_int64 idx ) const
{
    return sd_.atIndex( idx );
}


template <class T> inline
bool SamplingValues<T>::setSize( od_int64 sz )
{
    sz_ = sz;
    return true;
}


template <class T> inline
void SamplingValues<T>::setSampling( const SamplingData<T>& sd, od_int64 sz )
{
    sd_.set( sd );
    if ( sz >= 0 )
	setSize( sz );
}


template <class T> inline
void SamplingValues<T>::setSampling( const StepInterval<T>& rg )
{
    sd_.set( rg );
    setSize( od_int64 (rg.nrSteps()) + 1 );
}
