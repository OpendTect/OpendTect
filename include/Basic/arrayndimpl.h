#ifndef arrayndimpl_h
#define arrayndimpl_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id$
________________________________________________________________________

*/


#include "arraynd.h"

#ifdef __debug__
# include "debug.h"
#endif

#define mDeclArrayNDProtMemb(inftyp) \
    inftyp			in_; \
    ValueSeries<T>*		stor_; \
 \
    const ValueSeries<T>*	getStorage_() const { return stor_; }

#define mDeclArrayNDCopyTools(nd) \
    inline			Array##nd##Impl(const Array##nd<T>&); \
    inline			Array##nd##Impl(const Array##nd##Impl<T>&); \
    inline Array##nd##Impl<T>&	operator =( const Array##nd<T>& ai ) \
				{ copyFrom(ai); return *this; } \
    inline Array##nd##Impl<T>&	operator =( const Array##nd##Impl<T>& ai ) \
				{ copyFrom(ai); return *this; } \
    inline bool			setStorageNoResize(ValueSeries<T>*);


/*!
\brief Implementation of Array1D.
*/

template <class T>
mClass(Basic) Array1DImpl : public Array1D<T>
{
public:

    inline			Array1DImpl(int sz);
    inline			~Array1DImpl();
    				mDeclArrayNDCopyTools(1D);

    ValueSeries<T>*		clone() const
    				{ return new Array1DImpl<T>(*this); }
    		
    inline bool			isOK() const { return stor_ && stor_->isOK(); }
    inline void			copyFrom(const Array1D<T>&);
    inline bool			setStorage(ValueSeries<T>*);
    inline bool			canSetStorage() const	{ return true; }

    inline void			set(int pos,T);
    inline T			get(int pos) const;
			
    inline const Array1DInfo&	info() const		{ return in_; }
    inline bool			canSetInfo() const	{ return true; }
    inline bool			setInfo(const ArrayNDInfo&);
    inline bool			setSize(int);
    inline bool			setSize( od_int64 sz )
    				{ return setSize( ((int)sz) ); }

				// ValueSeries interface
    inline T*			arr()			{ return ptr_; }
    inline const T*		arr() const		{ return ptr_; }

protected:
    inline const T*		getData_() const	{ return ptr_; }
    T*				ptr_;	//not owned, only a shortcut
    					//for the 99% percent case

    mDeclArrayNDProtMemb(Array1DInfoImpl);

};


/*!
\brief Implementation of Array2D.
*/

template <class T>
mClass(Basic) Array2DImpl : public Array2D<T>
{
public:
			Array2DImpl(int sz0,int sz1);
			Array2DImpl(const Array2DInfo&);
			~Array2DImpl();
			mDeclArrayNDCopyTools(2D)

    inline bool		isOK() const { return stor_ && stor_->isOK(); }
    bool		canSetStorage() const		{ return true; }
    bool		setStorage(ValueSeries<T>*);

    virtual void	set(int,int,T);
    virtual T		get(int,int) const;
    void		copyFrom(const Array2D<T>&);

    inline const Array2DInfo& info() const		{ return in_; }
    inline bool		canSetInfo() const		{ return true; }

    bool		setInfo(const ArrayNDInfo&);
    bool		setSize(int,int);

protected:

    inline const T*		getData_() const	{ return ptr_; }
    T*				ptr_;	//not owned, only a shortcut
    					//for the 99% percent case

    mDeclArrayNDProtMemb(Array2DInfoImpl);

};


/*!
\brief Implementation of Array3D.
*/

template <class T>
mClass(Basic) Array3DImpl : public Array3D<T>
{
public:
    inline		Array3DImpl(int sz0,int sz1,int sz2);
    inline		Array3DImpl(const Array3DInfo&);
			mDeclArrayNDCopyTools(3D)
    inline		~Array3DImpl();

    inline bool		isOK() const { return stor_ && stor_->isOK(); }
    inline bool		canSetStorage() const	{ return true; }
    inline bool		setStorage(ValueSeries<T>*);

    inline void		set(int,int,int,T);
    inline T		get(int,int,int) const;
    inline void		copyFrom(const Array3D<T>&);

    inline const Array3DInfo& info() const		{ return in_; }
    inline bool		canSetInfo() const		{ return true; }
    inline bool		setInfo(const ArrayNDInfo&);
    inline bool		setSize(int,int,int);

protected:
    inline const T*	getData_() const	{ return ptr_; }
    T*			ptr_;	//not owned, only a shortcut
    					//for the 99% percent case

    mDeclArrayNDProtMemb(Array3DInfoImpl);
};


/*!
\brief Implementation of ArrayND.
*/

template <class T>
mClass(Basic) ArrayNDImpl : public ArrayND<T>
{
public:

    static ArrayND<T>*		create(const ArrayNDInfo& nsz);

    inline			ArrayNDImpl(const ArrayNDInfo&);
				mDeclArrayNDCopyTools(ND)
    inline			~ArrayNDImpl();

    inline bool			isOK() const { return stor_ && stor_->isOK(); }
    inline bool			canSetStorage() const	{ return true; }
    inline bool			setStorage(ValueSeries<T>*);

    inline void			setND(const int*,T);
    inline T			getND(const int*) const;

    inline const ArrayNDInfo&	info() const		{ return *in_; }
    inline bool			canSetInfo() const	{ return true; }
    inline bool			canChangeNrDims() const	{ return true; }
    inline bool			setInfo(const ArrayNDInfo&);
 
    inline bool			setSize(const int*);
    inline void			copyFrom(const ArrayND<T>&);

protected:

    inline const T*		getData_() const	{ return ptr_; }
    T*				ptr_;	//not owned, only a shortcut
    					//for the 99% percent case

    mDeclArrayNDProtMemb(ArrayNDInfo*)

}; 


#define mArrNDImplConstructor \
    , stor_(0) \
    , ptr_(0) \
{ \
    if ( !info().isOK() ) \
    { \
	pErrMsg( "Invalid size" ); \
	return; \
    } \
    setStorageNoResize( (ValueSeries<T>*)new MultiArrayValueSeries<T,T>( \
					info().getTotalSz())); \
}

#define mArrNDImplCopyConstructor(clss,from) \
template <class T> inline \
clss<T>::clss( const from<T>& templ ) \
    : in_(templ.info())  \
    , stor_(0) \
    , ptr_(0) \
{ \
    const ValueSeries<T>* storage = templ.getStorage(); \
    ValueSeries<T>* newstor = storage && storage->selfSufficient() \
    	? storage->clone() \
	: 0; \
    if ( !newstor || !setStorageNoResize( newstor ) )  \
    { \
	if ( !info().isOK() ) \
	{ \
	    pErrMsg( "Invalid size" ); \
	    return; \
	} \
	setStorageNoResize( \
		new MultiArrayValueSeries<T,T>(info().getTotalSz()) ); \
	copyFrom( templ ); \
    } \
}

#define mArrNDImplDestructor( clss ) \
	template <class T> inline clss<T>::~clss() { delete stor_; }

#define mArrNDImplSetStorage( clss ) \
template <class T> inline \
bool clss<T>::setStorage( ValueSeries<T>* s ) \
{ \
    ptr_ = 0; \
    if ( !s->setSize(info().getTotalSz()) ) \
    { \
	delete s; \
	return false; \
    } \
    return setStorageNoResize( s ); \
} \
 \
 \
template <class T> inline \
bool clss<T>::setStorageNoResize( ValueSeries<T>* s ) \
{ \
    ptr_ = 0; \
    delete stor_; stor_ = s; ptr_ = stor_->arr(); \
    return true; \
}

#define mArrNDImplDoNormalCopy \
    if ( !this->isOK() ) \
	return; \
    if ( this->getData() ) \
    { \
	templ.getAll( this->getData() ); \
	return; \
    } \
    else if ( this->getStorage() ) \
    { \
	templ.getAll( *this->getStorage() ); \
	return; \
    }
#define mArrNDImplHandleNormalCopy(inf) \
    if ( inf != templ.info() ) \
	setInfo( templ.info() ); \
    mArrNDImplDoNormalCopy; \


template <class T> inline
Array1DImpl<T>::Array1DImpl( int nsz )
    : in_(nsz)
mArrNDImplConstructor
mArrNDImplCopyConstructor(Array1DImpl,Array1D)
mArrNDImplCopyConstructor(Array1DImpl,Array1DImpl)
mArrNDImplDestructor( Array1DImpl )
mArrNDImplSetStorage( Array1DImpl )

template <class T> inline
void Array1DImpl<T>::set( int pos, T v )	
{
#ifdef __debug__
    if ( !in_.validPos( pos ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    if ( ptr_ ) ptr_[pos] = v;
    else stor_->setValue(pos,v);
}


template <class T> inline
T Array1DImpl<T>::get( int pos ) const
{
#ifdef __debug__
    if ( !in_.validPos( pos ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    return ptr_ ? ptr_[pos] : stor_->value(pos);
}


template <class T> inline
void Array1DImpl<T>::copyFrom( const Array1D<T>& templ )
{
    mArrNDImplHandleNormalCopy(in_)

    const int totsz = (int) in_.getTotalSz();

    for ( int idx=0; idx<totsz; idx++ )
	set( idx, templ.get(idx) );
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
    if ( !stor_->setSize(s) )
	{ ptr_ = 0; return false; }
    ptr_ = stor_->arr();
    return true;
}


template <class T> inline
Array2DImpl<T>::Array2DImpl( int sz0, int sz1 )
    : in_(sz0,sz1)
mArrNDImplConstructor
template <class T> inline
Array2DImpl<T>::Array2DImpl( const Array2DInfo& nsz )
    : in_( nsz )
mArrNDImplConstructor
mArrNDImplCopyConstructor(Array2DImpl,Array2D)
mArrNDImplCopyConstructor(Array2DImpl,Array2DImpl)
mArrNDImplDestructor(Array2DImpl)
mArrNDImplSetStorage(Array2DImpl)


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
    const od_int64 offset = in_.getOffset( p0, p1 );
    if ( ptr_ ) ptr_[offset] = v;
    else stor_->setValue( offset, v );
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
    const od_int64 offset = in_.getOffset( p0, p1 );
    return ptr_ ? ptr_[offset] : stor_->value( offset );
}


template <class T> inline
void Array2DImpl<T>::copyFrom( const Array2D<T>& templ )
{
    mArrNDImplHandleNormalCopy(in_)

    const int sz0 = in_.getSize(0);
    const int sz1 = in_.getSize(1);
    for ( int id0=0; id0<sz0; id0++ )
    {
	for ( int id1=0; id1<sz1; id1++ )
	    set( id0, id1, templ.get(id0,id1) );
    }
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
    if ( !stor_->setSize( in_.getTotalSz() ) )
	{ ptr_ = 0; return false; }
    ptr_ = stor_->arr();
    return true;
}


template <class T> inline
Array3DImpl<T>::Array3DImpl( int sz0, int sz1, int sz2 )
    : in_(sz0,sz1,sz2)
mArrNDImplConstructor
template <class T> inline
Array3DImpl<T>::Array3DImpl( const Array3DInfo& nsz )
    : in_(nsz)
mArrNDImplConstructor
mArrNDImplCopyConstructor(Array3DImpl,Array3D)
mArrNDImplCopyConstructor(Array3DImpl,Array3DImpl)
mArrNDImplDestructor(Array3DImpl)
mArrNDImplSetStorage(Array3DImpl)

template <class T> inline
void Array3DImpl<T>::set( int p0, int p1, int p2, T v )
{
#ifdef __debug__
    if ( !in_.validPos( p0, p1, p2 ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    const od_int64 offset = in_.getOffset( p0, p1, p2 );
    if ( ptr_ ) ptr_[offset] = v;
    else stor_->setValue( offset, v );
}


template <class T> inline
T Array3DImpl<T>::get( int p0, int p1, int p2 ) const
{
#ifdef __debug__
    if ( !in_.validPos( p0, p1, p2 ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    const od_int64 offset = in_.getOffset( p0, p1, p2 );
    return ptr_ ? ptr_[offset] : stor_->value( offset );
}


template <class T> inline
void Array3DImpl<T>::copyFrom( const Array3D<T>& templ )
{
    mArrNDImplHandleNormalCopy(in_)

    int sz0 = in_.getSize(0);
    int sz1 = in_.getSize(1);
    int sz2 = in_.getSize(2);
    for ( int id0=0; id0<sz0; id0++ )
	for ( int id1=0; id1<sz1; id1++ )
	    for ( int id2=0; id2<sz2; id2++ )
		set( id0, id1, id2, templ.get(id0,id1,id2) );
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
    if ( !stor_->setSize( in_.getTotalSz() ) )
	{ ptr_ = 0; return false; }
    ptr_ = stor_->arr();
    return true;
}


#undef mArrNDImplCopyConstructor
#define mArrNDImplCopyConstructor(clss,from) \
template <class T> inline \
clss<T>::clss( const from<T>& templ ) \
    : in_(templ.info().clone())  \
    , stor_(0) \
    , ptr_(0) \
{ \
    if ( !info().isOK() ) \
    { \
	pErrMsg( "Invalid size" ); \
	return; \
    } \
    setStorage( new MultiArrayValueSeries<T,T>(in_->getTotalSz()) ); \
    copyFrom( templ ); \
}

template <class T> inline
ArrayNDImpl<T>::ArrayNDImpl( const ArrayNDInfo& nsz )
    : in_(nsz.clone())
mArrNDImplConstructor
mArrNDImplCopyConstructor(ArrayNDImpl,ArrayND)
mArrNDImplCopyConstructor(ArrayNDImpl,ArrayNDImpl)
template <class T> inline ArrayNDImpl<T>::~ArrayNDImpl()
{ delete stor_; delete in_; }

mArrNDImplSetStorage( ArrayNDImpl )

template <class T> inline
void ArrayNDImpl<T>::copyFrom( const ArrayND<T>& templ )
{
    if ( info()!=templ.info() )
    {
	delete in_;
	in_ = templ.info().clone();
    }

    mArrNDImplDoNormalCopy;

    if ( in_->getTotalSz() > 0 )
    {
	ArrayNDIter iter( *in_ );
	do
	{
	    const int* pos = iter.getPos();
	    setND( pos, templ.getND(pos) );
	} while ( iter.next() );
    }
}


template <class T> inline
void ArrayNDImpl<T>::setND( const int* pos, T v )
{
#ifdef __debug__
    if ( !in_->validPos( pos ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    const od_int64 offset = in_->getOffset(pos);
    if ( ptr_ ) ptr_[offset] = v ;
    else stor_->setValue( offset, v);
}


template <class T> inline
T ArrayNDImpl<T>::getND( const int* pos ) const
{
#ifdef __debug__
    if ( !in_->validPos( pos ) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    const od_int64 offset = in_->getOffset(pos);
    return ptr_ ? ptr_[offset] : stor_->value( offset );
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

    if ( !stor_->setSize(in_->getTotalSz()) )
	{ ptr_ = 0; return false; }

    ptr_ = stor_->arr();
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

    return new ArrayNDImpl<T>( nsz );
}


#endif
