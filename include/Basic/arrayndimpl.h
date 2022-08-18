#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "arraynd.h"
#include "valseriesimpl.h"

#ifdef __debug__
# include "debug.h"
#endif

/*!Helper class to handle internal storage of arrays */

template <class T>
mClass(Basic) ArrayImplBase
{
protected:
    virtual od_int64	getStorageSize() const			= 0;

			ArrayImplBase();
			~ArrayImplBase()	{ delete stor_; }

    bool		storageOK() const;
    bool		updateStorageSize();
			/*!<Sets storage size to getStorageSize()
			    and creates if need be. */

    bool		setStorageNoResize(ValueSeries<T>*);
    bool		setStorageInternal(ValueSeries<T>*);
    bool		getDataFrom(const ArrayND<T>& templ);

    ValueSeries<T>*	stor_ = nullptr;
    T*			ptr_ = nullptr;	//not owned, only a shortcut
					//for the 99% percent case
};


/*!
\brief Implementation of Array1D.
*/

template <class T>
mClass(Basic) Array1DImpl : public Array1D<T>, public ArrayImplBase<T>
{ typedef ArrayImplBase<T> base;
public:

			Array1DImpl(int sz);
			Array1DImpl(const Array1D<T>&);
			Array1DImpl(const Array1DImpl<T>&);
    Array1DImpl<T>&	operator =( const Array1D<T>& ai )
			    { copyFrom(ai); return *this; }
    Array1DImpl<T>&	operator =( const Array1DImpl<T>& ai )
			    { copyFrom(ai); return *this; }

    ValueSeries<T>*	clone() const override
			    { return new Array1DImpl<T>(*this); }

    bool		isOK() const override	{ return base::storageOK(); }
    void		copyFrom(const Array1D<T>&);
    bool		setStorage( ValueSeries<T>* vs ) override
			    { return base::setStorageInternal(vs); }
    bool		canSetStorage() const override	{ return true; }

    void		set(int pos,T) override;
    T			get(int pos) const override;

    const Array1DInfo&	info() const override		{ return in_; }
    bool		canSetInfo() const override	{ return true; }
    bool		setInfo(const ArrayNDInfo&) override;
    bool		setSize(int);
    bool		setSize( od_int64 sz ) override
			    { return setSize( ((int)sz) ); }

			// ValueSeries interface
    T*			arr() override		{ return base::ptr_; }
    const T*		arr() const override	{ return base::ptr_; }

protected:
    const T*		getData_() const override	{ return base::ptr_; }
    const ValueSeries<T>* getStorage_() const override	{ return base::stor_; }
    od_int64		getStorageSize() const override
			{ return in_.getTotalSz(); }

    Array1DInfoImpl	in_;

};


/*!
\brief Implementation of Array2D.
*/

template <class T>
mClass(Basic) Array2DImpl : public Array2D<T>, public ArrayImplBase<T>
{ typedef ArrayImplBase<T> base;
public:
			Array2DImpl(int sz0,int sz1);
			Array2DImpl(const Array2DInfo&);
			Array2DImpl(const Array2D<T>&);
			Array2DImpl(const Array2DImpl<T>&);
			~Array2DImpl() { deleteAndZeroArrPtr( ptr2d_ ); }

    Array2DImpl<T>&	operator =( const Array2D<T>& ai )
			    { copyFrom(ai); return *this; }
    Array2DImpl<T>&	operator =( const Array2DImpl<T>& ai )
			    { copyFrom(ai); return *this; }

    bool		isOK() const override	{ return base::storageOK(); }
    bool		canSetStorage() const override		{ return true; }
    bool		setStorage(ValueSeries<T>*) override;

    void		set(int,int,T) override;
    T			get(int,int) const override;
    void		copyFrom(const Array2D<T>&);

    const Array2DInfo&	info() const override		{ return in_; }
    bool		canSetInfo() const override	{ return true; }

    bool		setInfo(const ArrayNDInfo&) override;
    bool		setSize(int,int);

    T**			get2DData() override		{ return ptr2d_; }
    const T**		get2DData() const override { return (const T**) ptr2d_;}

protected:
    void		updateStorage();
    const T*		getData_() const override	{ return base::ptr_; }
    const ValueSeries<T>* getStorage_() const override	{ return base::stor_; }
    od_int64		getStorageSize() const override
			{ return in_.getTotalSz(); }

    void		updateCachePointers();

    Array2DInfoImpl	in_;
    T**			ptr2d_ = nullptr;
};


/*!
\brief Implementation of Array3D.
*/

template <class T>
mClass(Basic) Array3DImpl : public Array3D<T>, public ArrayImplBase<T>
{ typedef ArrayImplBase<T> base;
public:
			Array3DImpl(int sz0,int sz1,int sz2);
			Array3DImpl(const Array3DInfo&);
			Array3DImpl(const Array3D<T>&);
			Array3DImpl(const Array3DImpl<T>&);
			~Array3DImpl() { eraseCache(); }
    Array3DImpl<T>&	operator =( const Array3D<T>& ai )
			    { copyFrom(ai); return *this; }
    Array3DImpl<T>&	operator =( const Array3DImpl<T>& ai )
			    { copyFrom(ai); return *this; }

    bool		isOK() const override { return base::storageOK(); }
    bool		canSetStorage() const override	{ return true; }
    bool		setStorage(ValueSeries<T>*) override;

    void		set(int,int,int,T) override;
    T			get(int,int,int) const override;
    void		copyFrom(const Array3D<T>&);

    const Array3DInfo&	info() const override		{ return in_; }
    bool		canSetInfo() const override	{ return true; }
    bool		setInfo(const ArrayNDInfo&) override;
    bool		setSize(int,int,int);

    T***		get3DData() override		{ return ptr3d_; }
    const T***		get3DData() const override
			{ return (const T***) ptr3d_; }

protected:
    void		updateStorage();
    const T*		getData_() const override	{ return base::ptr_; }
    const ValueSeries<T>* getStorage_() const override	{ return base::stor_; }
    od_int64		getStorageSize() const override
			{ return in_.getTotalSz(); }

    void		updateCachePointers();
    void		eraseCache();

    TypeSet<T**>	cachestor_;
    T***		ptr3d_ = nullptr;
    Array3DInfoImpl	in_;
};


/*!\brief Flat-array implementation of Array4D. */

template <class T>
mClass(Basic) Array4DImpl : public Array4D<T>, public ArrayImplBase<T>
{ typedef ArrayImplBase<T> base;
public:
			Array4DImpl(int,int,int,int);
			Array4DImpl(const Array4DInfo&);
			Array4DImpl(const Array4D<T>&);
			Array4DImpl(const Array4DImpl<T>&);
			~Array4DImpl() { eraseCache(); }
    Array4DImpl<T>&	operator =( const Array4D<T>& ai )
			    { copyFrom(ai); return *this; }
    Array4DImpl<T>&	operator =( const Array4DImpl<T>& ai )
			    { copyFrom(ai); return *this; }

    bool		isOK() const override { return base::storageOK(); }
    bool		canSetStorage() const override	{ return true; }
    bool		setStorage(ValueSeries<T>*) override;

    void		set(int,int,int,int,T) override;
    T			get(int,int,int,int) const override;
    void		copyFrom(const Array4D<T>&);

    const Array4DInfo&	info() const override		{ return inf_; }
    bool		canSetInfo() const override	{ return true; }
    bool		setInfo(const ArrayNDInfo&) override;
    bool		setSize(int,int,int,int);

    T****		get4DData() override	{ return arr4d_; }
    const T****		get4DData() const override
			{ return (const T****)arr4d_; }

protected:

    void		updateStorage();
    const T*		getData_() const override	{ return base::ptr_; }
    const ValueSeries<T>* getStorage_() const override	{ return base::stor_; }
    od_int64		getStorageSize() const override
			{ return inf_.totalSize(); }

    void		updateCachePointers();
    void		eraseCache();

    TypeSet<T***>	cachestor_;
    T****		arr4d_ = nullptr;
    Array4DInfoImpl	inf_;
};


/*!
\brief Implementation of ArrayND.
*/

mGlobal(Basic) void* getArrayND(const ArrayNDInfo&,const OD::DataRepType);

template <class T>
mClass(Basic) ArrayNDImpl : public ArrayND<T>, public ArrayImplBase<T>
{ typedef ArrayImplBase<T> base;
public:

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

    bool		isOK() const override	{ return base::storageOK(); }
    bool		canSetStorage() const override	{ return true; }
    bool		setStorage( ValueSeries<T>* vs ) override
			    { return base::setStorageInternal(vs); }

    void		setND(const int*,T) override;
    T			getND(const int*) const override;

    const ArrayNDInfo&	info() const override		{ return *in_; }
    bool		canSetInfo() const override	{ return true; }
    bool		canChangeNrDims() const override { return true; }
    bool		setInfo(const ArrayNDInfo&) override;

    bool		setSize(const int*);
    void		copyFrom(const ArrayND<T>&);

protected:

    const T*		getData_() const override	{ return base::ptr_; }
    const ValueSeries<T>* getStorage_() const override	{ return base::stor_; }
    od_int64		getStorageSize() const override
			{ return in_->getTotalSz(); }

    ArrayNDInfo*	in_;
};


template <class T> inline
ArrayImplBase<T>::ArrayImplBase()
{}


template <class T> inline
bool ArrayImplBase<T>::setStorageNoResize( ValueSeries<T>* s )
{
    ptr_ = nullptr;
    delete stor_;

    stor_ = s;

    ptr_ = stor_->arr();

    return true;
}


template <class T> inline
bool ArrayImplBase<T>::setStorageInternal( ValueSeries<T>* s )
{
    ptr_ = nullptr;
    if ( !s->setSize(getStorageSize()) )
    {
	delete s;
	return false;
    }

    return setStorageNoResize( s );
}


template <class T> inline
bool ArrayImplBase<T>::updateStorageSize()
{
    if ( !stor_ )
    {
	setStorageNoResize(
	    new MultiArrayValueSeries<T,T>(getStorageSize()));
    }

    if ( !stor_ || !stor_->setSize( getStorageSize() ) )
    {
	ptr_ = nullptr;
	delete stor_;
	stor_ = nullptr;
	return false;
    }

    ptr_ = stor_->arr();
    return true;
}


template <class T> inline
bool ArrayImplBase<T>::storageOK() const
{ return stor_ && stor_->isOK(); }


template <class T> inline
bool ArrayImplBase<T>::getDataFrom( const ArrayND<T>& templ )
{
    if ( !storageOK() )
	return false;

    if ( ptr_ )
    {
	templ.getAll( ptr_ );
	return true;
    }

    if ( stor_ )
    {
	templ.getAll( *stor_ );
	return true;
    }

    if ( getStorageSize() )
    {
	pErrMsg("Cannot store in array without storage" );
	return false;
    }

    return true;
}


template <class T> inline
Array1DImpl<T>::Array1DImpl( int nsz )
    : in_(nsz)
{
    base::updateStorageSize();
}


template <class T> inline
Array1DImpl<T>::Array1DImpl( const Array1D<T>& templ )
    : in_(templ.info())
{
    base::updateStorageSize();
    copyFrom( templ );
}


template <class T> inline
Array1DImpl<T>::Array1DImpl( const Array1DImpl<T>& templ )
    : in_(templ.info())
{
    base::updateStorageSize();
    copyFrom( templ );
}


template <class T> inline
void Array1DImpl<T>::set( int pos, T v )
{
#ifdef __debug__
    if ( !in_.validPos( pos ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    if ( base::ptr_ ) base::ptr_[pos] = v;
    else base::stor_->setValue(pos,v);
}


template <class T> inline
T Array1DImpl<T>::get( int pos ) const
{
#ifdef __debug__
    if ( !in_.validPos( pos ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    return base::ptr_ ? base::ptr_[pos] : base::stor_->value(pos);
}


template <class T> inline
void Array1DImpl<T>::copyFrom( const Array1D<T>& templ )
{
    if ( in_ != templ.info() )
	setInfo( templ.info() );

    base::getDataFrom( templ );
}


template <class T> inline
bool Array1DImpl<T>::setInfo( const ArrayNDInfo& ni )
{
    if ( ni.getNDim() != 1 ) return false;
    return setSize( ni.getSize(0) );
}


template <class T> inline
bool Array1DImpl<T>::setSize( int s )
{
    in_.setSize( 0, s );
    base::updateStorageSize();
    return true;
}


template <class T> inline
Array2DImpl<T>::Array2DImpl( int sz0, int sz1 )
    : in_(sz0,sz1)
{
    updateStorage();
}


template <class T> inline
Array2DImpl<T>::Array2DImpl( const Array2DInfo& nsz )
    : in_( nsz )
{
    updateStorage();
}


template <class T> inline
Array2DImpl<T>::Array2DImpl( const Array2D<T>& templ )
    : in_(templ.info())
{
    updateStorage();
    copyFrom( templ );
}


template <class T> inline
Array2DImpl<T>::Array2DImpl( const Array2DImpl<T>& templ )
    : in_(templ.info())
{
    updateStorage();
    copyFrom( templ );
}


template <class T> inline
void Array2DImpl<T>::set( int p0, int p1, T v )
{
#ifdef __debug__
    if ( !in_.validPos( p0, p1 ) )
    {
	pErrMsg("Invalid access");
	DBG::forceCrash(true);
    }
#endif
    if ( ptr2d_ )
    {
	ptr2d_[p0][p1] = v;
    }
    else
    {
	const od_int64 offset = in_.getOffset( p0, p1 );
	base::stor_->setValue( offset, v );
    }
}


template <class T> inline
T Array2DImpl<T>::get( int p0, int p1 ) const
{
#ifdef __debug__
    if ( !in_.validPos( p0, p1 ) )
    {
	pErrMsg("Invalid access");
	DBG::forceCrash(true);
    }
#endif

    if ( ptr2d_ )
	return ptr2d_[p0][p1];

    const od_int64 offset = in_.getOffset( p0, p1 );
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
    if ( in_ != templ.info() )
	setInfo( templ.info() );

    base::getDataFrom( templ );
}


template <class T> inline
bool Array2DImpl<T>::setInfo( const ArrayNDInfo& ni )
{
    if ( ni.getNDim() != 2 ) return false;
    return setSize( ni.getSize(0), ni.getSize(1) );
}


template <class T> inline
bool Array2DImpl<T>::setSize( int d0, int d1 )
{
    in_.setSize( 0, d0 ); in_.setSize( 1, d1 );
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
    deleteAndZeroArrPtr( ptr2d_ );

    if ( !base::ptr_ )
	return;

    const int n1 = in_.getSize( 0 );
    mTryAlloc(ptr2d_,T*[n1])
    if ( !ptr2d_ )
	return;

    const int n2 = in_.getSize( 1 );
    od_uint64 offset = 0;
    for ( int idx=0; idx<n1; idx++, offset+=n2 )
	ptr2d_[idx] = base::ptr_ + offset;
}


template <class T> inline
Array3DImpl<T>::Array3DImpl( int sz0, int sz1, int sz2 )
    : in_(sz0,sz1,sz2)
{
    updateStorage();
}


template <class T> inline
Array3DImpl<T>::Array3DImpl( const Array3DInfo& nsz )
    : in_(nsz)
{
    updateStorage();
}


template <class T> inline
Array3DImpl<T>::Array3DImpl( const Array3D<T>& templ )
    : in_(templ.info())
{
    updateStorage();
    copyFrom( templ );
}


template <class T> inline
Array3DImpl<T>::Array3DImpl( const Array3DImpl<T>& templ )
    : in_(templ.info())
{
    updateStorage();
    copyFrom( templ );
}


template <class T> inline
void Array3DImpl<T>::set( int p0, int p1, int p2, T v )
{
#ifdef __debug__
    if ( !in_.validPos( p0, p1, p2 ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    if ( ptr3d_ )
    {
	ptr3d_[p0][p1][p2] = v;
    }
    else
    {
	const od_int64 offset = in_.getOffset( p0, p1, p2 );
	base::stor_->setValue( offset, v );
    }
}


template <class T> inline
T Array3DImpl<T>::get( int p0, int p1, int p2 ) const
{
#ifdef __debug__
    if ( !in_.validPos( p0, p1, p2 ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    if ( ptr3d_ )
	return ptr3d_[p0][p1][p2];

    const od_int64 offset = in_.getOffset( p0, p1, p2 );
    return base::stor_->value( offset );
}


template <class T> inline
void Array3DImpl<T>::copyFrom( const Array3D<T>& templ )
{
    if ( in_ != templ.info() )
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
    if ( ni.getNDim() != 3 ) return false;
    return setSize( ni.getSize(0), ni.getSize(1), ni.getSize(2) );
}


template <class T> inline
bool Array3DImpl<T>::setSize( int d0, int d1, int d2 )
{
    in_.setSize( 0, d0 ); in_.setSize( 1, d1 ); in_.setSize( 2, d2 );
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
    {
	delete [] cachestor_[idx];
    }

    cachestor_.erase();
    ptr3d_ = nullptr;
}


template <class T> inline
void Array3DImpl<T>::updateCachePointers()
{
    eraseCache();

    if ( !base::ptr_ )
	return;

    const int n1 = in_.getSize( 0 );
    const int n2 = in_.getSize( 1 );
    const int n3 = in_.getSize( 2 );
    od_uint64 offset = 0;
    for ( int idx=0; idx<n1; idx++ )
    {
	mDeclareAndTryAlloc(T**,ptr2d,T*[n2])
	if ( !ptr2d )
	    return;

	for ( int idy=0; idy<n2; idy++, offset+=n3 )
	    ptr2d[idy] = base::ptr_ + offset;

	cachestor_ += ptr2d;
    }

    ptr3d_ = &cachestor_[0];
}


//Array4DImpl---

template <class T> inline
Array4DImpl<T>::Array4DImpl( int sz0, int sz1, int sz2, int sz3 )
    : inf_(sz0,sz1,sz2,sz3)
{
    updateStorage();
}


template <class T> inline
Array4DImpl<T>::Array4DImpl( const Array4DInfo& nsz )
    : inf_(nsz)
{
    updateStorage();
}


template <class T> inline
Array4DImpl<T>::Array4DImpl( const Array4D<T>& templ )
    : inf_(templ.info())
{
    updateStorage();
    copyFrom( templ );
}


template <class T> inline
Array4DImpl<T>::Array4DImpl( const Array4DImpl<T>& templ )
    : inf_(templ.info())
{
    updateStorage();
    copyFrom( templ );
}


template <class T> inline
void Array4DImpl<T>::set( int p0, int p1, int p2, int p3, T v )
{
#ifdef __debug__
    if ( !inf_.validPos( p0, p1, p2, p3 ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    if ( arr4d_ )
	arr4d_[p0][p1][p2][p3] = v;
    else
    {
	const od_uint64 offset = inf_.getOffset( p0, p1, p2, p3 );
	base::stor_->setValue( offset, v );
    }
}


template <class T> inline
T Array4DImpl<T>::get( int p0, int p1, int p2, int p3 ) const
{
#ifdef __debug__
    if ( !inf_.validPos( p0, p1, p2, p3 ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    if ( arr4d_ )
	return arr4d_[p0][p1][p2][p3];

    const od_uint64 offset = inf_.getOffset( p0, p1, p2, p3 );
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
bool Array4DImpl<T>::setSize( int d0, int d1, int d2,
			      int d3 )
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
    const int n2 = inf_.getSize( 1 );
    for ( int idx=0; idx<cachestor_.size(); idx++ )
    {
	T*** cachestoridx = cachestor_[idx];
	for ( int idy=0; idy<n2; idy++ )
	    delete [] cachestoridx[idy];
	delete [] cachestoridx;
    }

    cachestor_.erase();
    arr4d_ = nullptr;
}


template <class T> inline
void Array4DImpl<T>::updateCachePointers()
{
    eraseCache();

    if ( !base::ptr_ )
	return;

    const int n1 = inf_.getSize( 0 );
    const int n2 = inf_.getSize( 1 );
    const int n3 = inf_.getSize( 2 );
    const int n4 = inf_.getSize( 3 );
    od_uint64 offset = 0;
    for ( int idx=0; idx<n1; idx++ )
    {
	mDeclareAndTryAlloc(T***,ptr3d,T**[n2])
	if ( !ptr3d )
	    return;

	for ( int idy=0; idy<n2; idy++ )
	{
	    mDeclareAndTryAlloc(T**,ptr2d,T*[n3])
	    if ( !ptr2d )
		return;

	    for ( int idz=0; idz<n3; idz++, offset+=n4 )
		ptr2d[idz] = base::ptr_ + offset;

	    ptr3d[idy] = ptr2d;
	}

	cachestor_ += ptr3d;
    }

    arr4d_ = &cachestor_[0];
}


template <class T> inline
ArrayNDImpl<T>::ArrayNDImpl( const ArrayNDInfo& nsz )
    : in_(nsz.clone())
{
    base::updateStorageSize();
}


template <class T> inline
ArrayNDImpl<T>::ArrayNDImpl( const ArrayND<T>& templ )
    : in_( templ.info().clone() )
{
    base::updateStorageSize();
    copyFrom( templ );
}


template <class T> inline
ArrayNDImpl<T>::ArrayNDImpl( const ArrayNDImpl<T>& templ )
    : in_( templ.info().clone() )
{
    base::updateStorageSize();
    copyFrom( templ );
}


template <class T> inline ArrayNDImpl<T>::~ArrayNDImpl()
{ delete in_; }


template <class T> inline
void ArrayNDImpl<T>::copyFrom( const ArrayND<T>& templ )
{
    if ( info()!=templ.info() )
    {
	delete in_;
	in_ = templ.info().clone();
    }

    base::getDataFrom( templ );
}


template <class T> inline
void ArrayNDImpl<T>::setND( const int* pos, T v )
{
#ifdef __debug__
    if ( !in_->validPos( pos ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    const od_int64 offset = in_->getOffset(pos);
    if ( base::ptr_ ) base::ptr_[offset] = v ;
    else base::stor_->setValue( offset, v);
}


template <class T> inline
T ArrayNDImpl<T>::getND( const int* pos ) const
{
#ifdef __debug__
    if ( !in_->validPos( pos ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    const od_int64 offset = in_->getOffset(pos);
    return base::ptr_ ? base::ptr_[offset] : base::stor_->value( offset );
}


template <class T> inline
bool ArrayNDImpl<T>::setInfo( const ArrayNDInfo& ni )
{
    if ( ni.getNDim() != in_->getNDim() )
	return false;

    const int ndim = in_->getNDim();
    TypeSet<int> sizes( ndim, 0 );
    for ( int idx=0; idx<ndim; idx++ )
	sizes[idx] = ni.getSize(idx);

    return setSize( sizes.arr() );
}


template <class T> inline
bool ArrayNDImpl<T>::setSize( const int* d )
{
    const int ndim = in_->getNDim();
    for ( int idx=0; idx<ndim; idx++ )
	in_->setSize( idx, d[idx] );

    base::updateStorageSize();
    return true;
}


template <class T> inline
ArrayND<T>* ArrayNDImpl<T>::create( const ArrayNDInfo& nsz )
{
    int ndim = nsz.getNDim();

    if ( ndim==1 ) return new Array1DImpl<T>( nsz.getSize(0) );
    if ( ndim==2 ) return new Array2DImpl<T>( nsz.getSize(0), nsz.getSize(1) );
    if ( ndim==3 ) return new Array3DImpl<T>( nsz.getSize(0), nsz.getSize(1),
					      nsz.getSize(2) );
    if ( ndim==4 )
	return new Array4DImpl<T>( nsz.getSize(0), nsz.getSize(1),
				   nsz.getSize(2), nsz.getSize(3) );

    return new ArrayNDImpl<T>( nsz );
}


template <class T> inline
ArrayND<T>* ArrayNDImpl<T>::clone( const ArrayND<T>& oth )
{
    ArrayND<T>* out = create( oth.info() );
    if ( !out )
	return nullptr;

    if ( !out->isOK() )
	{ delete out; return nullptr; }

    const bool success = clone( oth, *out );
    if ( !success )
	{ delete out; return nullptr; }

    return out;
}


template <class T> inline
bool ArrayNDImpl<T>::clone( const ArrayND<T>& inp, ArrayND<T>& out )
{
    const od_uint64 sz = inp.info().getTotalSz();
    if ( !inp.isOK() )
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

    const int ndim = inp.info().getNDim();
    if ( ndim==1 )
    {
	mDynamicCastGet(const Array1DImpl<T>*,inp1d,&inp)
	mDynamicCastGet(Array1DImpl<T>*,out1d,&out)
	if ( inp1d && out1d )
	    { *out1d = *inp1d; return true; }
    }
    else if ( ndim==2 )
    {
	mDynamicCastGet(const Array2DImpl<T>*,inp2d,&inp)
	mDynamicCastGet(Array2DImpl<T>*,out2d,&out)
	if ( inp2d && out2d )
	    { *out2d = *inp2d; return true; }
    }
    else if ( ndim==3 )
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
	const int* pos = iter.getPos();
	out.setND( pos, inp.getND(pos) );
    } while ( iter.next() );

    return true;
}
