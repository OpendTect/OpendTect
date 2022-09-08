#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "valseries.h"
#include "arrayndinfo.h"
#include "varlenarray.h"
#include "odmemory.h"
#include "ptrman.h"

/*!
\brief An ArrayND is an array with a given number of dimensions and a size.

  The ArrayND can be accessed via set() and get().

  The ArrayND can give away a pointer to its storage, but there is no
  guarantee that it will. If no pointer is given, the user can copy the
  ArrayND by constructing an ArrayNDImpl with the original array as an argument
  to the constructor.
*/

template <class T>
mClass(Basic) ArrayND
{
public:

    virtual				~ArrayND()	{}

    virtual inline bool			isOK() const;
    virtual inline bool			isEmpty() const;

    virtual T				getND(const int*) const = 0;
    virtual bool			isSettable() const	{ return true; }
    virtual void			setND(const int*,T)	= 0;

    inline const ValueSeries<T>*	getStorage() const
					{ return getStorage_(); }
    inline ValueSeries<T>*		getStorage();
    virtual bool			canSetStorage() const { return false; }
    virtual bool			setStorage(ValueSeries<T>* s)
					{ delete s; return true; }
					/*!<becomes mine. The size must be
					    settable, or I return false. */

    inline const T*			getData() const
					{ return getData_(); }
    inline T*				getData();
    virtual const T*			get1D(const int*) const;
    virtual T*				get1D(const int*);
    virtual int				get1DDim() const;


    virtual const ArrayNDInfo&		info() const		= 0;
    virtual bool			canSetInfo() const
					{ return false; }
    virtual bool			canChangeNrDims() const
					{ return false; }
    virtual bool			setInfo( const ArrayNDInfo& )
					{ return false; }

    virtual void			setAll(T);
    virtual void			getAll(T* ptr) const;
					/*!<Fills ptr with values from array.
					    ptr is assumed to be allocated
					    with info().getTotalSz() number
					    of values. */
    virtual void			getAll(ValueSeries<T>& vs) const;
					/*!<Fills vs with values from array.
					    ptr is assumed to be allocated
					    with info().getTotalSz() number
					    of values. */

					// rank/size info
    inline int				nrDims() const
					{ return info().getNDim(); }
    inline int				getSize( int dim ) const
					{ return info().getSize(dim); }
    inline od_uint64			totalSize() const
					{ return info().getTotalSz(); }
    inline bool				validPos( int* pos ) const
					{ return info().validPos(pos); }
protected:

    virtual const ValueSeries<T>*	getStorage_() const { return nullptr; }

    virtual const T*			getData_() const
					{
					    if ( getStorage_() )
						return getStorage()->arr();
					    return nullptr;
					}
};


/*!
\brief Array1D ( Subclass of ArrayND ) is a one dimensional array.
*/

template <class T>
mClass(Basic) Array1D : public ArrayND<T>
		      , public ValueSeries<T>
{
public:

    virtual void		set(int,T)				= 0;
    virtual T			get(int) const				= 0;
    void			setND( const int* pos , T v ) override
				{ set( pos[0], v ); }
    T				getND(const int* pos) const override
				{ return get(pos[0]); }

				// implement ValueSeries interface
    T				value(od_int64 i) const override
				{ return get( (int) i); }
    bool			writable() const override	{ return true; }
    void			setValue( od_int64 i , T t ) override
				{ set( (int) i,t); }
    void			setAll( T t ) override
				{ ArrayND<T>::setAll(t); }

    const Array1DInfo&		info() const override = 0;

    // Compatibility with other classes:

    inline od_int64		size() const override
				{ return ArrayND<T>::totalSize(); }
    inline T			operator []( int idx ) const
				{ return get( idx ); }

};


/*!
\brief Array2D ( Subclass of ArrayND ) is a two dimensional array.
*/

template <class T>
mClass(Basic) Array2D : public ArrayND<T>
{
public:
    virtual void		set( int, int, T )			= 0;
    virtual T			get( int p0, int p1 ) const		= 0;
    void			setND(	const int* pos, T v ) override
				    { set( pos[0], pos[1], v);}
    T				getND( const int* pos ) const override
				    { return get( pos[0], pos[1] ); }

    virtual T**			get2DData()		{ return nullptr; }
    virtual const T**		get2DData() const	{ return nullptr; }

    const Array2DInfo&		info() const override = 0;
};


/*!
\brief Array3D ( Subclass of ArrayND ) is a three dimensional array.
*/

template <class T>
mClass(Basic) Array3D : public ArrayND<T>
{
public:

    virtual void		set( int, int, int, T )			= 0;
    virtual T			get( int p0, int p1, int p2 ) const	= 0;
    void			setND( const int* pos, T v ) override
				    { set( pos[0], pos[1], pos[2], v);}
    T				getND( const int* pos ) const override
				    { return get( pos[0], pos[1], pos[2] ); }

    virtual T***		get3DData()		{ return nullptr; }
    virtual const T***		get3DData() const	{ return nullptr; }

    const Array3DInfo&		info() const override = 0;
};


/*!\brief 4-Dim ArrayND */

template <class T>
mClass(Basic) Array4D : public ArrayND<T>
{
public:
				mTypeDefArrNDTypes;

    virtual void		set(int,int,int,int,T)= 0;
    virtual T			get(int,int,int,int) const = 0;
    void			setND( const int* pos, T v ) override
				{ set( pos[0], pos[1], pos[2], pos[3], v ); }
    T				getND( const int* pos ) const override
				{ return get( pos[0], pos[1], pos[2], pos[3] );}

    virtual T****		get4DData()		{ return nullptr; }
    virtual const T****		get4DData() const	{ return nullptr; }

    const Array4DInfo&		info() const override = 0;
};

/*!
\brief Iterates through all samples in an ArrayND.

  ArrayNDIter will be on the first position when initiated, and move to
  the second at the first call to next(). next() will return false when
  no more positions are available.
*/

mExpClass(Basic) ArrayNDIter
{
public:
				ArrayNDIter( const ArrayNDInfo& );
				~ArrayNDIter();

    bool			next();
    void			reset();

    bool			setGlobalPos(od_int64);

    template <class T> void inline setPos(const T& idxabl);
    const int*			getPos() const { return position_; }
    int				operator[](int) const;

protected:

    bool			inc(int);

    int*			position_;
    const ArrayNDInfo&		sz_;

};

#define mArrayNDVSAdapterNrDim 20


/*!
\brief Adapter that makes any ArrayND to a (slow) value series.

  Try using other methods (like getting the storage) as this is slow.
*/

template <class T>
mClass(Basic) ArrayNDValseriesAdapter : public ValueSeries<T>
{
public:
			ArrayNDValseriesAdapter( const ArrayND<T>& a )
			    : array_( a )
			{
			    if ( array_.getData() || array_.getStorage() )
			    {
				pErrMsg("Not a good idea to use adapter. "
					"Use getStorage() instead");
			    }
			}

    bool		isOK() const override
			{
			    if ( array_.info().getNDim()>mArrayNDVSAdapterNrDim)
			    {
				pErrMsg( "Too many dimensions");
				return false;
			    }

			    return true;
			}

    ValueSeries<T>*	clone() const override
			{ return new ArrayNDValseriesAdapter<T>( *this ); }

    T			value(od_int64 idx) const override
			{
			    int pos[mArrayNDVSAdapterNrDim];
			    array_.info().getArrayPos( idx, pos );
			    return array_.getND( pos );
			}

    const T*		arr() const override { return array_.getData(); }
    T*			arr() override { return nullptr; }

    od_int64		size() const override	{ return array_.totalSize(); }

protected:

    const ArrayND<T>&	array_;
};


template <class T> inline void ArrayNDIter::setPos( const T& idxable )
{
    for ( int idx=sz_.getNDim()-1; idx>=0; idx-- )
	position_[idx] = idxable[idx];
}


inline
bool ArrayNDIter::setGlobalPos( od_int64 globalidx )
{
    return sz_.getArrayPos(globalidx,position_);
}

#define mDefArrayNDStdMembers(nd) \
public: \
			Array##nd##Conv(Array##nd<TT>* arr) \
			    : arr_(arr) {} \
			~Array##nd##Conv()	{ delete arr_; } \
 \
    const Array##nd##Info& info() const override { return arr_->info(); } \
 \
protected: \
 \
    Array##nd<TT>*	arr_; \
 \
public:

#define mDefArrayNDConverter(nd) \
template <class T, class TT> \
class Array##nd##Conv : public Array##nd<T> \
{ mDefArrayNDStdMembers(nd);



template <class T, class TT>
class Array1DConv : public Array1D<T>
{ mDefArrayNDStdMembers(1D);
public:
    T			get( int p0 ) const override
					{ return (T)arr_->get( p0 ); }
    void		set( int p0, T v ) override
					{ arr_->set( p0, (TT)v ); }

};


template <class T, class TT>
class Array2DConv : public Array2D<T>
{ mDefArrayNDStdMembers(2D);
    T			get( int p0, int p1 ) const override
					{ return (T)arr_->get( p0, p1 ); }
    void		set( int p0, int p1, T v ) override
					{ arr_->set( p0, p1, (TT)v ); }

};

template <class T, class TT>
class Array3DConv : public Array3D<T>
{ mDefArrayNDStdMembers(3D);

    T			get( int p0, int p1, int p2 ) const override
					{ return (T)arr_->get( p0, p1, p2 ); }
    void		set( int p0, int p1, int p2, T v ) override
					{ arr_->set( p0, p1, p2, (TT)v ); }

};


template <class T, class TT>
class Array4DConv : public Array4D<T>
{ mDefArrayNDStdMembers(4D);

    T		get( int p0, int p1, int p2, int p3 ) const override
				{ return (T)arr_->get( p0, p1, p2, p3 ); }
    void	set( int p0, int p1, int p2, int p3, T v ) override
				{ arr_->set( p0, p1, p2, p3, (TT)v ); }

};

// Only implementations below

template <class T> inline
bool ArrayND<T>::isOK() const
{
    return getStorage() ? getStorage()->isOK() : true;
}


template <class T> inline
bool ArrayND<T>::isEmpty() const
{
    return !isOK() || info().getTotalSz() == 0;
}


template <class T> inline
const T* ArrayND<T>::get1D( const int* i ) const
{
    const T* ptr = getData();
    if ( !ptr ) return nullptr;

    int ndim = info().getNDim();

    mAllocLargeVarLenArr( int, pos, ndim );
    OD::memCopy(pos,i, (int) sizeof(int)*(ndim-1));

    pos[ndim-1] = 0;

    return &ptr[info().getOffset( pos )];
}


template <class T> inline
int ArrayND<T>::get1DDim() const
{ return info().getNDim()-1; }


template <class T> inline
T* ArrayND<T>::getData()
{
    return !isSettable() ? nullptr
			 : const_cast<T*>(((const ArrayND*)this)->getData_());
}


template <class T> inline
ValueSeries<T>* ArrayND<T>::getStorage()
{
    return !isSettable() ? nullptr :
	const_cast<ValueSeries<T>* >(((const ArrayND*)this)->getStorage_());
}


template <class T> inline
T* ArrayND<T>::get1D( const int* i )
{
    return !isSettable() ? nullptr :
	const_cast<T*>(((const ArrayND*)this)->get1D(i));
}


template <class T> inline
void ArrayND<T>::setAll( T val )
{
    if ( isEmpty() )
	return;

    ValueSeries<T>* stor = getStorage();
    if ( stor )
    {
	if ( stor->canSetAll() )
	    stor->setAll( val );
	else
	{
	    MemSetter<T> memsetter( *stor, val, info().getTotalSz() );
	    memsetter.execute();
	}

	return;
    }

    ArrayNDIter iterator( info() );
    do
    {
	setND( iterator.getPos(), val );
    } while ( iterator.next() );
}

/*!
\brief Gets a one dimensional array from an ArrayND.
*/

template <class T>
mClass(Basic) ArrayNDDataExtracter : public ParallelTask
{
public:
		ArrayNDDataExtracter( T* ptr, const ArrayND<T>& arr )
		    : ptr_( ptr )
		    , arr_( arr )
		    , totalnr_( arr.info().getTotalSz() )
		    , vs_( nullptr )
		{}

		ArrayNDDataExtracter( ValueSeries<T>& vs, const ArrayND<T>& arr)
		    : ptr_( vs.arr() )
		    , arr_( arr )
		    , totalnr_( arr.info().getTotalSz() )
		    , vs_( vs.arr() ? nullptr : &vs )
		{}

    bool	doWork( od_int64 start, od_int64 stop, int ) override
		{
		    mAllocVarLenArr( int, pos, arr_.info().getNDim() );
		    if ( !arr_.info().getArrayPos( start, pos ) )
			return false;

		    ArrayNDIter iterator( arr_.info() );
		    iterator.setPos( (int*) pos );

		    if ( vs_ )
		    {
			for ( od_int64 idx=start; idx<=stop; idx++ )
			{
			    vs_->setValue( idx,
				    arr_.getND( iterator.getPos() ) );
			    if ( idx==stop )
				break;

			    if ( !iterator.next() )
				return false;
			}
		    }
		    else
		    {
			T* res = ptr_ + start;
			for ( od_int64 idx=start; idx<=stop; idx++, res++ )
			{
			    *res = arr_.getND( iterator.getPos() );
			    if ( idx==stop )
				break;

			    if ( !iterator.next() )
				return false;
			}
		    }

		    return true;
		}

    od_int64	nrIterations() const override { return totalnr_; }

protected:

    od_int64		totalnr_;
    const ArrayND<T>&	arr_;
    T*			ptr_;
    ValueSeries<T>*	vs_;

};


template <class T> inline
void ArrayND<T>::getAll( ValueSeries<T>& vs ) const
{
    if ( vs.arr() )
	{ getAll( vs.arr() ); return; }

    const od_int64 totalsz = info().getTotalSz();
    if ( !totalsz )
	return;

    const ValueSeries<T>* stor = getStorage();
    if ( stor )
	stor->getValues( vs, totalsz );
    else
    {
	ArrayNDDataExtracter<T> extr( vs, *this );
	extr.execute();
    }
}


template <class T> inline
void ArrayND<T>::getAll( T* ptr ) const
{
    const od_int64 totalsz = info().getTotalSz();
    if ( !totalsz )
	return;

    const T* tdata = getData();
    if ( tdata )
	OD::memCopy( ptr, tdata, totalsz * sizeof(T) );
    else
    {
	const ValueSeries<T>* stor = getStorage();
	if ( stor )
	{
	    MemCopier<T> cpier( ptr, *stor, totalsz );
	    cpier.execute();
	}
	else
	{
	    ArrayNDDataExtracter<T> extr( ptr, *this );
	    extr.execute();
	}
    }
}
