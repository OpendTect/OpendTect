#ifndef arraynd_h
#define arraynd_h
/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id: arraynd.h,v 1.24 2007-08-22 12:01:09 cvskris Exp $
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
#include "ptrman.h"

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
    virtual T	                	get( const int* ) const	= 0;

    inline const ValueSeries<T>*	getStorage() const
					{ return getStorage_(); }

    inline const T*			getData() const
					{ return getData_(); }
    virtual const T*			get1D(const int*) const;
    virtual int				get1DDim() const;

					// Write specs
    virtual bool			isSettable() const
					{ return true; }
    virtual void			set( const int*, T )	= 0;

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
    virtual bool			setInfo( ArrayNDInfo& )
					{ return false; }

protected:
 
    virtual const ValueSeries<T>* getStorage_() const { return 0; }

    const T*			getData_() const
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
    void			set(const int* pos,T v) { set( pos[0], v ); }
    T	                	get(const int* pos) const {return get(pos[0]);}

				// implement ValueSeries interface
    T				value( int64 i ) const	{ return get(i); }
    bool			writable() const	{ return true; }
    void			setValue( int i, T t )	{ set(i,t); }

    virtual const mPolyArray1DInfoTp& info() const = 0;

};


template <class T>
class Array2D : public ArrayND<T>
{
public: 
    virtual void		set( int, int, T ) 			= 0;
    virtual T        		get( int p0, int p1 ) const		= 0;
    void			set(  const int* pos, T v )
				    { set( pos[0], pos[1], v);}
    T		                get( const int* pos ) const
				    { return get( pos[0], pos[1] ); }

    virtual const mPolyArray2DInfoTp& info() const = 0;
};


template <class T> class Array3D : public ArrayND<T>
{
public: 

    virtual void		set( int, int, int, T ) 		= 0;
    virtual T        		get( int p0, int p1, int p2 ) const	= 0;
    void			set( const int* pos, T v )
				    { set( pos[0], pos[1], pos[2], v);}
    T		                get( const int* pos ) const
				    { return get( pos[0], pos[1], pos[2] ); }

    virtual const mPolyArray3DInfoTp& info() const = 0;
};


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

#endif
