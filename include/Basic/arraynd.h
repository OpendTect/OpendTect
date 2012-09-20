#ifndef arraynd_h
#define arraynd_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id$
________________________________________________________________________

An ArrayND is an array with a given number of dimensions and a size. The
ArrayND can be accessed via set() and get().

The ArrayND can give away a pointer to it's storage, but there is no
guarantee that it will. If no pointer is given, the user can copy the
ArrayND by constructing an ArrayNDImpl with the original array as an argument
to the constructor.

*/

#include "basicmod.h"
#include "valseries.h"
#include "arrayndinfo.h"
#include "varlenarray.h"
#include "odmemory.h"
#include "ptrman.h"
#include <string.h>

#define mPolyArray1DInfoTp mPolyRet(ArrayNDInfo,Array1DInfo)
#define mPolyArray2DInfoTp mPolyRet(ArrayNDInfo,Array2DInfo)
#define mPolyArray3DInfoTp mPolyRet(ArrayNDInfo,Array3DInfo)

template <class T>
class ArrayND 
{
public:

    virtual				~ArrayND()	{}

    virtual inline bool			isOK() const;

    virtual T	                	getND(const int*) const	= 0;
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

    inline void				setAll(const T&);
    virtual void		getAll(T* ptr) const;
    					/*!<Fills ptr with values from array.
					    ptr is assumed to be allocated
					    with info().getTotalSz() number
					    of values. */
    virtual void		getAll(ValueSeries<T>& vs) const;
    					/*!<Fills vs with values from array.
					    ptr is assumed to be allocated
					    with info().getTotalSz() number
					    of values. */

protected:
 
    virtual const ValueSeries<T>* getStorage_() const { return 0; }

    virtual const T*		getData_() const
				{
				    if ( getStorage_() )
					return getStorage()->arr();
				    return 0;
				}

};


template <class T>
class Array1D : public ArrayND<T>
	      , public ValueSeries<T>
{
public: 

    virtual void		set(int,T)				= 0;
    virtual T			get(int) const				= 0;
    void			setND(const int* pos,T v) { set( pos[0], v ); }
    T	                	getND(const int* pos) const {return get(pos[0]);}

				// implement ValueSeries interface
    T				value(od_int64 i) const	{ return get( (int) i); }
    bool			writable() const	{ return true; }
    void			setValue(od_int64 i,T t){ set( (int) i,t); }

    virtual const Array1DInfo&	info() const = 0;

    inline T			operator []( int idx ) const
				{ return get( idx ); }

};


template <class T>
class Array2D : public ArrayND<T>
{
public: 
    virtual void		set( int, int, T ) 			= 0;
    virtual T        		get( int p0, int p1 ) const		= 0;
    void			setND(  const int* pos, T v )
				    { set( pos[0], pos[1], v);}
    T		                getND( const int* pos ) const
				    { return get( pos[0], pos[1] ); }

    virtual const Array2DInfo&	info() const = 0;
};


template <class T> class Array3D : public ArrayND<T>
{
public: 

    virtual void		set( int, int, int, T ) 		= 0;
    virtual T        		get( int p0, int p1, int p2 ) const	= 0;
    void			setND( const int* pos, T v )
				    { set( pos[0], pos[1], pos[2], v);}
    T		                getND( const int* pos ) const
				    { return get( pos[0], pos[1], pos[2] ); }

    virtual const Array3DInfo&	info() const = 0;
};


/*!\brief iterates through all samples in an ArrayND.

   ArrayNDIter will be on the first position when initiated, and move to
   the second at the fist call to next(). next() will return false when
   no more positions are avaliable.
*/

mClass(Basic) ArrayNDIter
{
public:
				ArrayNDIter( const ArrayNDInfo& );
				~ArrayNDIter();

    bool			next();
    void			reset();

    template <class T> void inline setPos(const T& idxabl);
    const int*			getPos() const { return position_; }
    int				operator[](int) const;

protected:

    bool			inc(int);

    int*			position_;
    const ArrayNDInfo&		sz_;

};

#define mArrayNDVSAdapterNrDim 20


/*! Adapter that makes any ArrayND to a (slow) value series. Try using 
    other methods (like getting the storage) as this is slow. */

template <class T>
class ArrayNDValseriesAdapter : public ValueSeries<T>
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
    
    bool		isOK() const
			{
			    if ( array_.info().getNDim()>mArrayNDVSAdapterNrDim)
			    {
				pErrMsg( "Too many dimensions");
				return false;
			    }
			    
			    return true;
			}
			
    ValueSeries<T>*	clone() const
			{ return new ArrayNDValseriesAdapter<T>( *this ); }
    
    T			value(od_int64 idx) const
			{
			    int pos[mArrayNDVSAdapterNrDim];
			    array_.info().getArrayPos( idx, pos );
			    return array_.getND( pos );
			}
    
    const T*		arr() const { return array_.getData(); }
    T*			arr() { return 0; }
    
protected:

    const ArrayND<T>&	array_;
};


template <class T> inline void ArrayNDIter::setPos( const T& idxable )
{
    for ( int idx=sz_.getNDim()-1; idx>=0; idx-- )
	position_[idx] = idxable[idx];
}


/*! Converter classes from one type to another. */

#define mDefArrayNDConverter(nd) \
template <class T, class TT> \
class Array##nd##Conv : public Array##nd<T> \
{ \
public: \
 \
    			Array##nd##Conv(Array##nd<TT>* arr) \
			    : arr_(arr)	{} \
    			~Array##nd##Conv()	{ delete arr_; } \
 \
    const Array##nd##Info&	info() const	{ return arr_->info(); } \
 \
protected: \
 \
    Array##nd<TT>*	arr_; \
 \
public:

mDefArrayNDConverter(1D)

    T        		get( int p0 ) const
    					{ return (T)arr_->get( p0 ); }
    void		set( int p0, T v )
    					{ arr_->set( p0, (TT)v ); }

};

mDefArrayNDConverter(2D)

    T        		get( int p0, int p1 ) const
    					{ return (T)arr_->get( p0, p1 ); }
    void		set( int p0, int p1, T v )
    					{ arr_->set( p0, p1, (TT)v ); }

};

mDefArrayNDConverter(3D)

    T        		get( int p0, int p1, int p2 ) const
    					{ return (T)arr_->get( p0, p1, p2 ); }
    void		set( int p0, int p1, int p2, T v )
    					{ arr_->set( p0, p1, p2, (TT)v ); }

};


// Only implementations below

template <class T> inline
bool ArrayND<T>::isOK() const
{
    return getStorage() ? getStorage()->isOK() : true;
}


template <class T> inline
const T* ArrayND<T>::get1D( const int* i ) const
{
    const T* ptr = getData();
    if ( !ptr ) return 0;

    int ndim = info().getNDim();

    ArrPtrMan<int> pos = new int[ndim];
    memcpy(pos,i, (int) sizeof(int)*(ndim-1));

    pos[ndim-1] = 0;
    
    return &ptr[info().getOffset( pos )];
}


template <class T> inline
int ArrayND<T>::get1DDim() const
{ return info().getNDim()-1; }


template <class T> inline
T* ArrayND<T>::getData()
{
    return !isSettable() ? 0
			 : const_cast<T*>(((const ArrayND*)this)->getData_());
}


template <class T> inline
ValueSeries<T>* ArrayND<T>::getStorage()
{
    return !isSettable() ? 0 : 
	const_cast<ValueSeries<T>* >(((const ArrayND*)this)->getStorage_());
}


template <class T> inline
T* ArrayND<T>::get1D( const int* i )
{
    return !isSettable() ? 0 : const_cast<T*>(((const ArrayND*)this)->get1D(i));
}


template <class T> inline
void ArrayND<T>::setAll( const T& val )
{
    ValueSeries<T>* stor = getStorage();
    if ( stor )
    {
	if ( stor->canSetAll() )
	    stor->setAll( val );
	else
	{
	    MemSetter<T> setter( *stor, val, info().getTotalSz() );
	    setter.execute();
	}

	return;
    }

    ArrayNDIter iterator( info() );
    do
    {
	setND( iterator.getPos(), val );
    } while ( iterator.next() );
}


template <class T>
class ArrayNDGetAll : public ParallelTask
{
public:
    		ArrayNDGetAll( T* ptr, const ArrayND<T>& arr )
		    : ptr_( ptr )
		    , arr_( arr )
		    , totalnr_( arr.info().getTotalSz() )
		    , vs_( 0 )
		{}

    		ArrayNDGetAll( ValueSeries<T>& vs, const ArrayND<T>& arr )
		    : ptr_( vs.arr() )
		    , arr_( arr )
		    , totalnr_( arr.info().getTotalSz() )
		    , vs_( vs.arr() ? 0 : &vs )
		{}

    bool	doWork( od_int64 start, od_int64 stop, int )
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
		    
    od_int64	nrIterations() const { return totalnr_; }
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
    {
	getAll( vs.arr() );
	return;
    }

    const od_int64 totalsz = info().getTotalSz();
    if ( !totalsz )
	return;

    const ValueSeries<T>* stor = getStorage();
    if ( stor )
    {
	stor->getValues( vs, totalsz );
	return;
    }

    ArrayNDGetAll<T> getter( vs, *this );
    getter.execute();
}


template <class T> inline
void ArrayND<T>::getAll( T* ptr ) const
{
    const od_int64 totalsz = info().getTotalSz();
    if ( !totalsz )
	return;

    const T* tp = getData();
    if ( tp )
    {
	MemCopier<T> cpier( ptr, tp, totalsz );
	cpier.execute();
	return;
    }

    const ValueSeries<T>* stor = getStorage();
    if ( stor )
    {
	MemCopier<T> cpier( ptr, *stor, totalsz );
	cpier.execute();
	return;
    }

    ArrayNDGetAll<T> getter( ptr, *this );
    getter.execute();
}


#endif

