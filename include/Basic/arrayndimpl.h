#pragma once
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		9-3-1999
________________________________________________________________________

*/


#include "arraynd.h"
#include "valseriesimpl.h"

#ifdef __debug__
# include "debug.h"
#endif

/*!Helper class to handle internal storage of arrays */

template <class T>
mClass(Basic) ArrayNDImplBase
{
protected:

			ArrayNDImplBase();
			~ArrayNDImplBase()	{ delete stor_; }


    virtual ArrayNDInfo::total_size_type getStorageSize() const		= 0;
    bool		storageOK() const;
    bool		updateStorageSize();
			/*!<Sets storage size to getStorageSize()
			    and creates if need be. */

    bool		setStorageNoResize(ValueSeries<T>*);
    bool		setStorageInternal(ValueSeries<T>*);
    bool		getDataFrom(const ArrayND<T>& templ);

    ValueSeries<T>*	stor_;
    T*			arr_;	//not owned, only a shortcut
				//for the 99% percent case
};


/*!\brief Flat-array implementation of Array1D. */

template <class T>
mClass(Basic) Array1DImpl : public Array1D<T>, public ArrayNDImplBase<T>
{ typedef ArrayNDImplBase<T> base;
public:
			mTypeDefArrNDTypes;

			Array1DImpl(size_type);
			Array1DImpl(const Array1D<T>&);
			Array1DImpl(const Array1DImpl<T>&);
    Array1DImpl<T>&	operator =( const Array1D<T>& ai )
			    { copyFrom(ai); return *this; }
    Array1DImpl<T>&	operator =( const Array1DImpl<T>& ai )
			    { copyFrom(ai); return *this; }

    ValueSeries<T>*	clone() const
			    { return new Array1DImpl<T>(*this); }

    bool		isOK() const { return base::storageOK(); }
    void		copyFrom(const Array1D<T>&);
    bool		setStorage(ValueSeries<T>* vs)
			    { return base::setStorageInternal(vs); }
    bool		canSetStorage() const	{ return true; }

    void		set(idx_type,T);
    T			get(idx_type) const;

    const Array1DInfo&	info() const		{ return inf_; }
    bool		canSetInfo() const	{ return true; }
    bool		setInfo(const ArrayNDInfo&);
    bool		setSize(size_type);
    bool		setSize( od_int64 sz )
			    { return setSize( ((size_type)sz) ); }

			// ValueSeries interface
    T*			arr()			{ return base::arr_; }
    const T*		arr() const		{ return base::arr_; }

protected:

    const T*		getData_() const	{ return base::arr_; }
    const ValueSeries<T>* getStorage_() const	{ return base::stor_; }
    total_size_type	getStorageSize() const  { return inf_.totalSize(); }

    Array1DInfoImpl	inf_;

};


/*!\brief Flat-array implementation of Array2D. */

template <class T>
mClass(Basic) Array2DImpl : public Array2D<T>, public ArrayNDImplBase<T>
{ typedef ArrayNDImplBase<T> base;
public:
			mTypeDefArrNDTypes;

			Array2DImpl(size_type,size_type);
			Array2DImpl(const Array2DInfo&);
			Array2DImpl(const Array2D<T>&);
			Array2DImpl(const Array2DImpl<T>&);
			~Array2DImpl() { deleteAndZeroArrPtr( arr2d_ ); }

    Array2DImpl<T>&	operator =( const Array2D<T>& ai )
			    { copyFrom(ai); return *this; }
    Array2DImpl<T>&	operator =( const Array2DImpl<T>& ai )
			    { copyFrom(ai); return *this; }

    bool		isOK() const { return base::storageOK(); }
    bool		canSetStorage() const		{ return true; }
    bool		setStorage(ValueSeries<T>* vs);

    void		set(idx_type,idx_type,T);
    T			get(idx_type,idx_type) const;
    void		copyFrom(const Array2D<T>&);

    const Array2DInfo&	info() const		{ return inf_; }
    bool		canSetInfo() const	{ return true; }

    bool		setInfo(const ArrayNDInfo&);
    bool		setSize(size_type,size_type);

    T**			get2DData()		{ return arr2d_; }
    const T**		get2DData() const	{ return (const T**)arr2d_; }

protected:

    void		updateStorage();
    const T*		getData_() const	{ return base::arr_; }
    const ValueSeries<T>* getStorage_() const	{ return base::stor_; }
    total_size_type	getStorageSize() const  { return inf_.totalSize(); }

    void		updateCachePointers();

    Array2DInfoImpl	inf_;
    T**			arr2d_;
};


/*!\brief Flat-array implementation of Array3D. */

template <class T>
mClass(Basic) Array3DImpl : public Array3D<T>, public ArrayNDImplBase<T>
{ typedef ArrayNDImplBase<T> base;
public:
			mTypeDefArrNDTypes;

			Array3DImpl(size_type,size_type,size_type);
			Array3DImpl(const Array3DInfo&);
			Array3DImpl(const Array3D<T>&);
			Array3DImpl(const Array3DImpl<T>&);
			~Array3DImpl() { eraseCache(); }
    Array3DImpl<T>&	operator =( const Array3D<T>& ai )
			    { copyFrom(ai); return *this; }
    Array3DImpl<T>&	operator =( const Array3DImpl<T>& ai )
			    { copyFrom(ai); return *this; }

    bool		isOK() const { return base::storageOK(); }
    bool		canSetStorage() const	{ return true; }
    bool		setStorage(ValueSeries<T>* vs);

    void		set(idx_type,idx_type,idx_type,T);
    T			get(idx_type,idx_type,idx_type) const;
    void		copyFrom(const Array3D<T>&);

    const Array3DInfo&	info() const			{ return inf_; }
    bool		canSetInfo() const		{ return true; }
    bool		setInfo(const ArrayNDInfo&);
    bool		setSize(size_type,size_type,size_type);

    T***		get3DData()		{ return arr3d_; }
    const T***		get3DData() const	{ return (const T***)arr3d_; }

protected:

    void		updateStorage();
    const T*		getData_() const	{ return base::arr_; }
    const ValueSeries<T>* getStorage_() const	{ return base::stor_; }
    total_size_type	getStorageSize() const  { return inf_.totalSize(); }

    void		updateCachePointers();
    void		eraseCache();

    TypeSet<T**>	cachestor_;
    T***		arr3d_;
    Array3DInfoImpl	inf_;

};


/*!\brief Flat-array implementation of Array4D. */

template <class T>
mClass(Basic) Array4DImpl : public Array4D<T>, public ArrayNDImplBase<T>
{ typedef ArrayNDImplBase<T> base;
public:
			mTypeDefArrNDTypes;

			Array4DImpl(size_type,size_type,size_type,size_type);
			Array4DImpl(const Array4DInfo&);
			Array4DImpl(const Array4D<T>&);
			Array4DImpl(const Array4DImpl<T>&);
			~Array4DImpl() { eraseCache(); }
    Array4DImpl<T>&	operator =( const Array4D<T>& ai )
			    { copyFrom(ai); return *this; }
    Array4DImpl<T>&	operator =( const Array4DImpl<T>& ai )
			    { copyFrom(ai); return *this; }

    bool		isOK() const { return base::storageOK(); }
    bool		canSetStorage() const	{ return true; }
    bool		setStorage(ValueSeries<T>* vs);

    void		set(idx_type,idx_type,idx_type,idx_type,T);
    T			get(idx_type,idx_type,idx_type,idx_type) const;
    void		copyFrom(const Array4D<T>&);

    const Array4DInfo&	info() const			{ return inf_; }
    bool		canSetInfo() const		{ return true; }
    bool		setInfo(const ArrayNDInfo&);
    bool		setSize(size_type,size_type,size_type,size_type);

    T****		get4DData()		{ return arr4d_; }
    const T****		get4DData() const	{ return (const T****)arr4d_; }

protected:

    void		updateStorage();
    const T*		getData_() const	{ return base::arr_; }
    const ValueSeries<T>* getStorage_() const	{ return base::stor_; }
    total_size_type	getStorageSize() const  { return inf_.totalSize(); }

    void		updateCachePointers();
    void		eraseCache();

    TypeSet<T***>	cachestor_;
    T****		arr4d_;
    Array4DInfoImpl	inf_;

};


/*!\brief Flat-array implementation of ArrayND. */
mGlobal(Basic) void* getArrayND(const ArrayNDInfo&,const OD::DataRepType);

template <class T>
mClass(Basic) ArrayNDImpl : public ArrayND<T>, public ArrayNDImplBase<T>
{ typedef ArrayNDImplBase<T> base;
public:
			mTypeDefArrNDTypes;

    static ArrayND<T>*	create(const ArrayNDInfo& nsz);
    static ArrayND<T>*	clone(const ArrayND<T>&);
    static bool		clone(const ArrayND<T>&,ArrayND<T>&);

			ArrayNDImpl(const ArrayNDInfo&);
			ArrayNDImpl(const ArrayND<T>&);
			ArrayNDImpl(const ArrayNDImpl<T>&);
    ArrayNDImpl<T>&	operator =( const ArrayND<T>& ai )
			    { copyFrom(ai); return *this; }
    ArrayNDImpl<T>&	operator =( const ArrayNDImpl<T>& ai )
			    { copyFrom(ai); return *this; }
			~ArrayNDImpl();

    bool		isOK() const { return base::storageOK(); }
    bool		canSetStorage() const	{ return true; }
    bool		setStorage(ValueSeries<T>* vs)
			    { return base::setStorageInternal(vs); }

    void		setND(NDPos,T);
    T			getND(NDPos) const;

    const ArrayNDInfo&	info() const		{ return *inf_; }
    bool		canSetInfo() const	{ return true; }
    bool		canChangeNrDims() const	{ return true; }
    bool		setInfo(const ArrayNDInfo&);

    bool		setSize(NDSize);
    void		copyFrom(const ArrayND<T>&);

protected:

    const T*		getData_() const	{ return base::arr_; }
    const ValueSeries<T>* getStorage_() const	{ return base::stor_; }
    total_size_type	getStorageSize() const  { return inf_->totalSize(); }

    ArrayNDInfo*	inf_;
};


template <class T> inline
ArrayNDImplBase<T>::ArrayNDImplBase()
    : stor_(0)
    , arr_(0)
{ }


template <class T> inline
bool ArrayNDImplBase<T>::setStorageNoResize( ValueSeries<T>* s )
{
    arr_ = 0;
    delete stor_;

    stor_ = s;

    arr_ = stor_->arr();

    return true;
}


template <class T> inline
bool ArrayNDImplBase<T>::setStorageInternal( ValueSeries<T>* s )
{
    arr_ = 0;
    if ( !s->setSize(getStorageSize()) )
	{ delete s; return false; }

    return setStorageNoResize( s );
}


template <class T> inline
bool ArrayNDImplBase<T>::updateStorageSize()
{
    if ( !stor_ )
	setStorageNoResize(
	    new MultiArrayValueSeries<T,T>(getStorageSize()));

    if ( !stor_ || !stor_->setSize( getStorageSize() ) )
	{ arr_ = 0; delete stor_; stor_ = 0; return false; }

    arr_ = stor_->arr();
    return true;
}


template <class T> inline
bool ArrayNDImplBase<T>::storageOK() const
{ return stor_ && stor_->isOK(); }


template <class T> inline
bool ArrayNDImplBase<T>::getDataFrom( const ArrayND<T>& templ )
{
    if ( !storageOK() )
	return false;

    if ( arr_ )
	{ templ.getAll( arr_ ); return true; }
    else if ( stor_ )
	{ templ.getAll( *stor_ ); return true; }
    else if ( getStorageSize() )
	{ pErrMsg("Cannot store in array without storage" ); return false; }

    return true;
}


template <class T> inline
Array1DImpl<T>::Array1DImpl( size_type nsz )
    : inf_(nsz)
{
    base::updateStorageSize();
}


template <class T> inline
Array1DImpl<T>::Array1DImpl( const Array1D<T>& templ )
    : inf_(templ.info())
{
    base::updateStorageSize();
    copyFrom( templ );
}


template <class T> inline
Array1DImpl<T>::Array1DImpl( const Array1DImpl<T>& templ )
    : inf_(templ.info())
{
    base::updateStorageSize();
    copyFrom( templ );
}


template <class T> inline
void Array1DImpl<T>::set( idx_type pos, T v )
{
#ifdef __debug__
    if ( !inf_.validPos( pos ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    if ( base::arr_ )
	base::arr_[pos] = v;
    else
	base::stor_->setValue(pos,v);
}


template <class T> inline
T Array1DImpl<T>::get( idx_type pos ) const
{
#ifdef __debug__
    if ( !inf_.validPos( pos ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    return base::arr_ ? base::arr_[pos] : base::stor_->value(pos);
}


template <class T> inline
void Array1DImpl<T>::copyFrom( const Array1D<T>& templ )
{
    if ( inf_ != templ.info() )
	setInfo( templ.info() );

    base::getDataFrom( templ );
}


template <class T> inline
bool Array1DImpl<T>::setInfo( const ArrayNDInfo& ni )
{
    if ( ni.nrDims() != 1 )
	return false;
    return setSize( ni.getSize(0) );
}


template <class T> inline
bool Array1DImpl<T>::setSize( size_type s )
{
    inf_.setSize( 0, s );
    base::updateStorageSize();
    return true;
}


template <class T> inline
Array2DImpl<T>::Array2DImpl( size_type sz0, size_type sz1 )
    : inf_(sz0,sz1)
    , arr2d_(0)
{
    updateStorage();
}


template <class T> inline
Array2DImpl<T>::Array2DImpl( const Array2DInfo& nsz )
    : inf_( nsz )
    , arr2d_(0)
{
    updateStorage();
}


template <class T> inline
Array2DImpl<T>::Array2DImpl( const Array2D<T>& templ )
    : inf_(templ.info())
    , arr2d_(0)
{
    updateStorage();
    copyFrom( templ );
}


template <class T> inline
Array2DImpl<T>::Array2DImpl( const Array2DImpl<T>& templ )
    : inf_(templ.info())
    , arr2d_(0)
{
    updateStorage();
    copyFrom( templ );
}


template <class T> inline
void Array2DImpl<T>::set( idx_type p0, idx_type p1, T v )
{
#ifdef __debug__
    if ( !inf_.validPos( p0, p1 ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    if ( arr2d_ )
	arr2d_[p0][p1] = v;
    else
    {
	const offset_type offset = inf_.getOffset( p0, p1 );
	base::stor_->setValue( offset, v );
    }
}


template <class T> inline
T Array2DImpl<T>::get( idx_type p0, idx_type p1 ) const
{
#ifdef __debug__
    if ( !inf_.validPos( p0, p1 ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif

    if ( arr2d_ )
	return arr2d_[p0][p1];

    const offset_type offset = inf_.getOffset( p0, p1 );
    return base::stor_->value( offset );
}


template <class T> inline
bool Array2DImpl<T>::setStorage(ValueSeries<T>* vs)
{
    bool res = base::setStorageInternal(vs);
    updateCachePointers();
    return res;
}


template <class T> inline
void Array2DImpl<T>::copyFrom( const Array2D<T>& templ )
{
    if ( inf_ != templ.info() )
	setInfo( templ.info() );

    base::getDataFrom( templ );
}


template <class T> inline
bool Array2DImpl<T>::setInfo( const ArrayNDInfo& ni )
{
    if ( ni.nrDims() != 2 )
	return false;
    return setSize( ni.getSize(0), ni.getSize(1) );
}


template <class T> inline
bool Array2DImpl<T>::setSize( size_type d0, size_type d1 )
{
    inf_.setSize( 0, d0 ); inf_.setSize( 1, d1 );
    updateStorage();
    return true;
}


template <class T> inline
void Array2DImpl<T>::updateStorage()
{
    base::updateStorageSize();
    updateCachePointers();
}


template <class T> inline
void Array2DImpl<T>::updateCachePointers()
{
    deleteAndZeroArrPtr( arr2d_ );

    if ( !base::arr_ )
	return;

    const size_type n1 = inf_.getSize( 0 );
    mTryAlloc(arr2d_,T*[n1])
    if ( !arr2d_ )
	return;

    const size_type n2 = inf_.getSize( 1 );
    offset_type offset = 0;
    for ( idx_type idx=0; idx<n1; idx++, offset+=n2 )
	arr2d_[idx] = base::arr_ + offset;
}


//Array3DImpl---

template <class T> inline
Array3DImpl<T>::Array3DImpl( size_type sz0, size_type sz1, size_type sz2 )
    : inf_(sz0,sz1,sz2)
    , arr3d_(0)
{
    updateStorage();
}


template <class T> inline
Array3DImpl<T>::Array3DImpl( const Array3DInfo& nsz )
    : inf_(nsz)
    , arr3d_(0)
{
    updateStorage();
}


template <class T> inline
Array3DImpl<T>::Array3DImpl( const Array3D<T>& templ )
    : inf_(templ.info())
    , arr3d_(0)
{
    updateStorage();
    copyFrom( templ );
}


template <class T> inline
Array3DImpl<T>::Array3DImpl( const Array3DImpl<T>& templ )
    : inf_(templ.info())
    , arr3d_(0)
{
    updateStorage();
    copyFrom( templ );
}


template <class T> inline
void Array3DImpl<T>::set( idx_type p0, idx_type p1, idx_type p2, T v )
{
#ifdef __debug__
    if ( !inf_.validPos( p0, p1, p2 ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    if ( arr3d_ )
	arr3d_[p0][p1][p2] = v;
    else
    {
	const offset_type offset = inf_.getOffset( p0, p1, p2 );
	base::stor_->setValue( offset, v );
    }
}


template <class T> inline
T Array3DImpl<T>::get( idx_type p0, idx_type p1, idx_type p2 ) const
{
#ifdef __debug__
    if ( !inf_.validPos( p0, p1, p2 ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    if ( arr3d_ )
	return arr3d_[p0][p1][p2];

    const offset_type offset = inf_.getOffset( p0, p1, p2 );
    return base::stor_->value( offset );
}


template <class T> inline
void Array3DImpl<T>::copyFrom( const Array3D<T>& templ )
{
    if ( inf_ != templ.info() )
	setInfo( templ.info() );

    base::getDataFrom( templ );
}


template <class T> inline
bool Array3DImpl<T>::setStorage(ValueSeries<T>* vs)
{
    bool res = base::setStorageInternal(vs);
    updateCachePointers();
    return res;
}

template <class T> inline
bool Array3DImpl<T>::setInfo( const ArrayNDInfo& ni )
{
    if ( ni.nrDims() != 3 )
	return false;
    return setSize( ni.getSize(0), ni.getSize(1), ni.getSize(2) );
}


template <class T> inline
bool Array3DImpl<T>::setSize( size_type d0, size_type d1, size_type d2 )
{
    inf_.setSize( 0, d0 ); inf_.setSize( 1, d1 ); inf_.setSize( 2, d2 );
    updateStorage();
    return true;
}


template <class T> inline
void Array3DImpl<T>::updateStorage()
{
    base::updateStorageSize();
    updateCachePointers();
}


template <class T> inline
void Array3DImpl<T>::eraseCache()
{
    for ( int idx=0; idx<cachestor_.size(); idx++ )
	delete [] cachestor_[idx];

    cachestor_.erase();
    arr3d_ = 0;
}


template <class T> inline
void Array3DImpl<T>::updateCachePointers()
{
    eraseCache();

    if ( !base::arr_ )
	return;

    const size_type n1 = inf_.getSize( 0 );
    const size_type n2 = inf_.getSize( 1 );
    const size_type n3 = inf_.getSize( 2 );
    offset_type offset = 0;
    for ( idx_type idx=0; idx<n1; idx++ )
    {
	mDeclareAndTryAlloc(T**,ptr2d,T*[n2])
	if ( !ptr2d )
	    return;

	for ( idx_type idy=0; idy<n2; idy++, offset+=n3 )
	    ptr2d[idy] = base::arr_ + offset;

	cachestor_ += ptr2d;
    }

    arr3d_ = &cachestor_[0];
}


//Array4DImpl---

template <class T> inline
Array4DImpl<T>::Array4DImpl( size_type sz0, size_type sz1, size_type sz2,
			     size_type sz3 )
    : inf_(sz0,sz1,sz2,sz3)
    , arr4d_(0)
{
    updateStorage();
}


template <class T> inline
Array4DImpl<T>::Array4DImpl( const Array4DInfo& nsz )
    : inf_(nsz)
    , arr4d_(0)
{
    updateStorage();
}


template <class T> inline
Array4DImpl<T>::Array4DImpl( const Array4D<T>& templ )
    : inf_(templ.info())
    , arr4d_(0)
{
    updateStorage();
    copyFrom( templ );
}


template <class T> inline
Array4DImpl<T>::Array4DImpl( const Array4DImpl<T>& templ )
    : inf_(templ.info())
    , arr4d_(0)
{
    updateStorage();
    copyFrom( templ );
}


template <class T> inline
void Array4DImpl<T>::set( idx_type p0, idx_type p1, idx_type p2, idx_type p3,
			  T v )
{
#ifdef __debug__
    if ( !inf_.validPos( p0, p1, p2, p3 ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    if ( arr4d_ )
	arr4d_[p0][p1][p2][p3] = v;
    else
    {
	const offset_type offset = inf_.getOffset( p0, p1, p2, p3 );
	base::stor_->setValue( offset, v );
    }
}


template <class T> inline
T Array4DImpl<T>::get( idx_type p0, idx_type p1, idx_type p2,
			idx_type p3 ) const
{
#ifdef __debug__
    if ( !inf_.validPos( p0, p1, p2, p3 ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    if ( arr4d_ )
	return arr4d_[p0][p1][p2][p3];

    const offset_type offset = inf_.getOffset( p0, p1, p2, p3 );
    return base::stor_->value( offset );
}


template <class T> inline
void Array4DImpl<T>::copyFrom( const Array4D<T>& templ )
{
    if ( inf_ != templ.info() )
	setInfo( templ.info() );

    base::getDataFrom( templ );
}


template <class T> inline
bool Array4DImpl<T>::setStorage( ValueSeries<T>* vs )
{
    bool res = base::setStorageInternal(vs);
    updateCachePointers();
    return res;
}

template <class T> inline
bool Array4DImpl<T>::setInfo( const ArrayNDInfo& ni )
{
    if ( ni.nrDims() != 4 )
	return false;
    return setSize( ni.getSize(0), ni.getSize(1), ni.getSize(2), ni.getSize(3));
}


template <class T> inline
bool Array4DImpl<T>::setSize( size_type d0, size_type d1, size_type d2,
			      size_type d3 )
{
    inf_.setSize( 0, d0 ); inf_.setSize( 1, d1 );
    inf_.setSize( 2, d2 ); inf_.setSize( 3, d3 );
    updateStorage();
    return true;
}


template <class T> inline
void Array4DImpl<T>::updateStorage()
{
    base::updateStorageSize();
    updateCachePointers();
}


template <class T> inline
void Array4DImpl<T>::eraseCache()
{
    const size_type n2 = inf_.getSize( 1 );
    for ( int idx=0; idx<cachestor_.size(); idx++ )
    {
	T*** cachestoridx = cachestor_[idx];
	for ( int idy=0; idy<n2; idy++ )
	    delete [] cachestoridx[idy];
	delete [] cachestoridx;
    }

    cachestor_.erase();
    arr4d_ = 0;
}


template <class T> inline
void Array4DImpl<T>::updateCachePointers()
{
    eraseCache();

    if ( !base::arr_ )
	return;

    const size_type n1 = inf_.getSize( 0 );
    const size_type n2 = inf_.getSize( 1 );
    const size_type n3 = inf_.getSize( 2 );
    const size_type n4 = inf_.getSize( 3 );
    offset_type offset = 0;
    for ( idx_type idx=0; idx<n1; idx++ )
    {
	mDeclareAndTryAlloc(T***,ptr3d,T**[n2])
	if ( !ptr3d )
	    return;

	for ( idx_type idy=0; idy<n2; idy++ )
	{
	    mDeclareAndTryAlloc(T**,ptr2d,T*[n3])
	    if ( !ptr2d )
		return;

	    for ( idx_type idz=0; idz<n3; idz++, offset+=n4 )
		ptr2d[idz] = base::arr_ + offset;

	    ptr3d[idy] = ptr2d;
	}

	cachestor_ += ptr3d;
    }

    arr4d_ = &cachestor_[0];
}


template <class T> inline
ArrayNDImpl<T>::ArrayNDImpl( const ArrayNDInfo& nsz )
    : inf_(nsz.clone())
{
    base::updateStorageSize();
}


template <class T> inline
ArrayNDImpl<T>::ArrayNDImpl( const ArrayND<T>& templ )
    : inf_( templ.info().clone() )
{
    base::updateStorageSize();
    copyFrom( templ );
}


template <class T> inline
ArrayNDImpl<T>::ArrayNDImpl( const ArrayNDImpl<T>& templ )
    : inf_( templ.info().clone() )
{
    base::updateStorageSize();
    copyFrom( templ );
}


template <class T> inline ArrayNDImpl<T>::~ArrayNDImpl()
{ delete inf_; }


template <class T> inline
void ArrayNDImpl<T>::copyFrom( const ArrayND<T>& templ )
{
    if ( info() != templ.info() )
	{ delete inf_; inf_ = templ.info().clone(); }

    base::getDataFrom( templ );
}


template <class T> inline
void ArrayNDImpl<T>::setND( NDPos pos, T v )
{
#ifdef __debug__
    if ( !inf_->validPos( pos ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    const offset_type offset = inf_->getOffset(pos);
    if ( base::arr_ )
	base::arr_[offset] = v ;
    else
	base::stor_->setValue( offset, v);
}


template <class T> inline
T ArrayNDImpl<T>::getND( NDPos pos ) const
{
#ifdef __debug__
    if ( !inf_->validPos( pos ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    const offset_type offset = inf_->getOffset(pos);
    return base::arr_ ? base::arr_[offset] : base::stor_->value( offset );
}


template <class T> inline
bool ArrayNDImpl<T>::setInfo( const ArrayNDInfo& ni )
{
    if ( ni.nrDims() != inf_->nrDims() )
	{ pErrMsg("Changing dims in ND not supported"); return false; }

    const nr_dims_type ndim = inf_->nrDims();
    TypeSet<size_type> sizes( ndim, 0 );
    for ( dim_idx_type idx=0; idx<ndim; idx++ )
	sizes[idx] = ni.getSize(idx);

    return setSize( sizes.arr() );
}


template <class T> inline
bool ArrayNDImpl<T>::setSize( NDSize d )
{
    const nr_dims_type ndim = inf_->nrDims();
    for ( dim_idx_type idx=0; idx<ndim; idx++ )
	inf_->setSize( idx, d[idx] );

    base::updateStorageSize();
    return true;
}


template <class T> inline
ArrayND<T>* ArrayNDImpl<T>::create( const ArrayNDInfo& inf )
{
    const nr_dims_type ndim = inf.nrDims();

    if ( ndim==1 )
	return new Array1DImpl<T>( inf.getSize(0) );
    if ( ndim==2 )
	return new Array2DImpl<T>( inf.getSize(0), inf.getSize(1) );
    if ( ndim==3 )
	return new Array3DImpl<T>( inf.getSize(0), inf.getSize(1),
				   inf.getSize(2) );
    if ( ndim==4 )
	return new Array4DImpl<T>( inf.getSize(0), inf.getSize(1),
				   inf.getSize(2), inf.getSize(3) );

    return new ArrayNDImpl<T>( inf );
}


template <class T> inline
ArrayND<T>* ArrayNDImpl<T>::clone( const ArrayND<T>& oth )
{
    ArrayND<T>* out = create( oth.info() );
    if ( !out )
	return 0;
    else if ( !out->isOK() )
	{ delete out; return 0; }
    else if ( !clone(oth,*out) )
	{ delete out; return 0; }

    return out;
}


template <class T> inline
bool ArrayNDImpl<T>::clone( const ArrayND<T>& inp, ArrayND<T>& out )
{
    const total_size_type sz = inp.totalSize();
    if ( sz < 1 || !inp.isOK() )
	return false;

    if ( !out.isOK() || out.info() != inp.info() )
    {
	if ( !out.setInfo(inp.info()) ) //Also allocates storage
	    return false;
    }

    if ( inp.getData() && out.getData() )
    {
	OD::memCopy( out.getData(), inp.getData(), sz*sizeof(T) );
	return true;
    }

    const nr_dims_type ndim = inp.nrDims();
    if ( ndim == 1 )
    {
	mDynamicCastGet(const Array1DImpl<T>*,inp1d,&inp)
	mDynamicCastGet(Array1DImpl<T>*,out1d,&out)
	if ( inp1d && out1d )
	    { *out1d = *inp1d; return true; }
    }
    else if ( ndim == 2 )
    {
	mDynamicCastGet(const Array2DImpl<T>*,inp2d,&inp)
	mDynamicCastGet(Array2DImpl<T>*,out2d,&out)
	if ( inp2d && out2d )
	    { *out2d = *inp2d; return true; }
    }
    else if ( ndim == 3 )
    {
	mDynamicCastGet(const Array3DImpl<T>*,inp3d,&inp)
	mDynamicCastGet(Array3DImpl<T>*,out3d,&out)
	if ( inp3d && out3d )
	    { *out3d = *inp3d; return true; }
    }
    else if ( ndim == 4 )
    {
	mDynamicCastGet(const Array4DImpl<T>*,inp4d,&inp)
	mDynamicCastGet(Array4DImpl<T>*,out4d,&out)
	if ( inp4d && out4d )
	    { *out4d = *inp4d; return true; }
    }

    mDynamicCastGet(const ArrayNDImpl<T>*,inpnd,&inp)
    mDynamicCastGet(ArrayNDImpl<T>*,outnd,&out)
    if ( inpnd && outnd )
    {
	*outnd = *inpnd;
	return true;
    }

    ValueSeries<T>* newstor = out.getStorage();
    if ( newstor )
    {
	inp.getAll( *newstor );
	return true;
    }

    ArrayNDIter iter( inp.info() );
    do
    {
	NDPos pos = iter.getPos();
	out.setND( pos, inp.getND(pos) );
    } while ( iter.next() );

    return true;
}
