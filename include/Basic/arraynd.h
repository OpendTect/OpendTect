#ifndef arraynd_h
#define arraynd_h
/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id: arraynd.h,v 1.39 2009-04-09 00:55:55 cvskris Exp $
________________________________________________________________________

An ArrayND is an array with a given number of dimensions and a size. The
ArrayND can be accessed via set() and get().

The ArrayND can give away a pointer to it's storage, but there is no
guarantee that it will. If no pointer is given, the user can copy the
ArrayND by constructing an ArrayNDImpl with the original array as an argument
to the constructor.

*/

#include "valseries.h"
#include "arrayndinfo.h"
#include "varlenarray.h"
#include "memsetter.h"
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

					// Read specs
    virtual T	                	getND( const int* ) const	= 0;

    inline const ValueSeries<T>*	getStorage() const
					{ return getStorage_(); }

    inline const T*			getData() const
					{ return getData_(); }
    virtual const T*			get1D(const int*) const;
    virtual int				get1DDim() const;

					// Write specs
    virtual bool			isSettable() const
					{ return true; }
    virtual void			setND( const int*, T )	= 0;

    virtual bool			canSetStorage() const { return false; }
    virtual bool			setStorage(ValueSeries<T>* s)
    					{ delete s; return true; }
    					/*!<becomes mine. The size must be
					    settable, or I return false. */

    inline ValueSeries<T>*		getStorage();
    inline T*				getData();
    virtual T*				get1D( const int* i );

    virtual const ArrayNDInfo&		info() const		= 0;
    virtual bool			canSetInfo() const
    					{ return false; }
    					/*!< You might not be able to
					     change nr dimension, check
					     canChangeNrDims() if you
					     want to do that. */
    virtual bool			canChangeNrDims() const
    					{ return false; }
    virtual bool			setInfo( const ArrayNDInfo& )
					{ return false; }

    void				setAll(const T&);
    void				getAll(T* ptr) const;
    					/*!<Fills ptr with values from array.
					    ptr is assumed to be allocated
					    with info().getTotalSz() number
					    of values. */
    void				getAll(ValueSeries<T>& vs) const;
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
    T				value(od_int64 i) const	{ return get(i); }
    bool			writable() const	{ return true; }
    void			setValue(od_int64 i,T t){ set(i,t); }

    virtual const Array1DInfo&	info() const = 0;

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


/*!Converter class from one type to another. */

template <class T, class TT> class Array3DConv : public Array3D<T>
{
public:
    		Array3DConv(Array3D<TT>* arr) : arr_( arr ) {}
    		~Array3DConv() { delete arr_; }
    void	set(int p0,int p1,int p2,T v)	{arr_->set( p0, p1, p2,(TT)v );}
    T        	get(int p0,int p1,int p2) const {return (T)arr_->get(p0,p1,p2);}
    const Array3DInfo&	info() const		{return arr_->info(); }

protected:
    Array3D<TT>*	arr_;
};

//Only implementations below

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
    memcpy(pos,i,sizeof(int)*(ndim-1));

    pos[ndim-1] = 0;
    
    return &ptr[info().getOffset( pos )];
}


template <class T> inline
int ArrayND<T>::get1DDim() const
{ return info().getNDim()-1; }


template <class T> inline
T* ArrayND<T>::getData()
{
    return isSettable() ? const_cast<T*>(((const ArrayND*)this)->getData_()): 0;
}


template <class T> inline
ValueSeries<T>* ArrayND<T>::getStorage()
{
    return isSettable()
	? const_cast<ValueSeries<T>* >
		(((const ArrayND*)this)->getStorage_())
	: 0;
}


template <class T> inline
T* ArrayND<T>::get1D( const int* i )
{
    return isSettable() ? const_cast<T*>(((const ArrayND*)this)->get1D(i)) :0;
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
mClass ArrayNDGetAll : public ParallelTask
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
	for ( od_int64 idx=0; idx<totalsz; idx++ )
	    vs.setValue( idx, stor->value( idx ) );
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
